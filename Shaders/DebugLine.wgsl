////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) aPos : vec3f,
}
struct VertexOut {
	@location(0) vCol : vec4f,
	@builtin(position) Position : vec4f
}
@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;
	output.Position = vec4f(input.aPos, 1.0);
	output.vCol = vec4f(1.0, 0.0, 0.0, 1.0);
	return output;
}
@fragment
fn frag_main(@location(0) vCol : vec4f) -> @location(0) vec4f {
	return vCol;
}
