////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

fn main(@builtin(global_invocation_id) global_id: vec3u) {

    let comp_index = global_id.x;
    //let max_size = uCount;
    let max_size: u32 = 123;
    if (comp_index >= max_size) {
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
//bool ThingMain(int item_index, Thing thing, inout Thing new_thing) {


    //int int_value = int(floor(uValue));
    //if(uMode == 0) {
        //new_thing.Float += uValue;
        //new_thing.TwoDim += vec2(uValue);
        //new_thing.ThreeDim += vec3(uValue);
        //new_thing.FourDim += vec4(uValue);
        //new_thing.IntTwoDim += ivec2(int_value);
        //new_thing.IntThreeDim += ivec3(int_value);
        //new_thing.IntFourDim += ivec4(int_value);
    //} else if(uMode == 1) {
    //    if((new_thing.Int % int_value == 0)) {
    //        return true;
    //    }
    //}
    //return false;
//}