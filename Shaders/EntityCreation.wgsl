////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@group(0) @binding(0) var<storage, read_write> b_Entity: array<atomic<i32>>;
@group(0) @binding(1) var<storage, read> b_FreeList: array<i32>;
@group(0) @binding(2) var<storage, read> b_Data: array<i32>;
@group(0) @binding(3) var<storage, read_write> b_EntityInfo: array<atomic<i32>>;

@compute @workgroup_size(256)

////////////////////////////////////////////////////////////////////////////////
fn main(@builtin(global_invocation_id) global_id: vec3u) {

    let count: i32 = b_Data[0];
    let comp_index = i32(global_id.x);
    if (comp_index >= count) {
        return;
    }
    let item_size: i32 = b_Data[1];
    let id_offset: i32 = b_Data[2];

    // checks the free list, and allocates there if available, otherwises expands max index and places there
    atomicAdd(&b_EntityInfo[0], 1); //ioCount
    let prev_free_count: i32 = atomicAdd(&b_EntityInfo[1], -1);//ioFreeCount
    atomicMax(&b_EntityInfo[1], 0); // ensure never negative
    let new_id: i32 = atomicAdd(&b_EntityInfo[2], 1); //ioNextId
    var creation_index: i32 = -1;
    if (prev_free_count > 0) {
        creation_index = b_FreeList[prev_free_count - 1];
    } else {
        creation_index = atomicAdd(&b_EntityInfo[3], 1); //ioMaxIndex
    }

    let in_offset: i32 = comp_index * item_size + 3;
    let out_offset: i32 = ENTITY_OFFSET_INTS + creation_index * item_size;

    // copies all data except for ID, which is allocated here
    for(var i: i32; i < item_size; i++) {
        let input: i32 = select(b_Data[in_offset + i], new_id, i == id_offset);
        atomicStore(&b_Entity[out_offset + i], input);
    }
}