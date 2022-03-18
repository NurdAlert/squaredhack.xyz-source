#include "..\..\includes.hpp"

class Vector;
class QAngle;
class CViewSetup;

class c_nadepoint {
public:
	c_nadepoint() {
		m_valid = false;
	}

	c_nadepoint(Vector  start, Vector  end, bool plane, bool valid, Vector  normal, bool detonate, bool ground) {
		m_start = start;
		m_end = end;
		m_plane = plane;
		m_valid = valid;
		m_normal = normal;
		m_detonate = detonate;
		m_ground = ground;
	}

	Vector  m_start, m_end, m_normal;
	bool m_valid, m_plane, m_detonate, m_ground;
};

struct afterPredi
{
	Vector start;
	Vector end;
	bool isValid;
	bool isGround;
	bool isWall;
	bool isDetonate;
};

enum ACT
{
	ACT_NONE,
	ACT_THROW,
	ACT_LOB,
	ACT_DROP,
};

class GrenadePrediction : public singleton<GrenadePrediction>
{
private:
	std::vector<afterPredi> afterPredi;
	std::array< c_nadepoint, 500 >	_points{ };
	bool		 _predicted = false;

	std::vector<std::pair<Vector, Color>> others;
	int type = 0;
	int act = 0;

	void Setup(Vector& vecSrc, Vector& vecThrow, const Vector& viewangles);
	void Simulate(CViewSetup* setup);

	int Step(Vector& vecSrc, Vector& vecThrow, int tick, float interval);
	bool CheckDetonate(const Vector& vecThrow, const trace_t& tr, int tick, float interval);

	void TraceHull(Vector& src, Vector& end, trace_t& tr);
	void AddGravityMove(Vector& move, Vector& vel, float frametime, bool onground);
	void PushEntity(Vector& src, const Vector& move, trace_t& tr);
	void ResolveFlyCollisionCustom(trace_t& tr, Vector& vecVelocity, const Vector& move, float interval);
	int PhysicsClipVelocity(const Vector& in, const Vector& normal, Vector& out, float overbounce);
public:
	std::vector<Vector> path;
	void Tick(int buttons);
	void View(CViewSetup* setup, weapon_t* weapon);
	void Paint();
};