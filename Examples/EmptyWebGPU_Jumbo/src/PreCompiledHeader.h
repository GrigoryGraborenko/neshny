//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
//#define NESHNY_GL
#define NESHNY_TESTING
#define SDL_WEBGPU_LOOP
#define NESHNY_WEBGPU_PROFILE

#include <QCoreApplication>
#include <QString>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#ifdef QT_GUI_LIB
    #include <QImage>
#endif
#include <QtDebug>
#include <QDateTime>
#include <QCryptographicHash>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
	#include <emscripten/html5_webgpu.h>
#else
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
#include <SDL_mixer.h>
#include <Metastuff\Meta.h>

//////////////////////////////////
//#include <IncludeAll.h> // Neshny


#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui\imgui_internal.h>
#include <imgui\imconfig.h>
#include <imgui\imgui.h>
#include <imgui\imgui_stdlib.h>
#include <imgui\imstb_rectpack.h>
#include <imgui\imstb_textedit.h>
#include <imgui\imstb_truetype.h>
#include <imgui\imgui_impl_wgpu.h>
#include <imgui\imgui_impl_sdl2.h>


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

//#include "glad/glad.h"

#include "NeshnyUtils.h"

#include "LinearAlgebra.h"
//#include "GLUtils.h"
#include "Preprocessor.h"
#include "WGPUUtils.h"
#include "NeshnyStructs.h"
#include "Serialization.h"
#include "Core.h"
#include "Resources.h"
#include "GPUEntity.h"
#include "EditorViewers.h"
#include "Pipeline.h"
#include "Geometry.h"
#include "Testing.h"
//////////////////////////////////

#pragma msg("Compiling precompiled header...")