//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreCompiledHeader.h"

#include <imgui\imgui_impl_sdl.cpp>
#include <imgui\imgui.cpp>
#include <imgui\imgui_draw.cpp>
#include <imgui\imgui_impl_opengl3.cpp>
#include <imgui\imgui_demo.cpp>
#include <imgui\imgui_tables.cpp>
#include <imgui\imgui_widgets.cpp>
#include "imgui\imgui_stdlib.cpp"

#include "glad/glad.c"
// min and max get defined as macros by glad.c and ruin std::min and std::max
#undef min
#undef max