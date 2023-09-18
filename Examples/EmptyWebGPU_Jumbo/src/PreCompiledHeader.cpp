//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreCompiledHeader.h"

#include <imgui\imgui.cpp>
#include <imgui\imgui_draw.cpp>
#include <imgui\imgui_stdlib.cpp>
#include <imgui\imgui_tables.cpp>
#include <imgui\imgui_widgets.cpp>
#include <imgui\imgui_impl_wgpu.cpp>
#include <imgui\imgui_impl_sdl2.cpp>
#include <imgui\imgui_demo.cpp>

//////////////////////////////////
//#include <IncludeAll.cpp>

#include "LinearAlgebra.cpp"
#include "Preprocessor.cpp"
#include "NeshnyUtils.cpp"
#include "WGPUUtils.cpp"
#include "GPUEntity.cpp"
#include "Core.cpp"
#include "EditorViewers.cpp"
#include "Pipeline.cpp"
#include "Geometry.cpp"
#include "Testing.cpp"
//////////////////////////////////

#ifdef _DEBUG
#pragma comment(lib, "../external/WebGPU/lib/Debug/absl_int128.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/absl_raw_logging_internal.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/absl_str_format_internal.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/absl_strerror.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/absl_strings.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/absl_strings_internal.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/absl_throw_delegate.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawn_common.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawn_headers.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawn_native.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawn_platform.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawn_proc.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawn_utils.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawncpp.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/dawncpp_headers.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/SPIRV-Tools.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/SPIRV-Tools-opt.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/tint.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/tint_diagnostic_utils.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/tint_utils_io.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/tint_val.lib")
#pragma comment(lib, "../external/WebGPU/lib/Debug/webgpu_dawn.lib")
#else
#pragma comment(lib, "../external/WebGPU/lib/Release/absl_int128.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/absl_raw_logging_internal.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/absl_str_format_internal.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/absl_strerror.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/absl_strings.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/absl_strings_internal.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/absl_throw_delegate.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawn_common.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawn_headers.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawn_native.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawn_platform.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawn_proc.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawn_utils.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawncpp.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/dawncpp_headers.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/SPIRV-Tools.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/SPIRV-Tools-opt.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/tint.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/tint_diagnostic_utils.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/tint_utils_io.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/tint_val.lib")
#pragma comment(lib, "../external/WebGPU/lib/Release/webgpu_dawn.lib")
#endif