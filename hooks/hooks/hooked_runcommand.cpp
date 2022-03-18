#include "..\hooks.hpp"
#include "..\..\features\misc\prediction_system.h"
#include "..\..\features\lagcompensation\local_animations.h"
#include "..\..\features\misc\misc.h"
#include "..\..\features\misc\logs.h"

void FixRevolver()
{
	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();
	if (!weapon)
		return;

	if (weapon->m_iItemDefinitionIndex() == 64 && weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && weapon->m_flNextSecondaryAttack() == FLT_MAX)
		weapon->m_flNextSecondaryAttack() = g_ctx.local()->m_hActiveWeapon().Get()->m_flNextSecondaryAttack();
};

using RunCommand_t = void(__thiscall*)(void*, player_t*, CUserCmd*, IMoveHelper*);

void __fastcall hooks::hooked_runcommand(void* ecx, void* edx, player_t* player, CUserCmd* m_pcmd, IMoveHelper* move_helper)
{
	static auto original_fn = prediction_hook->get_func_address <RunCommand_t>(19);
	g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	if (!player || player != g_ctx.local() || !m_pcmd)
		return original_fn(ecx, player, m_pcmd, move_helper);

	int m_tickrate = ((int)(1.0f / m_globals()->m_intervalpertick));

	if (m_globals()->m_tickcount + m_tickrate + 8 <= m_pcmd->m_tickcount) //-V807
	{
		m_pcmd->m_predicted = true;
		return;
	}

	auto m_pWeapon = player->m_hActiveWeapon().Get();

	static float tickbase_records[MULTIPLAYER_BACKUP];
	static bool in_attack[MULTIPLAYER_BACKUP];
	static bool can_shoot[MULTIPLAYER_BACKUP];
	tickbase_records[m_pcmd->m_command_number % MULTIPLAYER_BACKUP] = player->m_nTickBase();
	in_attack[m_pcmd->m_command_number % MULTIPLAYER_BACKUP] = m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2;
	can_shoot[m_pcmd->m_command_number % MULTIPLAYER_BACKUP] = m_pWeapon->can_fire(false);

	auto m_flStoredVelMod = player->m_flVelocityModifier();
	auto m_nStoredTickbase = player->m_nTickBase();
	auto m_nStoredCurtime = m_globals()->m_curtime;
	auto backup_velocity_modifier = player->m_flVelocityModifier();

	if (m_pcmd->m_command_number == 1)
	{
		player->m_nTickBase() = g_ctx.globals.next_tickbase_shift - g_ctx.globals.tickbase_shift + 1;
		m_globals()->m_curtime = player->m_nTickBase() * m_globals()->m_intervalpertick;
	}

	g_ctx.globals.override_velmod = true;
	engineprediction::get().fix_attack_packet(m_pcmd, true);

	if (g_ctx.globals.last_velocity_modifier && m_pcmd->m_command_number == m_clientstate()->nLastCommandAck + 1)
		player->m_flVelocityModifier() = m_flStoredVelMod;

	if (m_pWeapon)
		FixRevolver();

	original_fn(ecx, player, m_pcmd, move_helper);

	if (m_pWeapon)
		FixRevolver();

	if (m_pcmd->m_command_number == 1)
	{
		player->m_nTickBase() = m_nStoredTickbase;
		m_globals()->m_curtime = m_nStoredCurtime;
	}

	if (!g_ctx.globals.last_velocity_modifier)
		player->m_flVelocityModifier() = backup_velocity_modifier;

	player->m_vphysicsCollisionState() = false;
	engineprediction::get().fix_attack_packet(m_pcmd, false);
	g_ctx.globals.override_velmod = false;

	if (!g_ctx.globals.in_createmove)
		player->m_flVelocityModifier() = backup_velocity_modifier;
}

using InPrediction_t = bool(__thiscall*)(void*);

bool __stdcall hooks::hooked_inprediction()
{
	static auto original_fn = prediction_hook->get_func_address <InPrediction_t>(16);
	static auto maintain_sequence_transitions = util::FindSignature(crypt_str("client.dll"), crypt_str("84 C0 74 17 8B 87"));

	if ((g_cfg.ragebot.enable || g_cfg.legitbot.enabled) && g_ctx.globals.setuping_bones && (uintptr_t)_ReturnAddress() == maintain_sequence_transitions)
		return true;

	return original_fn(m_prediction());
}

typedef void(cdecl* cl_move_t)(float, bool);
typedef void(cdecl* clMove_fn)(float, bool);

void __cdecl hooks::hooked_clmove(float accumulated_extra_samples, bool bFinalTick, CUserCmd* m_pcmd)
{
	bool in_attack = m_pcmd->m_buttons & IN_ATTACK;
	(cl_move_t(hooks::original_clmove))(accumulated_extra_samples, bFinalTick);

	if (g_cfg.ragebot.enable)
	{
		if (!g_ctx.local() && !g_ctx.local()->is_alive() && g_ctx.local()->m_fFlags() & 0x40)
			return;

		else if (m_clientstate())
		{
			INetChannel* net_channel = m_clientstate()->pNetChannel;
			if (net_channel && !(m_clientstate()->iChokedCommands % 4))
			{
				const auto current_choke = net_channel->m_nChokedPackets;
				const auto out_sequence_nr = net_channel->m_nOutSequenceNr;

				net_channel->m_nChokedPackets = 0;

				net_channel->send_datagram();

				net_channel->m_nOutSequenceNr = out_sequence_nr;
				net_channel->m_nChokedPackets = current_choke;
			}
		}

		if (g_ctx.available() && g_ctx.local()->is_alive())
		{
			if (BAIM_DOUBLE_TAP)
			{
				g_ctx.send_packet = g_ctx.globals.ticks_allowed == 14;
				m_pcmd->m_buttons &= ~(IN_ATTACK | IN_ATTACK2);
				return;

			}
			else if (in_attack)
			{
				g_ctx.globals.ticks_allowed == 14;
				g_ctx.globals.tickbase_shift == 14;
				g_ctx.globals.ticks_choke == 14;
				float last_double_tap;
				last_double_tap = g_ctx.globals.fixed_tickbase;
				return;
			}
		}
	}
}

using WriteUsercmdDeltaToBuffer_t = bool(__thiscall*)(void*, int, void*, int, int, bool);
void WriteUserCmd(void* buf, CUserCmd* incmd, CUserCmd* outcmd);

void WriteUserCmd(void* buf, CUserCmd* incmd, CUserCmd* outcmd)
{
	using WriteUserCmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	static auto Fn = (WriteUserCmd_t)util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 51 53 56 8B D9"));

	__asm
	{
		mov     ecx, buf
		mov     edx, incmd
		push    outcmd
		call    Fn
		add     esp, 4
	}
}

bool __fastcall hooks::hooked_writeusercmddeltatobuffer(void* ecx, void* edx, int slot, bf_write* buf, int from, int to, bool is_new_command)
{
	static auto original_fn = client_hook->get_func_address <WriteUsercmdDeltaToBuffer_t>(24);

	if (!g_ctx.globals.tickbase_shift)
		return original_fn(ecx, slot, buf, from, to, is_new_command);

	if (from != -1)
		return true;

	auto final_from = -1;

	uintptr_t frame_ptr;
	__asm mov frame_ptr, ebp;

	auto backup_commands = reinterpret_cast <int*> (frame_ptr + 0xFD8);
	auto new_commands = reinterpret_cast <int*> (frame_ptr + 0xFDC);

	auto newcmds = *new_commands;
	auto shift = g_ctx.globals.tickbase_shift;

	g_ctx.globals.tickbase_shift = 0;
	*backup_commands = 0;

	auto choked_modifier = newcmds + shift;

	if (choked_modifier > 62)
		choked_modifier = 62;

	*new_commands = choked_modifier;

	auto next_cmdnr = m_clientstate()->iChokedCommands + m_clientstate()->nLastOutgoingCommand + 1;
	auto final_to = next_cmdnr - newcmds + 1;

	if (final_to <= next_cmdnr)
	{
		while (original_fn(ecx, slot, buf, final_from, final_to, true))
		{
			final_from = final_to++;

			if (final_to > next_cmdnr)
				goto next_cmd;
		}

		return false;
	}
next_cmd:

	auto user_cmd = m_input()->GetUserCmd(final_from);

	if (!user_cmd)
		return true;

	CUserCmd to_cmd;
	CUserCmd from_cmd;

	from_cmd = *user_cmd;
	to_cmd = from_cmd;

	to_cmd.m_command_number++;


	to_cmd.m_tickcount += ((int)(1.0f / m_globals()->m_intervalpertick) * 3);

	if (newcmds > choked_modifier)
		return true;

	for (auto i = choked_modifier - newcmds + 1; i > 0; --i)
	{
		WriteUserCmd(buf, &to_cmd, &from_cmd);

		from_cmd = to_cmd;
		to_cmd.m_command_number++;
		to_cmd.m_tickcount++;
	}
	uintptr_t stackbase;
	__asm mov stackbase, ebp;

	auto m_pnNewCmds = reinterpret_cast <int*> (stackbase + 0xFCC);

	int m_nNewCmds = *m_pnNewCmds;

	int m_nTickbase = g_ctx.globals.tickbase_shift;

	int m_nTotalNewCmds = min(m_nNewCmds + abs(m_nTickbase), 62);

	if (from != -1)
		return true;

	int m_nNewCommands = m_nTotalNewCmds;
	int m_nBackupCommands = 0;
	int m_nNextCmd = m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands + 1;

	if (to > m_nNextCmd)
	{
	Run:
		CUserCmd* m_pCmd = m_input()->GetUserCmd(from);
		if (m_pCmd)
		{
			CUserCmd m_nToCmd = *m_pCmd, m_nFromCmd = *m_pCmd;
			m_nToCmd.m_command_number++;
			m_nToCmd.m_tickcount += m_globals()->m_tickcount + 2 * m_globals()->m_tickcount;
			for (int i = m_nNewCmds; i <= m_nTotalNewCmds; i++)
			{
				int m_shift = m_nTotalNewCmds - m_nNewCmds + 1;

				do
				{
					m_nFromCmd.m_buttons = m_nToCmd.m_buttons;
					m_nFromCmd.m_viewangles.x = m_nToCmd.m_viewangles.x;
					m_nFromCmd.m_impulse = m_nToCmd.m_impulse;
					m_nFromCmd.m_weaponselect = m_nToCmd.m_weaponselect;
					m_nFromCmd.m_aimdirection.y = m_nToCmd.m_aimdirection.y;
					m_nFromCmd.m_weaponsubtype = m_nToCmd.m_weaponsubtype;
					m_nFromCmd.m_upmove = m_nToCmd.m_upmove;
					m_nFromCmd.m_random_seed = m_nToCmd.m_random_seed;
					m_nFromCmd.m_mousedx = m_nToCmd.m_mousedx;
					m_nFromCmd.pad_0x4C[3] = m_nToCmd.pad_0x4C[3];
					m_nFromCmd.m_command_number = m_nToCmd.m_command_number;
					m_nFromCmd.m_tickcount = m_nToCmd.m_tickcount;
					m_nFromCmd.m_mousedy = m_nToCmd.m_mousedy;
					m_nFromCmd.pad_0x4C[19] = m_nToCmd.pad_0x4C[19];
					m_nFromCmd.m_predicted = m_nToCmd.m_predicted;
					m_nFromCmd.pad_0x4C[23] = m_nToCmd.pad_0x4C[23];
					m_nToCmd.m_command_number++;
					m_nToCmd.m_tickcount++;
					--m_shift;
				} while (m_shift);
			}
			return true;
		}
	}
	return true;
}
