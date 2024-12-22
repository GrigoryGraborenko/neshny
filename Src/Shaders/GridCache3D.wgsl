////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CacheUtils.wgsl"

//////////////////////////////////////////////////////////////////////////////
fn ItemMain(item_index: i32, pos: vec3f) {

    let grid_pos = GetGridPos3D(pos, Uniform.GridMin.xyz, Uniform.GridMax.xyz, Uniform.GridSize.xyz);
    let index: i32 = (grid_pos.x + (grid_pos.y + grid_pos.z * Uniform.GridSize.y) * Uniform.GridSize.x) * 3;

#ifdef PHASE_INDEX
    atomicAdd(&b_Index[index], 1);
    atomicCompareExchangeWeak(&b_Index[index + 1], 0, -1);
#elifdef PHASE_ALLOCATE
    if (!atomicCompareExchangeWeak(&b_Index[index + 1], -1, 0).exchanged) {
        return;
    }
    let cell_count: i32 = atomicLoad(&b_Index[index]);
    let offset: i32 = atomicAdd(&AllocationCount, cell_count);
    atomicStore(&b_Index[index + 1], offset);
#elifdef PHASE_FILL
    let outer_offset: i32 = atomicLoad(&b_Index[index + 1]);
    let inner_offset: i32 = atomicAdd(&b_Index[index + 2], 1);
    b_Cache[outer_offset + inner_offset] = item_index;
#endif
}

//////////////////////////////////////////////////////////////////////////////
fn ItemRadiusMain(item_index: i32, pos: vec3f, radius: f32) {

    let range = Uniform.GridMax.xyz - Uniform.GridMin.xyz;
    let inv_range = vec3f(1.0 / range.x, 1.0 / range.y, 1.0 / range.z);
    let grid_extent = vec3i(Uniform.GridSize.x - 1, Uniform.GridSize.y - 1, Uniform.GridSize.z - 1);

    let min_grid_pos = clamp(
        vec3i(floor(vec3f(Uniform.GridSize.xyz) * inv_range * (pos - vec3f(radius) - Uniform.GridMin.xyz))),
        vec3i(0, 0, 0),
        grid_extent);
    let max_grid_pos = clamp(
        vec3i(ceil(vec3f(Uniform.GridSize.xyz) * inv_range * (pos + vec3f(radius) - Uniform.GridMin.xyz))),
        vec3i(0, 0, 0),
        grid_extent);

    for (var x = min_grid_pos.x; x <= max_grid_pos.x; x++) {
        for (var y = min_grid_pos.y; y <= max_grid_pos.y; y++) {
            for (var z = min_grid_pos.z; z <= max_grid_pos.z; z++) {

                let grid_pos = vec3i(x, y, z);
                let index : i32 = (grid_pos.x + (grid_pos.y + grid_pos.z * Uniform.GridSize.y) * Uniform.GridSize.x) * 3;
#ifdef PHASE_INDEX
                atomicAdd(&b_Index[index], 1);
                atomicCompareExchangeWeak(&b_Index[index + 1], 0, -1);
#elifdef PHASE_ALLOCATE
                if (!atomicCompareExchangeWeak(&b_Index[index + 1], -1, 0).exchanged) {
                    continue;
                }
                let cell_count: i32 = atomicLoad(&b_Index[index]);
                let offset: i32 = atomicAdd(&AllocationCount, cell_count);
                atomicStore(&b_Index[index + 1], offset);
#elifdef PHASE_FILL
                let outer_offset: i32 = atomicLoad(&b_Index[index + 1]);
                let inner_offset: i32 = atomicAdd(&b_Index[index + 2], 1);
                b_Cache[outer_offset + inner_offset] = item_index;
#endif
            }
        }
    }
}