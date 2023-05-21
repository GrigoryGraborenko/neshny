////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

Core* g_StaticInstance = nullptr;

#define EDITOR_INTERFACE_FILENAME "interface.json"
//#define EDITOR_INTERFACE_FILENAME "interface.bin"

////////////////////////////////////////////////////////////////////////////////
DebugTiming::DebugTiming(const char* label) :
	m_Label(label)
{
	m_Timer.start();
}

////////////////////////////////////////////////////////////////////////////////
DebugTiming::~DebugTiming(void) {

	qint64 nanos = m_Timer.nsecsElapsed();

	auto& timings = GetTimings();
	for (auto iter = timings.begin(); iter != timings.end(); iter++) {
		if ((iter->p_Label == m_Label) || (strcmp(iter->p_Label, m_Label) == 0)) {
			iter->Add(nanos);
			return;
		}
	}
	timings.emplace_back(m_Label, nanos);
}

////////////////////////////////////////////////////////////////////////////////
QStringList DebugTiming::Report(void) {

	auto& timings = GetTimings();
	qint64 total_nanos = 0;
	for (auto iter = timings.begin(); iter != timings.end(); iter++) {
		total_nanos += iter->p_Nanos;
	}
	QStringList result;
	for (auto iter = timings.begin(); iter != timings.end(); iter++) {
		result += iter->Report(total_nanos);
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////
void WorkerThreadPool::Start(int thread_count) {
	if (thread_count <= 0) {
		return;
	}
	m_Threads.reserve(thread_count);
	for (int i = 0; i < thread_count; i++) {

		m_Threads.push_back({});
		ThreadInfo& info = m_Threads.back();

#ifdef NESHNY_GL
		info.m_GLContext = Core::Singleton().CreateGLContext();
#endif
		info.m_Thread = new std::thread([&info, &lock = m_Lock, &tasks = m_Tasks, &finished = m_FinishedTasks]() {

#ifdef NESHNY_GL
			bool valid = Core::Singleton().ActivateGLContext(info.m_GLContext);
#endif
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
#ifdef NESHNY_GL
					Core::OpenGLSync();
#endif
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
#ifdef NESHNY_GL
		Core::Singleton().DeleteGLContext(thread.m_GLContext);
#endif
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
Core::Core(void) {

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
Core::~Core(void) {

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
bool Core::LoopInit(IEngine* engine) {

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

	if (!engine->Init()) {
		qDebug() << "Could not init engine";
	}
	m_Ticks = 0;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void Core::LoopInner(IEngine* engine, int width, int height, bool& fullscreen_hover) {

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

#ifdef NESHNY_GL
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
#endif

	engine->Render(width, height);

	ImGui::SetCursorPos(ImVec2(0, 0));
	ImGui::InvisibleButton("##FullScreen", ImVec2(width - 4, height - 4));

	if ((fullscreen_hover = ImGui::IsItemHovered())) {
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

#ifdef SDL_OPENGL_LOOP
////////////////////////////////////////////////////////////////////////////////
bool Core::SDLLoop(SDL_Window* window, IEngine* engine) {

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
	DebugTiming::MainLoopTimer(); // init to zero
	bool fullscreen_hover = true;
	while (!engine->ShouldExit()) {

		qint64 loop_nanos = DebugTiming::MainLoopTimer();
#ifdef NESHNY_EDITOR_VIEWERS
		InfoViewer::LoopTime(loop_nanos);
#endif

		int mouse_dx = 0;
		int mouse_dy = 0;

		//if (is_mouse_relative ^ thing->IsMouseRelative()) {
		//	is_mouse_relative = !is_mouse_relative;
		//	SDL_SetRelativeMouseMode(is_mouse_relative ? SDL_TRUE : SDL_FALSE);
		//}

		if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)) {
			unfocus_timeout = 2;
		}

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

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
				engine->MouseMove(Vec2(mouse_dx, mouse_dy), !fullscreen_hover);
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
		engine->ManageResources(Core::Singleton().GetResourceManagementToken(), GetMemoryAllocated(), GetGPUMemoryAllocated());

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

#ifdef SDL_WEBGPU_LOOP
////////////////////////////////////////////////////////////////////////////////
bool Core::SDLLoop(SDL_Window* window, IEngine* engine) {
/*
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
	DebugTiming::MainLoopTimer(); // init to zero
	bool fullscreen_hover = true;
	while (!engine->ShouldExit()) {

		qint64 loop_nanos = DebugTiming::MainLoopTimer();
#ifdef NESHNY_EDITOR_VIEWERS
		InfoViewer::LoopTime(loop_nanos);
#endif

		int mouse_dx = 0;
		int mouse_dy = 0;

		//if (is_mouse_relative ^ thing->IsMouseRelative()) {
		//	is_mouse_relative = !is_mouse_relative;
		//	SDL_SetRelativeMouseMode(is_mouse_relative ? SDL_TRUE : SDL_FALSE);
		//}

		if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)) {
			unfocus_timeout = 2;
		}

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			if (unfocus_timeout > 0) {
				unfocus_timeout--;
				continue;
}

			ImGui_ImplSDL2_ProcessEvent(&event);
			if ((event.type == SDL_QUIT) || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))) {
				engine->ExitSignal();
			}
			else if (event.type == SDL_MOUSEMOTION) {
				mouse_dx = event.motion.xrel;
				mouse_dy = event.motion.yrel;
				engine->MouseMove(Vec2(mouse_dx, mouse_dy), !fullscreen_hover);
			}
			else if (event.type == SDL_KEYDOWN) {
				engine->Key(event.key.keysym.sym, true);
			}
			else if (event.type == SDL_KEYUP) {
				engine->Key(event.key.keysym.sym, false);
			}
		}

		SDL_GetWindowSize(window, &width, &height);

		qint64 nanos = frame_timer.nsecsElapsed();
		frame_timer.restart();
		if (engine->Tick(nanos, m_Ticks)) {
			m_Ticks++;
		}
		engine->ManageResources(Core::Singleton().GetResourceManagementToken(), GetMemoryAllocated(), GetGPUMemoryAllocated());

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
*/
	qDebug() << "Finished";

	return true;
}
#endif

#ifdef QT_LOOP
////////////////////////////////////////////////////////////////////////////////
bool Core::QTLoop(QOpenGLWindow* window, IEngine* engine) {

	if (!LoopInit(engine)) {
		return false;
	}

	QElapsedTimer frame_timer;
	frame_timer.restart();
	const double inv_nano = 1.0 / 1000000000;

	bool fullscreen_hover = true;

	ImGuiIO& io = ImGui::GetIO();
	DebugTiming::MainLoopTimer(); // init to zero
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
		engine->ManageResources(Core::Singleton().GetResourceManagementToken(), GetMemoryAllocated(), GetGPUMemoryAllocated());

		if (io.DeltaTime <= 0.0f) {
			io.DeltaTime = 0.00001f;
		}

		window->makeCurrent();

		QTNewFrame();
		LoopInner(engine, width, height, fullscreen_hover);
		// Finish imgui rendering
		ImGui::End();
		QTRender();

		window->context()->swapBuffers(window->context()->surface());
		window->doneCurrent();
	}
	return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////
void Core::IRenderEditor(void) {

	ImGui::SetCursorPos(ImVec2(10.0, 10.0));
	auto info_label = QString("[%2 FPS] %1 info view###info_view").arg(m_Interface.p_InfoView.p_Visible ? "Hide" : "Show").arg((double)ImGui::GetIO().Framerate, 0, 'f', 2).toLocal8Bit();
	if (ImGui::Button(info_label.constData(), ImVec2(250, 0))) {
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

#ifdef NESHNY_EDITOR_VIEWERS
	InfoViewer::RenderImGui(m_Interface.p_InfoView);
	BufferViewer::Singleton().RenderImGui(m_Interface.p_BufferView);
	ShaderViewer::RenderImGui(m_Interface.p_ShaderView);
	ResourceViewer::RenderImGui(m_Interface.p_ResourceView);
	Scrapbook2D::RenderImGui(m_Interface.p_Scrapbook2D);
	Scrapbook3D::RenderImGui(m_Interface.p_Scrapbook3D);
#endif

#ifdef NESHNY_TESTING
	UnitTester::RenderImGui();
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool Core::IIsBufferEnabled(QString name) {
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

#if defined(NESHNY_GL)
////////////////////////////////////////////////////////////////////////////////
void Core::DispatchMultiple(GLShader* prog, int count, int total_local_groups, bool mem_barrier) {
	glUniform1i(prog->GetUniform("uCount"), count);
	const int max_dispatch = 4 * 4 * 4 * total_local_groups;
	for (int offset = 0; offset < count; offset += max_dispatch) {
		glUniform1i(prog->GetUniform("uOffset"), offset);
		glDispatchCompute(4, 4, 4);
		if (mem_barrier) {
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
GLShader* Core::IGetShader(QString name, QString insertion) {

	if (!m_EmbeddableLoader.has_value()) {
		return nullptr;
	}

	QString lookup_name = name;
	if (!insertion.isNull()) {
		auto hash_val = QCryptographicHash::hash(insertion.toLocal8Bit(), QCryptographicHash::Md5).toHex(0); // security is no concern, only speed and lack of collisions
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
GLShader* Core::IGetComputeShader(QString name, QString insertion) {

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
GLBuffer* Core::IGetBuffer(QString name) {

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
	} else if(name == "Point") {
		new_buffer = new GLBuffer();
		new_buffer->Init(3, GL_POINTS, std::vector<GLfloat>({0.0f, 0.0f, 0.0f}));
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
	} else if(name == "Circle") {

		new_buffer = new GLBuffer();

		constexpr int num_segments = 32;
		constexpr double seg_rads = (2 * PI) / num_segments;

		std::vector<GLfloat> vertices;
		double ang = 0;
		for (int i = 0; i < num_segments; i++) {
			vertices.push_back(cos(ang));
			vertices.push_back(sin(ang));
			ang += seg_rads;
		}
		new_buffer->Init(2, GL_LINE_LOOP, vertices);
	} else if(name == "SquareOutline") {
		new_buffer->Init(2, GL_LINE_LOOP
			,std::vector<GLfloat>({
				-1.0f, -1.0f
				,1.0f, -1.0f
				,1.0f, 1.0f
				,-1.0f, 1.0f
			})
			,std::vector<GLuint>({ 0, 2, 1, 0, 3, 2 }));
	} else if(name == "Cylinder") {

		new_buffer = new GLBuffer();

		constexpr int num_segments = 16;
		constexpr double seg_rads = (2 * PI) / num_segments;

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
		new_buffer->Init(3, GL_LINES, std::vector<GLfloat>({0, 0, 0, 1, 1, 1}));
	} else if(name == "DebugSquare") {
		new_buffer = new GLBuffer();
		new_buffer->Init(2, GL_LINE_LOOP, std::vector<GLfloat>({0, 0, 0, 1, 1, 1, 1, 0 }));
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
#elif defined(NESHNY_WEBGPU)
////////////////////////////////////////////////////////////////////////////////
WebGPUShader* Core::IGetShader(QString name, QString insertion) {

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

	QString wgsl_name = name + ".wgsl";

	WebGPUShader* new_shader = new WebGPUShader();
	m_Shaders.insert_or_assign(lookup_name, new_shader);
	if (!new_shader->Init(m_EmbeddableLoader.value(), wgsl_name, insertion)) {
		for (auto err : new_shader->GetErrors()) {
			qDebug() << err.m_Message;
			printf("COMPILE ERROR: %s\n", (char*)err.m_Message.data());

		}
		m_Interface.p_ShaderView.p_Visible = true;
	}
	return new_shader;
}
#endif

////////////////////////////////////////////////////////////////////////////////
void Core::UnloadAllShaders(void) {

	for (auto it = m_Shaders.begin(); it != m_Shaders.end(); it++) {
		delete it->second;
	}
	m_Shaders.clear();

#ifdef NESHNY_GL
	for (auto it = m_ComputeShaders.begin(); it != m_ComputeShaders.end(); it++) {
		delete it->second;
	}
	m_ComputeShaders.clear();
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Core::UnloadAllResources(void) {
	for (auto& resource : m_Resources) {
		delete resource.second.m_Resource;
	}
	m_Resources.clear();
}

#ifdef NESHNY_GL
////////////////////////////////////////////////////////////////////////////////
int Core::CreateGLContext(void) {
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
bool Core::ActivateGLContext(int index) {
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
void Core::DeleteGLContext(int index) {
#ifdef SDL_h_
	if ((index < 0) || (index >= m_Contexts.size())) {
		return;
	}
	SDL_GL_DeleteContext(m_Contexts[index]);
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Core::OpenGLSync(void) {
	GLsync fenceId = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fenceId, GL_SYNC_FLUSH_COMMANDS_BIT, GLuint64(1000000000)); // 1 second timeout
}
#endif

////////////////////////////////////////////////////////////////////////////////
ResourceManagementToken Core::GetResourceManagementToken(void) {
	return ResourceManagementToken(
		[this](std::vector<ResourceManagementToken::ResourceEntry>& entries) {
			entries.clear();
			entries.reserve(m_Resources.size());
			for (const auto& resource : m_Resources) {
				if (resource.second.m_State == ResourceState::DONE) {
					entries.emplace_back(resource.second.m_Resource, resource.first, resource.second.m_Memory, resource.second.m_GPUMemory, m_Ticks - resource.second.m_LastTickAccessed);
				}
			}
		}
		,[this] (const std::vector<ResourceManagementToken::ResourceEntry>& entries) {
			for (const auto& entry : entries) {
				if (entry.p_FlagForDeletion) {
					auto found = m_Resources.find(entry.p_Id);
					if (found != m_Resources.end()) {
						// better to be consistent and out of date even if internal mem usage for a resource changes
						m_MemoryAllocated -= found->second.m_Memory;
						m_GPUMemoryAllocated -= found->second.m_GPUMemory;
						delete found->second.m_Resource;
						m_Resources.erase(found);
					}
				}
			}
		}
	);
}

////////////////////////////////////////////////////////////////////////////////
void IEngine::ManageResources(ResourceManagementToken token, qint64 allocated_ram, qint64 allocated_gpu_ram) {

	qint64 excess_ram = allocated_ram - m_MaxMemory;
	qint64 excess_gpu_ram = allocated_gpu_ram - m_MaxGPUMemory;
	if ((excess_ram <= 0) && (excess_gpu_ram <= 0)) {
		return;
	}

	// this does a copy of resource info, so avoid calling it every frame - only populate this token when needing to purge memory
	token.Populate();

	const double ram_tick_conversion = 1.0;
	const double ram_mult = (excess_ram > 0) ? ram_tick_conversion : 0.0;
	const double gpu_ram_mult = (excess_gpu_ram > 0) ? ram_tick_conversion : 0.0;

	// assign a score to all resources, higher = better candidate for deletion
	// take into account amount and type of mem as well as age (older is better)
	for (auto& entry : token.p_Entries) {
		double elapsed = log10((double)std::max(10, entry.p_TicksSinceAccess));
		entry.p_Score = (ram_mult * entry.p_Memory + gpu_ram_mult * entry.p_GPUMemory) * elapsed;
	}
	std::sort(token.p_Entries.begin(), token.p_Entries.end(), [](const ResourceManagementToken::ResourceEntry& a, const ResourceManagementToken::ResourceEntry& b) -> bool {
		return a.p_Score > b.p_Score;
	});

	for (auto& entry : token.p_Entries) {
		if ((excess_ram <= 0) && (excess_gpu_ram <= 0)) {
			break;
		}
		excess_ram -= entry.p_Memory;
		excess_gpu_ram -= entry.p_GPUMemory;
		entry.FlagForDeletion();
	}
}

} // namespace Neshny