////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) Pos : vec2f,
	@builtin(instance_index) Instance: u32
}
struct VertexOut {
	@location(0) vPos : vec2f,
	@builtin(position) Position : vec4f
}
@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;
	if (i32(input.Instance) >= Get_ioMaxIndex) {
		output.Position = vec4f(0.0);
		return output;
	}

	let drone = GetPlayerDrone(i32(input.Instance));
	if (drone.Id < 0) {
		output.Position = vec4f(0.0);
		return output;
	}
	
	output.Position = Uniform.ViewPerspective * vec4f(input.Pos * 0.4 + drone.Pos, 0.0, 1.0);
	output.vPos = (input.Pos + vec2f(1.0, 1.0)) * 0.5;
	output.vPos.y = 1.0 - output.vPos.y;

	return output;
}
@fragment
fn frag_main(input: VertexOut) -> @location(0) vec4f {
	let border: f32 = 0.02;

	var tex_lookup = textureSample(Sprites, SmoothSampler, input.vPos, 0);

	return tex_lookup;
}
