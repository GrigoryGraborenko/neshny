////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) aPos : vec3f,
	@location(1) aCol : vec4f
}
struct VertexOut {
	@location(0) vCol : vec4f,
	@builtin(position) Position : vec4f
}
struct Uniforms {
    p_VP : mat4x4f
}
@group(0) @binding(0) var<uniform> uniforms : Uniforms;

@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;
	output.Position = uniforms.p_VP * vec4f(input.aPos, 1.0);
	output.vCol = input.aCol;
	return output;
}
@fragment
fn frag_main(@location(0) vCol : vec4f) -> @location(0) vec4f {
	return vCol;
}
