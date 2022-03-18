#include "..\hooks.hpp"
#include "..\..\menu\menu.h"
#include "..\..\features\lagcompensation\animation_system.h"
#include "..\..\features\visuals\player_esp.h"
#include "..\..\features\visuals\other_esp.h"
#include "..\..\features\misc\logs.h"
#include "..\..\features\visuals\world_esp.h"
#include "..\..\features\misc\misc.h"
#include "..\..\features\visuals\GrenadePrediction.h"
#include "..\..\features\visuals\bullet_tracers.h"
#include "..\..\features\visuals\dormant_esp.h"
#include "..\..\features\lagcompensation\local_animations.h"

#include "../../features/exploit/exploit.h"

using PaintTraverse_t = void(__thiscall*)(void*, vgui::VPANEL, bool, bool);

bool reload_fonts()
{
	static int old_width, old_height;
	static int width, height;

	m_engine()->GetScreenSize(width, height);

	if (width != old_width || height != old_height)
	{
		old_width = width;
		old_height = height;

		return true;
	}

	return false;
}

void __fastcall hooks::hooked_painttraverse(void* ecx, void* edx, vgui::VPANEL panel, bool force_repaint, bool allow_force)
{
	static auto original_fn = panel_hook->get_func_address <PaintTraverse_t>(41);
	g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	static auto set_console = true;

	if (set_console)
	{
		set_console = false;

		m_cvar()->FindVar(crypt_str("developer"))->SetValue(FALSE); //-V807
		m_cvar()->FindVar(crypt_str("con_filter_enable"))->SetValue(TRUE);
		m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str(""));
		m_engine()->ExecuteClientCmd(crypt_str("clear"));
		m_cvar()->ConsoleColorPrintf(Color(23, 249, 252), "Squaredhack.xyz - free cheats lmao. \n");
		m_cvar()->ConsoleColorPrintf(Color::White, "Last build: " __DATE__ " / " __TIME__);
		m_cvar()->ConsoleColorPrintf(Color(23, 249, 252), "\n");
	}

	static auto log_value = true;

	if (log_value != g_cfg.misc.show_default_log)
	{
		log_value = g_cfg.misc.show_default_log;

		if (log_value)
			m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str(""));
		else
			m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str("IrWL5106TZZKNFPz4P4Gl3pSN?J370f5hi373ZjPg%VOVh6lN"));
	}

	static vgui::VPANEL panel_id = 0;
	static auto in_game = false;

	if (!in_game && m_engine()->IsInGame())
	{
		in_game = true;

		for (auto i = 1; i < 65; i++)
		{
			g_ctx.globals.fired_shots[i] = 0;
			g_ctx.globals.missed_shots[i] = 0;
			player_records[i].clear();
			lagcompensation::get().is_dormant[i] = false;
			playeresp::get().esp_alpha_fade[i] = 0.0f;
			playeresp::get().health[i] = 100;
			c_dormant_esp::get().m_cSoundPlayers[i].reset();
			otheresp::get().damage_marker[i].reset();
		}

		antiaim::get().freeze_check = false;
		g_ctx.globals.next_lby_update = FLT_MIN;
		g_ctx.globals.last_lby_move = FLT_MIN;
		g_ctx.globals.last_aimbot_shot = 0;
		g_ctx.globals.bomb_timer_enable = true;
		g_ctx.globals.backup_model = false;
		g_ctx.globals.should_remove_smoke = false;
		g_ctx.globals.should_update_beam_index = true;
		g_ctx.globals.should_update_playerresource = true;
		g_ctx.globals.should_update_gamerules = true;
		g_ctx.globals.should_update_radar = true;
		g_ctx.globals.kills = 0;
		g_ctx.shots.clear();
		g_ctx.globals.commands.clear();
		SkinChanger::model_indexes.clear();
		SkinChanger::player_model_indexes.clear();
	}
	else if (in_game && !m_engine()->IsInGame())
	{
		in_game = false;

		g_ctx.globals.should_update_weather = true;
		g_ctx.globals.m_networkable = nullptr;

		exploit::get().double_tap_enabled = false;
		exploit::get().double_tap_key = false;

		exploit::get().hide_shots_enabled = false;
		exploit::get().hide_shots_key = false;
	}

	if (m_engine()->IsTakingScreenshot() && g_cfg.misc.anti_screenshot)
		return;

	static uint32_t HudZoomPanel = 0;

	if (!HudZoomPanel)
		if (!strcmp(crypt_str("HudZoom"), m_panel()->GetName(panel)))
			HudZoomPanel = panel;

	if (HudZoomPanel == panel && g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SCOPE])
		return;

	original_fn(ecx, panel, force_repaint, allow_force);

	if (!panel_id)
	{
		auto panelName = m_panel()->GetName(panel);

		if (!strcmp(panelName, crypt_str("MatSystemTopPanel")))
			panel_id = panel;
	}

	if (reload_fonts())
	{
		static auto create_font = [](const char* name, int size, int weight, DWORD flags) -> vgui::HFont
		{
			g_ctx.last_font_name = name;

			auto font = m_surface()->FontCreate();
			m_surface()->SetFontGlyphSet(font, name, size, weight, NULL, NULL, flags);

			return font;
		};

		fonts[LOGS] = create_font(crypt_str("Raleway"), 15, FW_DONTCARE, FONTFLAG_ANTIALIAS);
		fonts[ESP] = create_font(crypt_str("Smallest Pixel-7"), 11, FW_MEDIUM, FONTFLAG_OUTLINE);
		fonts[NAME] = create_font(crypt_str("Square Sans Serif 7"), 11, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[SUBTABWEAPONS] = create_font(crypt_str("undefeated"), 13, FW_DONTCARE, FONTFLAG_ANTIALIAS);
		fonts[KNIFES] = create_font(crypt_str("icomoon"), 13, FW_DONTCARE, FONTFLAG_ANTIALIAS);
		fonts[GRENADES] = create_font(crypt_str("undefeated"), 20, FW_MEDIUM, FONTFLAG_OUTLINE);
		fonts[INDICATORFONT] = create_font(crypt_str("Verdana"), 25, FW_HEAVY, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[DAMAGE_MARKER] = create_font(crypt_str("Raleway"), 17, FW_MEDIUM, FONTFLAG_ANTIALIAS);
		fonts[WATERMARK] = create_font(crypt_str("Raleway"), 12, FW_EXTRALIGHT, FONTFLAG_NONE | FONTFLAG_NONE);
		g_ctx.last_font_name.clear();
	}

	if (panel_id == panel)
	{
		if (g_ctx.available())
		{
			static auto alive = false;

			if (!alive && g_ctx.local()->is_alive())
			{
				alive = true;
				g_ctx.globals.should_clear_death_notices = true;
			}
			else if (alive && !g_ctx.local()->is_alive())
			{
				alive = false;

				for (auto i = 1; i < m_globals()->m_maxclients; i++)
				{
					g_ctx.globals.fired_shots[i] = 0;
					g_ctx.globals.missed_shots[i] = 0;
				}

				local_animations::get().local_data.prediction_animstate = nullptr;
				local_animations::get().local_data.animstate = nullptr;

				g_ctx.globals.weapon = nullptr;
				g_ctx.globals.should_choke_packet = false;
				g_ctx.globals.should_send_packet = false;
				g_ctx.globals.kills = 0;
				g_ctx.globals.should_buy = 3;
			}

			g_ctx.globals.bomb_carrier = -1;

			static std::vector <const char*> molly_materials =
			{
				"particle/fire_burning_character/fire_env_fire.vmt",
				"particle/fire_burning_character/fire_env_fire_depthblend.vmt",
				"particle/fire_burning_character/fire_burning_character_depthblend.vmt",
				"particle/fire_burning_character/fire_burning_character.vmt",
				"particle/fire_burning_character/fire_burning_character_nodepth.vmt",
				"particle/particle_flares/particle_flare_001.vmt",
				"particle/particle_flares/particle_flare_004.vmt",
				"particle/particle_flares/particle_flare_004b_mod_ob.vmt",
				"particle/particle_flares/particle_flare_004b_mod_z.vmt",
				"particle/fire_explosion_1/fire_explosion_1_bright.vmt",
				"particle/fire_explosion_1/fire_explosion_1b.vmt",
				"particle/fire_particle_4/fire_particle_4.vmt",
				"particle/fire_explosion_1/fire_explosion_1_oriented.vmt"
			};

			for (auto material_name : molly_materials)
			{
				auto material = m_materialsystem()->FindMaterial(material_name, nullptr);

				if (!material)
					continue;

				material->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true);
			}

			misc::get().desync_arrows();
			otheresp::get().penetration_reticle();

			auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

			bullettracers::get().draw_beams();
		}

		static auto framerate = 0.0f;
		framerate = 0.9f * framerate + 0.1f * m_globals()->m_absoluteframetime;

		if (framerate <= 0.0f)
			framerate = 1.0f;

		g_ctx.globals.framerate = (int)(1.0f / framerate);
		auto nci = m_engine()->GetNetChannelInfo();

		if (nci)
		{
			auto latency = m_engine()->IsPlayingDemo() ? 0.0f : nci->GetAvgLatency(FLOW_OUTGOING);

			if (latency)
			{
				static auto cl_updaterate = m_cvar()->FindVar(crypt_str("cl_updaterate"));
				latency -= 0.5f / cl_updaterate->GetFloat();
			}

			g_ctx.globals.ping = (int)(max(0.0f, latency) * 1000.0f);
		};

		static auto fov_cs_debug = m_cvar()->FindVar(("fov_cs_debug"));
		if (g_ctx.globals.scoped && g_ctx.globals.weapon->is_sniper() && g_cfg.esp.viewmodel_in_scope)
			fov_cs_debug->SetValue(90);
		else if (g_ctx.globals.scoped && g_ctx.globals.weapon->is_sniper())
			fov_cs_debug->SetValue(0);

		static auto cvar_2 = m_cvar()->FindVar("@panorama_disable_blur");
		if (g_cfg.esp.disablepanoramablur)
			cvar_2->SetValue(true);
		else
			cvar_2->SetValue(false);

		static auto cvar_3 = m_cvar()->FindVar("weapon_debug_spread_show");
		if (g_cfg.esp.sniper_crosshair)
			cvar_3->SetValue(g_ctx.globals.scoped ? 0 : 3);
		else
			cvar_3->SetValue(0);

		misc::get().NightmodeFix();
		otheresp::get().draw_indicators();
		eventlogs::get().paint_traverse();

		if (g_ctx.globals.loaded_script)
			for (auto current : c_lua::get().hooks.getHooks(crypt_str("on_paint")))
				current.func();
	}
}

