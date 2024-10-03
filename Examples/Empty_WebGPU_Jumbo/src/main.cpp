//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreCompiledHeader.h"

using namespace Neshny;

#include "Engine.h"

#include "EmbeddedDirectories.cpp"
#if !defined _DEBUG || defined __EMSCRIPTEN__
	#include "EmbeddedFiles.cpp"
#endif

#ifdef __APPLE__
	extern void* MacOSGetWindowLayer(void* nsWindow);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int, char**) {

#ifdef __APPLE__
	std::filesystem::current_path(GetMacOSExecutableDir());
#endif

	Core::Singleton().SetResourceDirs(g_ShaderBaseDirs);
#if !defined _DEBUG || defined __EMSCRIPTEN__
	Core::Singleton().SetEmbeddedFiles(g_EmbeddedFiles);
#endif

#ifdef __EMSCRIPTEN__
	std::filesystem::current_path("./assets/images/");
#endif

	Engine engine;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_Window* sdl_window = SDL_CreateWindow("Test", 100, 100, engine.GetWidth(), engine.GetHeight(), SDL_WINDOW_RESIZABLE);

	void* windowLayer = nullptr;
	Core::WebGPUNativeBackend backend = Core::WebGPUNativeBackend::D3D12;
#ifdef __APPLE__
	backend = Core::WebGPUNativeBackend::Metal; // \/\/

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(sdl_window, &wmInfo);

	NSWindow* cocoa_win = wmInfo.info.cocoa.window;
	windowLayer = MacOSGetWindowLayer(cocoa_win);
#endif

	Core::Singleton().WebGPUSDLLoop(backend, sdl_window, &engine, engine.GetWidth(), engine.GetHeight(), windowLayer);

	return 0;
}

#include "Engine.cpp"