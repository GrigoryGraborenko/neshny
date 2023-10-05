////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	}
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Init(void) {
	printf("Initializing...\n");

	m_DepthTex = new WebGPUTexture();
	m_DepthTex->InitDepth(m_Width, m_Height);

	WebGPURenderBuffer* render_buffer = Core::GetBuffer("Square");
	m_QuadPipeline
		.FinalizeRender("Quad", *render_buffer);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Render(int width, int height) {

	int win_width, win_height;
	SDL_GetWindowSize(Core::Singleton().GetSDLWindow(), &win_width, &win_height);
	if ((win_width != m_Width) || (win_height != m_Height)) {

		printf("Resizing from %i x %i to %i x %i \n", m_Width, m_Height, win_width, win_height);
		m_Width = win_width;
		m_Height = win_height;
		delete m_DepthTex;
		m_DepthTex = new WebGPUTexture();
		m_DepthTex->InitDepth(m_Width, m_Height);

		Core::Singleton().SetResolution(m_Width, m_Height);
		return;
	}

	{
		WebGPURTT rtt;
		auto token = rtt.Activate({ Core::Singleton().GetCurrentSwapTextureView() }, m_DepthTex->GetTextureView());
		rtt.Render(&m_QuadPipeline);
	}

	DebugRender::Clear();
	Core::RenderEditor();
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Tick(qint64 delta_nanoseconds, int tick) {

	m_AccumilatedNanoseconds += delta_nanoseconds;
	if (m_AccumilatedNanoseconds < MINIMUM_DELTA_NANOSECONDS) {
		return false;
	}

	double delta_seconds = m_AccumilatedNanoseconds * NANO_CONVERT;
	m_AccumilatedNanoseconds = 0;

	return true;
}