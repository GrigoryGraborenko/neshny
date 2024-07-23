//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreCompiledHeader.h"

using namespace Neshny;

#include "Engine.h"

#ifdef __EMSCRIPTEN__
	#include "EmbeddedFiles.cpp"
#else
	#ifndef _DEBUG
		#include "EmbeddedFiles.cpp"
	#endif
#endif
#include "EmbeddedDirectories.cpp"

#ifdef __APPLE__
extern void* MacOSGetWindowLayer(void* nsWindow);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int, char**) {

#ifdef __EMSCRIPTEN__
	std::filesystem::current_path("./assets/images/");
	Core::Singleton().SetEmbeddableFileLoader([](QString path, QString& err_msg) -> QByteArray {
		auto path_str = path.toStdString();
		auto found = g_EmbeddedFiles.find(path_str);
		if (found == g_EmbeddedFiles.end()) {
			err_msg = "Cannot find embedded file error - " + path;
			return QByteArray();
		}
		return QByteArray((char*)found->second.p_Data, found->second.p_Size);
	});
#else
	Core::Singleton().SetEmbeddableFileLoader([](QString path, QString& err_msg) -> QByteArray {
		QFile file;
		for (auto prefix : g_ShaderBaseDirs) {
			file.setFileName(QString("%1\\%2").arg(prefix.c_str()).arg(path));
			if (file.open(QIODevice::ReadOnly)) {
				break;
			}
		}
		if (!file.isOpen()) {
			err_msg = "File error - " + file.errorString();
			return QByteArray();
		}
		return file.readAll();
	});
#endif

	Engine engine;
	SDL_Init(SDL_INIT_VIDEO);
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