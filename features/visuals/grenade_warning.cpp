// This is an independent project of an individual developer.Dear PVS - Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "grenade_warning.h"
#include "world_esp.h"
#include "../../utils/render/Render.h"
#include "../../menu/menu.h"

const char* index_to_grenade_name(int index)
{
	switch (index)
	{
	case WEAPON_SMOKEGRENADE: return "smoke"; break;
	case WEAPON_HEGRENADE: return "he grenade"; break;
	case WEAPON_MOLOTOV:return "molotov"; break;
	case WEAPON_INCGRENADE:return "molotov"; break;
	}
}
const char* index_to_grenade_name_icon(int index)
{
	switch (index)
	{
	case WEAPON_SMOKEGRENADE: return "k"; break;
	case WEAPON_HEGRENADE: return "j"; break;
	case WEAPON_MOLOTOV:return "l"; break;
	case WEAPON_INCGRENADE:return "n"; break;
	}
}

void rotate_point(Vector2D& point, Vector2D origin, bool clockwise, float angle) {
	Vector2D delta = point - origin;
	Vector2D rotated;

	if (clockwise) {
		rotated = Vector2D(delta.x * cosf(angle) - delta.y * sinf(angle), delta.x * sinf(angle) + delta.y * cosf(angle));
	}
	else {
		rotated = Vector2D(delta.x * sinf(angle) - delta.y * cosf(angle), delta.x * cosf(angle) + delta.y * sinf(angle));
	}

	point = rotated + origin;
}

bool c_grenade_prediction::data_t::draw() const
{
	if (m_path.size() <= 1u
		|| m_globals()->m_curtime >= m_expire_time)
		return false;

	float distance = g_ctx.local()->m_vecOrigin().DistTo(m_origin) / 12;

	if (distance > 200.f)
		return false;

	static int iScreenWidth, iScreenHeight;
	m_engine()->GetScreenSize(iScreenWidth, iScreenHeight);

	auto prev_screen = ZERO;
	auto prev_on_screen = math::world_to_screen(std::get< Vector >(m_path.front()), prev_screen);

	for (auto i = 1u; i < m_path.size(); ++i) {
		auto cur_screen = ZERO, last_cur_screen = ZERO;
		const auto cur_on_screen = math::world_to_screen(std::get< Vector >(m_path.at(i)), cur_screen);

		if (prev_on_screen && cur_on_screen)
		{
			auto color = g_cfg.esp.grenade_proximity_warning_progress_color;
			if (i % 4)
				g_Render->DrawLine((int)prev_screen.x, (int)prev_screen.y, (int)cur_screen.x, (int)cur_screen.y, color, 2);
		}

		prev_screen = cur_screen;
		prev_on_screen = cur_on_screen;
	}

	float percent = ((m_expire_time - m_globals()->m_curtime) / TICKS_TO_TIME(m_tick));
	int alpha_damage = 0;

	if (m_index == WEAPON_HEGRENADE && distance <= 20)
	{
		alpha_damage = 255 - 255 * (distance / 20);
	}

	if ((m_index == WEAPON_MOLOTOV || m_index == WEAPON_INCGRENADE) && distance <= 15)
	{
		alpha_damage = 255 - 255 * (distance / 15);
	}

	auto col = g_cfg.esp.grenade_proximity_warning_progress_color;
	Ray_t ray;
	CTraceFilterWorldAndPropsOnly filter;
	CGameTrace trace;

	ray.Init(m_origin, m_origin - Vector(0.0f, 0.0f, 105.0f));
	m_trace()->TraceRay(ray, MASK_SOLID, &filter, &trace);

	if (trace.fraction < 1.0f && m_index == 46) 
	{
		auto vvt = trace.endpos + Vector(0.0f, 0.0f, 2.0f);
		g_Render->DrawRing3D1(vvt.x, vvt.y, vvt.z, 90, 140, Color(col.r(), col.g(), col.b(), 255), Color(140, 140, 140, 35), 2, percent);
		static auto g_size = Vector2D(35.0f, 5.0f);
		Vector saddadsadadasdasda;
		if (math::world_to_screen(vvt, saddadsadadasdasda))
			g_Render->CircleFilled(saddadsadadasdasda.x, saddadsadadasdasda.y - g_size.y * 0.5f - 12, 21, Color(25, 25, 25, col.a()), 60);
		g_Render->CircleFilled(saddadsadadasdasda.x, saddadsadadasdasda.y - g_size.y * 0.5f - 12, 21, Color(200, 25, 25, alpha_damage), 60);
		g_Render->two_sided_arc(saddadsadadasdasda.x, saddadsadadasdasda.y - g_size.y * 0.5f - 12, 20, percent, Color(col.r(), col.g(), col.b(), 255), 3);
		g_Render->DrawString(saddadsadadasdasda.x, saddadsadadasdasda.y - g_size.y * 0.5f - 12, Color::White, HFONT_CENTERED_X | HFONT_CENTERED_Y, c_menu::get().weapons, index_to_grenade_name_icon(m_index));
	}
	else 
	{
		static auto g_size = Vector2D(35.0f, 5.0f);
		g_Render->CircleFilled(prev_screen.x, prev_screen.y - g_size.y * 0.5f - 12, 21, Color(25, 25, 25, col.a()), 60);
		g_Render->CircleFilled(prev_screen.x, prev_screen.y - g_size.y * 0.5f - 12, 21, Color(200, 25, 25, alpha_damage), 60);
		g_Render->two_sided_arc(prev_screen.x, prev_screen.y - g_size.y * 0.5f - 12, 20, percent, Color(col.r(), col.g(), col.b(), 255), 3);
		g_Render->DrawString(prev_screen.x, prev_screen.y - g_size.y * 0.5f - 12, Color::White, HFONT_CENTERED_X | HFONT_CENTERED_Y, c_menu::get().weapons, index_to_grenade_name_icon(m_index));
	}


	auto isOnScreen = [](Vector origin, Vector& screen) -> bool
	{
		if (!math::world_to_screen(origin, screen))
			return false;

		static int iScreenWidth, iScreenHeight;
		m_engine()->GetScreenSize(iScreenWidth, iScreenHeight);

		auto xOk = iScreenWidth > screen.x;
		auto yOk = iScreenHeight > screen.y;

		return xOk && yOk;
	};

	Vector screenPos;
	Vector vEnemyOrigin = m_origin;
	Vector vLocalOrigin = g_ctx.local()->GetAbsOrigin();

	if (!g_ctx.local()->is_alive())
		vLocalOrigin = m_input()->m_vecCameraOffset;

	Vector viewAngles;
	m_engine()->GetViewAngles(viewAngles);

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	if (g_cfg.esp.offscreen_proximity && !isOnScreen(vEnemyOrigin, screenPos))
	{
		auto screenCenter = Vector2D(width * 0.5f, height * 0.5f);
		auto angleYawRad = DEG2RAD(viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, vEnemyOrigin).y - 90.0f);

		auto radius = 80 - 70 * float(alpha_damage / 255.f);
		auto size = 6 + 6 * float(alpha_damage / 255.f);

		auto newPointX = screenCenter.x + ((((width - (size * 3)) * 0.5f) * (radius / 100.0f)) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
		auto newPointY = screenCenter.y + ((((height - (size * 3)) * 0.5f) * (radius / 100.0f)) * sin(angleYawRad));
		auto newPointX2 = screenCenter.x + ((((width - (size * 15)) * 0.5f) * (radius / 100.0f)) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
		auto newPointY2 = screenCenter.y + ((((height - (size * 15)) * 0.5f) * (radius / 100.0f)) * sin(angleYawRad));
		std::array <Vector2D, 3> points
		{
			Vector2D(newPointX - size, newPointY - size),
			Vector2D(newPointX + size, newPointY),
			Vector2D(newPointX - size, newPointY + size)
		};

		auto warn = Vector2D(newPointX2, newPointY2);
		math::rotate_triangle(points, viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, vEnemyOrigin).y - 90.0f);

		g_Render->CircleFilled(warn.x, warn.y - (size * 3) * 0.5f, 21, Color(25, 25, 25, col.a()), 60);
		g_Render->CircleFilled(warn.x, warn.y - (size * 3) * 0.5f, 21, Color(200, 25, 25, alpha_damage), 60);
		g_Render->two_sided_arc(warn.x, warn.y - (size * 3) * 0.5f, 20, percent, Color(col.r(), col.g(), col.b(), 255), 3);
		g_Render->DrawString(warn.x, warn.y - (size * 3) * 0.5f, Color::White, HFONT_CENTERED_X | HFONT_CENTERED_Y, c_menu::get().weapons, index_to_grenade_name_icon(m_index));
		g_Render->TriangleFilled(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(25, 25, 25, 150));
		g_Render->Triangle(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(col.r(), col.g(), col.b(), 255));
	}

	return true;
}

void c_grenade_prediction::grenade_warning(projectile_t* entity)
{
	auto& predicted_nades = c_grenade_prediction::get().get_list();

	static auto last_server_tick = m_globals()->m_curtime;
	if (last_server_tick != m_globals()->m_curtime) {
		predicted_nades.clear();

		last_server_tick = m_globals()->m_curtime;
	}

	if (entity->IsDormant() || !g_cfg.esp.grenade_proximity_warning)
		return;

	const auto client_class = entity->GetClientClass();
	if (!client_class
		|| client_class->m_ClassID != CMolotovProjectile && client_class->m_ClassID != CBaseCSGrenadeProjectile)
		return;

	if (client_class->m_ClassID == CBaseCSGrenadeProjectile) {
		const auto model = entity->GetModel();
		if (!model)
			return;

		const auto studio_model = m_modelinfo()->GetStudioModel(model);
		if (!studio_model
			|| std::string_view(studio_model->szName).find("fraggrenade") == std::string::npos)
			return;
	}

	const auto handle = entity->GetRefEHandle().ToLong();

	if (entity->m_nExplodeEffectTickBegin() || !entity->m_hThrower().IsValid() || (entity->m_hThrower().Get()->m_iTeamNum() == g_ctx.local()->m_iTeamNum() && entity->m_hThrower().Get() != g_ctx.local())) {
		predicted_nades.erase(handle);

		return;
	}

	if (predicted_nades.find(handle) == predicted_nades.end()) 
	{
		predicted_nades.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(handle),
			std::forward_as_tuple(
				entity->m_hThrower().Get(),
				client_class->m_ClassID == CMolotovProjectile ? WEAPON_MOLOTOV : WEAPON_HEGRENADE,
				entity->m_vecOrigin(), reinterpret_cast<player_t*>(entity)->m_vecVelocity(),
				entity->m_flSpawnTime(), TIME_TO_TICKS(reinterpret_cast<player_t*>(entity)->m_flSimulationTime() - entity->m_flSpawnTime())
			)
		);
	}

	if (predicted_nades.at(handle).draw())
		return;

	predicted_nades.erase(handle);
}