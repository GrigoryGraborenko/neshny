////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) aPos : vec2f
}
struct VertexOut {
	@builtin(position) Position : vec4f,
	@location(0) vScreenPos : vec2f
}

@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;
	output.vScreenPos = input.aPos;
	output.Position = vec4f(input.aPos, 0.0, 1.0);
	return output;
}
@fragment
fn frag_main(input: VertexOut) -> @location(0) vec4f {

	var eye_pos_a : vec4f = Uniform.Matrix4x4 * vec4f(input.vScreenPos.xy, -1.0, 1.0);
	var eye_pos_b : vec4f = Uniform.Matrix4x4 * vec4f(input.vScreenPos.xy, -0.9, 1.0);
	eye_pos_a /= eye_pos_a.w;
	eye_pos_b /= eye_pos_b.w;
	let eye_pos_dir : vec3f = normalize(eye_pos_b.xyz - eye_pos_a.xyz);

	let skybox_lookup = textureSampleLevel(SkyBox, Sampler, eye_pos_dir, 0.0).xyz;

	return vec4f(skybox_lookup, 1.0);
}
