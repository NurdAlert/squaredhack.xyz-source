#include "..\..\includes.hpp"
#include "../../utils/steam/steam_api.h"

struct trail_pos_info
{
	Vector position;
	float time = -1;
};

class misc : public singleton <misc> 
{
public:
	void watermark();
	void NoDuck(CUserCmd* cmd);
	void AutoCrouch(CUserCmd* cmd);
	void SlideWalk(CUserCmd* cmd);
	void automatic_peek(CUserCmd* cmd, float wish_yaw);
	void ViewModel();
	void FullBright();
	void PovArrows(player_t* e, Color color);
	void NightmodeFix();
	void desync_arrows();
	void keybinds();
	void KillEffect(IGameEvent* pEvent);
	void spectators_list();
	void aimbot_hitboxes();
	void ragdolls();
	void rank_reveal();
	void fast_stop(CUserCmd* m_pcmd);
	void MovementTrails();
	void ChatSpammer(); 
	void indicators();

	void break_prediction(CUserCmd* cmd);

	std::vector<trail_pos_info> trail_pos;
};

class CTeslaInfo
{
public:
	Vector m_vPos;
	Vector m_vAngles;
	int m_nEntIndex;
	const char* m_pszSpriteName;
	float m_flBeamWidth;
	int m_nBeams;
	Vector m_vColor;
	float m_flTimeVisible;
	float m_flRadius;
	float m_flRed;
	float m_flGreen;
	float m_flBlue;
	float m_flBrightness;
};