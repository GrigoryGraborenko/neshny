////////////////////////////////////////////////////////////////////////////////

fn GetRelativePos(index: i32) -> vec2f {
    let is_pos: bool = (index % 2) == 0;

    let x: f32 = f32(index / 2 + 2) * 1.5 * select(-1.0, 1.0, is_pos);
    let y = 0.0;
    return vec2f(x, y);
}

fn PlayerDroneMain(item_index: i32, drone: PlayerDrone, new_drone : ptr<function, PlayerDrone>) -> bool {

    let goal_pos: vec2f = Uniform.ShipPos + GetRelativePos(drone.Index);

    const goal_dist_speed: f32 = 10.0;

    var goal_dir: vec2f = goal_pos - drone.Pos;
    let goal_dist = length(goal_dir);
    let goal_travel = goal_dist_speed * Uniform.DeltaSeconds;
    goal_dir *= 1.0 / goal_dist;
    if (goal_dist < goal_travel) {
        (*new_drone).Pos = goal_pos;
    } else {
        (*new_drone).Pos += goal_dir * goal_travel;
    }

    //(*new_drone).Pos += normalize(Uniform.ShipPos - (*new_drone).Pos) * 10.0 * Uniform.DeltaSeconds;

    //(*new_drone).Pos += drone.Velocity * Uniform.DeltaSeconds;
    //(*new_drone).Pos += normalize(Uniform.ShipPos - (*new_drone).Pos) * 10.1 * Uniform.DeltaSeconds;

    //(*new_drone).Velocity = Uniform.ShipPos;

    (*new_drone).Cooldown = max(0.0, drone.Cooldown - Uniform.DeltaSeconds);
    if ((*new_drone).Cooldown <= 0.0) {

        var projectile: PlayerProjectile;
        projectile.Pos = (*new_drone).Pos;

        let spread: f32 = 0.1;
        let ang : f32 = RandomRange(-spread, spread);
        projectile.Velocity = vec2f(sin(ang), cos(ang)) * 10.0;
        //projectile.Velocity = vec2f(RandomRange(-1.0, 1.0), RandomRange(1.0, 10.0));

        projectile.Lifespan = 2.0;
        CreatePlayerProjectile(projectile);

        (*new_drone).Cooldown = 0.2;
    }

    return false;
}