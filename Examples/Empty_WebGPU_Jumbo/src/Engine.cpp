////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Engine::Engine(void) {
}

////////////////////////////////////////////////////////////////////////////////
Engine::~Engine(void) {
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseButton(int button, bool is_down) {
	m_Mice[button] = is_down;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseMove(Vec2 delta, bool occluded) {

	if ((m_Width < 0) || (m_Height < 0)) {
		return;
	}
	auto mouse = ImGui::GetMousePos();
	m_LastMouseWorld = m_Cam.ScreenToWorld(Vec2(mouse.x, mouse.y), m_Width, m_Height);

	if (m_Mice[2]) {
		m_Cam.Pan(m_Width, delta.x, delta.y);
	}

}

////////////////////////////////////////////////////////////////////////////////
void Engine::MouseWheel(bool up) {

	const double ZOOM_SPEED = 0.1;
	double new_zoom = m_Cam.p_Zoom * (1.0 + (up ? -1 : 1) * ZOOM_SPEED);

	if ((m_Width <= 0) || (m_Height <= 0)) {
		m_Cam.p_Zoom = new_zoom;
		return;
	}
	m_Cam.Zoom(new_zoom, m_LastMouseWorld, m_Width, m_Height);
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Key(int key, bool is_down) {
	if (key == SDLK_ESCAPE) {
		m_ShouldExit = true;
	}
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Init(void) {
	Core::Log("Initializing...\n");

	//UnitTester::Execute();

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

	auto view_persp = m_Cam.Get4x4Matrix(width, height);

	if (1) { // examples of simple rendering for debug or early prototyping
		Vec2 a(0, -1);
		Vec2 b(2, 2);
		Vec2 c(0, 2);

		SimpleRender2D::Line(Vec2(3, 0.5), Vec2(4, 0), Vec4(1, 0, 0, 1));

		SimpleRender2D::Square(Vec2(3, 0.5), Vec2(4, 0), Vec4(1, 0, 0, 1), false);
		SimpleRender2D::Square(Vec2(3, 1.5), Vec2(4, 1), Vec4(0.5, 0.5, 0, 1), true);

		SimpleRender2D::Triangle(a, b, c, Vec4(1, 0, 0, 1), 0.5);
		SimpleRender2D::Triangle(a + Vec2(0.2, -0.2), b + Vec2(0.2, -0.2), c + Vec2(0.2, -0.2), Vec4(0, 1, 0, 1), 0.3);

		SimpleRender2D::Texture(Vec2(-2.0, 1.0), Vec2(1.0, -4.0), "../../assets/images/Tick.png", 0.5);
		SimpleRender2D::Texture(Vec2(-1.0, 0.0), Vec2(1.6, -2.5), "../../assets/images/Tick.png", 0.4);
	}

	{
		WebGPURTT rtt;
		auto token = rtt.Activate({ Core::Singleton().GetCurrentSwapTextureView() }, m_DepthTex->GetTextureView());
		rtt.Render(&m_QuadPipeline);
		SimpleRender2D::Render(rtt, view_persp, width, height);
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

	return true;
}