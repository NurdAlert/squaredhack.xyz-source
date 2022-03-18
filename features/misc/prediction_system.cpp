#include "prediction_system.h"

void engineprediction::FixNetvarCompression(int time)
{
	auto data = &netvars_data[time % MULTIPLAYER_BACKUP];

	if (data->tickbase != g_ctx.local()->m_nTickBase())
		return;

	data->tickbase = g_ctx.local()->m_nTickBase();
	data->m_vecOrigin = g_ctx.local()->m_vecOrigin();

	data->m_aimPunchAngle = g_ctx.local()->m_aimPunchAngle();
	data->m_aimPunchAngleVel = g_ctx.local()->m_aimPunchAngleVel();

	data->m_viewPunchAngle = g_ctx.local()->m_viewPunchAngle();
	data->m_vecViewOffset = g_ctx.local()->m_vecViewOffset();

	data->m_flDuckAmount = g_ctx.local()->m_flDuckAmount();
	data->m_flDuckSpeed = g_ctx.local()->m_flDuckSpeed();

	data->m_flFallVelocity = g_ctx.local()->m_flFallVelocity();
	data->m_flVelocityModifier = g_ctx.local()->m_flVelocityModifier();

	const auto aim_punch_vel_diff = data->m_aimPunchAngleVel - g_ctx.local()->m_aimPunchAngleVel();
	const auto aim_punch_diff = data->m_aimPunchAngle - g_ctx.local()->m_aimPunchAngle();
	const auto viewpunch_diff = data->m_viewPunchAngle.x - g_ctx.local()->m_viewPunchAngle().x;
	const auto velocity_diff = data->m_vecVelocity - g_ctx.local()->m_vecVelocity();
	const auto origin_diff = data->m_vecOrigin - g_ctx.local()->m_vecOrigin();

	if (std::abs(aim_punch_diff.x) <= 0.03125f && std::abs(aim_punch_diff.y) <= 0.03125f && std::abs(aim_punch_diff.z) <= 0.03125f)
		g_ctx.local()->m_aimPunchAngle() = data->m_aimPunchAngle;

	if (std::abs(aim_punch_vel_diff.x) <= 0.03125f && std::abs(aim_punch_vel_diff.y) <= 0.03125f && std::abs(aim_punch_vel_diff.z) <= 0.03125f)
		g_ctx.local()->m_aimPunchAngleVel() = data->m_aimPunchAngleVel;

	if (std::abs(g_ctx.local()->m_vecViewOffset().z - data->m_vecViewOffset.z) <= 0.25f)
		g_ctx.local()->m_vecViewOffset().z = data->m_vecViewOffset.z;

	if (std::abs(viewpunch_diff) <= 0.03125f)
		g_ctx.local()->m_viewPunchAngle().x = data->m_viewPunchAngle.x;

	if (abs(g_ctx.local()->m_flDuckAmount() - data->m_flDuckAmount) <= 0.03125f)
		g_ctx.local()->m_flDuckAmount() = data->m_flDuckAmount;

	if (std::abs(velocity_diff.x) <= 0.03125f && std::abs(velocity_diff.y) <= 0.03125f && std::abs(velocity_diff.z) <= 0.03125f)
		g_ctx.local()->m_vecVelocity() = data->m_vecVelocity;

	if (abs(origin_diff.x) <= 0.03125f && abs(origin_diff.y) <= 0.03125f && abs(origin_diff.z) <= 0.03125f) {
		g_ctx.local()->m_vecOrigin() = data->m_vecOrigin;
		g_ctx.local()->set_abs_origin(data->m_vecOrigin);
	}

	if (abs(g_ctx.local()->m_flDuckSpeed() - data->m_flDuckSpeed) <= 0.03125f)
		g_ctx.local()->m_flDuckSpeed() = data->m_flDuckSpeed;

	if (abs(g_ctx.local()->m_flFallVelocity() - data->m_flFallVelocity) <= 0.03125f)
		g_ctx.local()->m_flFallVelocity() = data->m_flFallVelocity;

	if (std::abs(g_ctx.local()->m_flVelocityModifier() - data->m_flVelocityModifier) <= 0.00625f)
		g_ctx.local()->m_flVelocityModifier() = data->m_flVelocityModifier;
}

void engineprediction::reset_data()
{
	m_data.fill(StoredData_t());
}

void engineprediction::setup()
{
	if (prediction_data.prediction_stage != SETUP)
		return;

	if (!prediction_data.sv_footsteps)
		prediction_data.sv_footsteps = m_cvar()->FindVar(crypt_str("sv_footsteps"));

	if (!prediction_data.sv_min_jump_landing_sound)
		prediction_data.sv_min_jump_landing_sound = m_cvar()->FindVar(crypt_str("sv_min_jump_landing_sound"));

	backup_data.flags = g_ctx.local()->m_fFlags(); //-V807
	backup_data.velocity = g_ctx.local()->m_vecVelocity();

	backup_data.sv_footsteps_backup = *(float*)(uintptr_t(prediction_data.sv_footsteps) + 0x2C);
	backup_data.sv_min_jump_landing_sound_backup = *(float*)(uintptr_t(prediction_data.sv_min_jump_landing_sound) + 0x2C);

	prediction_data.old_curtime = m_globals()->m_curtime; //-V807
	prediction_data.old_frametime = m_globals()->m_frametime;

	m_prediction()->InPrediction = true;
	m_prediction()->IsFirstTimePredicted = false;

	m_globals()->m_curtime = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);
	m_globals()->m_frametime = m_prediction()->EnginePaused ? 0.0f : m_globals()->m_intervalpertick;

	prediction_data.prediction_stage = PREDICT;
}

void engineprediction::OnRunCommand()
{
	player_t* player;

	if (!g_ctx.local() || g_ctx.local() != player)
		return;

	auto data = &netvars_data[(m_clientstate()->iCommandAck - 1) % MULTIPLAYER_BACKUP];

	data->m_aimPunchAngle = g_ctx.local()->m_aimPunchAngle();
	data->m_vecViewOffset = g_ctx.local()->m_vecViewOffset();
	data->m_vecVelocity = g_ctx.local()->m_vecVelocity();
	data->m_aimPunchAngleVel = g_ctx.local()->m_aimPunchAngleVel();
	data->m_flDuckAmount = g_ctx.local()->m_flDuckAmount();
	data->m_duckSpeed = g_ctx.local()->m_flDuckSpeed();
	data->m_viewPunchAngle = g_ctx.local()->m_viewPunchAngle();
	data->m_flFallVelocity = g_ctx.local()->m_flFallVelocity();
	data->m_flVelocityModifier = g_ctx.local()->m_flVelocityModifier();
	data->tickbase = g_ctx.local()->m_nTickBase();

	if (auto wpn = g_ctx.local()->m_hActiveWeapon(); wpn != nullptr)
	{
		wpn->m_fAccuracyPenalty() = data->m_flThirdpersonRecoil;
		wpn->m_flRecoilSeed() = data->m_flThirdpersonRecoil;
	}
}

void engineprediction::update_incoming_sequences()
{
	if (!m_clientstate()->pNetChannel)
		return;

	// store new stuff.
	if (m_sequence.empty() || m_clientstate()->pNetChannel->m_nInSequenceNr > m_sequence.front().m_seq) 
		m_sequence.emplace_front(m_globals()->m_realtime, m_clientstate()->pNetChannel->m_nInReliableState, m_clientstate()->pNetChannel->m_nInSequenceNr);

	// do not save too many of these.
	while (m_sequence.size() > 2048)
		m_sequence.pop_back();
}

void __stdcall hkSetupMove(player_t* m_pPlayer, CUserCmd* m_pCmd, CMoveData* m_pMoveData)
{
	if (!m_pPlayer)
		return;

	if (!(m_pPlayer->m_fFlags() & FL_ONGROUND) || !(m_pCmd->m_buttons & IN_JUMP))
		return;

	m_pPlayer->m_vecVelocity() *= Vector(1.2f, 1.2f, 1.f);

	m_pMoveData->m_vecVelocity = m_pPlayer->m_vecVelocity();
}

void engineprediction::update_vel()
{
	static int m_iLastCmdAck = 0;
	static float m_flNextCmdTime = 0.f;

	if (m_clientstate() && (m_iLastCmdAck != m_clientstate()->nLastCommandAck || m_flNextCmdTime != m_clientstate()->flNextCmdTime))
	{
		if (g_ctx.globals.last_velocity_modifier != g_ctx.local()->m_flVelocityModifier())
		{
			*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x24)) = 1;
			g_ctx.globals.last_velocity_modifier = g_ctx.local()->m_flVelocityModifier();
		}

		m_iLastCmdAck = m_clientstate()->nLastCommandAck;
		m_flNextCmdTime = m_clientstate()->flNextCmdTime;
	}
}

float engineprediction::GetSpread()
{
	return m_spread;
}

float engineprediction::GetInaccuracy()
{
	return m_inaccuracy;
}

void engineprediction::predict(CUserCmd* m_pcmd)
{
	if (prediction_data.prediction_stage != PREDICT)
		return;

	bool prediction_need_to_recount = false;

	if (m_clientstate()->iDeltaTick > 0) {
		m_prediction()->Update(
			m_clientstate()->iDeltaTick,
			m_clientstate()->iDeltaTick > 1,
			m_clientstate()->nLastCommandAck,
			m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands);
	}

	if (prediction_need_to_recount) { // predict recount compression.
		*(int*)((uintptr_t)m_prediction() + 0x1C) = 0;
		*(bool*)((uintptr_t)m_prediction() + 0x24) = true;
	}

	// backup footsteps.
	const auto backup_footsteps = m_cvar()->FindVar(crypt_str("sv_footsteps"))->GetInt();
	m_cvar()->FindVar(crypt_str("sv_footsteps"))->m_nFlags &= ~(1 << 14);
	m_cvar()->FindVar(crypt_str("sv_footsteps"))->m_nFlags &= ~(1 << 8);
	m_cvar()->FindVar(crypt_str("sv_footsteps"))->SetValue(0);
	float value = 0.0f;

	if (prediction_data.sv_footsteps)
		*(float*)(uintptr_t(prediction_data.sv_footsteps) + 0x2C) = (uint32_t)prediction_data.sv_footsteps ^ uint32_t(value);

	if (prediction_data.sv_min_jump_landing_sound)
		*(float*)(uintptr_t(prediction_data.sv_min_jump_landing_sound) + 0x2C) = (uint32_t)prediction_data.sv_min_jump_landing_sound ^ 0x7F7FFFFF;

	int m_nimpulse = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_nImpulse"));
	int get_buttons = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_nButtons"));
	int get_buttons_last = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_afButtonLast"));
	int get_buttons_pressed = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_afButtonPressed"));
	int get_buttons_released = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_afButtonReleased"));

	prediction_data.m_nServerCommandsAcknowledged = *(int*)(uintptr_t(m_prediction()) + 0x20);
	prediction_data.m_bInPrediction = *(bool*)(uintptr_t(m_prediction()) + 8);

	*reinterpret_cast<CUserCmd**>(reinterpret_cast<uintptr_t>(g_ctx.local()) + 0x3288) = m_pcmd;

	if (!prediction_data.prediction_random_seed)
		prediction_data.prediction_random_seed = *reinterpret_cast <int**> (util::FindSignature(crypt_str("client.dll"), crypt_str("A3 ? ? ? ? 66 0F 6E 86")) + 0x1);

	*prediction_data.prediction_random_seed = MD5_PseudoRandom(m_pcmd->m_command_number) & INT_MAX;

	if (!prediction_data.prediction_player)
		prediction_data.prediction_player = *reinterpret_cast <int**> (util::FindSignature(crypt_str("client.dll"), crypt_str("89 35 ? ? ? ? F3 0F 10 48")) + 0x2);

	*prediction_data.prediction_player = reinterpret_cast <int> (g_ctx.local());

	auto buttons_forced = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(g_ctx.local()) + 0x3334);
	auto buttons_disabled = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(g_ctx.local()) + 0x3330);

	m_pcmd->m_buttons |= buttons_forced;
	m_pcmd->m_buttons &= ~buttons_disabled;

	m_prediction()->InPrediction = true;
	m_movehelper()->set_host(g_ctx.local());
	m_gamemovement()->StartTrackPredictionErrors(g_ctx.local());

	static auto m_nImpulse = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_nImpulse"));
	static auto m_nButtons = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_nButtons"));

	if (m_pcmd->m_impulse)
		*reinterpret_cast<uint32_t*>(uint32_t(g_ctx.local()) + m_nImpulse) = m_pcmd->m_impulse;
	if (m_pcmd->m_buttons)
		*reinterpret_cast<uint32_t*>(uint32_t(g_ctx.local()) + m_nButtons) = m_pcmd->m_buttons;

	entity_t* vehicle = nullptr;

	if (g_ctx.local()->m_hVehicle().IsValid())
		vehicle = (entity_t*)g_ctx.local()->m_hVehicle().Get();

	if (m_pcmd->m_impulse && (!vehicle || g_ctx.local()->using_standard_weapons_in_vechile()))
		*reinterpret_cast <uint32_t*> (reinterpret_cast <uint32_t> (g_ctx.local()) + 0x31FC) = m_pcmd->m_impulse;

	m_pcmd->m_buttons |= *reinterpret_cast<uint32_t*>(reinterpret_cast<uint32_t>(g_ctx.local()) + 0x3330);

	Vector unpred_viewangles;
	Vector unpred_vel;
	Vector pred_vel;

	static auto oldorigin = g_ctx.local()->GetAbsOrigin();

	unpred_vel = (g_ctx.local()->GetAbsOrigin() - oldorigin) * (1.0 / m_globals()->m_intervalpertick);
	unpred_viewangles = m_pcmd->m_viewangles;

	m_gamemovement()->StartTrackPredictionErrors(g_ctx.local()); //-V807
	m_movehelper()->set_host(g_ctx.local());

	CMoveData move_data;
	
	if (FL_ONGROUND == 0)
	{
		static int m_iLastCmdAck = 0;
		static float m_flNextCmdTime = 0.f;

		if (m_clientstate() && (m_iLastCmdAck != m_clientstate()->nLastCommandAck || m_flNextCmdTime != m_clientstate()->flNextCmdTime))
		{
			if (g_ctx.globals.last_velocity_modifier != g_ctx.local()->m_flVelocityModifier())
			{
				*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x24)) = 1;
				g_ctx.globals.last_velocity_modifier = g_ctx.local()->m_flVelocityModifier();
			}

			m_iLastCmdAck = m_clientstate()->nLastCommandAck;
			m_flNextCmdTime = m_clientstate()->flNextCmdTime;
		}
	}

	move_data.m_flForwardMove = m_pcmd->m_forwardmove;
	move_data.m_flSideMove = m_pcmd->m_sidemove;
	move_data.m_flUpMove = m_pcmd->m_upmove;
	move_data.m_nButtons = m_pcmd->m_buttons;
	move_data.m_vecViewAngles = m_pcmd->m_viewangles;
	move_data.m_vecAngles = m_pcmd->m_viewangles;
	move_data.m_nImpulseCommand = m_pcmd->m_impulse;

	memset(&move_data, 0, sizeof(CMoveData));

	if (g_ctx.globals.weapon)
	{
		g_ctx.globals.weapon->update_accuracy_penality();
		m_spread = g_ctx.globals.weapon->get_spread_virtual();
		m_inaccuracy = g_ctx.globals.weapon->get_inaccuracy_virtual();

		m_cvar()->FindVar(crypt_str("sv_footsteps"))->SetValue(backup_footsteps);

		g_ctx.globals.inaccuracy = g_ctx.globals.weapon->get_inaccuracy();
		g_ctx.globals.spread = g_ctx.globals.weapon->get_spread();

		auto is_special_weapon = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08;
		auto inaccuracy = g_ctx.globals.weapon->get_csweapon_info()->flInaccuracyStand;

		if (m_pcmd->m_weaponselect)
		{
			auto weapon = reinterpret_cast<weapon_t*>(m_entitylist()->GetClientEntity(m_pcmd->m_weaponselect));

			if (weapon)
			{
				auto weapon_data = weapon->get_csweapon_info();
				weapon_data ? g_ctx.local()->select_item(weapon_data->szWeaponName, m_pcmd->m_weaponsubtype) : 0;
			}
		}

		if (g_ctx.local()->m_fFlags() & FL_DUCKING)
		{
			if (is_special_weapon)
				g_ctx.globals.weap_inaccuracy = g_ctx.globals.weapon->get_csweapon_info()->flInaccuracyCrouchAlt;
			else
				g_ctx.globals.weap_inaccuracy = g_ctx.globals.weapon->get_csweapon_info()->flInaccuracyCrouch;
		}
		else if (is_special_weapon)
			g_ctx.globals.weap_inaccuracy = g_ctx.globals.weapon->get_csweapon_info()->flInaccuracyStandAlt;
	}

	auto vehicle_handle = g_ctx.local()->m_hVehicle();
	auto vehicle2 = vehicle_handle.IsValid() ? reinterpret_cast<entity_t*>(vehicle_handle.Get()) : nullptr;

	if (m_pcmd->m_impulse
		&& (!vehicle2 || g_ctx.local()->using_standard_weapons_in_vechile()))
		m_nimpulse = m_pcmd->m_impulse;

	auto buttons = m_pcmd->m_buttons;
	auto buttons_changed = buttons ^ get_buttons;

	get_buttons_last = get_buttons;
	get_buttons = buttons;
	get_buttons_pressed = buttons_changed & buttons;
	get_buttons_released = buttons_changed & ~buttons;

	m_prediction()->CheckMovingGround(g_ctx.local(), m_globals()->m_frametime);

	g_ctx.local()->physics_run_think(0) ? g_ctx.local()->pre_think() : 0;

	if (g_ctx.local()->get_next_think_tick() && g_ctx.local()->get_next_think_tick() != -1 && g_ctx.local()->get_next_think_tick() <= g_ctx.local()->m_nTickBase()) 
	{
		g_ctx.local()->get_next_think_tick() = -1;
		g_ctx.local()->think();
	}

	if (g_ctx.local()->physics_run_think(0))
		g_ctx.local()->pre_think();

	auto thinktick = reinterpret_cast <int*> (reinterpret_cast <uint32_t> (g_ctx.local()) + 0x0FC);

	if (*thinktick > 0 && *thinktick <= g_ctx.local()->m_nTickBase())
	{
		*thinktick = -1;

		static auto fn = reinterpret_cast <void(__thiscall*)(int)> (util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B")));
		fn(0);

		g_ctx.local()->think();
	}

	if (m_pcmd != m_pcmd)
	{
		m_pcmd->m_aimdirection.y = m_pcmd->m_aimdirection.y;
		m_pcmd->m_aimdirection.z = m_pcmd->m_aimdirection.z;
		m_pcmd->m_aimdirection.x = m_pcmd->m_aimdirection.x;
		m_pcmd->m_command_number = m_pcmd->m_command_number;
		m_pcmd->m_tickcount = m_pcmd->m_tickcount;
		m_pcmd->m_viewangles.x = m_pcmd->m_viewangles.x;
		m_pcmd->m_viewangles.y = m_pcmd->m_viewangles.y;
		m_pcmd->m_viewangles.z = m_pcmd->m_viewangles.z;

		m_pcmd->m_forwardmove = m_pcmd->m_forwardmove;
		m_pcmd->m_sidemove = m_pcmd->m_sidemove;
		m_pcmd->m_upmove = m_pcmd->m_upmove;
		m_pcmd->m_buttons = m_pcmd->m_buttons;
		m_pcmd->m_impulse = m_pcmd->m_impulse;
		m_pcmd->m_weaponselect = m_pcmd->m_weaponselect;
		m_pcmd->m_weaponsubtype = m_pcmd->m_weaponsubtype;
		m_pcmd->m_random_seed = m_pcmd->m_random_seed;
		m_pcmd->m_mousedx = m_pcmd->m_mousedx;
		m_pcmd->m_mousedy = m_pcmd->m_mousedy;
		m_pcmd->m_predicted = m_pcmd->m_predicted;
		m_pcmd->m_forwardmove = m_pcmd->m_forwardmove;

	}

	float m_tickbase = 0.0f;

	if (!m_pcmd || m_pcmd->m_predicted)
		m_tickbase = g_ctx.local()->m_nTickBase();
	else
		++m_tickbase;

	m_prediction()->SetupMove(g_ctx.local(), m_pcmd, m_movehelper(), &move_data);
	m_gamemovement()->ProcessMovement(g_ctx.local(), &move_data);
	m_prediction()->FinishMove(g_ctx.local(), m_pcmd, &move_data);

	m_movehelper()->process_impacts();

	m_gamemovement()->FinishTrackPredictionErrors(g_ctx.local());
	m_movehelper()->set_host(nullptr);

	if (prediction_data.sv_footsteps)
		*(float*)(uintptr_t(prediction_data.sv_footsteps) + 0x2C) = backup_footsteps;

	this->OnRunCommand();

	if (!m_pcmd || m_pcmd->m_predicted)
		m_tickbase = g_ctx.local()->m_nTickBase();
	else
		++m_tickbase;

	auto viewmodel = g_ctx.local()->m_hViewModel().Get();
	viewmodel_data.weapon = 0;
	viewmodel->m_hWeapon() = 0;

	if (viewmodel)
	{
		this->viewmodel_data.weapon = viewmodel->m_hWeapon();
		this->viewmodel_data.viewmodel_index = viewmodel->m_nViewModelIndex();
		this->viewmodel_data.sequence = viewmodel->m_nSequence();

		this->viewmodel_data.cycle = viewmodel->m_flCycle();
		this->viewmodel_data.animation_parity = viewmodel->m_nAnimationParity();
		this->viewmodel_data.animation_time = viewmodel->m_flAnimTime();
	}

	prediction_data.prediction_stage = FINISH;
}

void engineprediction::fix_attack_packet(CUserCmd* m_pCmd, bool m_bPredict)
{
	static bool m_bLastAttack = false;
	static bool m_bInvalidCycle = false;
	static float m_flLastCycle = 0.f;

	if (!g_ctx.local())
		return;

	if (m_bPredict)
	{
		m_bLastAttack = m_pCmd->m_weaponselect || (m_pCmd->m_buttons & IN_ATTACK2);
		m_flLastCycle = g_ctx.local()->m_flCycle();
	}
	else if (m_bLastAttack && !m_bInvalidCycle)
		m_bInvalidCycle = g_ctx.local()->m_flCycle() == 0.f && m_flLastCycle > 0.f;

	if (m_bInvalidCycle)
		g_ctx.local()->m_flCycle() = m_flLastCycle;
};

void engineprediction::finish()
{
	if (prediction_data.prediction_stage != FINISH)
		return;

	m_gamemovement()->FinishTrackPredictionErrors(g_ctx.local());
	m_movehelper()->set_host(nullptr);

	*prediction_data.prediction_random_seed = -1;
	*prediction_data.prediction_player = 0;

	*(int*)(uintptr_t(m_prediction()) + 0x20) = prediction_data.m_nServerCommandsAcknowledged; //m_bPreviousAckHadErrors
	*(bool*)(uintptr_t(m_prediction()) + 8) = prediction_data.m_bInPrediction; // m_bInPrediction

	m_globals()->m_curtime = prediction_data.old_curtime;
	m_globals()->m_frametime = prediction_data.old_frametime;
}