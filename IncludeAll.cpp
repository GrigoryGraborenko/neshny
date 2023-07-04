//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "glad/glad.c"
// min and max get defined as macros by glad.c and ruin std::min and std::max
#undef min
#undef max

#include "NeshnyUtils.cpp"
#include "LinearAlgebra.cpp"
#include "GLUtils.cpp"
#include "GPUEntity.cpp"
#include "Core.cpp"
#include "EditorViewers.cpp"
#include "Pipeline.cpp"
#include "Geometry.cpp"
#include "Testing.cpp"
