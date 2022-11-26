//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "glad/glad.c"
// min and max get defined as macros by glad.c and ruin std::min and std::max
#undef min
#undef max

#include "GLUtils.cpp"
#include "Triple.cpp"
#include "GPUEntity.cpp"
#include "Core.cpp"
#include "EditorViewers.cpp"
#include "Pipeline.cpp"
#include "Geometry.cpp"
#include "Testing.cpp"

////////////////////////////////////////////////////////////////////////////////
double Random(void) {
    return (double)rand() / RAND_MAX;
}

////////////////////////////////////////////////////////////////////////////////
double Random(double min_val, double max_val) {
    return ((double)rand() / RAND_MAX) * (max_val - min_val) + min_val;
}

////////////////////////////////////////////////////////////////////////////////
int RandomInt(int min_val, int max_val) {
    return int(floor(Random(min_val, max_val)));
}

////////////////////////////////////////////////////////////////////////////////
int RoundUpPowerTwo(int value) {
    if (value <= 0) {
        return 0;
    }
    int pow_two = int(ceil(log2(double(value))));
    return 1 << pow_two;
}