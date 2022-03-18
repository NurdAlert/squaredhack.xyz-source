#include "Render.h"
#define PI			3.14159265358979323846
#include "../../features/visuals/player_esp.h"
#include "../../features/visuals/GrenadePrediction.h"
#include "../../features/visuals/other_esp.h"
#include "../../features/visuals/world_esp.h"
#include "../../features/legitbot/legitbot.h"
#include "../../features/ragebot/aim.h"
#include "../../menu/menu.h"
#include "../../features/misc/misc.h"

ImColor u32(Color col)
{
	return ImColor(col.r(), col.g(), col.b(), col.a());
}

void ImGuiRendering::BeginScene(IDirect3DDevice9* pDevice)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
	ImGui::Begin("##Backbuffer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

	ImGuiWindow* window = ImGui::GetCurrentWindow();

	static int w, h;
	m_engine()->GetScreenSize(w, h);

	static auto alpha = 0;
	auto speed = 800.0f * m_globals()->m_frametime;

	if (m_engine()->IsInGame() && m_engine()->IsConnected())
	{
		static float anim1 = 20;

		if (!g_ctx.local()->m_bIsScoped())
			anim1 = 20;

		if (g_cfg.esp.removals[REMOVALS_ZOOM] && g_ctx.globals.scoped && g_ctx.globals.weapon->is_sniper())
		{
			static int w, h;
			m_engine()->GetScreenSize(w, h);
			g_Render->FilledRect(w / 2 - 2, h / 2, 2, 2, Color(200, 50, 80));

			anim1 += m_globals()->m_frametime * 500;

			if (anim1 >= 130)
				anim1 = 130;

			window->DrawList->AddRectFilledMultiColor(ImVec2(w / 2 + 20, h / 2), ImVec2(w / 2 + anim1, h / 2 + 2), ImColor(255, 255, 255, 170), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 170));
			window->DrawList->AddRectFilledMultiColor(ImVec2(w / 2 - 20, h / 2), ImVec2(w / 2 - anim1, h / 2 + 2), ImColor(255, 255, 255, 170), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 170));
			window->DrawList->AddRectFilledMultiColor(ImVec2(w / 2, h / 2 - 20), ImVec2(w / 2 - 2, h / 2 - anim1), ImColor(255, 255, 255, 170), ImColor(255, 255, 255, 170), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 0));
			window->DrawList->AddRectFilledMultiColor(ImVec2(w / 2, h / 2 + 20), ImVec2(w / 2 - 2, h / 2 + anim1), ImColor(255, 255, 255, 170), ImColor(255, 255, 255, 170), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 0));
		}

		if (g_cfg.misc.trail_types)
			misc::get().MovementTrails();

		otheresp::get().automatic_peek_indicator();

		if (GrenadePrediction::get().path.size() > 2 && g_ctx.globals.weapon->is_grenade())
			GrenadePrediction::get().Paint();

		if (g_cfg.player.enable)
		{
			worldesp::get().paint_traverse();
			playeresp::get().paint_traverse();
		}

		if (g_cfg.player.enable && g_cfg.esp.damage_marker)
		{
			otheresp::get().hitmarker_paint();
			otheresp::get().damage_marker_paint();
		}

		if (hooks::menu_open)
		{
			if (alpha + speed <= 160)
				alpha += speed;

			window->DrawList->AddRectFilled(ImVec2(0, 0), ImVec2(w, h), Color(0, 0, 0, alpha).u32(), 0.f);

		}
		else
		{
			if (alpha - speed >= 0)
				alpha -= speed;

			window->DrawList->AddRectFilled(ImVec2(0, 0), ImVec2(w, h), Color(0, 0, 0, alpha).u32(), 0.f);
		}
	}
}

void ImGuiRendering::EndScene()
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->PushClipRectFullScreen();

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void ImGuiRendering::arc(float x, float y, float radius, float min_angle, float max_angle, Color col, float thickness) 
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->PathArcTo(ImVec2(x, y), radius, DEG2RAD(min_angle), DEG2RAD(max_angle), 32);
	window->DrawList->PathStroke(col.u32(), false, thickness);
}

void ImGuiRendering::two_sided_arc(float x, float y, float radius, float scale, Color col, float thickness)
{
	arc(x, y, radius, -270, -90 - 90 * (scale * 2), col, thickness);
	arc(x, y, radius, -90 + 90 * (scale * 2), 90, col, thickness);
};

void ImGuiRendering::PolyLine(const ImVec2* points, const int points_count, Color clr, bool closed, float thickness, Color fill)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddConvexPolyFilled(points, points_count, u32(fill));
	window->DrawList->AddPolyline(points, points_count, u32(clr), closed, thickness);
}

void ImGuiRendering::build_lookup_table()
{
	if (!lookup_table.empty())
		return;

	for (auto i = 0; i <= points; i++)
		lookup_table.emplace_back(
			std::cos(2.f * D3DX_PI * (i / static_cast<float>(points))),
			std::sin(2.f * D3DX_PI * (i / static_cast<float>(points)))
		);
}

void ImGuiRendering::circle_filled_radial_gradient(IDirect3DDevice9* dev, Vector2D center, float radius, Color color1, Color color2)
{
	const auto col1 = color1.u32();
	const auto col2 = color2.u32();
	build_lookup_table();

	vertex vert[points + 2] = {};

	for (auto i = 1; i <= points; i++)
		vert[i] =
	{
		center.x + radius * lookup_table[i].x,
		center.y - radius * lookup_table[i].y,
		0.0f,
		1.0f,
		col1
	};

	vert[0] = { center.x, center.y, 0.0f, 1.0f, col2 };
	vert[points + 1] = vert[1];

	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, true);
	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, points, &vert, sizeof vertex);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, false);
}


void ImGuiRendering::DrawRing3D1(int16_t x, int16_t y, int16_t z, int16_t radius, uint16_t points, Color color1, Color color2, float thickness, float prog, bool fill_prog)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	static float Step = (M_PI * 2.0f) / (points);
	std::vector<ImVec2> point;
	for (float lat = 0.f; lat <= M_PI * 2.0f; lat += Step)
	{
		const auto& point3d = Vector(sin(lat), cos(lat), 0.f) * radius;
		Vector point2d;
		if (math::world_to_screen(Vector(x, y, z) + point3d, point2d))
			point.push_back(ImVec2(point2d.x, point2d.y));
	}
	window->DrawList->AddConvexPolyFilled(point.data(), point.size(), u32(color2));
	window->DrawList->AddPolyline(point.data(), max(0, point.size() * prog), u32(color1), prog == 1.f, thickness);
}

void ImGuiRendering::DrawRing3D(int16_t x, int16_t y, int16_t z, int16_t radius, uint16_t points, Color color1, Color color2, float thickness)
{
	Vector pos = Vector(x, y, z);

	float step = (float)M_PI * 2.0f / points;

	for (float a = 0; a < (M_PI * 2.0f); a += step)
	{
		Vector start(radius * cosf(a) + pos.x, radius * sinf(a) + pos.y, pos.z);
		Vector end(radius * cosf(a + step) + pos.x, radius * sinf(a + step) + pos.y, pos.z);

		Vector start2d;
		Vector end2d;

		Vector Position;


		if (math::world_to_screen(start, start2d) &&
			math::world_to_screen(end, end2d) && math::world_to_screen(pos, Position))
		{

			DrawLine(start2d[0], start2d[1], end2d[0], end2d[1], color1);
			TriangleFilled(Position.x, Position.y, end2d.x, end2d.y, start2d.x, start2d.y, color2);
		}
	}
}

void ImGuiRendering::DrawCircle(float x, float y, float r, float s, Color color)
{
	float Step = PI * 2.0 / s;
	for (float a = 0; a < (PI * 2.0); a += Step)
	{
		float x1 = r * cos(a) + x;
		float y1 = r * sin(a) + y;
		float x2 = r * cos(a + Step) + x;
		float y2 = r * sin(a + Step) + y;

		DrawLine(x1, y1, x2, y2, color);
	}
}

struct CUSTOMVERTEX 
{
	FLOAT x, y, z;
	FLOAT rhw;
	DWORD color;
};

void ImGuiRendering::DrawGradientCircle(float x, float y, float rad, float rotate, int type, int resolution, DWORD color, DWORD color2)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2;

	std::vector<CUSTOMVERTEX> circle(resolution + 2);

	float angle = rotate * D3DX_PI / 180, pi = D3DX_PI;

	if (type == 1)
		pi = D3DX_PI;
	if (type == 2)
		pi = D3DX_PI / 2; 
	if (type == 3)
		pi = D3DX_PI / 4; 

	pi = D3DX_PI / type; 

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0;
	circle[0].rhw = 1;
	circle[0].color = color2;

	for (int i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - rad * cos(pi * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - rad * sin(pi * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0;
		circle[i].rhw = 1;
		circle[i].color = color;
	}

	int _res = resolution + 2;
	for (int i = 0; i < _res; i++)
	{
		circle[i].x = x + cos(angle) * (circle[i].x - x) - sin(angle) * (circle[i].y - y);
		circle[i].y = y + sin(angle) * (circle[i].x - x) + cos(angle) * (circle[i].y - y);
	}

	c_menu::get().device->CreateVertexBuffer((resolution + 2) * sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB2, NULL);

	VOID* pVertices;
	g_pVB2->Lock(0, (resolution + 2) * sizeof(CUSTOMVERTEX), (void**)&pVertices, 0);
	memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(CUSTOMVERTEX));
	g_pVB2->Unlock();

	c_menu::get().device->SetTexture(0, NULL);
	c_menu::get().device->SetPixelShader(NULL);
	c_menu::get().device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	c_menu::get().device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	c_menu::get().device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	c_menu::get().device->SetStreamSource(0, g_pVB2, 0, sizeof(CUSTOMVERTEX));
	c_menu::get().device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	c_menu::get().device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);
	if (g_pVB2 != NULL)
		g_pVB2->Release();
}

void ImGuiRendering::DrawString(float x, float y, Color color, int flags, ImFont* font, const char* message, ...)
{
	char output[4096] = {};
	va_list args;
	va_start(args, message);
	vsprintf(output, message, args);
	va_end(args);

	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->PushTextureID(font->ContainerAtlas->TexID);
	ImGui::PushFont(font);
	auto coord = ImVec2(x, y);
	auto size = ImGui::CalcTextSize(output);
	auto coord_out = ImVec2{ coord.x + 1.f, coord.y + 1.f };
	Color outline_clr = Color(0, 0, 0, color.a());

	int width = 0, height = 0;

	if (!(flags & render2::centered_x))
		size.x = 0;
	if (!(flags & render2::centered_y))
		size.y = 0;

	ImVec2 pos = ImVec2(coord.x - (size.x * .5), coord.y - (size.y * .5));

	if (flags & render2::outline)
	{
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y + 1), outline_clr.u32(), output);

		window->DrawList->AddText(ImVec2(pos.x, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y), outline_clr.u32(), output);
	}

	window->DrawList->AddText(pos, color.u32(), output);
	ImGui::PopFont();
}

void ImGuiRendering::DrawString2(float x, float y, Color color, int flags, ImFont* font, const std::string message, ...)
{
	char output[1024] = {};
	va_list args;
	va_start(args, message);
	vsprintf_s(output, message.c_str(), args);
	va_end(args);

	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->PushTextureID(font->ContainerAtlas->TexID);
	ImGui::PushFont(font);
	auto coord = ImVec2(x, y);
	auto size = ImGui::CalcTextSize(output);
	auto coord_out = ImVec2{ coord.x + 1.f, coord.y + 1.f };
	Color outline_clr = Color(0, 0, 0, color.a());

	int width = 0, height = 0;

	if (!(flags & render2::centered_x))
		size.x = 0;
	if (!(flags & render2::centered_y))
		size.y = 0;

	ImVec2 pos = ImVec2(coord.x - (size.x * .5), coord.y - (size.y * .5));

	if (flags & render2::outline)
	{
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y + 1), outline_clr.u32(), output);

		window->DrawList->AddText(ImVec2(pos.x, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y), outline_clr.u32(), output);
	}

	window->DrawList->AddText(pos, color.u32(), output);
	ImGui::PopFont();
}

void ImGuiRendering::DrawEspBox(Vector leftUpCorn, Vector rightDownCorn, Color clr, float width)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	ImVec2 min = ImVec2(leftUpCorn.x, leftUpCorn.y);
	ImVec2 max = ImVec2(rightDownCorn.x, rightDownCorn.y);
	window->DrawList->AddRect(min, max, clr.u32(), 0.0F, -1, width);
}

void ImGuiRendering::DrawLine(float x1, float y1, float x2, float y2, Color clr, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), clr.u32(), thickness);
}

void ImGuiRendering::filled_rect_world(Vector center, Vector2D size, Color color, int angle, float damage) {
	Vector add_top_left, add_top_right, add_bot_left, add_bot_right;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	switch (angle) 
	{
	case 0: 
		add_top_left = Vector(-size.x, -size.y, 0);
		add_top_right = Vector(size.x, -size.y, 0);

		add_bot_left = Vector(-size.x, size.y, 0);
		add_bot_right = Vector(size.x, size.y, 0);

		break;
	case 1: 
		add_top_left = Vector(-size.x, 0, -size.y);
		add_top_right = Vector(size.x, 0, -size.y);

		add_bot_left = Vector(-size.x, 0, size.y);
		add_bot_right = Vector(size.x, 0, size.y);

		break;
	case 2: 
		add_top_left = Vector(0, -size.y, -size.x);
		add_top_right = Vector(0, -size.y, size.x);

		add_bot_left = Vector(0, size.y, -size.x);
		add_bot_right = Vector(0, size.y, size.x);

		break;
	}

	Vector c_top_left = center + add_top_left;
	Vector c_top_right = center + add_top_right;

	Vector c_bot_left = center + add_bot_left;
	Vector c_bot_right = center + add_bot_right;

	Vector top_left, top_right, bot_left, bot_right;
	if (math::world_to_screen(c_top_left, top_left) &&
		math::world_to_screen(c_top_right, top_right) &&
		math::world_to_screen(c_bot_left, bot_left) &&
		math::world_to_screen(c_bot_right, bot_right)) 
	{

		std::vector<ImVec2> vertices;

		vertices.push_back(ImVec2(top_left.x, top_left.y));
		vertices.push_back(ImVec2(top_right.x, top_right.y));
		vertices.push_back(ImVec2(bot_right.x, bot_right.y));
		vertices.push_back(ImVec2(bot_left.x, bot_left.y));

		window->DrawList->AddConvexPolyFilled(vertices.data(), vertices.size(), u32(color));

	}
}

void ImGuiRendering::DrawLineGradient(float x1, float y1, float x2, float y2, Color clr1, Color cl2, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilledMultiColor(ImVec2(x1, y1), ImVec2(x2, y2),
		clr1.u32(), cl2.u32(), cl2.u32(), clr1.u32());
}

void ImGuiRendering::Rect(float x, float y, float w, float h, Color clr, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), clr.u32(), rounding);
}

void ImGuiRendering::FilledRect(float x, float y, float w, float h, Color clr, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), clr.u32(), rounding);
}

void ImGuiRendering::filled_rect_gradient(float x, float y, float w, float h, Color col_upr_left,
	Color col_upr_right, Color col_bot_right, Color col_bot_left)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + w, y + h),
		col_upr_left.u32(), col_upr_right.u32(), col_bot_right.u32(), col_bot_left.u32());
}

void ImGuiRendering::FilledRect2(float x, float y, float w, float h, Color clr, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilled(ImVec2(x, y), ImVec2(w, h), clr.u32(), rounding);
}

void ImGuiRendering::Triangle(float x1, float y1, float x2, float y2, float x3, float y3, Color clr, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddTriangle(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), clr.u32(), thickness);
}

void ImGuiRendering::TriangleFilled(float x1, float y1, float x2, float y2, float x3, float y3, Color clr)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), clr.u32());
}

void ImGuiRendering::QuadFilled(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, Color clr)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddQuadFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImVec2(x4, y4), clr.u32());
}

void ImGuiRendering::CircleFilled(float x1, float y1, float radius, Color col, int segments)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddCircleFilled(ImVec2(x1, y1), radius, col.u32(), segments);
}

void ImGuiRendering::DrawWave(Vector loc, float radius, Color color, float thickness)
{
	static float Step = PI * 3.0f / 40;
	Vector prev;
	for (float lat = 0; lat <= PI * 3.0f; lat += Step)
	{
		float sin1 = sin(lat);
		float cos1 = cos(lat);
		float sin3 = sin(0.0f);
		float cos3 = cos(0.0f);

		Vector point1;
		point1 = Vector(sin1 * cos3, cos1, sin1 * sin3) * radius;
		Vector point3 = loc;
		Vector Out;
		point3 += point1;

		if (math::world_to_screen(point3, Out))
		{
			if (lat > 0.000f)
				DrawLine(prev.x, prev.y, Out.x, Out.y, color, thickness);
		}
		prev = Out;
	}
}