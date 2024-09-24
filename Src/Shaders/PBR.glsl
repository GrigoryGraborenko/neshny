
float to_srgbf(float val) {return val < 0.0031308 ? val*12.92 : 1.055 * pow(val, 1.0/2.4) - 0.055; }
vec3 to_srgb(vec3 v) {return vec3(to_srgbf(v.x), to_srgbf(v.y), to_srgbf(v.z)); }

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a      = roughness*roughness;
	float a2     = a*a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH*NdotH;
		
	float num   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;
		
	return num / denom;
}
	
float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	
	float num   = NdotV;
	float denom = NdotV * (1.0 - k) + k;
		
	return num / denom;
}
	
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2  = GeometrySchlickGGX(NdotV, roughness);
	float ggx1  = GeometrySchlickGGX(NdotL, roughness);
		
	return ggx1 * ggx2;
}
	
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}  
	
vec3 pbr(vec3 lightDir, vec3 radiance, vec3 V, vec3 N, float metallic, float roughness, vec3 F0, vec3 albedo) {
	// calculate per-light radiance
	vec3 L = -lightDir;
	vec3 H = normalize(V + L);
	// float distance    = length(pos - WorldPos);
	// float attenuation = 1.0 / (distance * distance);
	// vec3 radiance     = color * attenuation;        
	
	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);        
	float G   = GeometrySmith(N, V, L, roughness);      
	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
	
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;
	
	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular     = numerator / max(denominator, 0.001);  
	
	// add to outgoing radiance Lo
	float NdotL = max(dot(N, L), 0.0);                
	return (kD * albedo / PI + specular) * radiance * NdotL; 
}