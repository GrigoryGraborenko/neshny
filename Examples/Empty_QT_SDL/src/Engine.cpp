////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Engine.h"

////////////////////////////////////////////////////////////////////////////////
Engine::Engine(void) {
}

////////////////////////////////////////////////////////////////////////////////
Engine::~Engine(void) {
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseButton(int button, bool is_down) {
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseMove(Vec2 delta, bool occluded) {
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseWheel(bool up) {
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Key(int key, bool is_down) {
	if (key == SDLK_ESCAPE) {
		m_ShouldExit = true;
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Init(void) {
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Tick(double delta_seconds, int tick) {
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Render(int width, int height) {


	GLShader* prog_debug = Core::GetShader("Debug");

	// dynamically load the shader that corresponds to Fullscreen.vert and Fullscreen.frag
	GLShader* prog = Core::GetShader("Fullscreen");
	prog->UseProgram();
	//glUniform1i(prog->GetUniform("uniform_name"), 123);

	GLBuffer* buff = Core::GetBuffer("Square"); // get a built-in model
	buff->UseBuffer(prog); // attach the program to the buffer
	buff->Draw(); // executes the draw call

	//DebugRender::Point(Triple(100.0, 30.0, 30.0), QVector4D(0.0, 0.5, 0.5, 1.0));
	//DebugRender::Render3DDebug(vp, width, height, Triple(0, 0, 0), 1.0);

	DebugRender::Clear();
	Core::RenderEditor();
}