////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) Pos : vec2f,
	@builtin(instance_index) Instance: u32
}
struct VertexOut {
	@location(0) vUV : vec2f,
	@builtin(position) Position : vec4f
}
@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;
	if (i32(input.Instance) >= Get_ioMaxIndex) {
		output.Position = vec4f(0.0);
		return output;
	}

	let proj = GetPlayerProjectile(i32(input.Instance));
	if (proj.Id < 0) {
		output.Position = vec4f(0.0);
		return output;
	}
	
	output.Position = Uniform.ViewPerspective * vec4f(input.Pos * 0.1 + proj.Pos, 0.0, 1.0);
	output.vUV = (input.Pos + vec2f(1.0, 1.0)) * 0.5;

	return output;
}
@fragment
fn frag_main(input: VertexOut) -> @location(0) vec4f {

	let dist = length(input.vUV - vec2f(0.5, 0.5));
	let mult = 1.0 - smoothstep(0.3, 0.5, dist);
	let col = vec3f(0.5, 1.0, 0.0) * mult;

	return vec4<f32>(col, mult);
}
