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

////////////////////////////////////////////////////////////////////////////////
fn RayCapsule2D(ray_origin: vec2f, ray_end: vec2f, pos_a: vec2f, pos_b: vec2f, radius: f32, hit_pos: ptr<function, vec2f>, normal: ptr<function, vec2f>, hit_frac: ptr<function, f32>) -> bool {
    let ray_delta_full = ray_end - ray_origin;
    let inv_ray_len = 1.0 / length(ray_delta_full);
    let ray_delta = ray_delta_full * inv_ray_len;

    let ba = pos_b - pos_a;
    let oa = ray_origin - pos_a;
    let baba = dot(ba, ba);
    let bard = dot(ba, ray_delta);
    let baoa = dot(ba, oa);
    let rdoa = dot(ray_delta, oa);
    let oaoa = dot(oa, oa);
    let a = baba - bard * bard;
    var b = baba * rdoa - baoa * bard;
    var c = baba * oaoa - baoa * baoa - radius * radius * baba;
    var h = b * b - a * c;
    if (h >= 0.0) {
        let t = (-b - sqrt(h)) / a;
        let y = baoa + t * bard;
        // body
        if ((y > 0.0) && (y < baba)) {
            (*hit_frac) = t * inv_ray_len;
            (*hit_pos) = ray_origin + ray_delta_full * (*hit_frac);

            (*normal) = normalize(vec2f(ba.y, -ba.x));
            (*normal) *= select(-1.0, 1.0, dot(*normal, ray_delta) < 0.0);

            return true;
        }
        // caps
        let oc = select(ray_origin - pos_b, oa, y <= 0.0);
        b = dot(ray_delta, oc);
        c = dot(oc, oc) - radius * radius;
        h = b * b - c;
        if (h > 0.0) {
            (*hit_frac) = (-b - sqrt(h)) * inv_ray_len;
            (*hit_pos) = ray_origin + ray_delta_full * (*hit_frac);
            (*normal) = normalize((*hit_pos) - select(pos_b, pos_a, y <= 0.0));
            return true;
        }
    }
    return false;
}
