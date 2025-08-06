////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CacheUtils.wgsl"

//////////////////////////////////////////////////////////////////////////////
fn ItemMain(item_index: i32, pos: vec2f) {

    let grid_pos = GetGridPos2D(pos, Uniform.GridMin.xy, Uniform.GridMax.xy, Uniform.GridSize.xy);
    let index: i32 = (grid_pos.x + grid_pos.y * Uniform.GridSize.x) * 3;

#ifdef PHASE_INDEX
    atomicAdd(&TotalSize, 1);
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
fn ItemRadiusMain(item_index: i32, pos: vec2f, radius: f32) {

    let range = Uniform.GridMax.xy - Uniform.GridMin.xy;
    let inv_range = vec2f(1.0 / range.x, 1.0 / range.y);
    let grid_extent = vec2i(Uniform.GridSize.x - 1, Uniform.GridSize.y - 1);

    let min_grid_pos = clamp(
        vec2i(floor(vec2f(Uniform.GridSize.xy) * inv_range * (pos - vec2f(radius) - Uniform.GridMin.xy))),
        vec2i(0, 0),
        grid_extent);
    let max_grid_pos = clamp(
        vec2i(ceil(vec2f(Uniform.GridSize.xy) * inv_range * (pos + vec2f(radius) - Uniform.GridMin.xy))),
        vec2i(0, 0),
        grid_extent);

    for (var x = min_grid_pos.x; x <= max_grid_pos.x; x++) {
        for (var y = min_grid_pos.y; y <= max_grid_pos.y; y++) {

            let grid_pos = vec2i(x, y);
            let index: i32 = (grid_pos.x + grid_pos.y * Uniform.GridSize.x) * 3;
#ifdef PHASE_INDEX
            atomicAdd(&TotalSize, 1);
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