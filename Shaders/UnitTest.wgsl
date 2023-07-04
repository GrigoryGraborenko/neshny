////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
@group(0) @binding(0) var<storage, read_write> output: array<f32>;
@group(0) @binding(1) var<uniform> size_offset : vec2i;

@compute @workgroup_size(256)

fn main(@builtin(global_invocation_id) global_id: vec3u) {

    let comp_index = global_id.x;
    let max_size = u32(size_offset.x);
    if (comp_index >= max_size) {
        return;
    }

    output[comp_index] += f32(comp_index) + 0.25;
}