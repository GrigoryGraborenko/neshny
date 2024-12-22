////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
fn HunterMain(item_index: i32, hunter: Hunter, new_hunter : ptr<function, Hunter>) -> bool {

	let radius : f32 = Uniform.Value;
	let radius_sqr : f32 = radius * radius;

#ifdef TWO_DIM
	let pos = vec2f(hunter.FourDim.x, hunter.FourDim.y);
	let rad_vec = vec2f(radius, radius);
	#define POS_MEMBER TwoDim
#else
	let pos = vec3f(hunter.FourDim.x, hunter.FourDim.y, hunter.FourDim.z);
	let rad_vec = vec3f(radius, radius, radius);
	#define POS_MEMBER ThreeDim
#endif

#ifdef USE_CURSOR
	var cursor = StartPreyCacheCursor(pos - rad_vec, pos + rad_vec);
	while (HasNextPrey(&cursor)) {
		var prey: Prey;
		if (NextPrey(&cursor, &prey) >= 0) {
			let delta = prey.POS_MEMBER - pos;
			let dist_sqr: f32 = dot(delta, delta);
			if (dist_sqr < radius_sqr) {
				(*new_hunter).Float += prey.Float;
				(*new_hunter).ParentIndex++;
			}
		}
	}
#elifdef TWO_DIM
	let grid_search_min = GetPreyGridPosAt(pos - rad_vec);
	let grid_search_max = GetPreyGridPosAt(pos + rad_vec);

	for (var x = grid_search_min.x; x <= grid_search_max.x; x++) {
		for (var y = grid_search_min.y; y <= grid_search_max.y; y++) {

			let indices: vec2i = GetPreyIndexRangeAt(vec2i(x, y));
			for(var index = indices.x; index < indices.y; index++) {

				let i: i32 = GetPreyIndexAtCache(index);
				let prey: Prey = GetPrey(i);
				if (prey.Id < 0) {
					continue;
				}

				let delta = prey.POS_MEMBER - pos;
				let dist_sqr: f32 = dot(delta, delta);
				if (dist_sqr < radius_sqr) {
					(*new_hunter).Float += prey.Float;
					(*new_hunter).ParentIndex++;
				}
			}
		}
	}

#elifdef USE_RADIUS

	let grid_pos = GetPreyGridPosAt(pos);
	let indices : vec2i = GetPreyIndexRangeAt(grid_pos);
	for (var index = indices.x; index < indices.y; index++) {
		let i : i32 = GetPreyIndexAtCache(index);
		let prey : Prey = GetPrey(i);
		if (prey.Id < 0) {
			continue;
		}

		let delta = prey.POS_MEMBER - pos;
		let dist_sqr : f32 = dot(delta, delta);

		let rad_sqr_check = prey.Float * prey.Float;
		if (dist_sqr < rad_sqr_check) {
			(*new_hunter).Float += prey.Float;
			(*new_hunter).ParentIndex++;
		}
	}

#else

	let grid_search_min = GetPreyGridPosAt(pos - rad_vec);
	let grid_search_max = GetPreyGridPosAt(pos + rad_vec);

	for (var x = grid_search_min.x; x <= grid_search_max.x; x++) {
		for (var y = grid_search_min.y; y <= grid_search_max.y; y++) {
			for (var z = grid_search_min.z; z <= grid_search_max.z; z++) {

				let indices : vec2i = GetPreyIndexRangeAt(vec3i(x, y, z));
				for (var index = indices.x; index < indices.y; index++) {

					let i : i32 = GetPreyIndexAtCache(index);
					let prey : Prey = GetPrey(i);
					if (prey.Id < 0) {
						continue;
					}

					let delta = prey.POS_MEMBER - pos;
					let dist_sqr : f32 = dot(delta, delta);
					if (dist_sqr < radius_sqr) {
						(*new_hunter).Float += prey.Float;
						(*new_hunter).ParentIndex++;
					}
				}
			}
		}
	}

#endif

    return false;
}