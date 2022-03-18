#include "world_esp.h"
#include "../../menu/menu.h"
#include "grenade_warning.h"
#include "../../utils/Render/Render.h"

void worldesp::paint_traverse()
{
	for (int i = 1; i <= m_entitylist()->GetHighestEntityIndex(); i++)
	{
		auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->is_player())
			continue;

		if (e->IsDormant())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class)
			continue;

		switch (client_class->m_ClassID)
		{
		case CEnvTonemapController:
			world_modulation(e);
			break;
		case CInferno:
			molotov_timer(e);
			break;
		case CSmokeGrenadeProjectile:
			smoke_timer(e);
			break;
		case CPlantedC4:
			bomb_timer(e);
			break;
		default:
			grenade_projectiles(e);
			grenade_warning(e);

			if (client_class->m_ClassID == CAK47 || client_class->m_ClassID == CDEagle || client_class->m_ClassID >= CWeaponAug && client_class->m_ClassID <= CWeaponZoneRepulsor)
				dropped_weapons(e);
			break;
		}
	}
}

void worldesp::skybox_changer()
{
	static auto load_skybox = reinterpret_cast<void(__fastcall*)(const char*)>(util::FindSignature(crypt_str("engine.dll"), crypt_str("55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45")));
	auto skybox_name = backup_skybox;

	if (!g_cfg.esp.skybox)
		return;

	switch (g_cfg.esp.skybox)
	{
	case 0: skybox_name = "cs_baggage_skybox_"; break;
	case 1: skybox_name = "cs_tibet"; break;
	case 2: skybox_name = "embassy"; break;
	case 3: skybox_name = "italy"; break;
	case 4: skybox_name = "jungle"; break;
	case 5: skybox_name = "nukeblank"; break;
	case 6: skybox_name = "office"; break;
	case 7: skybox_name = "sky_cs15_daylight01_hdr"; break;
	case 8: skybox_name = "sky_cs15_daylight02_hdr"; break;
	case 9: skybox_name = "sky_cs15_daylight03_hdr"; break;
	case 10: skybox_name = "sky_cs15_daylight04_hdr"; break;
	case 11: skybox_name = "sky_csgo_cloudy01"; break;
	case 12: skybox_name = "sky_csgo_night_flat"; break;
	case 13: skybox_name = "sky_csgo_night02"; break;
	case 14: skybox_name = "sky_day02_05_hdr"; break;
	case 15: skybox_name = "sky_day02_05"; break;
	case 16: skybox_name = "sky_dust"; break;
	case 17: skybox_name = "sky_l4d_rural02_ldr"; break;
	case 18: skybox_name = "sky_venice"; break;
	case 19: skybox_name = "vertigo_hdr"; break;
	case 20: skybox_name = "vertigo"; break;
	case 21: skybox_name = "vertigoblue_hdr"; break;
	case 22: skybox_name = "vietnam"; break;
	case 23:
		skybox_name = g_cfg.esp.custom_skybox;
		break;
	}

	static auto skybox_number = 0;
	static auto old_skybox_name = skybox_name;

	static auto color_r = (unsigned char)255;
	static auto color_g = (unsigned char)255;
	static auto color_b = (unsigned char)255;

	if (skybox_number != g_cfg.esp.skybox)
	{
		changed = true;
		skybox_number = g_cfg.esp.skybox;
	}
	else if (old_skybox_name != skybox_name)
	{
		changed = true;
		old_skybox_name = skybox_name;
	}
	else if (color_r != g_cfg.esp.skybox_color[0])
	{
		changed = true;
		color_r = g_cfg.esp.skybox_color[0];
	}
	else if (color_g != g_cfg.esp.skybox_color[1])
	{
		changed = true;
		color_g = g_cfg.esp.skybox_color[1];
	}
	else if (color_b != g_cfg.esp.skybox_color[2])
	{
		changed = true;
		color_b = g_cfg.esp.skybox_color[2];
	}

	if (changed)
	{
		changed = false;
		load_skybox(skybox_name.c_str());

		auto materialsystem = m_materialsystem();

		for (auto i = materialsystem->FirstMaterial(); i != materialsystem->InvalidMaterial(); i = materialsystem->NextMaterial(i))
		{
			auto material = materialsystem->GetMaterial(i);

			if (!material)
				continue;

			if (strstr(material->GetTextureGroupName(), crypt_str("SkyBox")))
				material->ColorModulate(g_cfg.esp.skybox_color[0] / 255.0f, g_cfg.esp.skybox_color[1] / 255.0f, g_cfg.esp.skybox_color[2] / 255.0f);
		}
	}
}

void worldesp::fog_changer()
{
	static auto fog_override = m_cvar()->FindVar(crypt_str("fog_override"));

	if (!g_cfg.esp.fog)
	{
		if (fog_override->GetBool())
			fog_override->SetValue(FALSE);

		return;
	}

	if (!fog_override->GetBool())
		fog_override->SetValue(TRUE);

	static auto fog_start = m_cvar()->FindVar(crypt_str("fog_start"));

	if (fog_start->GetInt())
		fog_start->SetValue(0);

	static auto fog_end = m_cvar()->FindVar(crypt_str("fog_end"));

	if (fog_end->GetInt() != g_cfg.esp.fog_distance)
		fog_end->SetValue(g_cfg.esp.fog_distance);

	static auto fog_maxdensity = m_cvar()->FindVar(crypt_str("fog_maxdensity"));

	if (fog_maxdensity->GetFloat() != (float)g_cfg.esp.fog_density * 0.01f)
		fog_maxdensity->SetValue((float)g_cfg.esp.fog_density * 0.01f);

	char buffer_color[12];
	sprintf_s(buffer_color, 12, "%i %i %i", g_cfg.esp.fog_color.r(), g_cfg.esp.fog_color.g(), g_cfg.esp.fog_color.b());

	static auto fog_color = m_cvar()->FindVar(crypt_str("fog_color"));

	if (strcmp(fog_color->GetString(), buffer_color))
		fog_color->SetValue(buffer_color);
}

void worldesp::world_modulation(entity_t* entity)
{
	if (!g_cfg.esp.world_modulation)
		return;

	entity->set_m_bUseCustomBloomScale(TRUE);
	entity->set_m_flCustomBloomScale(g_cfg.esp.bloom * 0.01f);

	entity->set_m_bUseCustomAutoExposureMin(TRUE);
	entity->set_m_flCustomAutoExposureMin(g_cfg.esp.exposure * 0.001f);

	entity->set_m_bUseCustomAutoExposureMax(TRUE);
	entity->set_m_flCustomAutoExposureMax(g_cfg.esp.exposure * 0.001f);
}

#define PI			3.14159265358979323846
void arc(int x, int y, int r1, int r2, int s, int d, Color col)
{
	for (auto i = s; i < s + d; i++)
	{

		auto rad = i * PI / 180;

		g_Render->DrawLine(x + cosf(rad) * r1, y + sinf(rad) * r1, x + cosf(rad) * r2, y + sinf(rad) * r2, col);
	}
}

void worldesp::molotov_timer(entity_t* entity)
{
	if (!g_cfg.esp.molotov_timer)
		return;

	auto inferno = reinterpret_cast<inferno_t*>(entity);
	auto origin = inferno->GetAbsOrigin();
	auto col = g_cfg.esp.molotov_timer_color;
	Vector screen_origin;
	auto spawn_time = inferno->get_spawn_time();
	auto factor = (spawn_time + inferno_t::get_expiry_time() - m_globals()->m_curtime) / inferno_t::get_expiry_time();

	auto dpos = origin;
	Vector mins, maxs;
	inferno->GetClientRenderable()->GetRenderBounds(mins, maxs);

	int* m_fireXDelta = reinterpret_cast<int*>((DWORD)inferno + 0x9E4);
	int* m_fireYDelta = reinterpret_cast<int*>((DWORD)inferno + 0xB74);
	int* m_fireZDelta = reinterpret_cast<int*>((DWORD)inferno + 0xD04);

	g_Render->DrawRing3D1(origin.x, origin.y, origin.z, 150, 360, Color(col.r(), col.g(), col.b(), 255), Color(col.r(), col.g(), col.b(), 35), 2, factor, true);

	if (!math::world_to_screen(origin, screen_origin))
		return;

	static auto size = Vector2D(35.0f, 5.0f);
	g_Render->CircleFilled(screen_origin.x, screen_origin.y - size.y * 0.5f - 12, 21, Color(25, 25, 25, col.a()), 60);
	g_Render->two_sided_arc(screen_origin.x, screen_origin.y - size.y * 0.5f - 12, 20, 1.f - factor, Color(col.r(), col.g(), col.b()), 3);
	g_Render->DrawString(screen_origin.x, screen_origin.y - size.y * 0.5f - 12, Color(col.r(), col.g(), col.b()), render2::centered_x | render2::centered_y | render2::outline, c_menu::get().Timers, "M");
}

void worldesp::smoke_timer(entity_t* entity)
{
	if (!g_cfg.esp.smoke_timer)
		return;

	auto smoke = reinterpret_cast<smoke_t*>(entity);

	if (!smoke->m_nSmokeEffectTickBegin() || !smoke->m_bDidSmokeEffect())
		return;
	auto spawn_time = TICKS_TO_TIME(smoke->m_nSmokeEffectTickBegin());
	auto factor = (spawn_time + smoke_t::get_expiry_time() - m_globals()->m_curtime) / smoke_t::get_expiry_time();
	auto origin = smoke->GetAbsOrigin();

	auto col = g_cfg.esp.smoke_timer_color;
	g_Render->DrawRing3D1(origin.x, origin.y, origin.z, 150, 360, Color(col.r(), col.g(), col.b(), 255), Color(col.r(), col.g(), col.b(), 35), 2, factor, true);
	Vector screen_origin;

	if (!math::world_to_screen(origin, screen_origin))
		return;

	static auto size = Vector2D(35.0f, 5.0f);
	g_Render->CircleFilled(screen_origin.x, screen_origin.y - size.y * 0.5f - 12, 19, Color(25, 25, 25, col.a()), 60);
	g_Render->two_sided_arc(screen_origin.x, screen_origin.y - size.y * 0.5f - 12, 18, 1.f - factor, Color(col.r(), col.g(), col.b()), 2);
	g_Render->DrawString(screen_origin.x, screen_origin.y - size.y * 0.5f - 12, Color(col.r(), col.g(), col.b()), render2::centered_x | render2::centered_y | render2::outline, c_menu::get().Timers, "S");
}

void worldesp::grenade_projectiles(entity_t* entity)
{
	auto client_class = entity->GetClientClass();

	if (!client_class)
		return;

	auto model = entity->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto name = (std::string)studio_model->szName;

	if (name.find("thrown") != std::string::npos ||
		client_class->m_ClassID == CBaseCSGrenadeProjectile || client_class->m_ClassID == CDecoyProjectile || client_class->m_ClassID == CMolotovProjectile)
	{
		auto grenade_origin = entity->GetAbsOrigin();
		auto grenade_position = ZERO;

		if (!math::world_to_screen(grenade_origin, grenade_position))
			return;

		std::string grenade_name, grenade_icon;

		if (name.find("flashbang") != std::string::npos)
		{
			grenade_name = "FLASHBANG";
			grenade_icon = "i";
		}
		else if (name.find("smokegrenade") != std::string::npos)
		{
			grenade_name = "SMOKE";
			grenade_icon = "k";
		}
		else if (name.find("incendiarygrenade") != std::string::npos)
		{
			grenade_name = "INCENDIARY";
			grenade_icon = "n";
		}
		else if (name.find("molotov") != std::string::npos)
		{
			grenade_name = "MOLOTOV";
			grenade_icon = "l";
		}
		else if (name.find("fraggrenade") != std::string::npos)
		{
			grenade_name = "HE GRENADE";
			grenade_icon = "j";
		}
		else if (name.find("decoy") != std::string::npos)
		{
			grenade_name = "DECOY";
			grenade_icon = "m";
		}
		else
			return;

		Box box;

		if (util::get_bbox(entity, box, false))
		{
			if (g_cfg.esp.grenade_esp[GRENADE_BOX])
			{
				g_Render->Rect(box.x, box.y, box.w, box.h, g_cfg.esp.grenade_box_color);
				g_Render->Rect(box.x - 1, box.y - 1, box.w + 2, box.h + 2, Color(0, 0, 0, 80));
				g_Render->Rect(box.x + 1, box.y + 1, box.w - 2, box.h - 2, Color(0, 0, 0, 80));

				if (g_cfg.esp.grenade_esp[GRENADE_ICON])
					g_Render->DrawString(box.x + box.w / 2, box.y - 21, g_cfg.esp.projectiles_color, render2::centered_x | render2::outline, c_menu::get().weapons, grenade_icon.c_str());

				if (g_cfg.esp.grenade_esp[GRENADE_TEXT])
					g_Render->DrawString(box.x + box.w / 2, box.y + box.h + 2, g_cfg.esp.projectiles_color, render2::centered_x | render2::outline, c_menu::get().OpenSans, grenade_name.c_str());
			}
			else
			{
				if (g_cfg.esp.grenade_esp[GRENADE_ICON] && g_cfg.esp.grenade_esp[GRENADE_TEXT])
				{
					g_Render->DrawString(box.x + box.w / 2, box.y + box.h / 2 - 10, g_cfg.esp.projectiles_color, render2::centered_x | render2::outline, c_menu::get().weapons, grenade_icon.c_str());
					g_Render->DrawString(box.x + box.w / 2, box.y + box.h / 2 + 7, g_cfg.esp.projectiles_color, render2::centered_x | render2::outline, c_menu::get().OpenSans, grenade_name.c_str());
				}
				else
				{
					if (g_cfg.esp.grenade_esp[GRENADE_ICON])
						g_Render->DrawString(box.x + box.w / 2, box.y + box.h / 2, g_cfg.esp.projectiles_color, render2::centered_x | render2::outline, c_menu::get().weapons, grenade_icon.c_str());

					if (g_cfg.esp.grenade_esp[GRENADE_TEXT])
						g_Render->DrawString(box.x + box.w / 2, box.y + box.h / 2, g_cfg.esp.projectiles_color, render2::centered_x | render2::outline, c_menu::get().OpenSans, grenade_name.c_str());
				}
			}
		}
	}
}

void worldesp::grenade_warning(entity_t* entity)
{
	if (!g_cfg.esp.grenade_proximity_warning)
		return;

	c_grenade_prediction::get().grenade_warning((projectile_t*)entity);
}

void worldesp::bomb_timer(entity_t* entity)
{
	if (!g_ctx.globals.bomb_timer_enable)
		return;

	static auto mp_c4timer = m_cvar()->FindVar(crypt_str("mp_c4timer"));
	auto bomb = (CCSBomb*)entity;

	auto c4timer = mp_c4timer->GetFloat();
	auto bomb_timer = bomb->m_flC4Blow() - m_globals()->m_curtime;

	if (bomb_timer < 0.0f)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto factor = bomb_timer / c4timer * height;

	auto red_factor = (int)(255.0f - bomb_timer / c4timer * 255.0f);
	auto green_factor = (int)(bomb_timer / c4timer * 255.0f);
	auto distance = g_ctx.local()->get_shoot_position().DistTo(entity->GetAbsOrigin());
	auto a = 450.7f;
	auto b = 75.68f;
	auto c = 789.2f;
	auto d = ((distance - b) / c);
	auto fl_damage = a * exp(-d * d);

	//g_Render->FilledRect(0, height - factor, 26, factor, Color(0, 0, 0, 100));

	float damage;
	damage = float((std::max)((int)ceilf(util::csgo_armor(fl_damage, g_ctx.local()->m_ArmorValue())), 0));

	//convert damage to string
	std::string damage_text;
	damage_text += "-";
	damage_text += std::to_string((int)(damage));
	damage_text += "HP";

	if (g_ctx.local()->is_alive() && damage >= g_ctx.local()->m_iHealth())
		damage_text = "FATAL";

	g_Render->FilledRect(0, height - factor, 18, factor, Color(red_factor, green_factor, 0, 255));
	g_Render->DrawString(19, height - factor, Color(255, 255, 255, 255), render2::none, c_menu::get().futura_small, damage_text.c_str());


	auto text_position = height - factor + 11;

	if (text_position > height - 9)
		text_position = height - 9;
	Vector screen;

	factor = bomb_timer / c4timer;

	auto c4_defuse_time = bomb->m_flDefuseCountDown() - (m_globals()->m_intervalpertick * g_ctx.local()->m_nTickBase());
	auto defuse_value = c4_defuse_time / 6;

	if (math::world_to_screen(entity->GetAbsOrigin(), screen))
	{
		if (bomb->m_hBombDefuser() > 0)
		{
			if (c4timer > c4_defuse_time)
			{
				g_Render->CircleFilled(screen.x, screen.y - 50, 30, Color(20, 20, 20, 175), 30);
				g_Render->DrawString(screen.x, screen.y - 75, Color(25, 80, 153, 200), render2::centered_x, c_menu::get().ico_menu, "r");
				g_Render->DrawString2(screen.x - 7, screen.y - 55, Color(232, 232, 232, 200), render2::none, c_menu::get().futura_small, std::to_string((int)(round(c4_defuse_time))) + " s");
				arc(screen.x, screen.y - 50, 30, 32, -90, 360 * defuse_value, Color(25, 80, 153, 200));
			}
			else
			{
				g_Render->CircleFilled(screen.x, screen.y - 50, 30, Color(20, 20, 20, 175), 30);
				g_Render->DrawString(screen.x, screen.y - 75, Color(25, 80, 153, 200), render2::centered_x, c_menu::get().ico_menu, "r");
				g_Render->DrawString2(screen.x - 7, screen.y - 55, Color(232, 232, 232, 200), render2::none, c_menu::get().futura_small, "NO TIME");
				arc(screen.x, screen.y - 50, 30, 32, -90, 360 * defuse_value, Color(25, 80, 153, 200));
			}
		}
		else
		{
			g_Render->CircleFilled(screen.x, screen.y - 50, 30, Color(20, 20, 20, 175), 30);
			g_Render->DrawString(screen.x, screen.y - 75, Color(255, 70, 70, 200), render2::centered_x, c_menu::get().ico_menu, "o");
			g_Render->DrawString2(screen.x - 7, screen.y - 55, Color(232, 232, 232, 200), render2::none, c_menu::get().futura_small, std::to_string((int)(round(bomb_timer))) + " s");
			arc(screen.x, screen.y - 50, 30, 32, -90, 360 * factor, Color(232, 232, 232, 200));
		}
	}
}

void worldesp::dropped_weapons(entity_t* entity)
{
	auto weapon = (weapon_t*)entity;
	auto owner = (player_t*)m_entitylist()->GetClientEntityFromHandle(weapon->m_hOwnerEntity());

	if (owner->is_player())
		return;

	Box box;

	if (util::get_bbox(weapon, box, false))
	{
		auto offset = 0;

		if (g_cfg.esp.weapon[WEAPON_BOX])
		{
			g_Render->Rect(box.x, box.y, box.w, box.h, g_cfg.esp.box_color);
			g_Render->Rect(box.x - 1, box.y - 1, box.w + 2, box.h + 2, Color(0, 0, 0, 80));
			g_Render->Rect(box.x + 1, box.y + 1, box.w - 2, box.h - 2, Color(0, 0, 0, 80));

			if (g_cfg.esp.weapon[WEAPON_ICON])
			{
				g_Render->DrawString(box.x + box.w / 2, box.y + box.h + 20, g_cfg.esp.weapon_color, render2::centered_x | render2::outline, c_menu::get().weapons, weapon->get_icon());
				offset = 14;
			}

			if (g_cfg.esp.weapon[WEAPON_TEXT])
				g_Render->DrawString(box.x + box.w / 2, box.y + box.h + 10, g_cfg.esp.weapon_color, render2::centered_x | render2::outline, c_menu::get().OpenSans, weapon->get_name().c_str());

			if (g_cfg.esp.weapon[WEAPON_AMMO] && entity->GetClientClass()->m_ClassID != CBaseCSGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSmokeGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSensorGrenadeProjectile && entity->GetClientClass()->m_ClassID != CMolotovProjectile && entity->GetClientClass()->m_ClassID != CDecoyProjectile)
			{
				auto inner_back_color = Color::Black;
				inner_back_color.SetAlpha(153);

				g_Render->FilledRect(box.x - 1, box.y + box.h + 5, box.w + 2, 4, Color(0, 0, 0, 120));
				g_Render->FilledRect(box.x, box.y + box.h + 6, weapon->m_iClip1() * box.w / m_weaponsystem()->GetWeaponData(weapon->m_iItemDefinitionIndex())->iMaxClip1, 2, g_cfg.esp.weapon_ammo_color);
			}

		}
		else
		{
			if (g_cfg.esp.weapon[WEAPON_ICON])
				g_Render->DrawString(box.x + box.w / 2, box.y + box.h + 20, g_cfg.esp.weapon_color, render2::centered_x | render2::outline, c_menu::get().weapons, weapon->get_icon());

			if (g_cfg.esp.weapon[WEAPON_TEXT])
				g_Render->DrawString(box.x + box.w / 2, box.y + box.h + 10, g_cfg.esp.weapon_color, render2::centered_x | render2::outline, c_menu::get().OpenSans, weapon->get_name().c_str());

			if (g_cfg.esp.weapon[WEAPON_AMMO] && entity->GetClientClass()->m_ClassID != CBaseCSGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSmokeGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSensorGrenadeProjectile && entity->GetClientClass()->m_ClassID != CMolotovProjectile && entity->GetClientClass()->m_ClassID != CDecoyProjectile)
			{
				static auto pos = 0;

				if (g_cfg.esp.weapon[WEAPON_ICON] && g_cfg.esp.weapon[WEAPON_TEXT])
					pos = 19;
				else if (g_cfg.esp.weapon[WEAPON_ICON])
					pos = 8;
				else if (g_cfg.esp.weapon[WEAPON_TEXT])
					pos = 19;

				auto inner_back_color = Color::Black;
				inner_back_color.SetAlpha(153);

				g_Render->FilledRect(box.x - 1, box.y + box.h / 2 + pos - 1, box.w + 2, 4, Color(0, 0, 0, 120));
				g_Render->FilledRect(box.x, box.y + box.h / 2 + pos, weapon->m_iClip1() * box.w / m_weaponsystem()->GetWeaponData(weapon->m_iItemDefinitionIndex())->iMaxClip1, 2, g_cfg.esp.weapon_ammo_color);
			}

		}
	}
}