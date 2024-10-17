//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreCompiledHeader.h"

using namespace Neshny;

#include "Entities.h"
#include "Specs.h"
#include "Game.h"
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

	Core::Singleton().SetResourceDirs(g_ShaderBaseDirs);
#if !defined _DEBUG || defined __EMSCRIPTEN__
	Core::Singleton().SetEmbeddedFiles(g_EmbeddedFiles);
#endif

#ifdef __EMSCRIPTEN__
	std::filesystem::current_path("./assets/images/");
#endif

	Engine engine;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
	SDL_Window* sdl_window = SDL_CreateWindow("Test", 100, 100, engine.GetWidth(), engine.GetHeight(), SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

	int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
	if ((IMG_Init(img_flags) & img_flags) != img_flags) {
		printf("SDL_image could not initialize fully: %s\n", IMG_GetError());
		return -1;
	}

	int audio_buffers = 4096;
	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, audio_buffers) < 0) {
		fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		return 1;
	}

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
#include "Game.cpp"