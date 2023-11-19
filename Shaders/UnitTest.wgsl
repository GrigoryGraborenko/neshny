////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
@group(0) @binding(0) var<storage, read_write> output: array<f32>;

#ifdef TEST_RANDOM
    #include "Random.wgsl"
#else
    @group(0) @binding(1) var<uniform> uCount : u32;
    @group(0) @binding(2) var<storage, read_write> atomic_output: array<atomic<i32>>;
#endif

@compute @workgroup_size(256)

fn main(@builtin(global_invocation_id) global_id: vec3u) {

    let comp_index = global_id.x;
#ifdef TEST_RANDOM
    let max_size = arrayLength(&output);
#else
    let max_size = uCount;
#endif
    if (comp_index >= max_size) {
        return;
    }

#ifdef TEST_RANDOM
    output[comp_index] = GetRandom(0.0, 1.0, comp_index);
#else
    output[comp_index] += f32(comp_index) + 0.25;
    atomicAdd(&(atomic_output[0]), 1);
#endif
}