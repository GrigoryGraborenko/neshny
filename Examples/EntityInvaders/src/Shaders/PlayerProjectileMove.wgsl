////////////////////////////////////////////////////////////////////////////////

#include "Utils.wgsl"

fn PlayerProjectileMain(item_index: i32, player_proj: PlayerProjectile, new_player_proj : ptr<function, PlayerProjectile>) -> bool {

    (*new_player_proj).Pos += player_proj.Velocity * Uniform.DeltaSeconds;

    (*new_player_proj).Lifespan -= Uniform.DeltaSeconds;
    if ((*new_player_proj).Lifespan <= 0.0) {
        return true;
    }
	if (any(clamp((*new_player_proj).Pos, Uniform.MapSize * -0.5, Uniform.MapSize * 0.5) != (*new_player_proj).Pos)) {
		return true;
	}

	var best_frac: f32 = 1.0;
	var best_enemy: i32 = -1;

	var cursor = StartEnemyCacheCursor((*new_player_proj).Pos, player_proj.Pos);
	while (HasNextEnemy(&cursor)) {
		var enemy: Enemy;
		let enemy_index: i32 = NextEnemy(&cursor, &enemy);
		if (enemy_index >= 0) {
			let enemy_rad = 10.0;
			let enemy_rad_sqr = enemy_rad * enemy_rad;

			var along_frac: f32;
			let nearest: vec2f = NearestToLine2D(enemy.Pos, player_proj.Pos, (*new_player_proj).Pos, true, &along_frac);

			let delta: vec2f = enemy.Pos - nearest;
			let dist_sqr: f32 = dot(delta, delta);
			if((dist_sqr <= enemy_rad_sqr) && (along_frac < best_frac)) {
				best_frac = along_frac;
				best_enemy = enemy_index;
			}
		}
	}
	if (best_enemy >= 0) {
		let prior_health = atomicAdd(&AccessEnemyHealth(best_enemy), -1000);
		return true;
	}

    return false;
}