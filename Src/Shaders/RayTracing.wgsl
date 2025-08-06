////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
fn RayCircle(circle_pos: vec2f, sphere_rad: f32, ray_origin: vec2f, ray_end: vec2f, hit_pos: ptr<function, vec2f>, normal: ptr<function, vec2f>, hit_frac: ptr<function, f32>) -> bool {

	let d = ray_end - ray_origin;
	let f = ray_origin - circle_pos;

	let sqr_size = sphere_rad * sphere_rad;
	let a = dot(d, d);
	let b = 2.0 * dot(f, d);
	let c = dot(f, f) - sqr_size;
		
	var det = b * b - 4.0 * a * c;
	if (det < 0.0) {
		return false;
	}
	
	det = sqrt(det);
	let t = (-b - det) / (2.0 * a); // assume smallest frac - doesn't handle exit point
	(*hit_frac) = t;
	(*hit_pos) = d * t + ray_origin;

	(*normal) = normalize((*hit_pos) - circle_pos);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
fn RaySphere(sphere_pos: vec3f, sphere_rad: f32, ray_origin: vec3f, ray_end: vec3f, hit_pos: ptr<function, vec3f>, normal: ptr<function, vec3f>, hit_frac: ptr<function, f32>) -> bool {

	let d = ray_end - ray_origin;
	let f = ray_origin - sphere_pos;

	let sqr_size = sphere_rad * sphere_rad;
	let a = dot(d, d);
	let b = 2.0 * dot(f, d);
	let c = dot(f, f) - sqr_size;
		
	var det = b * b - 4.0 * a * c;
	if (det < 0.0) {
		return false;
	}
	
	det = sqrt(det);
	let t = (-b - det) / (2.0 * a); // assume smallest frac - doesn't handle exit point
	(*hit_frac) = t;
	(*hit_pos) = d * t + ray_origin;

	(*normal) = normalize((*hit_pos) - sphere_pos);

	return true;
}