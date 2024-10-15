////////////////////////////////////////////////////////////////////////////////

fn GetGoalPos(raw_index: i32) -> vec2f {

    let move_count = atomicLoad(&ioMovementNum);
    let index: i32 = raw_index / move_count;
    let mve = GetMovement(raw_index % move_count);

    let deviation = Uniform.TotalSeconds + f32(index % mve.SubGroupSize) * mve.SubGroupDeviation + f32(index) * mve.IndexDeviation;

    var seed = vec2u(mve.Seed);

#define RNG(vct) (NextRandom((vct).x, (vct).y, &seed))

    var offset: vec2f = vec2f(f32(mve.PerRow) * -0.5 * mve.Spacing, 0.0) + mve.AnchorPos;
    for (var i = 0; i < 3; i++) {
        offset.x += sin(deviation * RNG(mve.MinMaxAngleMult) + RNG(mve.MinMaxAngleAdd)) * RNG(mve.MinMaxRadius);
        offset.y += sin(deviation * RNG(mve.MinMaxAngleMult) + RNG(mve.MinMaxAngleAdd)) * RNG(mve.MinMaxRadius);
    }

    let row: i32 = index / mve.PerRow;
    let row_ind: i32 = index % mve.PerRow;

    // sum of rows is in arithmetic series = (n / 2) * (2 * a + d * (n - 1))
    // where a = first term, d is common difference, n = num terms
    // so triangle sum = (n / 2) * (2 + (n - 1)) where a and d = 1

    return vec2f(f32(row_ind), f32(row)) * mve.Spacing + offset;
}

fn EnemyMain(item_index: i32, enemy: Enemy, new_enemy : ptr<function, Enemy>) -> bool {

    if (enemy.Health <= 0) {
        atomicAdd(&Deaths, 1);
        return true;
    }

    let spec = GetEnemySpec(enemy.Type);

    let goal = GetGoalPos(item_index);
    let delta_goal = goal - enemy.Pos;

    let delta_goal_dist = length(delta_goal);
    if (delta_goal_dist < spec.MaxSpeed) {
        (*new_enemy).Pos = goal;
    } else {
        (*new_enemy).Pos += delta_goal * (spec.MaxSpeed / delta_goal_dist);
    }

    //(*new_enemy).Pos += enemy.Velocity * Uniform.DeltaSeconds;

    (*new_enemy).Cooldown = max(0.0, enemy.Cooldown - Uniform.DeltaSeconds);
    if ((*new_enemy).Cooldown <= 0.0) {

        var projectile: EnemyProjectile;
        projectile.Pos = (*new_enemy).Pos;
        projectile.Velocity = vec2f(RandomRange(-10.0, 10.0), RandomRange(-10.0, 10.0));
        projectile.Lifespan = 1.0;
        CreateEnemyProjectile(projectile);

        (*new_enemy).Cooldown = spec.WeaponCooldown;
    }

    return false;
}