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
    #include "WebGPU/WGPUUtils.cpp"
    #include "WebGPU/EntityWebGPU.cpp"
    #include "WebGPU/PipelineWebGPU.cpp"
#elif defined(NESHNY_GL)
    #include "OpenGL/GLUtils.cpp"
    #include "OpenGL/EntityGL.cpp"
    #include "OpenGL/PipelineGL.cpp"
#endif
#include "Core.cpp"
#include "EditorViewers.cpp"
#include "Geometry.cpp"
#ifndef NESHNY_SKIP_TESTS
    #include "Testing.cpp"
#endif