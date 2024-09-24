////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) aPos : vec2f
}
struct VertexOut {
	@location(0) vPos : vec2f,
	@builtin(position) Position : vec4f
}
@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;
	output.Position = vec4f(input.aPos * 0.9, 0.9, 1.0);
	output.vPos = (input.aPos + vec2f(1.0, 1.0)) * 0.5;
	return output;
}
@fragment
fn frag_main(input: VertexOut) -> @location(0) vec4f {
	var col = vec3f(input.vPos * 0.2, 0.0);
	let border: f32 = 0.02;
	if (any(clamp(input.vPos, vec2f(border, border), vec2f(1.0 - border, 1.0 - border)) != input.vPos)) {
		col = vec3f(0.5, 0.5, 0.5);
	}
	return vec4<f32>(col, 1.0);
}
