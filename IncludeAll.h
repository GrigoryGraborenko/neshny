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

#ifdef NESHNY_GL
    #include "glad/glad.h"
#endif

#include "NeshnyUtils.h"
#include "LinearAlgebra.h"
#include "Preprocessor.h"
#ifdef NESHNY_WEBGPU
    #include "WGPUUtils.h"
#elif defined(NESHNY_GL)
    #include "GLUtils.h"
#endif
#include "NeshnyStructs.h"
#include "Serialization.h"
#include "Core.h"
#include "Resources.h"
#ifdef NESHNY_WEBGPU
    #include "EntityWebGPU.h"
#elif defined(NESHNY_GL)
    #include "EntityGL.h"
#endif
#include "EditorViewers.h"
#ifdef NESHNY_WEBGPU
    #include "PipelineWebGPU.h"
#elif defined(NESHNY_GL)
    #include "PipelineGL.h"
#endif
#include "Geometry.h"
#include "Testing.h"