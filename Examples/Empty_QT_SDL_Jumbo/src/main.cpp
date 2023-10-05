//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PreCompiledHeader.h"

using namespace Neshny;

#include "Engine.h"

#ifndef _DEBUG
	#include "EmbeddedFiles.cpp"
#endif
#include "EmbeddedDirectories.cpp"

int main(int, char**) {

	QCoreApplication::addLibraryPath("plugins");

	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// GL 4.5 + GLSL 450
	const char* glsl_version = "#version 450";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window* window = SDL_CreateWindow("Template", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	SDL_MaximizeWindow(window);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	//SDL_GL_SetSwapInterval(1); // Enable vsync
	SDL_GL_SetSwapInterval(0); // Disable vsync

	bool err = gladLoadGL() == 0;
	if (err) {
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	Core::Singleton().SetEmbeddableFileLoader([](QString path, QString& err_msg) -> QByteArray {

#ifdef _DEBUG
		QFile file;
		for (auto prefix : g_ShaderBaseDirs) {
			file.setFileName(QString("%1\\%2").arg(prefix.c_str()).arg(path));
			if (file.open(QIODevice::ReadOnly)) {
				break;
			}
		}
		if (!file.isOpen()) {
			err_msg = "File error - " + file.errorString(); // TODO: better error handling than just last file error
			return QByteArray();
		}
		return file.readAll();
#else
		auto byte_path = path.toLocal8Bit();
		auto found = g_EmbeddedFiles.find(std::string(byte_path.data()));
		if (found == g_EmbeddedFiles.end()) {
			err_msg = "Cannot find embedded file error - " + path;
			return QByteArray();
		}
		return QByteArray((char*)found->second.p_Data, found->second.p_Size);
#endif
	});

	Engine engine;
	bool result = Core::Singleton().SDLLoop(window, &engine);

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return result ? 0 : -1;
}

#include "Engine.cpp"