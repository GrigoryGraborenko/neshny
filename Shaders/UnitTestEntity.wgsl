////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
fn ThingMain(item_index: i32, thing: Thing, new_thing: ptr<function, Thing>) -> bool {

    let uValue: f32 = Uniform.Value;

    atomicAdd(&uCheckVal, 1);

    let int_value = i32(floor(uValue));
    if (Uniform.Mode == 0) {
        (*new_thing).Float += uValue;
        (*new_thing).TwoDim += vec2f(uValue);
        (*new_thing).ThreeDim += vec3f(uValue);
        (*new_thing).FourDim += vec4f(uValue);
        (*new_thing).IntTwoDim += vec2i(int_value);
        (*new_thing).IntThreeDim += vec3i(int_value);
        (*new_thing).IntFourDim += vec4i(int_value);

#ifdef CREATE_OTHER
        if (((*new_thing).Int % 2) == 0) {
            var other: Other;
            other.ParentIndex = item_index;
            other.Float = thing.Float;
            other.FourDim = vec4f(thing.TwoDim, thing.FourDim.zw);
            CreateOther(other);
        }
#endif
    } else if(Uniform.Mode == 1) {
        let data: DataItem = GetDataItem(thing.Int % int_value);
        (*new_thing).Float += data.Float;
        (*new_thing).TwoDim += data.TwoDim;
        (*new_thing).ThreeDim += data.ThreeDim;
        (*new_thing).FourDim += data.FourDim;
        (*new_thing).IntTwoDim += data.IntTwoDim;
        (*new_thing).IntThreeDim += data.IntThreeDim;
        (*new_thing).IntFourDim += data.IntFourDim;

    } else if (Uniform.Mode == 2) {
        if ((thing.Int % int_value == 0)) {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
fn OtherMain(item_index: i32, other: Other, new_other : ptr<function, Other>) -> bool {

    let parent: Thing = GetThing(other.ParentIndex);
    if ((parent.Id >= 0) && ((parent.Int % 3) == 1)) {
        return true;
    }
    return false;
}