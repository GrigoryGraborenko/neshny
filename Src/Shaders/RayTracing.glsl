////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//float SmoothMin(float a, float b, float k) {
//	float h = max(k - abs(a - b), 0.0) / k;
//	return min(a, b) - h * h * k * 0.25;
//}
//
////////////////////////////////////////////////////////////////////////////////
float SDFSphere(vec3 pos, vec3 sphere, float radius) {
	return length(pos - sphere) - radius;
}

////////////////////////////////////////////////////////////////////////////////
float SDFCapsule(vec3 pos, vec3 cyl_start, vec3 cyl_end, float radius) {
	vec3 pa = pos - cyl_start;
	vec3 ba = cyl_end - cyl_start;
	float h = dot(pa, ba) / dot(ba, ba);
	h = clamp(h, 0.0, 1.0);
	float dist = length(pa - ba * h) - radius;
	return dist;
}

////////////////////////////////////////////////////////////////////////////////
float SDFCylinder(vec3 pos, vec3 cyl_start, vec3 cyl_end, float radius) {
	vec3 ba = cyl_end - cyl_start;
	vec3 pa = pos - cyl_start;
	float baba = dot(ba, ba);
	float paba = dot(pa, ba);
	float x = length(pa * baba - ba * paba) - radius * baba;
	float y = abs(paba - baba * 0.5) - baba * 0.5;
	float x2 = x * x;
	float y2 = y * y * baba;
	float d = (max(x, y) < 0.0) ? -min(x2, y2) : (((x > 0.0) ? x2 : 0.0) + ((y > 0.0) ? y2 : 0.0));
	float dist = sign(d) * sqrt(abs(d)) / baba;
	return dist;
}

////////////////////////////////////////////////////////////////////////////////
bool RayPlane(vec3 plane_point, vec3 plane_normal, vec3 ray_origin, vec3 ray_end, out vec3 hit_pos, out float hit_frac) {

	vec3 ray_dir = normalize(ray_end - ray_origin);
	float denom = dot(plane_normal, ray_dir);
	if (abs(denom) < ALMOST_ZERO) {
		return false;
	}
	float t = dot(plane_point - ray_origin, plane_normal) / denom;
	hit_pos = ray_dir * t + ray_origin;
	hit_frac = t;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RaySphere(vec3 sphere_pos, float sphere_rad, vec3 ray_origin, vec3 ray_end, out vec3 hit_pos, out vec3 normal, out float hit_frac) {

	vec3 d = ray_end - ray_origin;
	vec3 f = ray_origin - sphere_pos;

	float sqr_size = sphere_rad * sphere_rad;
	float a = dot(d, d);
	float b = 2.0 * dot(f, d);
	float c = dot(f, f) - sqr_size;
		
	float det = b * b - 4.0 * a * c;
	if(det < 0.0) {
		return false;
	}
	
	det = sqrt(det);
	float t = (-b - det) / (2.0 * a); // assume smallest frac - doesn't handle exit point
	hit_frac = t;
	hit_pos = d * t + ray_origin;

	normal = normalize(hit_pos - sphere_pos);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RayCylinder(vec3 cyl_start, vec3 cyl_end, float cyl_rad, vec3 sideX, vec3 sideY, vec3 ray_origin, vec3 ray_end, bool is_capsule, out vec3 hit_pos, out vec3 normal, out float hit_frac) {

    vec3 offset_start = ray_origin - cyl_start;
    vec3 offset_end = ray_end - cyl_start;

    vec2 start = vec2(dot(offset_start, sideX), dot(offset_start, sideY));
    vec2 end = vec2(dot(offset_end, sideX), dot(offset_end, sideY));

	vec2 delta = end - start;
	float rad_sqr = cyl_rad * cyl_rad;
	float a = dot(delta, delta);
	float b = 2.0 * dot(delta, start);
	float c = dot(start, start) - rad_sqr;

	float det = b * b - 4 * a * c;
	if (det < 0) {
		return false;
	}

	det = sqrt(det);
	float t = (-b - det) / (2.0 * a);
	hit_frac = t;

	vec3 world_pos = ray_origin + (ray_end - ray_origin) * t;
	hit_pos = world_pos;
	
	vec3 rel_pos = world_pos - cyl_start;

	vec3 dir = cyl_end - cyl_start;
	float len = length(dir);
	vec3 cyl_dir = dir / len;
	double along = dot(cyl_dir, rel_pos);

	if ((along < 0) || (along > len)) {
		if(!is_capsule) {
			vec3 cyl_pos = along > len ? cyl_end : cyl_start;
			normal = along > len ? cyl_dir : -cyl_dir;
			if(!RayPlane(cyl_pos, cyl_dir, ray_origin, ray_end, hit_pos, hit_frac)) {
				return false;
			}
			vec3 delta = hit_pos - cyl_pos;
			return dot(delta, delta) < rad_sqr;
		}
		if(!RaySphere((along > len) ? cyl_end : cyl_start, cyl_rad, ray_origin, ray_end, hit_pos, normal, hit_frac)) {
			return false;
		}
	} else {
		vec2 cyl_pos = vec2(dot(rel_pos, sideX), dot(rel_pos, sideY));
		normal = normalize(sideX * cyl_pos.x + sideY * cyl_pos.y);
	}
	
	return true;
}
