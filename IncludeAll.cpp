//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef NESHNY_GL
    #include "glad/glad.c"
    // min and max get defined as macros by glad.c and ruin std::min and std::max
    #undef min
    #undef max
#endif

#include "LinearAlgebra.cpp"
#include "Preprocessor.cpp"
#include "NeshnyUtils.cpp"
#ifdef NESHNY_WEBGPU
    #include "WGPUUtils.cpp"
    #include "EntityWebGPU.cpp"
    #include "PipelineWebGPU.cpp"
#elif defined(NESHNY_GL)
    #include "GLUtils.cpp"
    #include "EntityGL.cpp"
    #include "PipelineGL.cpp"
#endif
#include "Core.cpp"
#include "EditorViewers.cpp"
#include "Geometry.cpp"
#include "Testing.cpp"