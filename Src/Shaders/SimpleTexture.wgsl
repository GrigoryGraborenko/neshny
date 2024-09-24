////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexIn {
	@location(0) aPos : vec2f
}
struct VertexOut {
	@location(0) vUV : vec2f,
	@builtin(position) Position : vec4f
}
struct Uniforms {
    p_VP : mat4x4f
}
@group(0) @binding(0) var<uniform> uniforms : Uniforms;
@group(0) @binding(1) var Texture: texture_2d<f32>;
@group(0) @binding(2) var SmoothSampler: sampler;

@vertex
fn vertex_main(input : VertexIn) -> VertexOut {
	var output : VertexOut;
	output.Position = uniforms.p_VP * vec4f(input.aPos.xy, 0.0, 1.0);
	output.vUV = (input.aPos - vec2f(1.0, 1.0)) * 0.5;
	return output;
}
@fragment
fn frag_main(input: VertexOut) -> @location(0) vec4f {
	var tex_lookup = textureSample(Texture, SmoothSampler, input.vUV);
	return tex_lookup;
}
