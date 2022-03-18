// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "../ragebot/aim.h"
#include "fakelag.h"
#include "../exploit/exploit.h"
#include "prediction_system.h"
#include "logs.h"

void fakelag::Fakelag(CUserCmd* m_pcmd)
{
	if (g_cfg.antiaim.fakelag && !g_ctx.globals.exploits)
	{
		static auto force_choke = false;

		if (force_choke)
		{
			force_choke = false;
			g_ctx.send_packet = false;
			return;
		}

		if (g_ctx.local()->m_fFlags() & FL_ONGROUND && !(engineprediction::get().backup_data.flags & FL_ONGROUND))
		{
			force_choke = true;
			g_ctx.send_packet = false;
			return;
		}
	}

	//static Vector oldOrigin;

	//if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND))
	//{
	//	g_ctx.canDrawLC = true;
	//}
	//else
	//{
	//	g_ctx.canDrawLC = false;
	//	g_ctx.canBreakLC = false;
	//}

	static auto fluctuate_ticks = 0;
	static auto switch_ticks = false;
	static auto random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);

	auto choked = m_clientstate()->iChokedCommands; //-V807
	auto flags = engineprediction::get().backup_data.flags; //-V807
	auto velocity = engineprediction::get().backup_data.velocity.Length(); //-V807
	auto velocity2d = engineprediction::get().backup_data.velocity.Length2D();

	auto max_speed = 260.0f;
	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

	if (weapon_info)
		max_speed = g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

	if (g_cfg.antiaim.fakelag)
		max_choke = g_cfg.antiaim.triggers_fakelag_amount;

	if (m_gamerules()->m_bIsValveDS()) //-V807
		max_choke = m_engine()->IsVoiceRecording() ? 1 : min(max_choke, 6);

	if (exploit::get().recharging_double_tap)
		max_choke = g_ctx.globals.weapon->get_max_tickbase_shift();

	if (g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND && !m_gamerules()->m_bIsValveDS() && key_binds::get().get_key_bind_state(20)) //-V807
	{
		max_choke = 14;

		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
			g_ctx.send_packet = true;
	}
	else
	{
		if (g_cfg.ragebot.enable && g_ctx.globals.current_weapon != -1 && !g_ctx.globals.exploits && g_cfg.antiaim.fakelag && g_cfg.antiaim.fakelag_enablers[FAKELAG_PEEK] && g_cfg.antiaim.triggers_fakelag_amount > 6 && !started_peeking && velocity >= 5.0f)
		{
			auto predicted_eye_pos = g_ctx.globals.eye_pos + engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * (float)g_cfg.antiaim.triggers_fakelag_amount * 0.5f;

			for (auto i = 1; i < m_globals()->m_maxclients; i++)
			{
				auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

				if (!e->valid(true))
					continue;

				auto records = &player_records[i]; //-V826

				if (records->empty())
					continue;

				auto record = &records->front();

				if (!record->valid())
					continue;

				scan_data predicted_data;
				aim::get().scan(record, predicted_data, predicted_eye_pos);

				if (predicted_data.valid())
				{
					scan_data data;
					aim::get().scan(record, data, g_ctx.globals.eye_pos);

					if (!data.valid())
					{
						random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
						switch_ticks = !switch_ticks;
						fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

						g_ctx.send_packet = true;
						started_peeking = true;

						return;
					}
				}
			}
		}

		if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && g_cfg.antiaim.fakelag_enablers[FAKELAG_PEEK] && started_peeking)
		{
			if (choked < max_choke)
				g_ctx.send_packet = false;
			else
			{
				started_peeking = false;

				random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
				switch_ticks = !switch_ticks;
				fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

				g_ctx.send_packet = true;
			}
		}
		else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && velocity >= 5.0f && g_ctx.globals.slowwalking && g_cfg.antiaim.fakelag_enablers[FAKELAG_SLOW_WALK])
		{
			if (choked < max_choke)
				g_ctx.send_packet = false;
			else
			{
				started_peeking = false;

				random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
				switch_ticks = !switch_ticks;
				fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

				g_ctx.send_packet = true;
			}
		}
		else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && velocity >= 5.0f && !g_ctx.globals.slowwalking && g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND && g_cfg.antiaim.fakelag_enablers[FAKELAG_MOVE])
		{
			if (choked < max_choke)
				g_ctx.send_packet = false;
			else
			{
				started_peeking = false;

				random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
				switch_ticks = !switch_ticks;
				fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

				g_ctx.send_packet = true;
			}
		}
		else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && !g_ctx.globals.slowwalking && !(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND) && g_cfg.antiaim.fakelag_enablers[FAKELAG_AIR])
		{
			if (choked < max_choke)
				g_ctx.send_packet = false;
			else
			{
				started_peeking = false;

				random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
				switch_ticks = !switch_ticks;
				fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

				g_ctx.send_packet = true;
			}
		}
		else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag)
		{
			max_choke = g_cfg.antiaim.fakelag_amount;

			if (m_gamerules()->m_bIsValveDS())
				max_choke = min(max_choke, 6);

			if (choked < max_choke)
				g_ctx.send_packet = false;
			else
			{
				started_peeking = false;

				random_factor = min(rand() % 16 + 1, g_cfg.antiaim.fakelag_amount);
				switch_ticks = !switch_ticks;
				fluctuate_ticks = switch_ticks ? g_cfg.antiaim.fakelag_amount : max(g_cfg.antiaim.fakelag_amount - 2, 1);

				g_ctx.send_packet = true;
			}
		}
		else if (g_ctx.globals.exploits || !antiaim::get().condition(m_pcmd, false) && (antiaim::get().type == ANTIAIM_LEGIT || g_cfg.antiaim.desync)) //-V648
		{
			condition = true;
			started_peeking = false;

			if (choked < 1)
				g_ctx.send_packet = false;
			else
				g_ctx.send_packet = true;
		}
		else
			condition = true;
	}

	//static Vector sent_origin = Vector();

	//if (g_ctx.canDrawLC)
	//{
	//	if (g_ctx.send_packet)
	//		sent_origin = g_ctx.local()->GetAbsOrigin();

	//	if ((sent_origin - oldOrigin).LengthSqr() > 4096.f) {
	//		g_ctx.canBreakLC = true;
	//	}
	//	else
	//		g_ctx.canBreakLC = false;

	//	if (g_ctx.send_packet)
	//		oldOrigin = g_ctx.local()->GetAbsOrigin();
	//}
}

void fakelag::Createmove()
{
	if (FakelagCondition(g_ctx.get_command()))
		return;

	Fakelag(g_ctx.get_command());

	if (!m_gamerules()->m_bIsValveDS() && m_clientstate()->iChokedCommands <= 16)
	{
		static auto Fn = util::FindSignature(crypt_str("engine.dll"), crypt_str("B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC")) + 0x1;
		DWORD old = 0;

		VirtualProtect((void*)Fn, sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &old);
		*(uint32_t*)Fn = 17;
		VirtualProtect((void*)Fn, sizeof(uint32_t), old, &old);
	}
}

bool fakelag::FakelagCondition(CUserCmd* m_pcmd)
{
	condition = false;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
		condition = true;

	if (antiaim::get().freeze_check && !exploit::get().double_tap_enabled && !exploit::get().hide_shots_enabled)
		condition = true;

	return condition;
}