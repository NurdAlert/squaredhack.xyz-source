#include "player_esp.h"
#include "..\misc\misc.h"
#include "..\ragebot\aim.h"
#include "dormant_esp.h"
#include "../../menu/menu.h"
#include "../../utils/Render/Render.h"

class RadarPlayer_t
{
public:
	Vector pos;
	Vector angle;
	Vector spotted_map_angle_related;
	DWORD tab_related;
	char pad_0x0028[0xC];
	float spotted_time;
	float spotted_fraction;
	float time;
	char pad_0x0040[0x4];
	__int32 player_index;
	__int32 entity_index;
	char pad_0x004C[0x4];
	__int32 health;
	char name[32];
	char pad_0x0074[0x75];
	unsigned char spotted;
	char pad_0x00EA[0x8A];
};

class CCSGO_HudRadar
{
public:
	char pad_0x0000[0x14C];
	RadarPlayer_t radar_info[65];
};


void playeresp::paint_traverse()
{
	static auto alpha = 1.0f;
	c_dormant_esp::get().start();

	if (g_cfg.player.arrows && g_ctx.local()->is_alive())
	{
		static auto switch_alpha = false;

		if (alpha <= 0.0f || alpha >= 1.0f)
			switch_alpha = !switch_alpha;

		alpha += switch_alpha ? 2.0f * m_globals()->m_frametime : -2.0f * m_globals()->m_frametime;
		alpha = math::clamp(alpha, 0.0f, 1.0f);
	}

	static auto FindHudElement = (DWORD(__thiscall*)(void*, const char*))util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));
	static auto hud_ptr = *(DWORD**)(util::FindSignature(crypt_str("client.dll"), crypt_str("81 25 ? ? ? ? ? ? ? ? 8B 01")) + 0x2);

	auto radar_base = FindHudElement(hud_ptr, "CCSGO_HudRadar");
	auto hud_radar = (CCSGO_HudRadar*)(radar_base - 0x14);

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e->valid(false, false))
			continue;

		type = ENEMY;

		if (e == g_ctx.local())
			type = LOCAL;
		else if (e->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
			type = TEAM;

		if (type == LOCAL && !m_input()->m_fCameraInThirdPerson)
			continue;

		auto valid_dormant = false;
		auto backup_flags = e->m_fFlags();
		auto backup_origin = e->GetAbsOrigin();

		if (e->IsDormant())
			valid_dormant = c_dormant_esp::get().adjust_sound(e);
		else
		{
			health[i] = e->m_iHealth();
			c_dormant_esp::get().m_cSoundPlayers[i].reset(true, e->GetAbsOrigin(), e->m_fFlags());
		}

		if (radar_base && hud_radar && e->IsDormant() && e->m_iTeamNum() != g_ctx.local()->m_iTeamNum() && e->m_bSpotted())
			health[i] = hud_radar->radar_info[i].health;

		if (!health[i])
		{
			if (e->IsDormant())
			{
				e->m_fFlags() = backup_flags;
				e->set_abs_origin(backup_origin);
			}

			continue;
		}

		auto fast = 2.5f * m_globals()->m_frametime;
		auto slow = 0.25f * m_globals()->m_frametime;

		if (e->IsDormant())
		{
			auto origin = e->GetAbsOrigin();

			if (origin.IsZero())
				esp_alpha_fade[i] = 0.0f;
			else if (!valid_dormant && esp_alpha_fade[i] > 0.0f)
				esp_alpha_fade[i] -= slow;
			else if (valid_dormant && esp_alpha_fade[i] < 1.0f)
				esp_alpha_fade[i] += fast;
		}
		else if (esp_alpha_fade[i] < 1.0f)
			esp_alpha_fade[i] += fast;

		esp_alpha_fade[i] = math::clamp(esp_alpha_fade[i], 0.0f, 1.0f);

		if (g_cfg.player.type[type].skeleton && !e->IsDormant())
		{
			auto color = g_cfg.player.type[type].skeleton_color;
			color.SetAlpha(min(255.0f * esp_alpha_fade[i], color.a()));

			draw_skeleton(e, color, e->m_CachedBoneData().Base());
		}

		Box box;

		if (util::get_bbox(e, box, true))
		{
			if (e->GetRenderOrigin() == Vector(0, 0, 0))
				continue;

			draw_box(e, box);
			draw_name(e, box);
			draw_health(e, box);
			draw_weapon(e, box, draw_ammobar(e, box));

			if (!e->IsDormant())
				draw_flags(e, box);
		}

		if (type == ENEMY || type == TEAM)
		{
			draw_lines(e);

			if (type == ENEMY)
			{
				if (g_cfg.player.arrows && g_ctx.local()->is_alive())
					misc::get().PovArrows(e, Color(g_cfg.player.arrows_color));
			}
		}

		if (e->IsDormant())
		{
			e->m_fFlags() = backup_flags;
			e->set_abs_origin(backup_origin);
		}
	}
}

void playeresp::draw_skeleton(player_t* e, Color color, matrix3x4_t matrix[MAXSTUDIOBONES])
{
	auto model = e->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto get_bone_position = [&](int bone) -> Vector
	{
		return Vector(matrix[bone][0][3], matrix[bone][1][3], matrix[bone][2][3]);
	};

	auto upper_direction = get_bone_position(7) - get_bone_position(6);
	auto breast_bone = get_bone_position(6) + upper_direction * 0.5f;

	for (auto i = 0; i < studio_model->numbones; i++)
	{
		auto bone = studio_model->pBone(i);

		if (!bone)
			continue;

		if (bone->parent == -1)
			continue;

		if (!(bone->flags & BONE_USED_BY_HITBOX))
			continue;

		auto child = get_bone_position(i);
		auto parent = get_bone_position(bone->parent);

		auto delta_child = child - breast_bone;
		auto delta_parent = parent - breast_bone;

		if (delta_parent.Length() < 9.0f && delta_child.Length() < 9.0f)
			parent = breast_bone;

		if (i == 5)
			child = breast_bone;

		if (fabs(delta_child.z) < 5.0f && delta_parent.Length() < 5.0f && delta_child.Length() < 5.0f || i == 6)
			continue;

		auto schild = ZERO;
		auto sparent = ZERO;

		if (math::world_to_screen(child, schild) && math::world_to_screen(parent, sparent))
			g_Render->DrawLine(schild.x, schild.y, sparent.x, sparent.y, color);
	}
}

void DrawLine(float x1, float y1, float x2, float y2, Color color, float size = 1.f)
{
	if (size == 1.f)
	{

		g_Render->DrawLine(x1, y1, x2, y2, Color(color.r(), color.g(), color.b(), color.a()));
	}
	else
	{
		g_Render->FilledRect(x1 - (size / 2.f), y1 - (size / 2.f), x2 + (size / 2.f), y2 + (size / 2.f), Color(color.r(), color.g(), color.b(), color.a()));
	}

}

void Corners(float x1, float y1, float x2, float y2, Color clr, float edge_size, float size)
{
	if (fabs(x1 - x2) < (edge_size * 2))
	{
		edge_size = fabs(x1 - x2) / 4.f;
	}

	DrawLine(x1, y1, x1, y1 + edge_size + (0.5f * edge_size), clr, size);
	DrawLine(x2, y1, x2, y1 + edge_size + (0.5f * edge_size), clr, size);
	DrawLine(x1, y2, x1, y2 - edge_size - (0.5f * edge_size), clr, size);
	DrawLine(x2, y2, x2, y2 - edge_size - (0.5f * edge_size), clr, size);
	DrawLine(x1, y1, x1 + edge_size, y1, clr, size);
	DrawLine(x2, y1, x2 - edge_size, y1, clr, size);
	DrawLine(x1, y2, x1 + edge_size, y2, clr, size);
	DrawLine(x2, y2, x2 - edge_size, y2, clr, size);
}

void playeresp::draw_box(player_t* m_entity, const Box& box)
{
	if (!g_cfg.player.type[type].box)
		return;

	auto alpha = 255.0f * esp_alpha_fade[m_entity->EntIndex()];

	auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : g_cfg.player.type[type].box_color;
	color.SetAlpha(min(alpha, color.a()));

	g_Render->Rect(box.x - 1, box.y - 1, box.w + 2, box.h + 2, Color::Black);
	g_Render->Rect(box.x, box.y, box.w, box.h, color);
	g_Render->Rect(box.x + 1, box.y + 1, box.w - 2, box.h - 2, Color::Black);
}

void playeresp::draw_health(player_t* m_entity, const Box& box)
{
	if (!g_cfg.player.type[type].health)
		return;

	auto alpha = (int)(255.0f * esp_alpha_fade[m_entity->EntIndex()]);
	auto back_color = Color(0, 0, 0, (int)(alpha * 0.6f));
	constexpr float SPEED_FREQ = 255 / 1.0f;
	int hp = m_entity->m_iHealth();

	if (hp > 100)
		hp = 100;


	int red = 255 - (hp * 2.55);
	int green = hp * 2.55;

	Box n_box =
	{
		box.x - 5,
		box.y,
		2,
		box.h
	};

	auto hp_color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : g_cfg.player.type[type].health_color;
	hp_color.SetAlpha(min(alpha, hp_color.a()));

	static float prev_player_hp[65];

	if (prev_player_hp[m_entity->EntIndex()] > hp)
		prev_player_hp[m_entity->EntIndex()] -= SPEED_FREQ * m_globals()->m_frametime;
	else
		prev_player_hp[m_entity->EntIndex()] = hp;

	int hp_percent = box.h - (int)((box.h * prev_player_hp[m_entity->EntIndex()]) / 100);
	int hp_percentw = box.w - (int)((box.w * prev_player_hp[m_entity->EntIndex()]) / 100);

	int healthpos_X = 0;
	int healthpos_Y = 0;

	g_Render->Rect(box.x - 6, box.y, 4, box.h, back_color);

	g_Render->FilledRect(n_box.x, n_box.y - 1, 2, n_box.h + 2, back_color);
	g_Render->FilledRect(n_box.x, n_box.y + hp_percent, 2, box.h - hp_percent, hp_color);

	if (hp < 100)
	{
		g_Render->DrawString(n_box.x, n_box.y - 1, Color(255, 255, 255), render2::outline, c_menu::get().g_pLoaderFont, std::to_string(hp).c_str());
	}

}

bool playeresp::draw_ammobar(player_t* m_entity, const Box& box)
{
	if (!m_entity->is_alive())
		return false;

	if (!g_cfg.player.type[type].ammo)
		return false;

	auto weapon = m_entity->m_hActiveWeapon().Get();

	if (weapon->is_non_aim())
		return false;

	auto ammo = weapon->m_iClip1();

	auto alpha = (int)(255.0f * esp_alpha_fade[m_entity->EntIndex()]);

	auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : g_cfg.player.type[type].ammobar_color;

	color.SetAlpha(min(alpha, color.a()));

	Box n_box =
	{
		box.x,
		box.y + box.h + 3,
		box.w + 2,
		2
	};

	auto weapon_info = m_weaponsystem()->GetWeaponData(m_entity->m_hActiveWeapon()->m_iItemDefinitionIndex());

	if (!weapon_info)
		return false;

	auto bar_width = ammo * box.w / weapon_info->iMaxClip1;
	auto reloading = false;

	auto animlayer = m_entity->get_animlayers()[1];
	int health_tik = ammo * box.h / weapon_info->iMaxClip1;

	if (animlayer.m_nSequence)
	{
		auto activity = m_entity->sequence_activity(animlayer.m_nSequence);

		reloading = activity == ACT_CSGO_RELOAD && animlayer.m_flWeight;

		if (reloading && animlayer.m_flCycle < 1.0f)
		{
			bar_width = animlayer.m_flCycle * box.w;
			health_tik = animlayer.m_flCycle * box.h;
		}
	}

	int healthpos_X = 0;
	int healthpos_Y = 0;

	g_Render->FilledRect(box.x - 1, box.y + 2 + box.h, (box.w + 2), 4, Color(0, 0, 0, 100));
	g_Render->FilledRect(box.x, box.y + 3 + box.h, bar_width, 2, color);
}

void playeresp::draw_name(player_t* m_entity, const Box& box)
{

	if (!g_cfg.player.type[type].name)
		return;

	static auto sanitize = [](char* name) -> std::string
	{
		name[127] = '\0';

		std::string tmp(name);

		if (tmp.length() > 20)
		{
			tmp.erase(20, tmp.length() - 20);
			tmp.append("...");
		}

		return tmp;
	};

	player_info_t player_info;

	if (m_engine()->GetPlayerInfo(m_entity->EntIndex(), &player_info))
	{
		ImGui::PushFont(c_menu::get().g_pLoaderFont);
		auto name = sanitize(player_info.szName);
		auto _string = "resolved yaw: " + std::to_string(m_entity->get_animation_state()->m_flGoalFeetYaw - 180.f);
		auto text_size = ImGui::CalcTextSize(name.c_str());

		auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : g_cfg.player.type[type].name_color;
		color.SetAlpha(min(255.0f * esp_alpha_fade[m_entity->EntIndex()], color.a()));

		g_Render->DrawString(box.x + box.w / 2 - text_size.x / 2, box.y - 15, color, render2::outline, c_menu::get().g_pLoaderFont, (name + " ").c_str());
		g_Render->DrawString(box.x + box.w / 2 - 20, box.y - 30, color, render2::outline, c_menu::get().g_pLoaderFont, _string.c_str());
		ImGui::PopFont();
	}

}

void playeresp::draw_weapon(player_t* m_entity, const Box& box, bool space)
{
	if (!g_cfg.player.type[type].weapon[WEAPON_ICON] && !g_cfg.player.type[type].weapon[WEAPON_TEXT])
		return;

	auto weapon = m_entity->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto pos = box.y + box.h + 2;

	if (space)
		pos += 5;

	auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : g_cfg.player.type[type].weapon_color;
	color.SetAlpha(min(255.0f * esp_alpha_fade[m_entity->EntIndex()], color.a()));

	if (g_cfg.player.type[type].weapon[WEAPON_TEXT])
	{
		ImGui::PushFont(c_menu::get().OpenSans);
		g_Render->DrawString(box.x + box.w / 2, pos, color, render2::centered_x | render2::outline, c_menu::get().g_pLoaderFont, weapon->get_name().c_str());
		pos += 11;
		ImGui::PopFont();
	}

	if (g_cfg.player.type[type].weapon[WEAPON_ICON])
	{
		ImGui::PushFont(c_menu::get().weapons);
		g_Render->DrawString(box.x + box.w / 2, pos, color, render2::centered_x | render2::outline, c_menu::get().weapons, weapon->get_icon());
		ImGui::PopFont();
	}
}

void playeresp::draw_flags(player_t* e, const Box& box)
{
	auto weapon = e->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto _x = box.x + box.w + 3, _y = box.y - 3;

	if (g_cfg.player.type[type].flags[FLAGS_MONEY])
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(g_cfg.player.type[type].FLAGS_MONEY);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		g_Render->DrawString(_x, _y, color, render2::none | render2::outline, c_menu::get().g_pLoaderFont, "%i$", e->m_iAccount());
		_y += 10;
	}

	if (g_cfg.player.type[type].flags[FLAGS_ARMOR])
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(g_cfg.player.type[type].FLAGS_ARMOR);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		auto kevlar = e->m_ArmorValue() > 0;
		auto helmet = e->m_bHasHelmet();

		std::string text;

		if (helmet && kevlar)
			text = "HK";
		else if (kevlar)
			text = "K";

		if (kevlar)
		{
			g_Render->DrawString(_x, _y, color, render2::none | render2::outline, c_menu::get().g_pLoaderFont, text.c_str());
			_y += 10;
		}
	}

	if (g_cfg.player.type[type].flags[FLAGS_KIT] && e->m_bHasDefuser())
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(g_cfg.player.type[type].FLAGS_KIT);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		g_Render->DrawString(_x, _y, color, render2::none | render2::outline, c_menu::get().g_pLoaderFont, "DEFUSE");
		_y += 10;
	}

	if (g_cfg.player.type[type].flags[FLAGS_SCOPED])
	{
		auto scoped = e->m_bIsScoped();

		if (e == g_ctx.local())
			scoped = g_ctx.globals.scoped;

		if (scoped)
		{
			auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(g_cfg.player.type[type].FLAGS_SCOPED);
			color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

			g_Render->DrawString(_x, _y, color, render2::none | render2::outline, c_menu::get().g_pLoaderFont, "SCOPED");
			_y += 10;
		}
	}

	if (g_cfg.player.type[type].flags[FLAGS_FAKEDUCKING])
	{
		auto animstate = e->get_animation_state();

		if (animstate)
		{
			auto fakeducking = [&]() -> bool
			{
				static auto stored_tick = 0;
				static int crouched_ticks[65];

				if (animstate->m_fDuckAmount)
				{
					if (animstate->m_fDuckAmount < 0.9f && animstate->m_fDuckAmount > 0.5f)
					{
						if (stored_tick != m_globals()->m_tickcount)
						{
							crouched_ticks[e->EntIndex()]++;
							stored_tick = m_globals()->m_tickcount;
						}

						return crouched_ticks[e->EntIndex()] > 16;
					}
					else
						crouched_ticks[e->EntIndex()] = 0;
				}

				return false;
			};

			if (fakeducking() && e->m_fFlags() & FL_ONGROUND && !animstate->m_bInHitGroundAnimation)
			{
				auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(g_cfg.player.type[type].FLAGS_FAKEDUCKING);
				color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

				g_Render->DrawString(_x, _y, color, render2::none | render2::outline, c_menu::get().g_pLoaderFont, "FD");
				_y += 10;
			}
		}
	}

	if (g_cfg.player.type[type].flags[FLAGS_C4] && e->EntIndex() == g_ctx.globals.bomb_carrier)
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(g_cfg.player.type[type].FLAGS_C4);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		g_Render->DrawString(_x, _y, color, render2::none | render2::outline, c_menu::get().g_pLoaderFont, "BOMB");
		_y += 10;
	}
}

void playeresp::draw_lines(player_t* e)
{
	if (!g_cfg.player.type[type].snap_lines)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	Vector angle;

	if (!math::world_to_screen(e->GetAbsOrigin(), angle))
		return;

	auto color = g_cfg.player.type[type].snap_lines_color;

	g_Render->DrawLine(width / 2, height, angle.x, angle.y, color);
}