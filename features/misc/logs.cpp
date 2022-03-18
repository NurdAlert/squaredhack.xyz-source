#include "logs.h"
#include "../../../menu/menu.h"

void eventlogs::paint_traverse()
{
	if (logs.empty())
		return;

	auto last_y = -19;

	const auto font = fonts[WATERMARK];

	const auto name_font = fonts[WATERMARK];

	for (unsigned int i = 0; i < logs.size(); i++)
	{
		auto& log = logs.at(i);

		if (util::epoch_time() - log.log_time > 5500)
		{
			float factor = (log.log_time + 5900) - util::epoch_time();
			factor /= 1000;

			auto opacity = int(255 * factor);

			if (opacity < 2) {
				logs.erase(logs.begin() + i);
				continue;
			}
			log.x -= 20 * (factor * 1.25);
			log.y -= 0 * (factor * 1.25);
		}

		const auto text = log.message.c_str();

		const auto color = log.color;

		auto name_size = render::get().text_width(name_font, text);

		float logs[4] = { g_cfg.misc.log_color.r() / 255, g_cfg.misc.log_color.g() / 255, g_cfg.misc.log_color.b() / 255, 1.f };

		render::get().rect_filled(log.x + 5, last_y + log.y + 20, name_size + 18, 20, Color(15, 15, 15, 245));

		render::get().text(font, log.x + 15, last_y + log.y + 23, Color(200, 200, 200), HFONT_CENTERED_NONE, text);

		last_y += 25;
	}
}

void eventlogs::events(IGameEvent* event)
{
	static auto get_hitgroup_name = [](int hitgroup) -> std::string
	{
		switch (hitgroup)
		{
		case HITGROUP_HEAD:
			return crypt_str("head");
		case HITGROUP_CHEST:
			return crypt_str("chest");
		case HITGROUP_STOMACH:
			return crypt_str("stomach");
		case HITGROUP_LEFTARM:
			return crypt_str("left arm");
		case HITGROUP_RIGHTARM:
			return crypt_str("right arm");
		case HITGROUP_LEFTLEG:
			return crypt_str("left leg");
		case HITGROUP_RIGHTLEG:
			return crypt_str("right leg");
		default:
			return crypt_str("generic");
		}
	};

	if (g_cfg.misc.events_to_log[EVENTLOG_HIT] && !strcmp(event->GetName(), crypt_str("player_hurt")))
	{
		auto userid = event->GetInt(crypt_str("userid")), attacker = event->GetInt(crypt_str("attacker"));

		if (!userid || !attacker)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid), attacker_id = m_engine()->GetPlayerForUserID(attacker);

		player_info_t userid_info, attacker_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		if (!m_engine()->GetPlayerInfo(attacker_id, &attacker_info))
			return;

		auto m_victim = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		std::stringstream ss;

		if (attacker_id == m_engine()->GetLocalPlayer() && userid_id != m_engine()->GetLocalPlayer())
		{
			ss << crypt_str("Hit ") << userid_info.szName << crypt_str(" in the ") << get_hitgroup_name(event->GetInt(crypt_str("hitgroup"))) << crypt_str(" for ") << event->GetInt(crypt_str("dmg_health"));
			ss << crypt_str(" damage (") << event->GetInt(crypt_str("health")) << crypt_str(" health remaining)");

			add(ss.str());
		}
		else if (userid_id == m_engine()->GetLocalPlayer() && attacker_id != m_engine()->GetLocalPlayer())
		{
			ss << crypt_str("Take ") << event->GetInt(crypt_str("dmg_health")) << crypt_str(" damage from ") << attacker_info.szName << crypt_str(" in the ") << get_hitgroup_name(event->GetInt(crypt_str("hitgroup")));
			ss << crypt_str(" (") << event->GetInt(crypt_str("health")) << crypt_str(" health remaining)");

			add(ss.str());
		}
	}

	if (g_cfg.misc.events_to_log[EVENTLOG_ITEM_PURCHASES] && !strcmp(event->GetName(), crypt_str("item_purchase")))
	{
		auto userid = event->GetInt(crypt_str("userid"));

		if (!userid)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid);

		player_info_t userid_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		auto m_player = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		if (!g_ctx.local() || !m_player)
			return;

		if (g_ctx.local() == m_player)
			g_ctx.globals.should_buy = 0;

		if (m_player->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
			return;

		std::string weapon = event->GetString(crypt_str("weapon"));

		std::stringstream ss;
		ss << userid_info.szName << crypt_str(" bought ") << weapon;

		add(ss.str());
	}

	if (g_cfg.misc.events_to_log[EVENTLOG_BOMB] && !strcmp(event->GetName(), crypt_str("bomb_beginplant")))
	{
		auto userid = event->GetInt(crypt_str("userid"));

		if (!userid)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid);

		player_info_t userid_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		auto m_player = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		if (!m_player)
			return;

		std::stringstream ss;
		ss << userid_info.szName << crypt_str(" has began planting the bomb");

		add(ss.str());
	}

	if (g_cfg.misc.events_to_log[EVENTLOG_BOMB] && !strcmp(event->GetName(), crypt_str("bomb_begindefuse")))
	{
		auto userid = event->GetInt(crypt_str("userid"));

		if (!userid)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid);

		player_info_t userid_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		auto m_player = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		if (!m_player)
			return;

		std::stringstream ss;
		ss << userid_info.szName << crypt_str(" has began defusing the bomb ") << (event->GetBool(crypt_str("haskit")) ? crypt_str("with defuse kit") : crypt_str("without defuse kit"));

		add(ss.str());
	}
}

void eventlogs::add(std::string text, bool full_display)
{
	logs.emplace_front(loginfo_t(util::epoch_time(), text, g_cfg.misc.log_color));

	if (!full_display)
		return;

	last_log = true;

	m_cvar()->ConsoleColorPrintf(g_cfg.misc.log_color, crypt_str("[ squaredhack ] "));
	m_cvar()->ConsoleColorPrintf(Color::White, text.c_str());
	m_cvar()->ConsolePrintf(crypt_str("\n"));
}