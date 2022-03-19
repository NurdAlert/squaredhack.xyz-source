#include "misc.h"
#include "../exploit/exploit.h"
#include "fakelag.h"
#include "..\ragebot\aim.h"
#include "..\visuals\world_esp.h"
#include "..\visuals\hitchams.h"
#include "prediction_system.h"
#include "logs.h"
#include "../../../menu/menu.h"
#include "../../utils/Render/Render.h"
#include "..\ragebot\antiaim.h"

void copy_convert(const uint8_t* rgba, uint8_t* out, const size_t size)
{
	auto in = reinterpret_cast<const uint32_t*>(rgba);
	auto buf = reinterpret_cast<uint32_t*>(out);
	for (auto i = 0u; i < (size / 4); ++i)
	{
		const auto pixel = *in++;
		*buf++ = (pixel & 0xFF00FF00) | ((pixel & 0xFF0000) >> 16) | ((pixel & 0xFF) << 16);
	}
}

std::string comp_name()
{
	char buff[MAX_PATH];
	GetEnvironmentVariableA("USERNAME", buff, MAX_PATH);

	return std::string(buff);
}

void misc::watermark()
{
	if (!g_cfg.menu.watermark)
		return;

	std::string framerate;
	std::string delay;
	std::string times;
	std::string name_cheat;

	time_t lt;
	struct tm* t_m;

	lt = time(nullptr);
	t_m = localtime(&lt);

	auto time_h = t_m->tm_hour;
	auto time_m = t_m->tm_min;
	auto time_s = t_m->tm_sec;

	std::string time;

	if (time_h < 10)
		time += "0";

	time += std::to_string(time_h) + ":";

	if (time_m < 10)
		time += "0";

	time += std::to_string(time_m) + ":";

	if (time_s < 10)
		time += "0";

	time += std::to_string(time_s);
	g_ctx.globals.time = std::move(time);

	auto tickrate = std::to_string((int)(1.0f / m_globals()->m_intervalpertick));

	name_cheat += crypt_str("squaredhack.xyz ");

	times += g_ctx.globals.time;
	 
	auto s = ImVec2{}, p = ImVec2{}, gs = ImVec2{ 250, 40 };
	ImGui::SetNextWindowSize(ImVec2(gs));
	ImGui::Begin("Watermark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNav);
	{
		s = ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowSize().y - ImGui::GetStyle().WindowPadding.y * 2);
		p = ImVec2(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y);
		auto draw = ImGui::GetWindowDrawList();

		draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 230, p.y + 27), ImColor(12, 12, 12), 5);
		draw->AddRect(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + 231, p.y + 28), ImColor(24, 24, 24), 5);
		draw->AddRect(ImVec2(p.x - 2, p.y - 2), ImVec2(p.x + 232, p.y + 29), ImColor(24, 24, 24), 5);
		
		ImGui::PushFont(c_menu::get().g_pinfoFont);
		draw->AddText(ImVec2(p.x + 7, p.y + 5), ImColor(100, 125, 200), name_cheat.c_str());
		draw->AddText(ImVec2(p.x + 58, p.y + 5), ImColor(100, 100, 100), "/");
		draw->AddText(ImVec2(p.x + 65, p.y + 5), ImColor(100, 125, 200), comp_name().c_str());
		draw->AddText(ImVec2(p.x + 109, p.y + 5), ImColor(100, 100, 100), "/");
		draw->AddText(ImVec2(p.x + 116, p.y + 5), ImColor(100, 100, 100), "ms:");
		draw->AddText(ImVec2(p.x + 140, p.y + 5), ImColor(100, 125, 200), tickrate.c_str());
		draw->AddText(ImVec2(p.x + 165, p.y + 5), ImColor(100, 100, 100), "/");
		draw->AddText(ImVec2(p.x + 173, p.y + 5), ImColor(100, 125, 200), times.c_str());
		ImGui::PopFont();
	}
	ImGui::End();
}

void misc::keybinds()
{
	if (!m_engine()->IsInGame() || !m_engine()->IsConnected())
		return;

	if (!g_cfg.menu.keybinds)
		return;

	const auto freq = 1.f;
	const auto real_time = m_globals()->m_realtime * freq;
	const int r = std::floor(std::sin(real_time + 0.f) * 127.f + 128.f);
	const int g = std::floor(std::sin(real_time + 2.f) * 127.f + 128.f);
	const int b = std::floor(std::sin(real_time + 4.f) * 127.f + 128.f);

	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";

	if (key_binds::get().get_key_bind_state(0)) {
		spect += crypt_str("Auto fire");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.legitbot.autofire_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(1)) {
		spect += crypt_str("Legit bot");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.legitbot.key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (g_cfg.ragebot.double_tap && g_cfg.ragebot.double_tap_key.key > KEY_NONE && g_cfg.ragebot.double_tap_key.key < KEY_MAX && exploit::get().double_tap_key) {
		spect += crypt_str("Doubletap");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.double_tap_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(3)) {
		spect += crypt_str("Safe point");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.safe_point_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (g_ctx.globals.current_weapon != -1 && key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon))
	{
		spect += crypt_str("Damage override");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.safe_point_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && exploit::get().hide_shots_key) {
		spect += crypt_str("Hide shots");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.antiaim.hide_shots_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(16)) {
		spect += crypt_str("Flip desync");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.antiaim.flip_desync.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(17)) {
		spect += crypt_str("Thirdperson");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.thirdperson_toggle.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(18)) {
		spect += crypt_str("Auto peek");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.automatic_peek.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(19)) {
		spect += crypt_str("Edge jump");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.edge_jump.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(20)) {
		spect += crypt_str("Fake duck");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.fakeduck_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(21)) {
		spect += crypt_str("Slow walk");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.slowwalk_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(22)) {
		spect += crypt_str("Body aim");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.body_aim_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	ImVec2 p;
	ImVec2 size_menu;

	static bool window_set = false;
	float speed = 10;
	static bool finish = false;
	static bool other_bind_pressed = false;
	static int sub_tabs = false;

	if (key_binds::get().get_key_bind_state(4 + hooks::rage_weapon) || key_binds::get().get_key_bind_state(16) || key_binds::get().get_key_bind_state(18) || key_binds::get().get_key_bind_state(20)
		|| key_binds::get().get_key_bind_state(21) || key_binds::get().get_key_bind_state(17) || key_binds::get().get_key_bind_state(22) || key_binds::get().get_key_bind_state(13) || key_binds::get().get_key_bind_state(14) || key_binds::get().get_key_bind_state(15)
		|| exploit::get().double_tap_key || exploit::get().hide_shots_key || key_binds::get().get_key_bind_state(0) || key_binds::get().get_key_bind_state(3) || key_binds::get().get_key_bind_state(23))
		other_bind_pressed = true;
	else
		other_bind_pressed = false;

	if (g_cfg.menu.windowsize_x_saved != g_cfg.menu.oldwindowsize_x_saved)
	{
		if (g_cfg.menu.windowsize_x_saved > g_cfg.menu.oldwindowsize_x_saved)
			g_cfg.menu.windowsize_x_saved -= g_cfg.menu.speed;

		if (g_cfg.menu.windowsize_x_saved < g_cfg.menu.oldwindowsize_x_saved)
			g_cfg.menu.windowsize_x_saved += g_cfg.menu.speed;
	}

	if (g_cfg.menu.windowsize_y_saved != g_cfg.menu.oldwindowsize_y_saved)
	{
		if (g_cfg.menu.windowsize_y_saved > g_cfg.menu.oldwindowsize_y_saved)
			g_cfg.menu.windowsize_y_saved -= g_cfg.menu.speed;

		if (g_cfg.menu.windowsize_y_saved < g_cfg.menu.oldwindowsize_y_saved)
			g_cfg.menu.windowsize_y_saved += g_cfg.menu.speed;
	}

	if (g_cfg.menu.windowsize_x_saved == g_cfg.menu.oldwindowsize_x_saved && g_cfg.menu.windowsize_y_saved == g_cfg.menu.oldwindowsize_y_saved)
		finish = true;
	else
		finish = false;

	if (!g_cfg.antiaim.flip_desync.key || !g_cfg.misc.automatic_peek.key || !g_cfg.misc.fakeduck_key.key || !g_cfg.misc.slowwalk_key.key || !g_cfg.ragebot.double_tap_key.key || !g_cfg.misc.fakeduck_key.key || !g_cfg.misc.thirdperson_toggle.key || !hooks::menu_open)
	{
		g_cfg.menu.windowsize_x_saved = 0;
		g_cfg.menu.windowsize_y_saved = 0;
	}

	static float alpha_menu = 0.1f;

	if (other_bind_pressed)
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;
	else
		if (alpha_menu > 0.1f)
			alpha_menu -= 0.05f;

	bool theme = true;

	if ((g_cfg.menu.keybinds))
	{
		if ((other_bind_pressed && alpha_menu > 0.1f) || hooks::menu_open) 
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_menu);
			if (ImGui::Begin("Binds", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
			{
				auto b_alpha = alpha_menu;
				size_menu = ImGui::GetWindowSize();
				p = ImGui::GetWindowPos();
				auto draw = ImGui::GetWindowDrawList();
				std::vector<std::string> keybind = { "Keybinds" };

				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 25 }, ImColor(15, 15, 15));

				draw->AddRectFilledMultiColor({ p.x + 0, p.y + 2 }, { p.x + 220, p.y + 5 }, ImColor(100, 125, 200), ImColor(100, 125, 200), ImColor(100, 125, 200, 25), ImColor(100, 125, 200, 25));
				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 4 }, ImColor(100, 125, 200), 6);

				ImGui::PushFont(c_menu::get().g_pinfoFont);
				draw->AddText(ImVec2(p.x + 10, p.y + 7), ImColor(200, 200, 200), keybind.at(0).c_str());

				if (specs > 0) spect += "\n";
				if (modes > 0) mode += "\n";
				ImVec2 size = ImGui::CalcTextSize(spect.c_str());
				ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
				ImGui::SetWindowSize(ImVec2(220, 25 + size.y));
				ImGui::SetCursorPosY(25);
				ImGui::Columns(2, "fart1", false);

				ImGui::SetColumnWidth(0, 220 - (size2.x + 8));
				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), spect.c_str());
				ImGui::NextColumn();

				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), mode.c_str());
				ImGui::Columns(1);
				ImGui::PopFont();
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}
	}
}

void misc::spectators_list()
{
	if (!g_cfg.misc.spectators_list)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	const auto freq = 1.f;

	const auto real_time = m_globals()->m_realtime * freq;
	const int r = std::floor(std::sin(real_time + 0.f) * 127.f + 128.f);
	const int g = std::floor(std::sin(real_time + 2.f) * 127.f + 128.f);
	const int b = std::floor(std::sin(real_time + 4.f) * 127.f + 128.f);

	std::vector <std::string> spectators;

	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";
	ImVec2 p;
	ImVec2 size_menu;

	static bool window_set = false;
	static float windowsize_x = 0;
	static float windowsize_y = 0;
	static float oldwindowsize_x = 300;
	static float oldwindowsize_y = 300;
	static float speed = 10;
	static bool finish = false;
	static int sub_tabs = false;
	static float alpha_menu = 0.1f;


	if (windowsize_x != oldwindowsize_x)
	{
		if (windowsize_x > oldwindowsize_x)
			windowsize_x -= speed;

		if (windowsize_x < oldwindowsize_x)
			windowsize_x += speed;
	}
	if (windowsize_y != oldwindowsize_y)
	{
		if (windowsize_y > oldwindowsize_y)
			windowsize_y -= speed;

		if (windowsize_y < oldwindowsize_y)
			windowsize_y += speed;
	}

	if (windowsize_x == oldwindowsize_x && windowsize_y == oldwindowsize_y)
		finish = true;
	else
		finish = false;

	for (int i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->is_alive())
			continue;

		if (e->IsDormant())
			continue;

		if (e->m_hObserverTarget().Get() != g_ctx.local())
			continue;

		player_info_t player_info;
		m_engine()->GetPlayerInfo(i, &player_info);
		spect += player_info.szName;
		spect += "\n";
		specs++;
	}

	bool theme = true;

	if ((spect.size() > 0) || hooks::menu_open)
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;
	else
		if (alpha_menu > 0.1f)
			alpha_menu -= 0.05f;

	if ((g_cfg.misc.spectators_list)) 
	{
		if ((spect.size() > 0 && alpha_menu > 0.1f) || hooks::menu_open) 
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_menu);
			if (ImGui::Begin("Spectators", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
			{
				auto b_alpha = alpha_menu;
				size_menu = ImGui::GetWindowSize();
				p = ImGui::GetWindowPos();
				auto draw = ImGui::GetWindowDrawList();

				std::vector<std::string> keybind = { "Spectators" };
				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 25 }, ImColor(15, 15, 15));
				draw->AddRectFilledMultiColor({ p.x + 0, p.y + 2 }, { p.x + 220, p.y + 5 }, ImColor(100, 125, 200), ImColor(100, 125, 200), ImColor(100, 125, 200, 25), ImColor(100, 125, 200, 25));
				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 4 }, ImColor(100, 125, 200), 6);

				ImGui::PushFont(c_menu::get().g_pinfoFont);
				draw->AddText(ImVec2(p.x + 10, p.y + 7), ImColor(200, 200, 200), keybind.at(0).c_str());

				if (specs > 0) spect += "\n";
				if (modes > 0) mode += "\n";
				ImVec2 size = ImGui::CalcTextSize(spect.c_str());
				ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
				ImGui::SetWindowSize(ImVec2(220, 25 + size.y));
				ImGui::SetCursorPosY(25);
				ImGui::Columns(2, "fart1", false);

				ImGui::SetColumnWidth(0, 220 - (size2.x + 8));
				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), spect.c_str());
				ImGui::NextColumn();

				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), mode.c_str());
				ImGui::Columns(1);
				ImGui::PopFont();
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}
	}
}

void misc::indicators()
{
	if (!g_cfg.menu.indicators)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	ImVec2 p;

	if (g_cfg.menu.indicators)
	{

	}
}

void misc::break_prediction(CUserCmd* cmd) 
{
	if (g_cfg.misc.break_prediction)
	{
		if (cmd->m_forwardmove > 0.0f)
		{
			g_ctx.send_packet = 4.0f;
			cmd->m_buttons |= IN_BACK;
			cmd->m_buttons &= ~IN_FORWARD;
		}
		else if (cmd->m_forwardmove < 0.0f)
		{
			g_ctx.send_packet = 4.0f;
			cmd->m_buttons |= IN_FORWARD;
			cmd->m_buttons &= ~IN_BACK;
		}

		if (cmd->m_sidemove > 0.0f)
		{
			g_ctx.send_packet = 4.0f;
			cmd->m_buttons |= IN_MOVELEFT;
			cmd->m_buttons &= ~IN_MOVERIGHT;
		}
		else if (cmd->m_sidemove < 0.0f)
		{
			g_ctx.send_packet = 4.0f;
			cmd->m_buttons |= IN_MOVERIGHT;
			cmd->m_buttons &= ~IN_MOVELEFT;
		}
	}
}

#define sig_player_by_index "85 C9 7E 32 A1"
#define sig_draw_server_hitboxes "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE"
void misc::draw_server_hitboxes() {

	if (!g_cfg.misc.server_hitbox)
		return;

	if (!m_engine()->IsConnected() || !m_engine()->IsInGame())
		return;

	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return;

	if (!m_input()->m_fCameraInThirdPerson)
		return;

	auto get_player_by_index = [](int index) -> player_t* {
		typedef player_t* (__fastcall* player_by_index)(int);
		static auto player_index = reinterpret_cast<player_by_index>(util::FindSignature(("server.dll"), sig_player_by_index));

		if (!player_index)
			return false;

		return player_index(index);
	};

	static auto fn = (util::FindSignature(("server.dll"), sig_draw_server_hitboxes));
	auto duration = -1.f;
	PVOID entity = nullptr;

	entity = get_player_by_index(g_ctx.local()->EntIndex());

	if (!entity)
		return;

	__asm {
		pushad
		movss xmm1, duration
		push 0 // 0 - colored, 1 - blue
		mov ecx, entity
		call fn
		popad
	}
}

void misc::ChatSpammer()
{
	if (!g_cfg.misc.chat_spammer)
		return;

	static std::string chatspam[] =
	{
		crypt_str("squaredhack.xyz - kfc?."),
		crypt_str("squaredhack.xyz - pasted?."),
		crypt_str("squaredhack.xyz - free?."),
		crypt_str("squaredhack.xyz - i'm trans?."),
	};

	static auto lastspammed = 0;

	if (GetTickCount() - lastspammed > 800)
	{
		lastspammed = GetTickCount();

		srand(m_globals()->m_tickcount);
		std::string msg = crypt_str("say ") + chatspam[rand() % 4];

		m_engine()->ExecuteClientCmd(msg.c_str());
	}
}

void misc::NoDuck(CUserCmd* cmd)
{
	if (!g_cfg.misc.noduck)
		return;

	if (m_gamerules()->m_bIsValveDS())
		return;

	cmd->m_buttons |= IN_BULLRUSH;
}

void misc::AutoCrouch(CUserCmd* cmd)
{
	if (fakelag::get().condition)
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (m_gamerules()->m_bIsValveDS())
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!key_binds::get().get_key_bind_state(20))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands != 7)
		return;

	if (m_clientstate()->iChokedCommands >= 7)
		cmd->m_buttons |= IN_DUCK;
	else
		cmd->m_buttons &= ~IN_DUCK;

	g_ctx.globals.fakeducking = true;
}

void misc::SlideWalk(CUserCmd* cmd)
{
	if (!g_ctx.local()->is_alive())
		return;

	if (g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (antiaim::get().condition(cmd, true) && (g_cfg.misc.slidewalk == 2 && math::random_int(0, 2) == 0 || g_cfg.misc.slidewalk == 1))
	{
		if (cmd->m_forwardmove > 0.0f)
		{
			cmd->m_buttons |= IN_BACK;
			cmd->m_buttons &= ~IN_FORWARD;
		}
		else if (cmd->m_forwardmove < 0.0f)
		{
			cmd->m_buttons |= IN_FORWARD;
			cmd->m_buttons &= ~IN_BACK;
		}

		if (cmd->m_sidemove > 0.0f)
		{
			cmd->m_buttons |= IN_MOVELEFT;
			cmd->m_buttons &= ~IN_MOVERIGHT;
		}
		else if (cmd->m_sidemove < 0.0f)
		{
			cmd->m_buttons |= IN_MOVERIGHT;
			cmd->m_buttons &= ~IN_MOVELEFT;
		}
	}
	else
	{
		auto buttons = cmd->m_buttons & ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD);

		if (g_cfg.misc.slidewalk == 2 && math::random_int(0, 1) || g_cfg.misc.slidewalk == 1)
		{
			if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
				return;

			if (cmd->m_forwardmove <= 0.0f)
				buttons |= IN_BACK;
			else
				buttons |= IN_FORWARD;

			if (cmd->m_sidemove > 0.0f)
				goto LABEL_15;
			else if (cmd->m_sidemove >= 0.0f)
				goto LABEL_18;

			goto LABEL_17;
		}
		else
			goto LABEL_18;

		if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
			return;

		if (cmd->m_forwardmove <= 0.0f)
			buttons |= IN_FORWARD;
		else
			buttons |= IN_BACK;

		if (cmd->m_sidemove > 0.0f)
		{
		LABEL_17:
			buttons |= IN_MOVELEFT;
			goto LABEL_18;
		}

		if (cmd->m_sidemove < 0.0f)
			LABEL_15:

		buttons |= IN_MOVERIGHT;

	LABEL_18:
		cmd->m_buttons = buttons;
	}
}

void misc::automatic_peek(CUserCmd* cmd, float wish_yaw)
{
	if (!g_ctx.globals.weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
	{
		if (g_ctx.globals.start_position.IsZero())
		{
			g_ctx.globals.start_position = g_ctx.local()->GetAbsOrigin();

			if (!(engineprediction::get().backup_data.flags & FL_ONGROUND))
			{
				Ray_t ray;
				CTraceFilterWorldAndPropsOnly filter;
				CGameTrace trace;

				ray.Init(g_ctx.globals.start_position, g_ctx.globals.start_position - Vector(0.0f, 0.0f, 1000.0f));
				m_trace()->TraceRay(ray, MASK_SOLID, &filter, &trace);

				if (trace.fraction < 1.0f)
					g_ctx.globals.start_position = trace.endpos + Vector(0.0f, 0.0f, 2.0f);
			}
		}
		else
		{
			auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (cmd->m_buttons & IN_ATTACK || cmd->m_buttons & IN_ATTACK2);

			if (cmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
				g_ctx.globals.fired_shot = true;

			if (g_ctx.globals.fired_shot)
			{
				auto current_position = g_ctx.local()->GetAbsOrigin();
				auto difference = current_position - g_ctx.globals.start_position;

				if (difference.Length2D() > 5.0f)
				{
					auto velocity = Vector(difference.x * cos(wish_yaw / 180.0f * M_PI) + difference.y * sin(wish_yaw / 180.0f * M_PI), difference.y * cos(wish_yaw / 180.0f * M_PI) - difference.x * sin(wish_yaw / 180.0f * M_PI), difference.z);

					cmd->m_forwardmove = -velocity.x * 20.0f;
					cmd->m_sidemove = velocity.y * 20.0f;
				}
				else
				{
					g_ctx.globals.fired_shot = false;
					g_ctx.globals.start_position.Zero();
				}
			}
		}
	}
	else
	{
		g_ctx.globals.fired_shot = false;
		g_ctx.globals.start_position.Zero();
	}
}

void misc::ViewModel()
{
	if (g_cfg.esp.viewmodel_fov)
	{
		auto viewFOV = (float)g_cfg.esp.viewmodel_fov + 68.0f;
		static auto viewFOVcvar = m_cvar()->FindVar(crypt_str("viewmodel_fov"));

		if (viewFOVcvar->GetFloat() != viewFOV)
		{
			*(float*)((DWORD)&viewFOVcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewFOVcvar->SetValue(viewFOV);
		}
	}

	if (g_cfg.esp.viewmodel_x)
	{
		auto viewX = (float)g_cfg.esp.viewmodel_x / 2.0f;
		static auto viewXcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_x"));

		if (viewXcvar->GetFloat() != viewX)
		{
			*(float*)((DWORD)&viewXcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewXcvar->SetValue(viewX);
		}
	}

	if (g_cfg.esp.viewmodel_y)
	{
		auto viewY = (float)g_cfg.esp.viewmodel_y / 2.0f;
		static auto viewYcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_y"));

		if (viewYcvar->GetFloat() != viewY)
		{
			*(float*)((DWORD)&viewYcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewYcvar->SetValue(viewY);
		}
	}

	if (g_cfg.esp.viewmodel_z)
	{
		auto viewZ = (float)g_cfg.esp.viewmodel_z / 2.0f;
		static auto viewZcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_z"));

		if (viewZcvar->GetFloat() != viewZ)
		{
			*(float*)((DWORD)&viewZcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewZcvar->SetValue(viewZ);
		}
	}
}

void misc::FullBright()
{
	if (!g_cfg.player.enable)
		return;

	static auto mat_fullbright = m_cvar()->FindVar(crypt_str("mat_fullbright"));

	if (mat_fullbright->GetBool() != g_cfg.esp.bright)
		mat_fullbright->SetValue(g_cfg.esp.bright);
}

void misc::PovArrows(player_t* e, Color color)
{
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

	if (isOnScreen(e->GetAbsOrigin(), screenPos))
		return;

	Vector viewAngles;
	m_engine()->GetViewAngles(viewAngles);

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto screenCenter = Vector2D(width * 0.5f, height * 0.5f);
	auto angleYawRad = DEG2RAD(viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);

	auto radius = g_cfg.player.distance;
	auto size = g_cfg.player.size;

	auto newPointX = screenCenter.x + ((((width - (size * 3)) * 0.5f) * (radius / 100.0f)) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
	auto newPointY = screenCenter.y + ((((height - (size * 3)) * 0.5f) * (radius / 100.0f)) * sin(angleYawRad));

	std::array <Vector2D, 3> points
	{
		Vector2D(newPointX - size, newPointY - size),
		Vector2D(newPointX + size, newPointY),
		Vector2D(newPointX - size, newPointY + size)
	};

	math::rotate_triangle(points, viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);
	g_Render->TriangleFilled(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(color.r(), color.g(), color.b(), math::clamp(color.a(), 0, 125)));
	g_Render->Triangle(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(color.r(), color.g(), color.b(), color.a()));
}

void misc::NightmodeFix()
{
	static auto in_game = false;

	if (m_engine()->IsInGame() && !in_game)
	{
		in_game = true;

		g_ctx.globals.change_materials = true;
		worldesp::get().changed = true;

		static auto skybox = m_cvar()->FindVar(crypt_str("sv_skyname"));
		worldesp::get().backup_skybox = skybox->GetString();
		return;
	}
	else if (!m_engine()->IsInGame() && in_game)
		in_game = false;

	static auto player_enable = g_cfg.player.enable;

	if (player_enable != g_cfg.player.enable)
	{
		player_enable = g_cfg.player.enable;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting = g_cfg.esp.nightmode;

	if (setting != g_cfg.esp.nightmode)
	{
		setting = g_cfg.esp.nightmode;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting_world = g_cfg.esp.world_color;

	if (setting_world != g_cfg.esp.world_color)
	{
		setting_world = g_cfg.esp.world_color;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting_props = g_cfg.esp.props_color;

	if (setting_props != g_cfg.esp.props_color)
	{
		setting_props = g_cfg.esp.props_color;
		g_ctx.globals.change_materials = true;
	}
}

void misc::desync_arrows()
{
	if (!g_ctx.local()->is_alive())
		return;

	if (!g_cfg.ragebot.enable)
		return;

	if (!g_cfg.antiaim.enable)
		return;

	if ((g_cfg.antiaim.manual_back.key <= KEY_NONE || g_cfg.antiaim.manual_back.key >= KEY_MAX) && (g_cfg.antiaim.manual_left.key <= KEY_NONE || g_cfg.antiaim.manual_left.key >= KEY_MAX) && (g_cfg.antiaim.manual_right.key <= KEY_NONE || g_cfg.antiaim.manual_right.key >= KEY_MAX))
		antiaim::get().manual_side = SIDE_NONE;

	if (!g_cfg.antiaim.flip_indicator)
		return;

	if (!g_ctx.globals.scoped == false)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto color = g_cfg.antiaim.flip_indicator_color;

	if (antiaim::get().manual_side == SIDE_LEFT)
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), color);
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), Color(55, 55, 55, 189));
	}
	else if (antiaim::get().manual_side == SIDE_RIGHT)
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), Color(55, 55, 55, 189));
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), color);
	}
	else
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), Color(55, 55, 55, 189));
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), Color(55, 55, 55, 189));
	}

	if (g_cfg.antiaim.desync == 2)
	{
		if (antiaim::get().flip == false)
		{
			render::get().rect(width / 2 - 10, height / 2 + 50, 20, 2, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 - 10, height / 2 + 50, 20, 2, Color(color));
		}
	}
	else
	{
		if (antiaim::get().flip == false)
		{
			render::get().rect(width / 2 - 50, height / 2 - 10, 2, 20, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 - 50, height / 2 - 10, 2, 20, Color(color));
		}
		else if (antiaim::get().flip == true)
		{
			render::get().rect(width / 2 + 48, height / 2 - 10, 2, 20, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 + 48, height / 2 - 10, 2, 20, Color(color));
		}
	}
}

void misc::aimbot_hitboxes()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.player.lag_hitbox)
		return;

	auto player = (player_t*)m_entitylist()->GetClientEntity(aim::get().last_target_index);

	if (!player)
		return;

	auto model = player->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto hitbox_set = studio_model->pHitboxSet(player->m_nHitboxSet());

	if (!hitbox_set)
		return;

	hit_chams::get().add_matrix(player, aim::get().last_target[aim::get().last_target_index].record.matrixes_data.main);
}

void misc::ragdolls()
{
	if (!g_cfg.misc.ragdolls)
		return;

	for (auto i = 1; i <= m_entitylist()->GetHighestEntityIndex(); ++i)
	{
		auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->IsDormant())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class)
			continue;

		if (client_class->m_ClassID != CCSRagdoll)
			continue;

		auto ragdoll = (ragdoll_t*)e;
		ragdoll->m_vecForce().z = 800000.0f;
	}
}

void misc::rank_reveal()
{
	if (!g_cfg.misc.rank_reveal)
		return;

	using RankReveal_t = bool(__cdecl*)(int*);
	static auto Fn = (RankReveal_t)(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 A1 ? ? ? ? 85 C0 75 37")));

	int array[3] =
	{
		0,
		0,
		0
	};

	Fn(array);
}

void misc::fast_stop(CUserCmd* m_pcmd)
{
	if (!g_cfg.misc.fast_stop)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (g_ctx.local()->get_move_type() == MOVETYPE_NOCLIP ||
		g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return;

	auto pressed_move_key = m_pcmd->m_buttons & IN_FORWARD || m_pcmd->m_buttons & IN_MOVELEFT || m_pcmd->m_buttons & IN_BACK || m_pcmd->m_buttons & IN_MOVERIGHT || m_pcmd->m_buttons & IN_JUMP;

	if (pressed_move_key)
		return;

	if (!((antiaim::get().type == ANTIAIM_LEGIT ? g_cfg.antiaim.desync : g_cfg.antiaim.type[antiaim::get().type].desync) && (antiaim::get().type == ANTIAIM_LEGIT ? !g_cfg.antiaim.legit_lby_type : !g_cfg.antiaim.lby_type) && (!g_ctx.globals.weapon->is_grenade() || g_cfg.esp.on_click & !(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2))) || antiaim::get().condition(m_pcmd))
	{
		if (g_ctx.local()->m_vecVelocity().Length2D() <= g_ctx.local()->GetMaxPlayerSpeed() * 0.34f)
		{
			m_pcmd->m_forwardmove = m_pcmd->m_sidemove = 0.0f;
			return;
		}

		QAngle angResistance = QAngle(0, 0, 0);
		math::AngleVectorsNothing2((g_ctx.local()->m_vecVelocity() * -1.f), angResistance);

		angResistance.yaw = m_pcmd->m_viewangles.y - angResistance.yaw;
		angResistance.pitch = m_pcmd->m_viewangles.x - angResistance.pitch;

		Vector vecResistance = Vector(0, 0, 0);
		math::AngleVectorsNothing(angResistance, vecResistance);

		m_pcmd->m_forwardmove = std::clamp(vecResistance.x, -450.f, 450.0f);
		m_pcmd->m_sidemove = std::clamp(vecResistance.y, -450.f, 450.0f);
	}
}

void FX_Tesla(CTeslaInfo& pInfo)
{
	using FX_TeslaFn = void(__thiscall*)(CTeslaInfo&);
	static FX_TeslaFn pEffects = (FX_TeslaFn)util::FindSignature("client.dll", "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 8B 47 18");
	if (!pEffects)
		return;

	pEffects(pInfo);
}

void misc::KillEffect(IGameEvent* pEvent)
{
	player_t* pEntity = (player_t*)m_entitylist()->GetClientEntity(m_engine()->GetPlayerForUserID(pEvent->GetInt(("userid"))));
	player_t* pAttacker = (player_t*)m_entitylist()->GetClientEntity(m_engine()->GetPlayerForUserID(pEvent->GetInt(("attacker"))));

	if (!g_cfg.misc.lightingonshot)
		return;

	if (pEntity == g_ctx.local())
		return;

	if (pEntity->EntIndex() <= 0 || pEntity->EntIndex() > 64)
		return;

	if (!pAttacker || !pEntity)
		return;

	if (pAttacker != g_ctx.local())
		return;

	CTeslaInfo teslaInfo;
	teslaInfo.m_flBeamWidth = 10.f;
	teslaInfo.m_flRadius = 500.f;
	teslaInfo.m_nEntIndex = pEntity->EntIndex();
	teslaInfo.m_vColor.Init(1.f, 1.f, 1.f);
	teslaInfo.m_vPos = pEntity->hitbox_position(8);
	teslaInfo.m_flTimeVisible = 0.75f;
	teslaInfo.m_nBeams = 12;
	teslaInfo.m_pszSpriteName = "sprites/physbeam.vmt";

	FX_Tesla(teslaInfo);
}

void misc::MovementTrails()
{
	if (!g_cfg.misc.movement_trails)
		return;

	if (g_ctx.local()->is_alive() && m_engine()->IsInGame() && m_engine()->IsConnected())
	{
		auto local_pos = g_ctx.local()->GetAbsOrigin();

		static float rainbow;
		rainbow += 0.001f;
		if (rainbow > 1.f)
			rainbow = 0.f;

		auto rainbow_col = Color::FromHSB(rainbow, 1, 1);

		switch (g_cfg.misc.trail_types)
		{
		case 1:
			iEffects()->EnergySplash(local_pos, Vector(0, 0, 0), true);
			break;
		case 2:
			float current_time = g_ctx.local()->m_nTickBase() * m_globals()->m_intervalpertick;
			trail_pos.push_back({ g_ctx.local()->GetAbsOrigin(), current_time + 1.f });
			Vector world_to_screen;
			Vector last_w2s;
			Vector last_pos = trail_pos[0].position;

			for (int c = 0; c < trail_pos.size(); c++) 
			{
				if (trail_pos[c].time < current_time)
				{
					trail_pos.erase(trail_pos.begin() + c);
					continue;
				}

				auto tracer = trail_pos.at(c);

				if (trail_pos.at(c).position.IsZero())
					continue;

				if (!trail_pos.at(c).position.IsValid())
					continue;

				if (math::world_to_screen(tracer.position, world_to_screen) && math::world_to_screen(last_pos, last_w2s))
					g_Render->DrawLine(world_to_screen.x, world_to_screen.y, last_w2s.x, last_w2s.y, g_cfg.misc.rainbow_trails ? rainbow_col : g_cfg.misc.trail_color);

				last_pos = tracer.position;
			}
			break;
		}
	}
}