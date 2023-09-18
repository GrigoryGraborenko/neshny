////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) aPos : vec2<f32>,
	@location(1) aCol : vec3<f32>
}
struct VertexOut {
	@location(0) vCol : vec3<f32>,
	@builtin(position) Position : vec4<f32>
}
struct Rotation {
	@location(0) degs : f32
}
@group(0) @binding(0) var<uniform> uRot : Rotation;
@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var rads : f32 = radians(uRot.degs);
	var cosA : f32 = cos(rads);
	var sinA : f32 = sin(rads);
	var rot : mat3x3<f32> = mat3x3<f32>(
		vec3<f32>( cosA, sinA, 0.0),
		vec3<f32>(-sinA, cosA, 0.0),
		vec3<f32>( 0.0,  0.0,  1.0));
	var output : VertexOut;
	output.Position = vec4<f32>(rot * vec3<f32>(input.aPos, 1.0), 1.0);
	output.vCol = input.aCol;
	return output;
}
@fragment
fn frag_main(@location(0) vCol : vec3<f32>) -> @location(0) vec4<f32> {
	return vec4<f32>(vCol, 1.0);
}
