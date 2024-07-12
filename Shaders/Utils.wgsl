////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PI 3.14159265359
#define TWOPI 6.28318530718
#define INV_PI 0.31830989
#define INV_TWOPI 0.1591549431
#define INV_255 0.0039215686274509803921568627451
#define INV_256 0.00390625
#define ALMOST_ZERO 0.0001

////////////////////////////////////////////////////////////////////////////////
fn SafeNormalize3D(val: vec3f) -> vec3f {
    let len: f32 = length(val);
    if (len <= 0.0) {
        return vec3f(0.0, 0.0, 0.0);
    }
    return val * (1.0 / len);
}

////////////////////////////////////////////////////////////////////////////////
fn SafeNormalize2D(val: vec2f) -> vec2f {
    let len: f32 = length(val);
    if (len <= 0.0) {
        return vec2f(0.0, 0.0);
    }
    return val * (1.0 / len);
}

////////////////////////////////////////////////////////////////////////////////
fn SafeDivide(numer: f32, denom: f32) -> f32 {
    return numer / select(denom, ALMOST_ZERO, abs(denom) == 0.0);
}

////////////////////////////////////////////////////////////////////////////////
fn Modulo(x: f32, y: f32) -> f32 {
   return x - y * floor(SafeDivide(x, y));
}

////////////////////////////////////////////////////////////////////////////////
fn ConvertFromFour(col: vec4u) -> u32 {
    return u32((col.x << 24) | (col.y << 16) | (col.z << 8) | col.w);
}

////////////////////////////////////////////////////////////////////////////////
fn ConvertToFour(value: u32) -> vec4u {
    return vec4u((value >> 24) & 255, (value >> 16) & 255, (value >> 8) & 255, value & 255);
}

////////////////////////////////////////////////////////////////////////////////
fn RadiansDiff(rad_a: f32, rad_b: f32) -> f32 {
	return Modulo(rad_b - rad_a + TWOPI + PI, TWOPI) - PI;
}

////////////////////////////////////////////////////////////////////////////////
fn NearestToLine(point: vec3f, start: vec3f, end: vec3f, clamp_line: bool, frac: ptr<function, f32>) -> vec3f {
	let p_to_lp0 = point - start;
	let lp1_to_lp0 = end - start;

	let numer: f32 = dot(p_to_lp0, lp1_to_lp0);
	let denom: f32 = dot(lp1_to_lp0, lp1_to_lp0);

	if (denom == 0.0) {
		*frac = 0.0;
		return start;
	}
	var u: f32 = SafeDivide(numer, denom);
	if (clamp_line) {
        u = clamp(u, 0.0, 1.0);
	}
	*frac = u;

	return start + (lp1_to_lp0 * u);
}

////////////////////////////////////////////////////////////////////////////////
fn NearestToLine2D(point: vec2f, start: vec2f, end: vec2f, clamp_line: bool, frac: ptr<function, f32>) -> vec2f {
	let p_to_lp0 = point - start;
	let lp1_to_lp0 = end - start;

	let numer: f32 = dot(p_to_lp0, lp1_to_lp0);
	let denom: f32 = dot(lp1_to_lp0, lp1_to_lp0);

	if(denom == 0.0) {
		*frac = 0.0;
		return start;
	}
	var u: f32 = SafeDivide(numer, denom);
	if (clamp_line) {
        u = clamp(u, 0.0, 1.0);
	}
	*frac = u;

	return start + (lp1_to_lp0 * u);
}

////////////////////////////////////////////////////////////////////////////////
fn GetInterceptPosition(target_pos: vec2f, target_vel: vec2f, start_pos: vec2f, intercept_speed: f32, intercept_pos: ptr<function, vec2f>, time_mult: ptr<function, f32>) -> bool {
	let delta = start_pos - target_pos;

	let dist = length(delta);
	let a_speed = length(target_vel);
	if (a_speed <= 0.0) {
		*intercept_pos = target_pos;
		return true;
	}
	let r = SafeDivide(intercept_speed, a_speed);
	let a = r * r - 1.0;

	// use law of cosins: c^2 = a^2 + b^2 - 2abCos(y)
	// where y is angle between position delta vector and target velocity
	let cos_a = SafeDivide(dot(delta, target_vel), (a_speed * dist));
	let b = 2 * dist * cos_a;
	let c = -(dist * dist);

	let det = b * b - 4.0 * a * c;
	if (det < 0.0) {
		return false;
	}
	let a_dist = (-b + sqrt(det)) / (2.0 * a);
	let t = SafeDivide(a_dist, a_speed);
	if (t < 0.0) {
		return false;
	}
	*time_mult = t;
	*intercept_pos = target_pos + target_vel * t;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
struct GridStep2DCursor {
	p_CurrentGrid: vec2i, // current integer location on grid
	p_CurrentFrac: f32, // current fraction of two endpoints traversed

	p_Start: vec2f,
    p_Delta: vec2f,
	p_GridDirs: vec2i,
	p_Left: vec2f,
	p_Amounts: vec2f
};

////////////////////////////////////////////////////////////////////////////////
fn StartGridStep2D(start: vec2f, end: vec2f) -> GridStep2DCursor {
	var cursor: GridStep2DCursor;
    let grid_pos = vec2i(floor(start));
	cursor.p_Start = start;
	cursor.p_Delta = end - start;
	let inv_delta = vec2f(SafeDivide(1.0, cursor.p_Delta.x), SafeDivide(1.0, cursor.p_Delta.y));
	cursor.p_GridDirs = vec2i(sign(cursor.p_Delta));
	cursor.p_Amounts = abs(inv_delta);
	cursor.p_Left = max((vec2f(grid_pos) - start) * inv_delta, (vec2f(grid_pos + vec2i(1, 1)) - start) * inv_delta);
	cursor.p_CurrentFrac = 0;
	cursor.p_CurrentGrid = grid_pos;
	return cursor;
}

////////////////////////////////////////////////////////////////////////////////
fn HasNextGridStep2D(cursor: ptr<function, GridStep2DCursor>) -> bool {
	return (*cursor).p_CurrentFrac < 1.0;
}

////////////////////////////////////////////////////////////////////////////////
fn NextGridStep2D(cursor: ptr<function, GridStep2DCursor>) {
	let move_dist = min((*cursor).p_Left.x, (*cursor).p_Left.y);
	// TODO: make 3d version of this
	// float move_dist = min(min(left.x, left.y), left.z);
	(*cursor).p_CurrentFrac += move_dist;
	(*cursor).p_Left -= move_dist;
	let hit_res = step(vec2f(0.0, 0.0), -(*cursor).p_Left);
	(*cursor).p_Left += hit_res * (*cursor).p_Amounts;
	(*cursor).p_CurrentGrid += vec2i(hit_res) * (*cursor).p_GridDirs;
}

////////////////////////////////////////////////////////////////////////////////
struct GridStep3DCursor {
	p_CurrentGrid: vec3i, // current integer location on grid
	p_CurrentFrac: f32, // current fraction of two endpoints traversed

	p_Start: vec3f,
    p_Delta: vec3f,
	p_GridDirs: vec3i,
	p_Left: vec3f,
	p_Amounts: vec3f
};

////////////////////////////////////////////////////////////////////////////////
fn StartGridStep3D(start: vec3f, end: vec3f) -> GridStep3DCursor {
	var cursor: GridStep3DCursor;
    let grid_pos = vec3i(floor(start));
	cursor.p_Start = start;
	cursor.p_Delta = end - start;
	let inv_delta = vec3f(SafeDivide(1.0, cursor.p_Delta.x), SafeDivide(1.0, cursor.p_Delta.y), SafeDivide(1.0, cursor.p_Delta.z));
	cursor.p_GridDirs = vec3i(sign(cursor.p_Delta));
	cursor.p_Amounts = abs(inv_delta);
	cursor.p_Left = max((vec3f(grid_pos) - start) * inv_delta, (vec3f(grid_pos + vec3i(1)) - start) * inv_delta);
	cursor.p_CurrentFrac = 0;
	cursor.p_CurrentGrid = grid_pos;
	return cursor;
}

////////////////////////////////////////////////////////////////////////////////
fn HasNextGridStep3D(cursor: ptr<function, GridStep3DCursor>) -> bool {
	return (*cursor).p_CurrentFrac < 1.0;
}

////////////////////////////////////////////////////////////////////////////////
fn NextGridStep3D(cursor: ptr<function, GridStep3DCursor>) {
	let move_dist = min((*cursor).p_Left.x, min((*cursor).p_Left.y, (*cursor).p_Left.z));
	(*cursor).p_CurrentFrac += move_dist;
	(*cursor).p_Left -= move_dist;
	let hit_res = step(vec3f(0.0, 0.0, 0.0), -(*cursor).p_Left);
	(*cursor).p_Left += hit_res * (*cursor).p_Amounts;
	(*cursor).p_CurrentGrid += vec3i(hit_res) * (*cursor).p_GridDirs;
}