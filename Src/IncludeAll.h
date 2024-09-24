////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <vector>
#include <list>
#include <set>
#include <deque>
#include <stack>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <optional>
#include <thread>
#include <mutex>
#include <variant>
#include <chrono>
#include <format>
#include <stdint.h>
#include <span>
#include <sstream>
#include <string_view>
#include <functional>
#include <source_location>

#ifdef NESHNY_GL
    #include "glad/glad.h"
#endif

#include "NeshnyUtils.h"
#include "LinearAlgebra.h"
#include "Preprocessor.h"
#ifdef NESHNY_WEBGPU
    #include "WebGPU/WGPUUtils.h"
#elif defined(NESHNY_GL)
    #include "OpenGL/GLUtils.h"
#endif
#include "NeshnyStructs.h"
#include "Serialization.h"
#include "Core.h"
#include "Resources.h"
#ifdef NESHNY_WEBGPU
    #include "WebGPU/EntityWebGPU.h"
#elif defined(NESHNY_GL)
    #include "OpenGL/EntityGL.h"
#endif
#include "EditorViewers.h"
#ifdef NESHNY_WEBGPU
    #include "WebGPU/PipelineWebGPU.h"
#elif defined(NESHNY_GL)
    #include "OpenGL/PipelineGL.h"
#endif
#include "Geometry.h"
#ifndef NESHNY_SKIP_TESTS
    #include "Testing.h"
#endif
