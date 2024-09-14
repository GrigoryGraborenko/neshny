//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

//#define IMGUI_IMPL_OPENGL_LOADER_GLAD
//#define NESHNY_GL
#define NESHNY_WEBGPU
#define NESHNY_TESTING
#define SDL_WEBGPU_LOOP
#define NESHNY_WEBGPU_PROFILE

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
	#include <emscripten/html5_webgpu.h>
#else
	#define WEBGPU_BACKEND_WGPU
	#include "utils/WGPUHelpers.h"
	#include "dawn/common/Platform.h"
	#include "dawn/common/SystemUtils.h"
	#include "dawn/dawn_proc.h"
	#include "dawn/native/DawnNative.h"
#endif

#include <SDL.h>
#ifndef __EMSCRIPTEN__
    #include <SDL_syswm.h>
#endif
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <Metastuff/Meta.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>
#include <imgui/imconfig.h>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/imstb_rectpack.h>
#include <imgui/imstb_textedit.h>
#include <imgui/imstb_truetype.h>
#include <imgui/imgui_impl_wgpu.h>
#include <imgui/imgui_impl_sdl2.h>

#include <IncludeAll.h> // Neshny

#pragma msg("Compiling precompiled header...")