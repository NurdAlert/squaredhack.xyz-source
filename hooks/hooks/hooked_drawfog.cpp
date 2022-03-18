#include "..\hooks.hpp"

bool __fastcall hooks::hooked_drawfog(void* ecx, void* edx)
{
	return !g_cfg.esp.removals[REMOVALS_FOGS] || g_cfg.esp.fog;
}