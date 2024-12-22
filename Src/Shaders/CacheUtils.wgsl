////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CacheUtils

////////////////////////////////////////////////////////////////////////////////
fn GetGridPos2D(pos: vec2f, grid_min: vec2f, grid_max: vec2f, grid_size: vec2i) -> vec2i {
    let range: vec2f = grid_max - grid_min;
    let inv_range: vec2f = vec2f(1.0 / range.x, 1.0 / range.y);
	let frac: vec2f = (pos - grid_min) * inv_range;
	return clamp(vec2i(floor(vec2f(grid_size) * frac)), vec2i(0, 0), vec2i(grid_size.x - 1, grid_size.y - 1));
}

////////////////////////////////////////////////////////////////////////////////
fn GetGridPos3D(pos: vec3f, grid_min: vec3f, grid_max: vec3f, grid_size: vec3i) -> vec3i {
    let range: vec3f = grid_max - grid_min;
    let inv_range: vec3f = vec3f(1.0 / range.x, 1.0 / range.y, 1.0 / range.z);
	let frac: vec3f = (pos - grid_min) * inv_range;
	return clamp(vec3i(floor(vec3f(grid_size) * frac)), vec3i(0), vec3i(grid_size.x - 1, grid_size.y - 1, grid_size.z - 1));
}

////////////////////////////////////////////////////////////////////////////////
struct Grid2DCacheCursor {
	p_MinPos: vec2i,
	p_MaxPos: vec2i,
	p_Pos: vec2i,
	p_Indices: vec2i,
	p_IndexIndexIndexIndex: i32
};

////////////////////////////////////////////////////////////////////////////////
struct Grid3DCacheCursor {
	p_MinPos: vec3i,
	p_MaxPos: vec3i,
	p_Pos: vec3i,
	p_Indices: vec2i,
	p_IndexIndexIndexIndex: i32
};