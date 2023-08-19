////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct UniformStruct {
    Count: i32,
    LifeCount: i32,
    IntsPer: i32
}

@group(0) @binding(0) var<uniform> Uniform : UniformStruct;
@group(0) @binding(1) var<storage, read_write> b_Control: array<atomic<i32>>;
@group(0) @binding(2) var<storage, read> b_Death: array<i32>;
@group(0) @binding(3) var<storage, read_write> b_Entity: array<i32>;

@compute @workgroup_size(256)

////////////////////////////////////////////////////////////////////////////////
fn main(@builtin(global_invocation_id) global_id: vec3u) {

    let comp_index = i32(global_id.x);
    if (comp_index >= Uniform.Count) {
        return;
    }

    let dead_index = b_Death[comp_index];

    //////////////////////////////////
    let remain_goal: i32 = Uniform.LifeCount - Uniform.Count;
    if (dead_index >= remain_goal) {
        return;
    }

    while (true) {
        let end_counter: i32 = atomicAdd(&(b_Control[0]), 1);
        let alive_index: i32 = Uniform.LifeCount - end_counter - 1;

        var value: i32 = b_Entity[alive_index * Uniform.IntsPer];
        if (value >= 0) {
            for (var i: i32 = 0; i < Uniform.IntsPer; i++) {
                value = b_Entity[alive_index * Uniform.IntsPer + i];
                b_Entity[dead_index * Uniform.IntsPer + i] = value;
            }
            break;
        }
    }
}