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

	let enemy = GetEnemy(i32(input.Instance));
	if (enemy.Id < 0) {
		output.Position = vec4f(0.0);
		return output;
	}
	
	output.Position = Uniform.ViewPerspective * vec4f(input.Pos * 0.2 + enemy.Pos, 0.0, 1.0);
	output.vPos = (input.Pos + vec2f(1.0, 1.0)) * 0.5;

	return output;
}
@fragment
fn frag_main(input: VertexOut) -> @location(0) vec4f {

	var tex_lookup = textureSample(Sprites, SmoothSampler, input.vPos, 2);
	if (tex_lookup.w < 0.75) {
		discard;
	}

	return tex_lookup;
}
