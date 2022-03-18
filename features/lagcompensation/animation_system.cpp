#include "animation_system.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"

std::deque <adjust_data> player_records[65];

void lagcompensation::fsn(ClientFrameStage_t stage)
{
	if (stage != FRAME_NET_UPDATE_END)
		return;

	if (!g_cfg.ragebot.enable)
		return;

	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (e == g_ctx.local())
			continue;

		if (!valid(i, e))
			continue;

		auto time_delta = abs(TIME_TO_TICKS(e->m_flSimulationTime()) - m_globals()->m_tickcount);

		if (time_delta > 1.0f / m_globals()->m_intervalpertick)
			continue;

		auto update = player_records[i].empty() || e->m_flSimulationTime() != e->m_flOldSimulationTime(); //-V550

		if (update && !player_records[i].empty())
		{
			auto server_tick = m_clientstate()->m_iServerTick - i % m_globals()->m_timestamprandomizewindow;
			auto current_tick = server_tick - server_tick % m_globals()->m_timestampnetworkingbase;

			if (TIME_TO_TICKS(e->m_flOldSimulationTime()) < current_tick && TIME_TO_TICKS(e->m_flSimulationTime()) == current_tick)
			{
				auto layer = &e->get_animlayers()[11];
				auto previous_layer = &player_records[i].front().layers[11];

				if (layer->m_flCycle == previous_layer->m_flCycle) //-V550
				{
					e->m_flSimulationTime() = e->m_flOldSimulationTime();
					update = false;
				}
			}
		}

		if (update) //-V550
		{
			if (!player_records[i].empty() && (e->m_vecOrigin() - player_records[i].front().origin).LengthSqr() > 4096.0f)
				for (auto& record : player_records[i])
					record.invalid = true;

			player_records[i].emplace_front(adjust_data());
			update_player_animations(e);

			while (player_records[i].size() > 32)
				player_records[i].pop_back();
		}
	}
}

void lagcompensation::upd_nw(player_t* m_pPlayer)
{
	float m_flSimulationTime = 0.0f;

	if (m_pPlayer->EntIndex() >= 64)
		return;

	int m_iNextSimulationTick = m_flSimulationTime / m_globals()->m_intervalpertick + 1;

	g_ctx.globals.updating_animation = true;

	if (m_pPlayer->get_animation_state()->m_iLastClientSideAnimationUpdateFramecount >= m_iNextSimulationTick)
		m_pPlayer->get_animation_state()->m_iLastClientSideAnimationUpdateFramecount = m_iNextSimulationTick - 1;

	m_pPlayer->m_bClientSideAnimation() = true;
	m_pPlayer->update_clientside_animation();

	//resolver::get().OnUpdateClientSideAnimation(m_pPlayer);

	g_ctx.globals.updating_animation = false;
}

void lagcompensation::extrapolation(player_t* player, Vector& origin, Vector& velocity, int& flags, bool on_ground)
{
	static const auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));
	static const auto sv_jump_impulse = m_cvar()->FindVar(crypt_str("sv_jump_impulse"));

	if (!(flags & FL_ONGROUND))
		velocity.z -= TICKS_TO_TIME(sv_gravity->GetFloat());

	else if (player->m_fFlags() & FL_ONGROUND && !on_ground)
		velocity.z = sv_jump_impulse->GetFloat();

	const auto src = origin;
	auto end = src + velocity * m_globals()->m_intervalpertick;

	Ray_t r;
	r.Init(src, end, player->GetCollideable()->OBBMins(), player->GetCollideable()->OBBMaxs());

	CGameTrace t;
	CTraceFilter filter;
	filter.pSkip = player;

	m_trace()->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

	if (t.fraction != 1.f)
	{
		for (auto i = 0; i < 2; i++)
		{
			velocity -= t.plane.normal * velocity.Dot(t.plane.normal);

			const auto dot = velocity.Dot(t.plane.normal);
			if (dot < 0.f)
				velocity -= Vector(dot * t.plane.normal.x,
					dot * t.plane.normal.y, dot * t.plane.normal.z);

			end = t.endpos + velocity * TICKS_TO_TIME(1.f - t.fraction);

			r.Init(t.endpos, end, player->GetCollideable()->OBBMins(), player->GetCollideable()->OBBMaxs());
			m_trace()->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

			if (t.fraction == 1.f)
				break;
		}
	}

	origin = end = t.endpos;
	end.z -= 2.f;

	r.Init(origin, end, player->GetCollideable()->OBBMins(), player->GetCollideable()->OBBMaxs());
	m_trace()->TraceRay(r, MASK_PLAYERSOLID, &filter, &t);

	flags &= ~FL_ONGROUND;

	if (t.DidHit() && t.plane.normal.z > .7f)
		flags |= FL_ONGROUND;
}

bool lagcompensation::valid(int i, player_t* e)
{
	if (!g_cfg.ragebot.enable || !e->valid(false))
	{
		if (!e->is_alive())
		{
			is_dormant[i] = false;
			player_resolver[i].reset();

			g_ctx.globals.fired_shots[i] = 0;
			g_ctx.globals.missed_shots[i] = 0;
		}
		else if (e->IsDormant())
			is_dormant[i] = true;

		player_records[i].clear();
		return false;
	}

	return true;
}

void lagcompensation::update_player_animations(player_t* e)
{
	auto animstate = e->get_animation_state();

	if (!animstate)
		return;

	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(e->EntIndex(), &player_info))
		return;

	auto records = &player_records[e->EntIndex()];

	if (records->empty())
		return;

	adjust_data* previous_record = nullptr;

	if (records->size() >= 2)
		previous_record = &records->at(1);

	auto record = &records->front();

	AnimationLayer animlayers[13];
	float pose_parametrs[24];

	memcpy(pose_parametrs, &e->m_flPoseParameter(), 24 * sizeof(float));
	memcpy(animlayers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	auto backup_lower_body_yaw_target = e->m_flLowerBodyYawTarget();
	auto backup_duck_amount = e->m_flDuckAmount();
	auto backup_flags = e->m_fFlags();
	auto backup_eflags = e->m_iEFlags();

	auto backup_curtime = m_globals()->m_curtime;
	auto backup_frametime = m_globals()->m_frametime;
	auto backup_realtime = m_globals()->m_realtime;
	auto backup_framecount = m_globals()->m_framecount;
	auto backup_tickcount = m_globals()->m_tickcount;
	auto backup_absolute_frametime = m_globals()->m_absoluteframetime;
	auto backup_interpolation_amount = m_globals()->m_interpolation_amount;

	m_globals()->m_curtime = e->m_flSimulationTime();
	m_globals()->m_frametime = m_globals()->m_intervalpertick;

	if (previous_record)
	{
		auto velocity = e->m_vecVelocity();
		auto was_in_air = e->m_fFlags() & FL_ONGROUND && previous_record->flags & FL_ONGROUND;

		auto time_difference = max(m_globals()->m_intervalpertick, e->m_flSimulationTime() - previous_record->simulation_time);
		auto origin_delta = e->m_vecOrigin() - previous_record->origin;

		auto animation_speed = 0.0f;

		if (!origin_delta.IsZero() && TIME_TO_TICKS(time_difference) > 0)
		{
			e->m_vecVelocity() = origin_delta * (1.0f / time_difference);

			if (e->m_fFlags() & FL_ONGROUND && animlayers[11].m_flWeight > 0.0f && animlayers[11].m_flWeight < 1.0f && animlayers[11].m_flCycle > previous_record->layers[11].m_flCycle)
			{
				auto weapon = e->m_hActiveWeapon().Get();

				if (weapon)
				{
					auto max_speed = 260.0f;
					auto weapon_info = e->m_hActiveWeapon().Get()->get_csweapon_info();

					if (weapon_info)
						max_speed = e->m_bIsScoped() ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

					auto modifier = 0.35f * (1.0f - animlayers[11].m_flWeight);

					if (modifier > 0.0f && modifier < 1.0f)
						animation_speed = max_speed * (modifier + 0.55f);
				}
			}

			if (animation_speed > 0.0f)
			{
				animation_speed /= e->m_vecVelocity().Length2D();

				e->m_vecVelocity().x *= animation_speed;
				e->m_vecVelocity().y *= animation_speed;
			}

			if (records->size() >= 3 && e->m_vecVelocity().Length2D() > 0 && e->m_fFlags() & FL_ONGROUND && record->flags & FL_ONGROUND)
			{
				auto previous_velocity = (previous_record->origin - records->at(2).origin) * (1.0f / time_difference);

				if (!previous_velocity.IsZero() && !was_in_air)
				{
					auto current_direction = math::normalize_yaw(RAD2DEG(atan2(e->m_vecVelocity().y, e->m_vecVelocity().x)));
					const auto previous_direction = math::normalize_yaw(RAD2DEG(atan2(previous_velocity.y, previous_velocity.x)));

					auto real_velocity = record->velocity.Length2D();

					float delta = current_direction - previous_direction;

					if (delta <= 180.0f)
						if (delta <= -180.0f)
							delta = delta + 360;
					else
						delta = delta - 360;

					float v63 = delta * 0.5f + current_direction;

					auto direction = (v63 + 90.f) * 0.017453292f;

					e->m_vecVelocity().x = sinf(direction) * real_velocity;
					e->m_vecVelocity().y = cosf(direction) * real_velocity;
				}
			}

			if (!(record->flags & FL_ONGROUND))
			{
				velocity = (record->origin - previous_record->origin) / record->simulation_time;

				float_t flWeight = 1.0f - record->layers[ANIMATION_LAYER_ALIVELOOP].m_flWeight;

				if (flWeight > 0.0f)
				{
					float_t flPreviousRate = previous_record->layers[ANIMATION_LAYER_ALIVELOOP].m_flPlaybackRate;
					float_t flCurrentRate = record->layers[ANIMATION_LAYER_ALIVELOOP].m_flPlaybackRate;

					if (flPreviousRate == flCurrentRate)
					{
						int32_t iPreviousSequence = previous_record->layers[ANIMATION_LAYER_ALIVELOOP].m_nSequence;
						int32_t iCurrentSequence = record->layers[ANIMATION_LAYER_ALIVELOOP].m_nSequence;

						if (iPreviousSequence == iCurrentSequence)
						{
							float_t flSpeedNormalized = (flWeight / 2.8571432f) + 0.55f;

							if (flSpeedNormalized > 0.0f)
							{
								float_t flSpeed = flSpeedNormalized * e->GetMaxPlayerSpeed();

								if (flSpeed > 0.0f)
								{
									if (velocity.Length2D() > 0.0f)
									{
										velocity.x /= velocity.Length2D() / flSpeed;
										velocity.y /= velocity.Length2D() / flSpeed;
									}
								}
							}
						}
					}
				}

				static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));
				velocity.z -= sv_gravity->GetFloat() * 0.5f * TICKS_TO_TIME(record->simulation_time);
			}
			else
				velocity.z = 0.0f;
		}
	}

	const auto simtime_delta = e->m_flSimulationTime() - e->m_flOldSimulationTime();
	const auto choked_ticks = ((simtime_delta / m_globals()->m_intervalpertick) + 0.5);
	const auto simulation_tick_delta = choked_ticks - 2;
	const auto delta_ticks = (std::clamp(TIME_TO_TICKS(m_engine()->GetNetChannelInfo()->GetLatency(1) + m_engine()->GetNetChannelInfo()->GetLatency(0)) + m_globals()->m_tickcount - TIME_TO_TICKS(e->m_flSimulationTime() + util::get_interpolation()), 0, 100)) - simulation_tick_delta;

	if (delta_ticks > 0 && records->size() >= 2)
	{
		auto ticks_left = static_cast<int>(simulation_tick_delta);

		ticks_left = std::clamp(ticks_left, 1, 10);

		do
		{
			auto data_origin = record->origin;
			auto data_velocity = record->velocity;
			auto data_flags = record->flags;

			extrapolation(e, data_origin, data_velocity, data_flags, !(e->m_fFlags() & FL_ONGROUND));

			record->simulation_time += m_globals()->m_intervalpertick;
			record->origin = data_origin;
			record->velocity = data_velocity;
			--ticks_left;
		} while (ticks_left > 0);
	}

	e->m_iEFlags() &= ~0x1800;

	if (e->m_fFlags() & FL_ONGROUND && e->m_vecVelocity().Length() > 0.0f && animlayers[6].m_flWeight <= 0.0f)
		e->m_vecVelocity().Zero();

	e->m_vecAbsVelocity() = e->m_vecVelocity();
	e->m_bClientSideAnimation() = true;

	if (is_dormant[e->EntIndex()])
	{
		is_dormant[e->EntIndex()] = false;

		if (e->m_fFlags() & FL_ONGROUND)
		{
			animstate->m_bOnGround = true;
			animstate->m_bInHitGroundAnimation = false;
		}

		animstate->time_since_in_air() = 0.0f;
		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);
	}

	auto updated_animations = false;

	c_baseplayeranimationstate state;
	memcpy(&state, animstate, sizeof(c_baseplayeranimationstate));

	if (animstate->m_iLastClientSideAnimationUpdateFramecount >= m_globals()->m_tickcount)
		animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount - 1;

	if (animstate->m_iLastClientSideAnimationUpdateFramecount == m_globals()->m_framecount)
	{
		animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount - 1;
	}

	if (animstate->m_flLastClientSideAnimationUpdateTime == m_globals()->m_curtime)
	{
		animstate->m_flLastClientSideAnimationUpdateTime = m_globals()->m_curtime - m_globals()->m_intervalpertick;
	}

	if (previous_record)
	{
		memcpy(e->get_animlayers(), previous_record->layers, e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(&e->m_flPoseParameter(), pose_parametrs, 24 * sizeof(float));

		auto ticks_chocked = 1;
		auto simulation_ticks = TIME_TO_TICKS(e->m_flSimulationTime() - previous_record->simulation_time);

		if (simulation_ticks > 0 && simulation_ticks < 31)
			ticks_chocked = simulation_ticks;

		if (ticks_chocked > 1)
		{
			auto land_time = 0.0f;
			auto land_in_cycle = false;
			auto is_landed = false;
			auto on_ground = false;

			if (animlayers[4].m_flCycle < 0.5f && (!(e->m_fFlags() & FL_ONGROUND) || !(previous_record->flags & FL_ONGROUND)))
			{
				land_time = e->m_flSimulationTime() - animlayers[4].m_flPlaybackRate * animlayers[4].m_flCycle;
				land_in_cycle = land_time >= previous_record->simulation_time;
			}

			auto duck_amount_per_tick = (e->m_flDuckAmount() - previous_record->duck_amount) / ticks_chocked;

			for (auto i = 0; i < ticks_chocked; ++i)
			{
				auto lby_delta = fabs(math::normalize_yaw(e->m_flLowerBodyYawTarget() - previous_record->lby));

				if (lby_delta > 0.0f && e->m_vecVelocity().Length() < 5.0f)
				{
					auto delta = ticks_chocked - i;
					auto use_new_lby = true;

					if (lby_delta < 1.0f)
						use_new_lby = !delta;
					else
						use_new_lby = delta < 2;

					e->m_flLowerBodyYawTarget() = use_new_lby ? backup_lower_body_yaw_target : previous_record->lby;
				}

				auto simulated_time = previous_record->simulation_time + TICKS_TO_TIME(i);

				if (duck_amount_per_tick)
					e->m_flDuckAmount() = previous_record->duck_amount + duck_amount_per_tick * (float)i;

				on_ground = e->m_fFlags() & FL_ONGROUND;

				if (land_in_cycle && !is_landed)
				{
					if (land_time <= simulated_time)
					{
						is_landed = true;
						on_ground = true;
					}
					else
						on_ground = previous_record->flags & FL_ONGROUND;
				}

				if (on_ground)
					e->m_fFlags() |= FL_ONGROUND;
				else
					e->m_fFlags() &= ~FL_ONGROUND;

				auto simulated_ticks = TIME_TO_TICKS(simulated_time);

				m_globals()->m_realtime = simulated_time;
				m_globals()->m_curtime = simulated_time;
				m_globals()->m_framecount = simulated_ticks;
				m_globals()->m_tickcount = simulated_ticks;
				m_globals()->m_absoluteframetime = m_globals()->m_intervalpertick;
				m_globals()->m_interpolation_amount = 0.0f;

				g_ctx.globals.updating_animation = true;
				e->update_clientside_animation();
				g_ctx.globals.updating_animation = false;

				m_globals()->m_realtime = backup_realtime;
				m_globals()->m_curtime = backup_curtime;
				m_globals()->m_framecount = backup_framecount;
				m_globals()->m_tickcount = backup_tickcount;
				m_globals()->m_absoluteframetime = backup_absolute_frametime;
				m_globals()->m_interpolation_amount = backup_interpolation_amount;

				updated_animations = true;
			}
		}
	}

	if (!updated_animations)
	{
		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;
	}

	memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

	auto setup_matrix = [&](player_t* e, AnimationLayer* layers, const int& matrix) -> void
	{
		e->invalidate_physics_recursive(8);

		AnimationLayer backup_layers[13];
		memcpy(backup_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

		memcpy(e->get_animlayers(), layers, e->animlayer_count() * sizeof(AnimationLayer));

		switch (matrix)
		{
		case MAIN:
			e->setup_bones_fixed(record->matrixes_data.main, BONE_USED_BY_ANYTHING);
			break;
		case NONE:
			e->setup_bones_fixed(record->matrixes_data.zero, BONE_USED_BY_ANYTHING);
			break;
		case FIRST:
			e->setup_bones_fixed(record->matrixes_data.first, BONE_USED_BY_ANYTHING);
			break;
		case SECOND:
			e->setup_bones_fixed(record->matrixes_data.second, BONE_USED_BY_ANYTHING);
			break;
		case LOW_FIRST:
			e->setup_bones_fixed(record->matrixes_data.low_first, BONE_USED_BY_ANYTHING);
			break;
		case LOW_SECOND:
			e->setup_bones_fixed(record->matrixes_data.low_second, BONE_USED_BY_ANYTHING);
			break;
		}

		memcpy(e->get_animlayers(), backup_layers, e->animlayer_count() * sizeof(AnimationLayer));
	};

	if (previous_record)
	{
		const float curtime = m_globals()->m_curtime;
		const float frametime = m_globals()->m_frametime;

		m_globals()->m_frametime = m_globals()->m_intervalpertick;
		m_globals()->m_curtime = e->m_flSimulationTime();

		int backup_eflags = e->m_iEFlags();

		e->m_iEFlags() &= ~0x1000;
		e->m_vecAbsVelocity() = e->m_vecVelocity();

		if (animstate->m_iLastClientSideAnimationUpdateFramecount == m_globals()->m_frametime)
			animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_frametime - 1;

		e->m_bClientSideAnimation() = true;
		e->update_clientside_animation();
		e->m_bClientSideAnimation() = false;

		e->m_iEFlags() = backup_eflags;

		m_globals()->m_curtime = curtime;
		m_globals()->m_frametime = frametime;

		e->invalidate_bone_cache();
		e->SetupBones(nullptr, -1, 0x7FF00, m_globals()->m_curtime);
	}

	if (!player_info.fakeplayer && g_ctx.local()->is_alive() && e->m_iTeamNum() != g_ctx.local()->m_iTeamNum())
	{
		animstate->m_flGoalFeetYaw = previous_goal_feet_yaw[e->EntIndex()];

		g_ctx.globals.updating_animation = true;
		e->m_bClientSideAnimation() = true;
		e->update_clientside_animation();
		e->m_bClientSideAnimation() = false;
		g_ctx.globals.updating_animation = false;

		previous_goal_feet_yaw[e->EntIndex()] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);

		g_ctx.globals.updating_animation = true;
		e->m_bClientSideAnimation() = true;
		e->update_clientside_animation();
		e->m_bClientSideAnimation() = false;
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, NONE);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[0] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		memcpy(player_resolver[e->EntIndex()].resolver_layers[0], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + e->get_max_desync_delta());

		g_ctx.globals.updating_animation = true;
		e->m_bClientSideAnimation() = true;
		e->update_clientside_animation();
		e->m_bClientSideAnimation() = false;
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, FIRST);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[2] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(player_resolver[e->EntIndex()].resolver_layers[2], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - e->get_max_desync_delta());

		g_ctx.globals.updating_animation = true;
		e->m_bClientSideAnimation() = true;
		e->update_clientside_animation();
		e->m_bClientSideAnimation() = false;
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, SECOND);
		player_resolver[e->EntIndex()].resolver_goal_feet_yaw[1] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(player_resolver[e->EntIndex()].resolver_layers[1], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

		memcpy(&e->m_flPoseParameter(), pose_parametrs, 24 * sizeof(float));
		player_resolver[e->EntIndex()].initialize(e, record, previous_goal_feet_yaw[e->EntIndex()], e->m_angEyeAngles().x);
		player_resolver[e->EntIndex()].resolve_yaw(); // l3g5nd edition!

		if (g_cfg.player_list.low_delta[e->EntIndex()])
		{
			switch (record->side)
			{
			case RESOLVER_FIRST:
				record->side = RESOLVER_LOW_FIRST;
				break;
			case RESOLVER_SECOND:
				record->side = RESOLVER_LOW_SECOND;
				break;
			case RESOLVER_LOW_FIRST:
				record->side = RESOLVER_FIRST;
				break;
			case RESOLVER_LOW_SECOND:
				record->side = RESOLVER_SECOND;
				break;
			}
		}
		else
		{
			for (auto it = lagcompensation::get().player_sets.begin(); it != lagcompensation::get().player_sets.end(); ++it)
			{
				if (it->id == player_info.steamID64 && it->res_type == HISTORY_LOW)
				{
					switch (record->side)
					{
					case RESOLVER_FIRST:
						record->side = RESOLVER_LOW_FIRST;
						break;
					case RESOLVER_SECOND:
						record->side = RESOLVER_LOW_SECOND;
						break;
					case RESOLVER_LOW_FIRST:
						record->side = RESOLVER_FIRST;
						break;
					case RESOLVER_LOW_SECOND:
						record->side = RESOLVER_SECOND;
						break;
					}

					break;
				}
			}
		}

		switch (record->side)
		{
		case RESOLVER_ORIGINAL:
			animstate->m_flGoalFeetYaw = previous_goal_feet_yaw[e->EntIndex()];
			break;
		case RESOLVER_FIRST:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 40.0f);
			break;
		case RESOLVER_ZERO:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);
			break;
		case RESOLVER_SECOND:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 40.0f);
			break;
		case RESOLVER_LOW_FIRST:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 32.0f);
			break;
		case RESOLVER_LOW_SECOND:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 32.0f);
			break;
		case RESOLVER_HIGH_FIRST:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 58.0f);
			break;
		case RESOLVER_HIGH_SECOND:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 58.0f);
			break;
		case RESOLVER_BULLSHIT_FIRST:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(rand() % 45);
			break;
		case RESOLVER_BULLSHIT_SECOND:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(rand() % 45);
			break;
		case RESOLVER_SUPER_LOW_FIRST:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 20);
			break;
		case RESOLVER_SUPER_LOW_SECOND:
			animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 20);
			break;
		}

		e->m_angEyeAngles().x = player_resolver[e->EntIndex()].resolve_pitch();
	}

	g_ctx.globals.updating_animation = true;
	e->m_bClientSideAnimation() = true;
	e->update_clientside_animation();
	e->m_bClientSideAnimation() = false;
	g_ctx.globals.updating_animation = false;

	upd_nw(e);

	setup_matrix(e, animlayers, MAIN);
	memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.main, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

	setup_matrix(e, animlayers, FIRST);
	memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.first, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

	setup_matrix(e, animlayers, SECOND);
	memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.second, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

	m_globals()->m_curtime = backup_curtime;
	m_globals()->m_frametime = backup_frametime;

	e->m_flLowerBodyYawTarget() = backup_lower_body_yaw_target;
	e->m_flDuckAmount() = backup_duck_amount;
	e->m_fFlags() = backup_flags;
	e->m_iEFlags() = backup_eflags;

	memcpy(e->get_animlayers(), animlayers, e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(player_resolver[e->EntIndex()].previous_layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	record->store_data(e, false);

	if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
		record->invalid = true;
}

//void lagcompensation::PVSFix(ClientFrameStage_t stage)
//{
//	if (stage != FRAME_RENDER_START)
//		return;
//
//	for (int i = 1; i <= m_globals()->m_maxclients; i++)
//	{
//		auto local = g_ctx.local();
//		auto local_idx = local->EntIndex();
//
//		if (i == local_idx) continue;
//
//		IClientEntity* pCurEntity = m_entitylist()->GetClientEntity(i);
//		if (!pCurEntity) continue;
//
//		*(int*)((uintptr_t)pCurEntity + 0xA30) = m_globals()->m_framecount; //we'll skip occlusion checks now
//		*(int*)((uintptr_t)pCurEntity + 0xA28) = 0;//clear occlusion flags
//	}
//}