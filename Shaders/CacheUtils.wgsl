////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CacheUtils

////////////////////////////////////////////////////////////////////////////////
fn GetGridPos2D(pos: vec2f, grid_min: vec2f, grid_max: vec2f, grid_size: vec2i) -> vec2i {
    let range: vec2f = grid_max - grid_min;
    let inv_range: vec2f = vec2f(1.0 / range.x, 1.0 / range.y);
	let frac: vec2f = (pos - grid_min) * inv_range;
	return max(vec2i(0, 0), min(vec2i(grid_size.x - 1, grid_size.y - 1), vec2i(floor(vec2f(grid_size) * frac))));
}