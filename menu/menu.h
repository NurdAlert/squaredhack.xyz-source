#pragma once
#include "../includes.hpp"

; class c_menu : public singleton<c_menu>
{
public:
	void draw(bool is_open);
	void menu_setup(ImGuiStyle& style);

	float dpi_scale = 1.f;
	float public_alpha;
	float color_buffer[4] = { 1.f, 1.f, 1.f, 1.f };

	//Fonts
	ImFont* futura;
	ImFont* futura_large;
	ImFont* futura_small;
	ImFont* gotham;
	ImFont* ico_menu;

	ImFont* ico_bottom;
	LPDIRECT3DTEXTURE9 ct_a = nullptr;
	LPDIRECT3DTEXTURE9 tt_a = nullptr;

	// New fonts
	ImFont* OpenSans;
	ImFont* OpenSansBold;
	ImFont* weapons;
	ImFont* Timers;
	ImFont* g_pinfoFont;
	ImFont* g_pIconsFont_Sub;
	ImFont* g_pMenuFont;
	ImFont* g_pinfoFont2;
	ImFont* g_pIconsFont;
	ImFont* Info_Font_2;
	ImFont* Logo_Font;
	ImFont* Iconsis_Font;
	ImFont* Iconsis_Font_2;
	ImFont* weapon_icons;
	ImFont* g_pLoaderFont;
	ImFont* g_pTabsFont;
	ImFont* keybinds;
	ImFont* interfaces;
	ImFont* g_sub_sab;
	ImFont* g_pIconsisFont2_sub;

	IDirect3DTexture9* logo = nullptr;
	IDirect3DTexture9* foto = nullptr;
	IDirect3DDevice9* device;

private:
	struct
	{
		ImVec2 WindowPadding;
		float  WindowRounding;
		ImVec2 WindowMinSize;
		float  ChildRounding;
		float  PopupRounding;
		ImVec2 FramePadding;
		float  FrameRounding;
		ImVec2 ItemSpacing;
		ImVec2 ItemInnerSpacing;
		ImVec2 TouchExtraPadding;
		float  IndentSpacing;
		float  ColumnsMinSpacing;
		float  ScrollbarSize;
		float  ScrollbarRounding;
		float  GrabMinSize;
		float  GrabRounding;
		float  TabRounding;
		float  TabMinWidthForUnselectedCloseButton;
		ImVec2 DisplayWindowPadding;
		ImVec2 DisplaySafeAreaPadding;
		float  MouseCursorScale;
	} styles;

	bool update_dpi = false;
	bool update_scripts = false;
	void dpi_resize(float scale_factor, ImGuiStyle& style);

	int active_tab_index;
	ImGuiStyle style;
	int width = 850, height = 560;
	float child_height;

	float preview_alpha = 1.f;

	int active_tab;

	int rage_section;
	int legit_section;
	int visuals_section;
	int players_section;
	int misc_section;
	int settings_section;

	int current_profile = -1;

	std::string preview = crypt_str("None");
	

	void rage();

	void antiaim();

	void visuals();

	void players();

	void misc();

	void configs();

	void sub_tubs();

	void script();

	void tabs();

	void legit();

};
