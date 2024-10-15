////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

/*
gameplan:
	player drones have boundaries?
		no, just bound main ship
	enemies come in and stay/leave
		
		leave ones
			also limited moving slots, just that they die once they leave the area

async read with token
	instance insertion, eventual id and index result
	extractAll sync/async
trigger once-off perf warnings on sync read
clean up API for get values etc, sync/async naming
how do you have persistant values you can just cart around from place to place

replacemain for render should work
PROTECT AGAINST ENTITY OVERFLOW ON CREATE!
maybe share some common buffers like death?
number of buffers is a big problem!!!!
	could you combine the two cache buffers into one
	freelist could be at end of main ssbo
		or if you wanted to be a real h4x0r, make it a single linked list IN PLACE? not sure how that would work tho, maybe atomic exchange? pulling off the list might be trouble

*/

#define ASPECT_RATIO 1.618
#define MAP_BASE_SIZE 20
#define AUTOFIRE_COOLDOWN 0.2

#pragma pack(push)
#pragma pack (1)

struct RenderUniform {
	fMatrix4		p_ViewPerspective; // be careful with alignment here: https://www.w3.org/TR/WGSL/#alignment-and-size
};

struct MoveUniform {
	float			p_DeltaSeconds;
	float			p_TotalSeconds;
	fVec2			p_MapSize;
	fVec2			p_ShipPos;
};

struct MiscUniform {
	fMatrix4		p_ViewPerspective;
	fVec2			p_ShipPos;
};

struct EnemyMovementSpec {
	iVec2			p_Seed;
	fVec2			p_AnchorPos;
	fVec2			p_MinMaxAngleMult;
	fVec2			p_MinMaxAngleAdd;
	fVec2			p_MinMaxRadius;
	float			p_Spacing;
	int				p_PerRow;

	int				p_SubGroupSize;
	float			p_SubGroupDeviation;
	float			p_IndexDeviation;
};

#pragma pack(pop)

namespace meta {
	template<> inline auto registerMembers<RenderUniform>() {
		return members(
			member("ViewPerspective", &RenderUniform::p_ViewPerspective)
		);
	}
	template<> inline auto registerMembers<MoveUniform>() {
		return members(
			member("DeltaSeconds", &MoveUniform::p_DeltaSeconds)
			,member("TotalSeconds", &MoveUniform::p_TotalSeconds)
			,member("MapSize", &MoveUniform::p_MapSize)
			,member("ShipPos", &MoveUniform::p_ShipPos)
		);
	}
	template<> inline auto registerMembers<MiscUniform>() {
		return members(
			member("ViewPerspective", &MiscUniform::p_ViewPerspective)
			,member("ShipPos", &MiscUniform::p_ShipPos)
		);
	}
	template<> inline auto registerMembers<EnemyMovementSpec>() {
		return members(
			member("Seed", &EnemyMovementSpec::p_Seed)
			,member("AnchorPos", &EnemyMovementSpec::p_AnchorPos)
			,member("MinMaxAngleMult", &EnemyMovementSpec::p_MinMaxAngleMult)
			,member("MinMaxAngleAdd", &EnemyMovementSpec::p_MinMaxAngleAdd)
			,member("MinMaxRadius", &EnemyMovementSpec::p_MinMaxRadius)
			,member("Spacing", &EnemyMovementSpec::p_Spacing)
			,member("PerRow", &EnemyMovementSpec::p_PerRow)
			,member("SubGroupSize", &EnemyMovementSpec::p_SubGroupSize)
			,member("SubGroupDeviation", &EnemyMovementSpec::p_SubGroupDeviation)
			,member("IndexDeviation", &EnemyMovementSpec::p_IndexDeviation)
		);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct SkillState {
	int		p_EnergyMax = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct GameMetaState {
	double	p_MainGunEnergy = 10.0;
	double	p_MainGunSpeed = 20.0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct MapMetaState {
	double	p_EnemySpawnCooldown = 0.0;
	int		p_EnemyMax = 50;
	std::vector<EnemyMovementSpec> p_EnemyMovement;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct GameState {
	int		p_TotalKills = 0;

	Vec2	p_ShipPos;
	double	p_Energy = 0.0;
	double	p_EnemySpawnCooldown = 0.0;
	double	p_AutoFireCooldown = -1.0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Game {

public:
												Game						( void );
												~Game						( void );

	void										MouseButton					( int button, bool is_down );
	void										MouseMove					( Vec2 delta );
	void										MouseWheel					( bool up );
	void										Key							( int key, bool is_down );

	bool										Init						( void );
	bool										Tick						( double delta_seconds, int tick );
	void										Render						( WebGPURTT& rtt, int width, int height );

private:

	inline double								GetEnergyRate				( void ) { return g_Skills[0].GetValue(m_SkillLevels[0]); }
	inline double								GetEnergyMax				( void ) { return g_Skills[1].GetValue(m_SkillLevels[1]); }

	void										GameStateTick				( double delta_seconds );
	void										AddEnemies					( std::vector<Vec2> positions );
	void										AddDrones					( std::vector<Vec2> positions );
	void										AddPlayerProjectiles		( std::vector<fVec2> positions );

	bool										m_Mice[3] = { false, false, false };

	Camera2D									m_Cam = { Vec2(), 20 };
	bool										m_Playing = true;
	std::chrono::duration<double>				m_TotalDuration;

	GPUEntity									m_Enemies;
	GPUEntity									m_EnemyProjectiles;
	GPUEntity									m_PlayerDrones;
	GPUEntity									m_PlayerProjectiles;
	GPUEntity									m_Debris;

	Vec2										m_MapSize = Vec2(MAP_BASE_SIZE * ASPECT_RATIO, MAP_BASE_SIZE);
	Grid2DCache									m_EnemyCache;
	Grid2DCache									m_DroneCache;

	std::vector<GPUProjectileSpec>				m_ProjectileSpecs;
	std::vector<GPUDroneSpec>					m_WeaponSpecs;

	WebGPUPipeline								m_ShipRenderPipeline;
	WebGPUBuffer								m_MiscUniform;

	MapMetaState								m_MapMetaState;
	std::vector<int>							m_SkillLevels;
	GameMetaState								m_GameMetaState;
	GameState									m_GameState;

	int											m_LastWidth = -1;
	int											m_LastHeight = -1;
	Vec2										m_LastMouseWorld;

};