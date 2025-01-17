
fn ToSRGBf(val: f32) -> f32 { return select(1.055 * pow(val, 1.0 / 2.4) - 0.055, val * 12.92, val < 0.0031308); }
fn ToSRGB(v: vec3f) -> vec3f { return vec3f(ToSRGBf(v.x), ToSRGBf(v.y), ToSRGBf(v.z)); }

fn DistributionGGX(N: vec3f, H: vec3f, roughness: f32) -> f32 {
	let a = roughness * roughness;
	let a2 = a * a;
	let NdotH = max(dot(N, H), 0.0);
	let NdotH2 = NdotH * NdotH;
		
	let num = a2;
	var denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;
		
	return num / denom;
}
	
fn GeometrySchlickGGX(NdotV : f32, roughness: f32) -> f32 {
	let r = (roughness + 1.0);
	let k = (r * r) / 8.0;
	
	let num = NdotV;
	let denom = NdotV * (1.0 - k) + k;
		
	return num / denom;
}
	
fn GeometrySmith(N: vec3f, V: vec3f, L: vec3f, roughness: f32) -> f32 {
	let NdotV = max(dot(N, V), 0.0);
	let NdotL = max(dot(N, L), 0.0);
	let ggx2  = GeometrySchlickGGX(NdotV, roughness);
	let ggx1  = GeometrySchlickGGX(NdotL, roughness);
	return ggx1 * ggx2;
}
	
fn FresnelSchlick(cosTheta: f32, F0: vec3f) -> vec3f {
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}  

fn NeshnyPBR(surface_albedo: vec3f, normal : vec3f, to_light_dir: vec3f, light_radiance: vec3f, to_eye_dir: vec3f, metallic: f32, roughness: f32) -> vec3f {
	// calculate per-light radiance
	let L = to_light_dir;
	let H = normalize(to_eye_dir + L);

	let F0 = mix(vec3f(0.04), surface_albedo, metallic);

	// cook-torrance brdf
	let NDF = DistributionGGX(normal, H, roughness);        
	let G   = GeometrySmith(normal, to_eye_dir, L, roughness);
	let F   = FresnelSchlick(max(dot(H, to_eye_dir), 0.0), F0);
	
	let kS = F;
	let kD = (vec3(1.0) - kS) * (1.0 - metallic);
	
	let numerator = NDF * G * F;
	let denominator = 4.0 * max(dot(normal, to_eye_dir), 0.0) * max(dot(normal, L), 0.0);
	let specular = numerator / max(denominator, 0.001);  
	
	// add to outgoing radiance Lo
	let NdotL = max(dot(normal, L), 0.0);
	return (kD * surface_albedo / PI + specular) * light_radiance * NdotL;
}