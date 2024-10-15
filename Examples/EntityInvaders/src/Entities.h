////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#pragma pack(push)
#pragma pack (1)

struct GPUEnemy {
	int				p_Id;
	int				p_Type;
	int				p_Flags;
	int				p_Health;
	fVec2			p_Pos;
	fVec2			p_Velocity;
	float			p_Cooldown = 0.0;
};

struct GPUEnemyProjectile {
	int				p_Id;
	int				p_Type;
	int				p_Flags;
	float			p_Lifespan;
	float			p_Damage;
	fVec2			p_Pos;
	fVec2			p_Velocity;
};

struct GPUPlayerDrone {
	int				p_Id;
	int				p_Type;
	int				p_Flags;
	int				p_Index;
	int				p_Health;
	fVec2			p_Pos;
	fVec2			p_Velocity;
	float			p_Cooldown = 0.0;
};

struct GPUPlayerProjectile {
	int				p_Id;
	int				p_Type;
	int				p_Flags;
	float			p_Lifespan;
	float			p_Damage;
	fVec2			p_Pos;
	fVec2			p_Velocity;
};

struct GPUDebris {
	int				p_Id;
	int				p_Flags;
	float			p_Lifespan;
	float			p_MaxLifespan;
	fVec2			p_Pos;
	fVec2			p_Velocity;
};

struct GPUProjectileSpec {
	int				p_Flags;
	int				p_Damage;
	int				p_Render;
	float			p_ProjSpeed;
	float			p_ProjLifespan;
};

struct GPUDroneSpec {
	int				p_Flags;
	int				p_Health;
	int				p_Weapon;
	int				p_Render;
	int				p_Behavior;
	float			p_MaxSpeed;
	float			p_Acceleration;
	float			p_Radius;
};

struct GPUEnemySpec {
	float	p_MaxSpeed;
	float	p_WeaponCooldown;
};

#pragma pack(pop)

namespace meta {

	template<> inline auto registerMembers<GPUEnemy>() {
		return members(
			member("Id", &GPUEnemy::p_Id)
			,member("Type", &GPUEnemy::p_Type)
			,member("Flags", &GPUEnemy::p_Flags)
			,member("Health", &GPUEnemy::p_Health)
			,member("Pos", &GPUEnemy::p_Pos)
			,member("Velocity", &GPUEnemy::p_Velocity)
			,member("Cooldown", &GPUEnemy::p_Cooldown)
		);
	}

	template<> inline auto registerMembers<GPUEnemyProjectile>() {
		return members(
			member("Id", &GPUEnemyProjectile::p_Id)
			,member("Type", &GPUEnemyProjectile::p_Type)
			,member("Flags", &GPUEnemyProjectile::p_Flags)
			,member("Lifespan", &GPUEnemyProjectile::p_Lifespan)
			,member("Damage", &GPUEnemyProjectile::p_Damage)
			,member("Pos", &GPUEnemyProjectile::p_Pos)
			,member("Velocity", &GPUEnemyProjectile::p_Velocity)
		);
	}

	template<> inline auto registerMembers<GPUPlayerDrone>() {
		return members(
			member("Id", &GPUPlayerDrone::p_Id)
			,member("Type", &GPUPlayerDrone::p_Type)
			,member("Flags", &GPUPlayerDrone::p_Flags)
			,member("Index", &GPUPlayerDrone::p_Index)
			,member("Health", &GPUPlayerDrone::p_Health)
			,member("Pos", &GPUPlayerDrone::p_Pos)
			,member("Velocity", &GPUPlayerDrone::p_Velocity)
			,member("Cooldown", &GPUPlayerDrone::p_Cooldown)
		);
	}

	template<> inline auto registerMembers<GPUPlayerProjectile>() {
		return members(
			member("Id", &GPUPlayerProjectile::p_Id)
			,member("Type", &GPUPlayerProjectile::p_Type)
			,member("Flags", &GPUPlayerProjectile::p_Flags)
			,member("Lifespan", &GPUPlayerProjectile::p_Lifespan)
			,member("Damage", &GPUPlayerProjectile::p_Damage)
			,member("Pos", &GPUPlayerProjectile::p_Pos)
			,member("Velocity", &GPUPlayerProjectile::p_Velocity)
		);
	}

	template<> inline auto registerMembers<GPUDebris>() {
		return members(
			member("Id", &GPUDebris::p_Id)
			,member("Flags", &GPUDebris::p_Flags)
			,member("Lifespan", &GPUDebris::p_Lifespan)
			,member("MaxLifespan", &GPUDebris::p_MaxLifespan)
			,member("Pos", &GPUDebris::p_Pos)
			,member("Velocity", &GPUDebris::p_Velocity)
		);
	}

	template<> inline auto registerMembers<GPUProjectileSpec>() {
		return members(
			member("Flags", &GPUProjectileSpec::p_Flags)
			,member("Damage", &GPUProjectileSpec::p_Damage)
			,member("Render", &GPUProjectileSpec::p_Render)
			,member("ProjSpeed", &GPUProjectileSpec::p_ProjSpeed)
			,member("ProjLifespan", &GPUProjectileSpec::p_ProjLifespan)
		);
	}

	template<> inline auto registerMembers<GPUDroneSpec>() {
		return members(
			member("Flags", &GPUDroneSpec::p_Flags)
			,member("Health", &GPUDroneSpec::p_Health)
			,member("Weapon", &GPUDroneSpec::p_Weapon)
			,member("Render", &GPUDroneSpec::p_Render)
			,member("Behavior", &GPUDroneSpec::p_Behavior)
			,member("MaxSpeed", &GPUDroneSpec::p_MaxSpeed)
			,member("Acceleration", &GPUDroneSpec::p_Acceleration)
			,member("Radius", &GPUDroneSpec::p_Radius)
		);
	}

	template<> inline auto registerMembers<GPUEnemySpec>() {
		return members(
			member("MaxSpeed", &GPUEnemySpec::p_MaxSpeed)
			,member("WeaponCooldown", &GPUEnemySpec::p_WeaponCooldown)
		);
	}
}