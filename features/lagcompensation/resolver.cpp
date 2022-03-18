// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "../lagcompensation/animation_system.h"
#include "..\ragebot\aim.h"

float calculateSD(float data[])
{
	float delta_y = 0.0, mean, standardDeviation = 0.0;
	int i;
	for (i = 0; i < 10; ++i)
	{
		delta_y += data[i];
	}
	mean = delta_y / 10;
	for (i = 0; i < 10; ++i)
		standardDeviation += pow(data[i] - mean, 2);
	return sqrt(standardDeviation / 10);
}

bool resolver::can_shoot_antiaim(player_t* ent)
{

	if (!ent || ent->m_iHealth() <= 0 || ent->m_flNextAttack() > m_globals()->m_curtime)
		return false;

	weapon_t* weapon = ent->m_hActiveWeapon();
	if (!weapon)
		return false;

	if (weapon->is_grenade())
	{
		if (weapon->m_bPinPulled())
			return false;

		if (weapon->m_fThrowTime() <= 0 || weapon->m_fThrowTime() > m_globals()->m_curtime)
			return false;

		return true;
	}

	if (weapon->m_flNextPrimaryAttack() > m_globals()->m_curtime)
		return false;

	/// revolver
	if (weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && weapon->m_flPostponeFireReadyTime() > m_globals()->m_curtime)
		return false;


	return true;

}

void resolver::initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
	player = e;
	player_record = record;

	original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);
	original_pitch = math::normalize_pitch(pitch);
}

void resolver::reset()
{
	player = nullptr;
	player_record = nullptr;

	side = false;
	fake = false;

	was_first_bruteforce = false;
	was_second_bruteforce = false;

	was_first_lowdelta = false;
	was_second_lowdelta = false;

	original_goal_feet_yaw = 0.0f;
	original_pitch = 0.0f;

}

void resolver::resolve_yaw()
{
	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(player->EntIndex(), &player_info))
		return;

	if (!player || !player_record)

		if (player_info.fakeplayer || !g_ctx.local()->is_alive() || player->m_iTeamNum() == g_ctx.local()->m_iTeamNum()) //-V807
		{
			return;
		}

	if (!can_shoot_antiaim(player))
	{
		player_record->type = ORIGINAL;
		player_record->side = RESOLVER_ORIGINAL;
		return;
	}

	if (g_ctx.globals.missed_shots[player->EntIndex()] >= g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].max_misses || g_ctx.globals.missed_shots[player->EntIndex()] && aim::get().last_target[player->EntIndex()].record.type != LBY)
	{
		switch (last_side)
		{
		case RESOLVER_ORIGINAL:
			player_record->type = BRUTEFORCE;
			g_ctx.globals.missed_shots[player->EntIndex()] = 0;
			fake = true;
			break;
		case RESOLVER_ZERO:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_LOW_FIRST;

			was_first_bruteforce = false;
			was_second_bruteforce = false;
			return;
		case RESOLVER_FIRST:
			player_record->type = BRUTEFORCE;
			player_record->side = was_second_bruteforce ? RESOLVER_FIRST : RESOLVER_LOW_FIRST;

			was_first_bruteforce = true;
			return;
		case RESOLVER_SECOND:
			player_record->type = BRUTEFORCE;
			player_record->side = was_first_bruteforce ? RESOLVER_SECOND : RESOLVER_LOW_SECOND;

			was_second_bruteforce = true;
			return;
		case RESOLVER_LOW_FIRST:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_LOW_SECOND;
			return;
		case RESOLVER_LOW_SECOND:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_FIRST;
			return;
		case RESOLVER_HIGH_FIRST:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_HIGH_FIRST;
			return;
		case RESOLVER_HIGH_SECOND:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_HIGH_SECOND;
			return;
		case RESOLVER_BULLSHIT_FIRST:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_BULLSHIT_FIRST;
			return;
		case RESOLVER_BULLSHIT_SECOND:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_BULLSHIT_SECOND;
			return;
		case RESOLVER_SUPER_LOW_FIRST:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_SUPER_LOW_FIRST;
			return;
		case RESOLVER_SUPER_LOW_SECOND:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_SUPER_LOW_SECOND;
			return;
		case CUSTOM_OVERRIDE:
			player_record->type = MOVEMENT;
		}
	}

	auto animstate = player->get_animation_state();
	

	if (!animstate)
	{
		player_record->side = RESOLVER_ORIGINAL;
		return;
	}

	if (!animstate->m_flTimeSinceStoppedMoving > animstate->m_flTimeSinceStartedMoving && g_ctx.globals.missed_shots[player->EntIndex()] > 2) // if they've been sitting around for a bit
	{

		// assume they're crouched right (like an absolute nerd)

		if (animstate->m_flCurrentFeetYaw > animstate->m_flCurrentTorsoYaw) // if their feet's yaw is greater than their torso yaw they're doing something funky asf
		{

			int randomint = rand() % 2;

			switch (randomint)
			{

			case 1:

				player_record->type = MOVEMENT;
				player_record->side = RESOLVER_HIGH_FIRST;
				return;

			case 2:

				player_record->type = MOVEMENT;
				player_record->side = RESOLVER_HIGH_SECOND;
				return;

			}

		}

	}

	else if (g_ctx.globals.missed_shots[player->EntIndex()] > 2)// they're currently moving or sum idfk (moving = low delta lol)
	{

		int randomint = rand() % 4;

		switch (randomint)
		{

		case 1:

			player_record->type = MOVEMENT;
			player_record->side = RESOLVER_LOW_FIRST;
			return;

		case 2:

			player_record->type = MOVEMENT;
			player_record->side = RESOLVER_LOW_SECOND;
			return;

		case 3:

			player_record->type = MOVEMENT;
			player_record->side = RESOLVER_SUPER_LOW_SECOND;
			return;

		case 4:

			player_record->type = MOVEMENT;
			player_record->side = RESOLVER_SUPER_LOW_FIRST;
			return;

		}

	} // if they aren't doing any of that crap, which they most likely aren't (btw this is prolly not gonna work, as that statement covers like everything soooo)

	auto delta = math::normalize_yaw(player->m_angEyeAngles().y - original_goal_feet_yaw);
	auto valid_lby = true;

	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.f)
		valid_lby = animstate->m_flTimeSinceStartedMoving < 0.22f;

	if (fabs(delta) > 30.0f && valid_lby)
	{
		if (g_ctx.globals.missed_shots[player->EntIndex()])
			delta = -delta;

		if (delta > 30.0f)
		{
			player_record->type = LBY;

			if (!was_first_lowdelta)
			{
				player_record->side = RESOLVER_SUPER_LOW_FIRST;
				was_first_lowdelta = true;
				was_second_lowdelta = false;
			}
			else
			{
				player_record->side = RESOLVER_LOW_FIRST;
				was_first_lowdelta = true;
				was_second_lowdelta = false;
			}



		}
		else if (delta < -30.0f)
		{
			player_record->type = LBY;

			if (!was_first_lowdelta)
			{
				player_record->side = RESOLVER_SUPER_LOW_SECOND;
				was_first_lowdelta = true;
				was_second_lowdelta = false;
			}
			else
			{
				player_record->side = RESOLVER_LOW_SECOND;
				was_second_lowdelta = true;
				was_first_lowdelta = false;
			}

		}
	}
	else
	{
		auto trace = false;

		if (m_globals()->m_curtime - lock_side > 2.0f)
		{
			auto first_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first), player, g_ctx.local());
			auto second_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second), player, g_ctx.local());

			if (first_visible != second_visible)
			{
				trace = true;
				side = second_visible;
				lock_side = m_globals()->m_curtime;
			}
			else
			{
				auto first_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first));
				auto second_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second));

				if (fabs(first_position - second_position) > 1.0f)
					side = first_position > second_position;
			}
		}
		else
			trace = true;

		if (side)
		{
			player_record->type = trace ? TRACE : DIRECTIONAL;
			player_record->side = trace ? RESOLVER_HIGH_FIRST : RESOLVER_FIRST;
		}
		else
		{
			player_record->type = trace ? TRACE : DIRECTIONAL;
			player_record->side = trace ? RESOLVER_HIGH_SECOND : RESOLVER_SECOND;
		}
	}

}

float resolver::resolve_pitch()
{
	return original_pitch;
}