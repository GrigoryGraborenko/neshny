////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) Pos : vec2f
}
struct VertexOut {
	@location(0) vPos : vec2f,
	@builtin(position) Position : vec4f
}
struct UniformStruct {
	ViewPerspective : mat4x4f,
	ShipPos : vec2f
}

@group(0) @binding(0) var<uniform> Uniform: UniformStruct;

@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;

	output.Position = Uniform.ViewPerspective * vec4f(input.Pos * 1.5 + Uniform.ShipPos, 0.0, 1.0);

	output.vPos = (input.Pos + vec2f(1.0, 1.0)) * 0.5;

	return output;
}
@fragment
fn frag_main(input: VertexOut) -> @location(0) vec4f {
	var col = vec3f(input.vPos, 0.0);
	let border: f32 = 0.02;
	if (any(clamp(input.vPos, vec2f(border, border), vec2f(1.0 - border, 1.0 - border)) != input.vPos)) {
		col = vec3f(0.5, 0.5, 0.5);
	}
	return vec4<f32>(col, 1.0);
}
