////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PI 3.14159265359
#define TWOPI 6.28318530718
#define INV_PI 0.31830989
#define INV_TWOPI 0.1591549431
#define INV_255 0.0039215686274509803921568627451
#define INV_256 0.00390625
#define ALMOST_ZERO 0.0001f

#ifdef BUFFER_TEX_SIZE
////////////////////////////////////////////////////////////////////////////////
ivec2 GetBaseLocation(int index, int item_size) {
    int y = int(floor(float(index) / float(BUFFER_TEX_SIZE)));
    return ivec2(index - y * BUFFER_TEX_SIZE, y * item_size);
}
#endif

////////////////////////////////////////////////////////////////////////////////
vec3 SafeNormalize(vec3 val) {
    float len = length(val);
    if (len <= 0.0) {
        return vec3(0.0);
    }
    return val * (1.0 / len);
}

////////////////////////////////////////////////////////////////////////////////
vec2 SafeNormalize(vec2 val) {
    float len = length(val);
    if (len <= 0.0) {
        return vec2(0.0);
    }
    return val * (1.0 / len);
}

////////////////////////////////////////////////////////////////////////////////
float TrigHash(float num) {
	return fract(sin(num * 0.01 + 0.45) + cos(num * 1.04573 + 0.1) + sin(num * 11.32523 + 1.674) + sin(num * 1076.043 + 563.50));
}

////////////////////////////////////////////////////////////////////////////////
float Hash(float num) {
	return TrigHash(TrigHash(TrigHash(TrigHash(num))));
}

////////////////////////////////////////////////////////////////////////////////
float Random(float min_val, float max_val, float seed) {
    return Hash(seed) * (max_val - min_val) + min_val;
}

////////////////////////////////////////////////////////////////////////////////
uint ConvertFromFour(uvec4 col) {
    return uint((col.x << 24) | (col.y << 16) | (col.z << 8) | col.w);
}

////////////////////////////////////////////////////////////////////////////////
uvec4 ConvertToFour(uint value) {
    return uvec4(
        (value >> 24) & 255
        ,(value >> 16) & 255
        ,(value >> 8) & 255
        ,value & 255
    );
}

////////////////////////////////////////////////////////////////////////////////
float RadiansDiff(float rad_a, float rad_b) {
    float diff = mod(rad_b - rad_a + TWOPI + PI, TWOPI) - PI;
    return diff;
}

////////////////////////////////////////////////////////////////////////////////
vec3 NearestToLine(vec3 point, vec3 start, vec3 end, bool clamp_line, out float frac) {
	vec3 p_to_lp0 = point - start;
	vec3 lp1_to_lp0 = end - start;

	float numer = dot(p_to_lp0, lp1_to_lp0);
	float denom = dot(lp1_to_lp0, lp1_to_lp0);

	if(denom == 0.0) {
		frac = 0.0;
		return start;
	}
	float u = numer / denom;
	if(clamp_line) {
        u = clamp(u, 0.0, 1.0);
	}
	frac = u;

	return start + (lp1_to_lp0 * u);
}

////////////////////////////////////////////////////////////////////////////////
vec2 NearestToLine2D(vec2 point, vec2 start, vec2 end, bool clamp_line, out float frac) {
	vec2 p_to_lp0 = point - start;
	vec2 lp1_to_lp0 = end - start;

	float numer = dot(p_to_lp0, lp1_to_lp0);
	float denom = dot(lp1_to_lp0, lp1_to_lp0);

	if(denom == 0.0) {
		frac = 0.0;
		return start;
	}
	float u = numer / denom;
	if(clamp_line) {
        u = clamp(u, 0.0, 1.0);
	}
	frac = u;

	return start + (lp1_to_lp0 * u);
}

////////////////////////////////////////////////////////////////////////////////
ivec2 GetGridPos(vec2 pos, vec2 grid_min, vec2 grid_max, ivec2 grid_size) {
    vec2 range = grid_max - grid_min;
    vec2 inv_range = vec2(1.0 / range.x, 1.0 / range.y);
	vec2 frac = (pos - grid_min) * inv_range;
	return max(ivec2(0, 0), min(ivec2(grid_size.x - 1, grid_size.y - 1), ivec2(floor(vec2(grid_size) * frac))));
}

////////////////////////////////////////////////////////////////////////////////
struct GridStep2DCursor {
	ivec2 p_CurrentGrid; // current integer location on grid
	float p_CurrentFrac; // current fraction of two endpoints traversed

	vec2 p_Start;
    vec2 p_Delta;
	ivec2 p_GridDirs;
	vec2 p_Left;
	vec2 p_Amounts;
};

////////////////////////////////////////////////////////////////////////////////
GridStep2DCursor StartGridStep2D(vec2 start, vec2 end) {
	GridStep2DCursor cursor;
    ivec2 grid_pos = ivec2(floor(start));
	cursor.p_Start = start;
	cursor.p_Delta = end - start;
	cursor.p_GridDirs = ivec2(sign(cursor.p_Delta));
	cursor.p_Amounts = abs(vec2(1.0) / cursor.p_Delta);
	cursor.p_Left = max((vec2(grid_pos) - start) / cursor.p_Delta, (vec2(grid_pos + ivec2(1)) - start) / cursor.p_Delta);
	cursor.p_CurrentFrac = 0;
	cursor.p_CurrentGrid = grid_pos;
	return cursor;
}

////////////////////////////////////////////////////////////////////////////////
bool NextGridStep2D(inout GridStep2DCursor cursor) {
	float move_dist = min(cursor.p_Left.x, cursor.p_Left.y);
	// TODO: make 3d version of this
	// float move_dist = min(min(left.x, left.y), left.z);
	cursor.p_CurrentFrac += move_dist;
	cursor.p_Left -= move_dist;
	vec2 hit_res = step(0, -cursor.p_Left);
	cursor.p_Left += hit_res * cursor.p_Amounts;
	cursor.p_CurrentGrid += ivec2(hit_res) * cursor.p_GridDirs;

	return cursor.p_CurrentFrac < 1.0;
}

#define LOOKUP(tex, base, index) (texelFetch((tex), (base) + ivec2(0, (index)), 0).r)
#define IM_LOOKUP(im, base, index) (imageLoad((im), (base) + ivec2(0, (index))).r)
#define IM_SET(im, base, index, value) (imageStore((im), (base) + ivec2(0, (index)), vec4(value)).r)
