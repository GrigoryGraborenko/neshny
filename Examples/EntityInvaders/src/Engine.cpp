////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Engine::Engine(void) {
}

////////////////////////////////////////////////////////////////////////////////
Engine::~Engine(void) {
	delete m_Game;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseButton(int button, bool is_down) {
	if (m_Game) {
		m_Game->MouseButton(button, is_down);
	}
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseMove(Vec2 delta, bool occluded) {
	if (occluded) {
		return;
	}
	if (m_Game) {
		m_Game->MouseMove(delta);
	}
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseWheel(bool up) {
	if (m_Game) {
		m_Game->MouseWheel(up);
	}
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Key(int key, bool is_down) {
	if (key == SDLK_ESCAPE) {
		m_ShouldExit = true;
	}
	if (m_Game) {
		m_Game->Key(key, is_down);
	}
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Init(void) {
	printf("Initializing...\n");

	//UnitTester::Execute();

	m_DepthTex = new WebGPUTexture();
	m_DepthTex->InitDepth(m_Width, m_Height);

	m_Game = new Game();
	return m_Game->Init();
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

	if (m_Game) {
		WebGPURTT rtt;
		auto token = rtt.Activate({ Core::Singleton().GetCurrentSwapTextureView() }, m_DepthTex->GetTextureView());
		m_Game->Render(rtt, width, height);
	}

	SimpleRender2D::Clear();
	Core::RenderEditor();
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Tick(double delta_seconds, int tick) {

	m_AccumilatedSeconds += delta_seconds;
	if (m_AccumilatedSeconds < MINIMUM_DELTA_SECONDS) {
		return false;
	}

	double accumilated_seconds = m_AccumilatedSeconds;
	m_AccumilatedSeconds = 0;

	if (m_Game) {
		return m_Game->Tick(accumilated_seconds, tick);
	}
	return true;
}