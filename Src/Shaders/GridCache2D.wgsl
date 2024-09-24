////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CacheUtils.wgsl"

//////////////////////////////////////////////////////////////////////////////
fn ItemMain(item_index: i32, pos: vec2f) {

    let grid_pos: vec2i = GetGridPos2D(pos, Uniform.GridMin, Uniform.GridMax, Uniform.GridSize);
    let index: i32 = (grid_pos.x + grid_pos.y * Uniform.GridSize.x) * 3;

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