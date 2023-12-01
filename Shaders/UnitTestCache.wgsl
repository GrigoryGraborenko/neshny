////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
fn HunterMain(item_index: i32, hunter: Hunter, new_hunter : ptr<function, Hunter>) -> bool {

	let pos = vec2f(hunter.FourDim.x, hunter.FourDim.y);
	let radius: f32 = Uniform.Value;
	let radius_sqr: f32 = radius * radius;

	let grid_search_min: vec2i = GetPreyGridPosAt(pos - vec2f(radius, radius));
	let grid_search_max: vec2i = GetPreyGridPosAt(pos + vec2f(radius, radius));

	for (var x = grid_search_min.x; x <= grid_search_max.x; x++) {
		for (var y = grid_search_min.y; y <= grid_search_max.y; y++) {

			let indices: vec2i = GetPreyIndexRangeAt(vec2i(x, y));
			for(var index = indices.x; index < indices.y; index++) {

				let i: i32 = GetPreyIndexAtCache(index);
				let prey: Prey = GetPrey(i);
				if (prey.Id < 0) {
					continue;
				}
				let delta: vec2f = prey.TwoDim - pos;
				let dist_sqr: f32 = dot(delta, delta);
				if (dist_sqr < radius_sqr) {
					(*new_hunter).Float += prey.Float;
					(*new_hunter).ParentIndex++;
				}
			}
		}
	}

    return false;
}