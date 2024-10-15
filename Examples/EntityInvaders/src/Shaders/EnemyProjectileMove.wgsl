////////////////////////////////////////////////////////////////////////////////

fn EnemyProjectileMain(item_index: i32, enemy_proj: EnemyProjectile, new_enemy_proj : ptr<function, EnemyProjectile>) -> bool {

    (*new_enemy_proj).Pos += enemy_proj.Velocity * Uniform.DeltaSeconds;

	if (any(clamp((*new_enemy_proj).Pos, Uniform.MapSize * -0.5, Uniform.MapSize * 0.5) != (*new_enemy_proj).Pos)) {
		return true;
	}

    (*new_enemy_proj).Lifespan -= Uniform.DeltaSeconds;
    if ((*new_enemy_proj).Lifespan <= 0.0) {
        return true;
    }

    return false;
}