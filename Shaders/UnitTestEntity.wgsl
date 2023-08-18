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
    } else if (Uniform.Mode == 1) {
        if ((thing.Int % int_value == 0)) {
            return true;
        }
    }

    return false;
}