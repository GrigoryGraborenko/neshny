////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Neshny* g_StaticInstance = nullptr;

#define EDITOR_INTERFACE_FILENAME "interface.json"
//#define EDITOR_INTERFACE_FILENAME "interface.bin"

////////////////////////////////////////////////////////////////////////////////
void WorkerThreadPool::Start(int thread_count) {
	if (thread_count <= 0) {
		return;
	}
	m_Threads.reserve(thread_count);
	for (int i = 0; i < thread_count; i++) {

		m_Threads.push_back({});
		ThreadInfo& info = m_Threads.back();

		info.m_GLContext = Neshny::Singleton().CreateGLContext();

		info.m_Thread = new std::thread([&info, &lock = m_Lock, &tasks = m_Tasks, &finished = m_FinishedTasks]() {

			bool valid = Neshny::Singleton().ActivateGLContext(info.m_GLContext);
			//auto err = SDL_GetError();

			while (true) {
				lock.lock();
				if (info.m_StopRequested) {
					lock.unlock();
					break;
				}
				if (!tasks.empty()) {
					Task* task = tasks.front();
					tasks.pop_front();
					lock.unlock();
					task->p_Result = task->p_Task();
					Neshny::OpenGLSync();
					lock.lock();
					finished.push_back(task);
					lock.unlock();
					continue;
				}
				lock.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		});
	}
}

////////////////////////////////////////////////////////////////////////////////
void WorkerThreadPool::Stop(void) {
	{
		const std::lock_guard<std::mutex> lock(m_Lock);
		for (auto& thread : m_Threads) {
			thread.m_StopRequested = true;
		}
	}
	for (auto& thread : m_Threads) {
		thread.m_Thread->join();
		delete thread.m_Thread;
		Neshny::Singleton().DeleteGLContext(thread.m_GLContext);
	}
	m_Threads.clear();
}

////////////////////////////////////////////////////////////////////////////////
void WorkerThreadPool::DoTask(std::function<void*()> task, std::function<void(void* result)> callback) {
	const std::lock_guard<std::mutex> lock(m_Lock);
	m_Tasks.push_back(new Task{ task, callback });
}

////////////////////////////////////////////////////////////////////////////////
void WorkerThreadPool::Sync(void) {
	m_Lock.lock();
	std::vector<Task*> finished = m_FinishedTasks;
	m_FinishedTasks.clear();
	m_Lock.unlock();
	for (auto task_ptr : finished) {
		task_ptr->p_Callback(task_ptr->p_Result);
		delete task_ptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Neshny::Neshny(void) {

	QFile file(EDITOR_INTERFACE_FILENAME);
	if (file.open(QIODevice::ReadOnly)) {

		//QDataStream in(&file);
		//in >> m_Interface;

		Json::ParseError err;
		Json::FromJson<InterfaceCore>(file.readAll(), m_Interface, err);
		if (m_Interface.p_Version != INTERFACE_SAVE_VERSION) {
			m_Interface = InterfaceCore{};
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
Neshny::~Neshny(void) {

	m_ResourceThreads.Stop();

	for (auto& resource : m_Resources) {
		delete resource.second.m_Resource;
	}
	m_Resources.clear();

	QFile file(EDITOR_INTERFACE_FILENAME);
	if (file.open(QIODevice::WriteOnly)) {
		
		Json::ParseError err;
		QByteArray data = Json::ToJson<InterfaceCore>(m_Interface, err);
		if (!err) {
			file.write(data);
		}

		//QDataStream out(&file);
		//out << m_Interface;
	}
}

////////////////////////////////////////////////////////////////////////////////
bool Neshny::LoopInit(IEngine* engine) {

	m_ResourceThreads.Start(1); // started here so you have access to m_Window value

	g_StaticInstance = this;

	m_LogFile.setFileName("Core.log");
	if (!m_LogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& msg) {
		switch (type) {
		case QtDebugMsg:
		case QtWarningMsg:
		case QtCriticalMsg:
		case QtFatalMsg:
			g_StaticInstance->m_LogFile.write(QString("[%1] %2 \n").arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")).arg(msg).toLocal8Bit());
			g_StaticInstance->m_LogFile.flush();
			break;
		case QtInfoMsg:
		default:
			break;
		}
		});

	qDebug() << "Core initializing...";

	engine->Init();
	m_Ticks = 0;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void Neshny::LoopInner(IEngine* engine, int width, int height, bool& fullscreen_hover) {

	ImGui::SetNextWindowPos(ImVec2(-2, -2), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width + 4, height + 4), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::Begin("Window", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus
	);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, width, height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	engine->Render(width, height);

	ImGui::SetCursorPos(ImVec2(0, 0));
	ImGui::InvisibleButton("##FullScreen", ImVec2(width - 4, height - 4));

	if (fullscreen_hover = ImGui::IsItemHovered()) {
		ImGuiIO& io = ImGui::GetIO();
		if (io.MouseWheel != 0.0f) {
			engine->MouseWheel(io.MouseWheel > 0.0f);
		}
		for(int i = 0; i < 3; i++) {
			if (ImGui::IsMouseReleased(i)) {
				engine->MouseButton(i, false);
			} else if (ImGui::IsMouseClicked(i)) {
				engine->MouseButton(i, true);
			}
		}
	}
}

#ifdef SDL_h_
////////////////////////////////////////////////////////////////////////////////
bool Neshny::SDLLoop(SDL_Window* window, IEngine* engine) {

	m_Window = window;
	if (!LoopInit(engine)) {
		return false;
	}

	//bool is_mouse_relative = true;
	//SDL_SetRelativeMouseMode(is_mouse_relative ? SDL_TRUE : SDL_FALSE);
	SDL_SetRelativeMouseMode(SDL_FALSE);
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	QElapsedTimer frame_timer;
	frame_timer.restart();
	const double inv_nano = 1.0 / 1000000000;

	int unfocus_timeout = 0;

	// Main loop
	bool fullscreen_hover = true;
	while (!engine->ShouldExit()) {

		qint64 loop_nanos = DebugTiming::MainLoopTimer();
		InfoViewer::LoopTime(loop_nanos);

		int mouse_dx = 0;
		int mouse_dy = 0;

		//if (is_mouse_relative ^ thing->IsMouseRelative()) {
		//	is_mouse_relative = !is_mouse_relative;
		//	SDL_SetRelativeMouseMode(is_mouse_relative ? SDL_TRUE : SDL_FALSE);
		//}

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)) {
				unfocus_timeout = 2;
				continue;
			}
			if (unfocus_timeout > 0) {
				unfocus_timeout--;
				continue;
			}

			ImGui_ImplSDL2_ProcessEvent(&event);
			if ((event.type == SDL_QUIT) || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))) {
				engine->ExitSignal();
			} else if (event.type == SDL_MOUSEMOTION) {
				mouse_dx = event.motion.xrel;
				mouse_dy = event.motion.yrel;
				engine->MouseMove(QVector2D(mouse_dx, mouse_dy), !fullscreen_hover);
			} else if (event.type == SDL_KEYDOWN) {
				engine->Key(event.key.keysym.sym, true);
			} else if (event.type == SDL_KEYUP) {
				engine->Key(event.key.keysym.sym, false);
			}
		}

		SDL_GetWindowSize(window, &width, &height);

		qint64 nanos = frame_timer.nsecsElapsed();
		frame_timer.restart();
		if (engine->Tick(nanos, m_Ticks)) {
			m_Ticks++;
		}

		///////////////////////////////////////////// render engine

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);

		ImGui::NewFrame();
		LoopInner(engine, width, height, fullscreen_hover);
		// Finish imgui rendering
		ImGui::End();
		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

		m_ResourceThreads.Sync();
	}

	//delete engine;
	//engine = nullptr;

	qDebug() << "Finished";

	return true;
}
#endif

#ifdef QT_LOOP
////////////////////////////////////////////////////////////////////////////////
bool Neshny::QTLoop(QOpenGLWindow* window, IEngine* engine) {

	if (!LoopInit(engine)) {
		return false;
	}

	QElapsedTimer frame_timer;
	frame_timer.restart();
	const double inv_nano = 1.0 / 1000000000;

	bool fullscreen_hover = true;

	ImGuiIO& io = ImGui::GetIO();
	while (!engine->ShouldExit()) {

		qint64 loop_nanos = DebugTiming::MainLoopTimer();
		InfoViewer::LoopTime(loop_nanos);

		QApplication::processEvents(QEventLoop::WaitForMoreEvents, 1);
		int width = window->width();
		int height = window->height();

		qint64 nanos = frame_timer.nsecsElapsed();
		frame_timer.restart();
		if (engine->Tick(nanos, m_Ticks)) {
			m_Ticks++;
		}

		if (io.DeltaTime <= 0.0f) {
			io.DeltaTime = 0.00001f;
		}

		window->makeCurrent();

		QtImGui::newFrame();
		LoopInner(engine, width, height, fullscreen_hover);
		// Finish imgui rendering
		ImGui::End();
		QtImGui::render();

		window->context()->swapBuffers(window->context()->surface());
		window->doneCurrent();
	}
	return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////
void Neshny::IRenderEditor(void) {

	ImGui::SetCursorPos(ImVec2(10.0, 10.0));
	if (ImGui::Button(m_Interface.p_InfoView.p_Visible ? "Hide info view" : "Show info view")) {
		m_Interface.p_InfoView.p_Visible = !m_Interface.p_InfoView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button(m_Interface.p_BufferView.p_Visible ? "Hide buffer view" : "Show buffer view")) {
		m_Interface.p_BufferView.p_Visible = !m_Interface.p_BufferView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button(m_Interface.p_ShaderView.p_Visible ? "Hide shader view" : "Show shader view")) {
		m_Interface.p_ShaderView.p_Visible = !m_Interface.p_ShaderView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button(m_Interface.p_ResourceView.p_Visible ? "Hide resource view" : "Show resource view")) {
		m_Interface.p_ResourceView.p_Visible = !m_Interface.p_ResourceView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button(m_Interface.p_Scrapbook2D.p_Visible ? "Hide 2D scrapbook" : "Show 2D scrapbook")) {
		m_Interface.p_Scrapbook2D.p_Visible = !m_Interface.p_Scrapbook2D.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button(m_Interface.p_Scrapbook3D.p_Visible ? "Hide 3D scrapbook" : "Show 3D scrapbook")) {
		m_Interface.p_Scrapbook3D.p_Visible = !m_Interface.p_Scrapbook3D.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button(m_Interface.p_ShowImGuiDemo ? "Hide ImGUI demo" : "Show ImGUI demo")) {
		m_Interface.p_ShowImGuiDemo = !m_Interface.p_ShowImGuiDemo;
	}
	if (m_Interface.p_ShowImGuiDemo) {
		ImGui::ShowDemoWindow();
	}

	/*
	auto debug_strs = DebugRender::GetStrings();
	for (auto str : debug_strs) {
		auto byte_str = str.toLocal8Bit();
		ImGui::Text(byte_str.data());
	}
	auto debug_persist_strs = DebugRender::GetPersistantStrings();
	for (auto str : debug_persist_strs) {
		auto byte_key_str = str.first.toLocal8Bit();
		auto byte_val_str = str.second.toLocal8Bit();
		ImGui::Text(byte_val_str.data());
	}
	*/

	InfoViewer::RenderImGui(m_Interface.p_InfoView);
	BufferViewer::Singleton().RenderImGui(m_Interface.p_BufferView);
	ShaderViewer::RenderImGui(m_Interface.p_ShaderView);
	ResourceViewer::RenderImGui(m_Interface.p_ResourceView);
	Scrapbook2D::RenderImGui(m_Interface.p_Scrapbook2D);
	Scrapbook3D::RenderImGui(m_Interface.p_Scrapbook3D);
}

////////////////////////////////////////////////////////////////////////////////
bool Neshny::IIsBufferEnabled(QString name) {
	if (m_Interface.p_BufferView.p_AllEnabled) {
		return true;
	}
	for (auto& buff : m_Interface.p_BufferView.p_Items) {
		if (buff.p_Name == name) {
			return buff.p_Enabled;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
void Neshny::DispatchMultiple(GLShader* prog, int count, bool mem_barrier) {
	glUniform1i(prog->GetUniform("uCount"), count);
	constexpr int max_dispatch = 4 * 4 * 4 * 8 * 8 * 8;
	for (int offset = 0; offset < count; offset += max_dispatch) {
		glUniform1i(prog->GetUniform("uOffset"), offset);
		glDispatchCompute(4, 4, 4);
		if (mem_barrier) {
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
GLShader* Neshny::IGetShader(QString name, QString insertion) {

	if (!m_EmbeddableLoader.has_value()) {
		return nullptr;
	}

	QString lookup_name = name;
	if (!insertion.isNull()) {
		auto hash_val = QCryptographicHash::hash(insertion.toLocal8Bit(), QCryptographicHash::Md5).toHex(0);; // security is no concern, only speed and lack of collisions
		lookup_name += "_" + hash_val;
	}

	auto found = m_Shaders.find(lookup_name);
	if (found != m_Shaders.end()) {
		return found->second;
	}
	m_Shaders.insert_or_assign(lookup_name, nullptr);

	QString vertex_name = name + ".vert", fragment_name = name + ".frag", geometry_name = "";

	GLShader* new_shader = new GLShader();
	m_Shaders.insert_or_assign(lookup_name, new_shader);
	QString err_msg = "";
	if (!new_shader->Init(err_msg, m_EmbeddableLoader.value(), vertex_name, fragment_name, geometry_name, insertion)) {
		qDebug() << err_msg;
		m_Interface.p_ShaderView.p_Visible = true;
	}
	return new_shader;
}

////////////////////////////////////////////////////////////////////////////////
GLShader* Neshny::IGetComputeShader(QString name, QString insertion) {

	if (!m_EmbeddableLoader.has_value()) {
		return nullptr;
	}

	QString lookup_name = name;
	if (!insertion.isNull()) {
		auto hash_val = QCryptographicHash::hash(insertion.toLocal8Bit(), QCryptographicHash::Md5).toHex(0);; // security is no concern, only speed and lack of collisions
		lookup_name += "_" + hash_val;
	}

	auto found = m_ComputeShaders.find(lookup_name);
	if (found != m_ComputeShaders.end()) {
		return found->second;
	}
	m_ComputeShaders.insert_or_assign(lookup_name, nullptr);

	QString shader_name = name + ".comp";

	GLShader* new_shader = new GLShader();
	m_ComputeShaders.insert_or_assign(lookup_name, new_shader);
	QString err_msg;
	if (!new_shader->InitCompute(err_msg, m_EmbeddableLoader.value(), shader_name, insertion)) {
		m_Interface.p_ShaderView.p_Visible = true;
		qDebug() << err_msg;
	}
	return new_shader;
}

////////////////////////////////////////////////////////////////////////////////
GLBuffer* Neshny::IGetBuffer(QString name) {

	auto found = m_Buffers.find(name);
	if (found != m_Buffers.end()) {
		return found->second;
	}
	m_Buffers.insert_or_assign(name, nullptr);

	int vertex_counter = 0;
	GLBuffer* new_buffer = nullptr;
	if(name == "Square") {

		new_buffer = new GLBuffer();

		new_buffer->Init(2, GL_TRIANGLES
			,std::vector<GLfloat>({
				-1.0f, -1.0f
				,1.0f, -1.0f
				,1.0f, 1.0f
				,-1.0f, 1.0f
			})
			,std::vector<GLuint>({ 0, 2, 1, 0, 3, 2 }));

	} else if(name == "Cube") {
		new_buffer = new GLBuffer();

		const float S = 0.5;
		std::vector<float> coords({
			-S, -S, -S
			,-S, S, -S
			,S, S, -S
			,S, -S, -S
			,-S, -S, S
			,-S, S, S
			,S, S, S
			,S, -S, S
		});

		new_buffer->Init(3, GL_TRIANGLES
			,coords
			,std::vector<GLuint>({
				0, 1, 4
				,4, 1, 5

				,1, 2, 5
				,5, 2, 6

				,2, 3, 6
				,6, 3, 7

				,3, 0, 7
				,7, 0, 4

				,4, 5, 6
				,4, 6, 7

				,1, 0, 2
				,2, 0, 3
			})
		);
	} else if(name == "Cylinder") {

		new_buffer = new GLBuffer();

		constexpr int num_segments = 16;
		const double seg_rads = (2 * PI) / num_segments;

		std::vector<GLfloat> vertices;
		double ang = 0;
		for (int i = 0; i <= num_segments; i++) {
			double x = -cos(ang);
			double y = sin(ang);
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(0);
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(1);
			ang += seg_rads;
		}
		new_buffer->Init(3, GL_TRIANGLE_STRIP, vertices);
	} else if(name == "DebugLine") {
		new_buffer = new GLBuffer();
		new_buffer->Init(2, GL_LINES, std::vector<GLfloat>({1, 0, 0, 1}));
	} else if(name == "DebugTriangle") {
		new_buffer = new GLBuffer();
		new_buffer->Init(2, GL_TRIANGLES, std::vector<GLfloat>({0, 0, 1, 0, 0, 1}));
	} else if(name == "Mess") {

		std::vector<GLfloat> vertices;
		for (int i = 0; i < 3 * 16; i++) {
			vertices.push_back(Random(-1, 1));
		}
		new_buffer = new GLBuffer();
		new_buffer->Init(3, GL_TRIANGLE_STRIP, vertices);
	} else if(name == "TriangleBlob") {

		new_buffer = new GLBuffer();

		new_buffer->Init(3, GL_TRIANGLES
			, std::vector<GLfloat>({
				1.0f, 0.0f, 0.0f
				,-1.0f, 0.3f, 0.0f
				,-1.0f, -0.3f, -0.3f
				,-1.0f, -0.3f, 0.3f
				})
			, std::vector<GLuint>({ 0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 3, 2 }));

	} else {
		return nullptr;
	}
	m_Buffers.insert_or_assign(name, new_buffer);
	return new_buffer;
}

////////////////////////////////////////////////////////////////////////////////
void Neshny::UnloadAllShaders(void) {

	for (auto it = m_Shaders.begin(); it != m_Shaders.end(); it++) {
		delete it->second;
	}
	m_Shaders.clear();

	for (auto it = m_ComputeShaders.begin(); it != m_ComputeShaders.end(); it++) {
		delete it->second;
	}
	m_ComputeShaders.clear();
}

////////////////////////////////////////////////////////////////////////////////
int Neshny::CreateGLContext(void) {
#ifdef SDL_h_
	auto previous = SDL_GL_GetCurrentContext();
	auto context = SDL_GL_CreateContext(m_Window);
	SDL_GL_MakeCurrent(m_Window, previous);
	m_Contexts.push_back(context);
	return int(m_Contexts.size()) - 1;
#else
	return -1;
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool Neshny::ActivateGLContext(int index) {
#ifdef SDL_h_
	if ((index < 0) || (index >= m_Contexts.size())) {
		return false;
	}
	return SDL_GL_MakeCurrent(m_Window, m_Contexts[index]) == 0;
#else
	return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Neshny::DeleteGLContext(int index) {
#ifdef SDL_h_
	if ((index < 0) || (index >= m_Contexts.size())) {
		return;
	}
	SDL_GL_DeleteContext(m_Contexts[index]);
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Neshny::OpenGLSync(void) {
	GLsync fenceId = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fenceId, GL_SYNC_FLUSH_COMMANDS_BIT, GLuint64(1000000000)); // 1 second timeout
}
