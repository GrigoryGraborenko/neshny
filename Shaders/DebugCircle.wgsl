////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) aPos : vec2f,
}
struct VertexOut {
	@location(0) vCol : vec4f,
	@builtin(position) Position : vec4f
}
struct Uniforms {
    p_VP : mat4x4f
}
struct Circle {
    p_PosRadius : vec4f,
	p_Col : vec4f
}
@group(0) @binding(0) var<uniform> uniforms : Uniforms;
@group(0) @binding(1) var<storage, read> circles: array<Circle>;

@vertex
fn vertex_main(@builtin(instance_index) index: u32, input : VertexIn) -> VertexOut {
	var output : VertexOut;

	let circle = circles[index];
	output.Position = uniforms.p_VP * vec4f(circle.p_PosRadius.xy + input.aPos * circle.p_PosRadius.z, 0.0, 1.0);
	output.vCol = circle.p_Col;
	return output;
}
@fragment
fn frag_main(@location(0) vCol : vec4f) -> @location(0) vec4f {
	return vCol;
}
