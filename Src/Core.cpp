////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Core.h"

namespace Neshny {

#define EDITOR_INTERFACE_FILENAME "interface.json"
//#define EDITOR_INTERFACE_FILENAME "interface.bin"

////////////////////////////////////////////////////////////////////////////////
DebugTiming::DebugTiming(const char* label) :
	m_Label(label)
{
	m_Timer = std::chrono::steady_clock::now();
}

////////////////////////////////////////////////////////////////////////////////
DebugTiming::~DebugTiming(void) {
	Finish();
}

////////////////////////////////////////////////////////////////////////////////
void DebugTiming::Finish(void) {
	if (m_Finished) {
		return;
	}
	m_Finished = true;

	TimerNanos nanos = std::chrono::steady_clock::now() - m_Timer;
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
std::vector<std::string> DebugTiming::Report(void) {

	auto& timings = GetTimings();
	int64_t total_nanos = 0;
	for (auto iter = timings.begin(); iter != timings.end(); iter++) {
		total_nanos += iter->p_Nanos;
	}
	std::vector<std::string> result;
	for (auto iter = timings.begin(); iter != timings.end(); iter++) {
		result.push_back(iter->Report(total_nanos));
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
	if (m_Threads.empty()) {
		callback(task());
		return;
	}
	const std::lock_guard<std::mutex> lock(m_Lock);
	m_Tasks.push_back(new Task{ task, callback });
}

////////////////////////////////////////////////////////////////////////////////
void WorkerThreadPool::Sync(void) {
	if (m_Threads.empty()) {
		return;
	}
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

	m_MainThreadId = std::this_thread::get_id();
	m_TotalDurationStart = std::chrono::steady_clock::now();

	bool loaded = Core::LoadJSON(m_Interface, EDITOR_INTERFACE_FILENAME);
	//bool loaded = Core::LoadBinary(m_Interface, EDITOR_INTERFACE_FILENAME);
	if ((m_Interface.p_Version != INTERFACE_SAVE_VERSION) || (!loaded)) {
		m_Interface = InterfaceCore{};
	}

}

////////////////////////////////////////////////////////////////////////////////
Core::~Core(void) {

	m_ResourceThreads.Stop();

	for (auto& resource : m_Resources) {
		delete resource.second.m_Resource;
	}
	m_Resources.clear();
	m_SyncLock.unlock();

	Core::SaveJSON(m_Interface, EDITOR_INTERFACE_FILENAME);
	//Core::SaveBinary(m_Interface, EDITOR_INTERFACE_FILENAME);
}

////////////////////////////////////////////////////////////////////////////////
std::string Core::LoadEmbedded(std::string_view filename, std::string& err_msg) {
	EnsureEmbeddableLoaderInit();
	return (*m_EmbeddableLoader)(filename, err_msg);
}

////////////////////////////////////////////////////////////////////////////////
Token Core::SyncWithMainThread(void) {

#ifdef __EMSCRIPTEN__
	bool ignore = true;
#else
	bool ignore = m_MainThreadId == std::this_thread::get_id();
#endif
	if (ignore) {
		return Token([this]() {}, true);
	}
	m_SyncsRequested.fetch_add(1, std::memory_order_relaxed);
	m_SyncLock.lock();
	return Token([this] () {
		m_SyncsRequested.fetch_add(-1, std::memory_order_relaxed);
		m_SyncLock.unlock();
	}, true);
}

////////////////////////////////////////////////////////////////////////////////
void Core::ILog(std::string_view message, ImVec4 col) {

	const auto now = std::chrono::system_clock::now();
#if defined __APPLE__ || defined __EMSCRIPTEN__
	const auto zoned_now = now; // TODO: update to use zoned time
#else
	const std::chrono::zoned_time zoned_now{ std::chrono::current_zone(), now };
#endif

#if defined __EMSCRIPTEN__
	auto time_str = "_"; // cannot format time yet
#else
	auto time_str = std::format("{:%OH:%OM}:{:%S}", zoned_now, std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()));
#endif

#ifdef __EMSCRIPTEN__
	auto formatted_message = std::format("{}\n", message);
	printf(formatted_message.c_str());
#else
	auto output = std::format("[{}] {} \n", time_str, message);
	m_LogFile.write(output.data(), output.size());
	m_LogFile.flush();
#endif

#ifdef NESHNY_EDITOR_VIEWERS
	LogViewer::Singleton().Log({ time_str, std::string(message), col });
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool Core::LoopInit(IEngine* engine) {

	// m_ResourceThreads started here so you have access to m_Window value
#ifdef __EMSCRIPTEN__
	m_ResourceThreads.Start(0); // could not get multithreading to work with webGPU in emscripten - why?
#else
	m_ResourceThreads.Start(4);
#endif

	m_LogFile.open("Core.log", std::ios::out | std::ios::trunc);
	if (!m_LogFile.is_open()) {
		return false;
	}

	Core::Log("Core initializing...");

	if (!engine->Init()) {
		Core::Log("Could not init engine");
		return false;
	}
	m_Ticks = 0;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void Core::LoopInner(IEngine* engine, int width, int height) {

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

	// open up lock for sync across resource threads
	int requests = m_SyncsRequested.load(std::memory_order_relaxed);
	if(requests > 0) {
		DebugTiming dt0("Core::SyncRequest");
		m_SyncLock.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		m_SyncLock.lock();
	}
}

////////////////////////////////////////////////////////////////////////////////
void Core::LoopFinishImGui(IEngine* engine, int width, int height) {

	ImGui::SetCursorPos(ImVec2(0, 0));
	ImGui::InvisibleButton("##FullScreen", ImVec2(width - 4, height - 4));

	if ((m_FullScreenHover = ImGui::IsItemHovered())) {
		ImGuiIO& io = ImGui::GetIO();
		if (io.MouseWheel != 0.0f) {
			engine->MouseWheel(io.MouseWheel > 0.0f);
		}
		for (int i = 0; i < 3; i++) {
			if (ImGui::IsMouseReleased(i)) {
				engine->MouseButton(i, false);
			} else if (ImGui::IsMouseClicked(i)) {
				engine->MouseButton(i, true);
			}
		}
	}

	ImGui::End();
	ImGui::Render();
}

#ifdef SDL_OPENGL_LOOP
////////////////////////////////////////////////////////////////////////////////
bool Core::SDLLoop(SDL_Window* window, IEngine* engine) {

	m_Window = window;
	if (!LoopInit(engine)) {
		m_SyncLock.lock();
		return false;
	}

	//bool is_mouse_relative = true;
	//SDL_SetRelativeMouseMode(is_mouse_relative ? SDL_TRUE : SDL_FALSE);
	SDL_SetRelativeMouseMode(SDL_FALSE);
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	TimerPoint frame_timer = std::chrono::steady_clock::now();

	int unfocus_timeout = 0;

	// Main loop
	DebugTiming::MainLoopTimer(); // init to zero
	m_SyncLock.lock();
	while (!engine->ShouldExit()) {

		TimerNanos loop_nanos = DebugTiming::MainLoopTimer();
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
				engine->MouseMove(Vec2(mouse_dx, mouse_dy), !m_FullScreenHover);
			} else if (event.type == SDL_KEYDOWN) {
				engine->Key(event.key.keysym.sym, true);
			} else if (event.type == SDL_KEYUP) {
				engine->Key(event.key.keysym.sym, false);
			}
		}

		SDL_GetWindowSize(window, &width, &height);

		TimerPoint curr_now = std::chrono::steady_clock::now();
		TimerSeconds seconds = curr_now - frame_timer;
		frame_timer = curr_now;
		if (engine->Tick(seconds.count(), m_Ticks)) {
			m_Ticks++;
		}
		engine->ManageResources(Core::Singleton().GetResourceManagementToken(), GetMemoryAllocated(), GetGPUMemoryAllocated());

		///////////////////////////////////////////// render engine

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();

		ImGui::NewFrame();
		LoopInner(engine, width, height);
		// Finish imgui rendering
		LoopFinishImGui(engine, width, height);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

		m_ResourceThreads.Sync();
	}

	//delete engine;
	//engine = nullptr;

	Core::Log("Finished");

	return true;
}
#endif

#ifdef SDL_WEBGPU_LOOP

////////////////////////////////////////////////////////////////////////////////
void Core::WebGPUErrorCallback(WGPUErrorType type, char const* message) {

	if (WGPUErrorType_NoError == type) {
		return;
	}
	Core::Log(std::format("WebGPU validation error: {}", message), ImVec4(1.0, 0.25f, 0.25f, 1.0));
}

////////////////////////////////////////////////////////////////////////////////
WGPULimits Core::GetDefaultLimits(void) {
	WGPULimits limits;

	limits.maxTextureDimension1D = 8192;
	limits.maxTextureDimension2D = 8192;
	limits.maxTextureDimension3D = 2048;
	limits.maxTextureArrayLayers = 256;
	limits.maxBindGroups = 4;
	limits.maxDynamicUniformBuffersPerPipelineLayout = 8;
	limits.maxDynamicStorageBuffersPerPipelineLayout = 4;
	limits.maxSampledTexturesPerShaderStage = 16;
	limits.maxSamplersPerShaderStage = 16;
	limits.maxStorageBuffersPerShaderStage = 8;
	limits.maxStorageTexturesPerShaderStage = 4;
	limits.maxUniformBuffersPerShaderStage = 12;
	limits.maxUniformBufferBindingSize = 65536;
	limits.maxStorageBufferBindingSize = 134217728;
	limits.minUniformBufferOffsetAlignment = 256;
	limits.minStorageBufferOffsetAlignment = 256;
	limits.maxVertexBuffers = 8;
	limits.maxVertexAttributes = 16;
	limits.maxVertexBufferArrayStride = 2048;
	limits.maxInterStageShaderComponents = 60;
	limits.maxInterStageShaderVariables = 16;
	limits.maxColorAttachments = 8;
	limits.maxComputeWorkgroupStorageSize = 16384;
	limits.maxComputeInvocationsPerWorkgroup = 256;
	limits.maxComputeWorkgroupSizeX = 256;
	limits.maxComputeWorkgroupSizeY = 256;
	limits.maxComputeWorkgroupSizeZ = 64;
	limits.maxComputeWorkgroupsPerDimension = 65535;
	limits.maxBindingsPerBindGroup = 1000;
	limits.maxBufferSize = 268435456;
	limits.maxColorAttachmentBytesPerSample = 32;

	return limits;
}

////////////////////////////////////////////////////////////////////////////////
void Core::InitWebGPU(WebGPUNativeBackend backend, SDL_Window* window, int width, int height, void* layer) {
#ifdef __EMSCRIPTEN__
	m_Device = emscripten_webgpu_get_device();
	m_Queue = wgpuDeviceGetQueue(m_Device);

	WGPUSurfaceDescriptorFromCanvasHTMLSelector canvDesc = {};
	canvDesc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
	canvDesc.selector = "canvas";

	WGPUSurfaceDescriptor surfDesc = {};
	surfDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canvDesc);

	auto instance = wgpuCreateInstance(nullptr);
	m_Surface = wgpuInstanceCreateSurface(instance, &surfDesc);

	WGPUSupportedLimits supported;
	if (wgpuDeviceGetLimits(m_Device, &supported)) {
		m_Limits = supported.limits;
	} else {
		m_Limits = GetDefaultLimits();
	}
#else

	WGPUInstanceDescriptor instanceDescriptor{};
	instanceDescriptor.features.timedWaitAnyEnable = true;
	instanceDescriptor.nextInChain = nullptr;
	instanceDescriptor.features.nextInChain = nullptr;

	dawn::native::Instance instance(&instanceDescriptor);

	::wgpu::RequestAdapterOptions options = {};
	options.backendType = ::wgpu::BackendType::Null;
	switch (backend) {
		case WebGPUNativeBackend::D3D12: options.backendType = ::wgpu::BackendType::D3D12; break;
		case WebGPUNativeBackend::Metal: options.backendType = ::wgpu::BackendType::Metal; break;
		case WebGPUNativeBackend::Vulkan: options.backendType = ::wgpu::BackendType::Vulkan; break;
		case WebGPUNativeBackend::OpenGL: options.backendType = ::wgpu::BackendType::OpenGL; break;
		case WebGPUNativeBackend::OpenGLES: options.backendType = ::wgpu::BackendType::OpenGLES; break;
	};

	// Get the first adapter for the correct backend
	auto adapters = instance.EnumerateAdapters(&options);
	if (adapters.empty()) {
		throw "No adapters found for this backend";
	}
	dawn::native::Adapter backendAdapter = adapters[0];
	::wgpu::DawnAdapterPropertiesPowerPreference power_props{};
	::wgpu::AdapterProperties adapterProperties{};
	adapterProperties.nextInChain = &power_props;
	backendAdapter.GetProperties(&adapterProperties);

	WGPUSupportedLimits supported;
	supported.nextInChain = nullptr;
	bool limits_success = backendAdapter.GetLimits(&supported);
	//bool limits_success = wgpuAdapterGetLimits(backendAdapter.Get(), &supported);
	if (limits_success) {
		m_Limits = supported.limits;
	} else {
		m_Limits = GetDefaultLimits();
	}
	//bool limits_success = wgpuAdapterGetLimits(backendAdapter.Get(), &supported);

	std::vector<const char*> enableToggleNames;
	std::vector<const char*> disabledToggleNames;
	//for (const std::string& toggle : enableToggles) {
	//	enableToggleNames.push_back(toggle.c_str());
	//}

	//for (const std::string& toggle : disableToggles) {
	//	disabledToggleNames.push_back(toggle.c_str());
	//}
	WGPUDawnTogglesDescriptor toggles;
	toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
	toggles.chain.next = nullptr;
	toggles.enabledToggles = enableToggleNames.data();
	toggles.enabledToggleCount = static_cast<uint32_t>(enableToggleNames.size());
	toggles.disabledToggles = disabledToggleNames.data();
	toggles.disabledToggleCount = static_cast<uint32_t>(disabledToggleNames.size());

	WGPUDeviceDescriptor deviceDesc = {};
	deviceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&toggles);

	WGPURequiredLimits requiredLimits;
	requiredLimits.nextInChain = nullptr;
	requiredLimits.limits = supported.limits;
	deviceDesc.nextInChain = nullptr;
	deviceDesc.requiredFeatures = nullptr;
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.label = nullptr;
	deviceDesc.requiredLimits = &requiredLimits;

	m_Device = backendAdapter.CreateDevice(&deviceDesc);
	DawnProcTable backendProcs = dawn::native::GetProcs();
	dawnProcSetProcs(&backendProcs);

	static auto cCallback = [](WGPUErrorType type, char const* message, void* userdata) -> void {
		Core::Log(std::format("Uncaptured error {}", message), ImVec4(1.0, 0.25f, 0.25f, 1.0));
	};
	wgpuDeviceSetUncapturedErrorCallback(m_Device, cCallback, nullptr);

#ifdef __APPLE__

	std::unique_ptr<wgpu::SurfaceDescriptorFromMetalLayer> surfaceChainedDesc = std::make_unique<wgpu::SurfaceDescriptorFromMetalLayer>();
	surfaceChainedDesc->layer = layer;

#elif __WIN32__

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);

	HWND hwnd = wmInfo.info.win.window;
	std::unique_ptr<wgpu::SurfaceDescriptorFromWindowsHWND> surfaceChainedDesc = std::make_unique<wgpu::SurfaceDescriptorFromWindowsHWND>();
	surfaceChainedDesc->hwnd = hwnd;
	surfaceChainedDesc->hinstance = GetModuleHandle(nullptr);

#endif

	WGPUSurfaceDescriptor surfaceDesc;
	surfaceDesc.label = nullptr;
	surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(surfaceChainedDesc.get());
	m_Surface = backendProcs.instanceCreateSurface(instance.Get(), &surfaceDesc);
	m_Queue = wgpuDeviceGetQueue(m_Device);

	//WGPUSurfaceCapabilities capabilities;
	//bool capabilities_success = wgpuSurfaceGetCapabilities(m_Surface, backendAdapter.Get(), &capabilities) == WGPUStatus_Success;

#endif

	SetResolution(width, height);
	SyncResolution();
}

////////////////////////////////////////////////////////////////////////////////
void Core::SyncResolution(void) {

	if ((m_CurrentWidth == m_RequestedWidth) && (m_CurrentHeight == m_RequestedHeight)) {
		return;
	}

	WGPUSurfaceConfiguration config;
	config.nextInChain = nullptr;
	config.device = m_Device;
	config.format = WGPUTextureFormat_BGRA8Unorm;
	config.width = m_RequestedWidth;
	config.height = m_RequestedHeight;
	config.viewFormatCount = 0;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;
	config.presentMode = WGPUPresentMode_Fifo;
	config.usage = WGPUTextureUsage_RenderAttachment;

	wgpuSurfaceConfigure(m_Surface, &config);

	delete m_DepthTex;
	m_DepthTex = new WebGPUTexture();
	m_DepthTex->InitDepth(m_RequestedWidth, m_RequestedHeight);

	m_CurrentWidth = m_RequestedWidth;
	m_CurrentHeight = m_RequestedHeight;
}

////////////////////////////////////////////////////////////////////////////////
void Core::SDLLoopInner() {
	TimerNanos loop_nanos = DebugTiming::MainLoopTimer();
#ifdef NESHNY_EDITOR_VIEWERS
	InfoViewer::LoopTime(loop_nanos);
#endif

	wgpuDevicePushErrorScope(Core::Singleton().GetWebGPUDevice(), WGPUErrorFilter_Validation);

	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	SDL_Event event;
	while (SDL_PollEvent(&event)) {

		ImGui_ImplSDL2_ProcessEvent(&event);
		if ((event.type == SDL_QUIT) || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_Window))) {
			m_Engine->ExitSignal();
		} else if (event.type == SDL_MOUSEMOTION) {
			m_Engine->MouseMove(Vec2(event.motion.xrel, event.motion.yrel), !m_FullScreenHover);
		} else if (event.type == SDL_KEYDOWN) {
			m_Engine->Key(event.key.keysym.sym, true);
		} else if (event.type == SDL_KEYUP) {
			m_Engine->Key(event.key.keysym.sym, false);
		}
	}

	TimerPoint time_now = std::chrono::steady_clock::now();
	TimerSeconds seconds = time_now - m_FrameTimer;
	m_FrameTimer = time_now;
	if (m_Engine->Tick(seconds.count(), m_Ticks)) {
		m_Ticks++;
	}
	m_Engine->ManageResources(GetResourceManagementToken(), GetMemoryAllocated(), GetGPUMemoryAllocated());

	SyncResolution();
#ifdef NESHNY_WEBGPU

	wgpuDeviceTick(Core::Singleton().GetWebGPUDevice());
	if (m_SurfaceTextureView) {
		wgpuTextureViewRelease(m_SurfaceTextureView); // release textureView
	}

	WGPUSurfaceTexture surface_texture = { nullptr };
	wgpuSurfaceGetCurrentTexture(m_Surface, &surface_texture);
	if (!surface_texture.texture) {
		//LoopFinishImGui(m_Engine, m_CurrentWidth, m_CurrentHeight);
		wgpuDevicePopErrorScope(Core::Singleton().GetWebGPUDevice(), WebGPUErrorCallbackStatic, this);
#ifndef __EMSCRIPTEN__
		wgpuDeviceTick(Core::Singleton().GetWebGPUDevice());
#endif
		return;
	}
#endif
	m_SurfaceTextureView = wgpuTextureCreateView(surface_texture.texture, nullptr);

	///////////////////////////////////////////// render engine

	// Start the Dear ImGui frame
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();
	LoopInner(m_Engine, m_CurrentWidth, m_CurrentHeight);
	LoopFinishImGui(m_Engine, m_CurrentWidth, m_CurrentHeight);

	m_ResourceThreads.Sync();

#ifdef NESHNY_WEBGPU

	WGPURenderPassColorAttachment color_desc = {};
	color_desc.view = m_SurfaceTextureView;
	//color_desc.loadOp = WGPULoadOp_Clear;
	color_desc.loadOp = WGPULoadOp_Load;
	color_desc.storeOp = WGPUStoreOp_Store;
	color_desc.clearValue.r = 0.0f;
	color_desc.clearValue.g = 0.0f;
	color_desc.clearValue.b = 0.0f;
	color_desc.clearValue.a = 1.0f;
#ifndef __EMSCRIPTEN__
	color_desc.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
	WGPURenderPassDepthStencilAttachment depth_desc = {};
	depth_desc.view = m_DepthTex->GetTextureView();
	depth_desc.depthClearValue = 1.0f;
	depth_desc.depthLoadOp = WGPULoadOp_Clear;
	depth_desc.depthStoreOp = WGPUStoreOp_Store;
	depth_desc.depthReadOnly = false;
	depth_desc.stencilClearValue = 0;
	depth_desc.stencilLoadOp = WGPULoadOp_Undefined;
	depth_desc.stencilStoreOp = WGPUStoreOp_Undefined;
	depth_desc.stencilReadOnly = true;

	WGPURenderPassDescriptor render_pass = {};
	render_pass.colorAttachmentCount = 1;
	render_pass.colorAttachments = &color_desc;
	render_pass.depthStencilAttachment = &depth_desc;

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(GetWebGPUDevice(), nullptr); // create encoder
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass);	// create pass

	auto draw_data = ImGui::GetDrawData();
	if (draw_data) {
		ImGui_ImplWGPU_RenderDrawData(draw_data, pass);
	}

	wgpuRenderPassEncoderEnd(pass);
	wgpuRenderPassEncoderRelease(pass);

	WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);				// create commands
	wgpuCommandEncoderRelease(encoder);														// release encoder
	wgpuQueueSubmit(GetWebGPUQueue(), 1, &commands);
	wgpuCommandBufferRelease(commands);														// release commands

#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(m_Surface);
#endif
	wgpuDevicePopErrorScope(Core::Singleton().GetWebGPUDevice(), WebGPUErrorCallbackStatic, this);
#ifndef __EMSCRIPTEN__
	wgpuDeviceTick(Core::Singleton().GetWebGPUDevice());
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool Core::WebGPUSDLLoop(WebGPUNativeBackend backend, SDL_Window* window, IEngine* engine, int width, int height, void* layer) {
	m_Window = window;
	m_Engine = engine;

	InitWebGPU(backend, m_Window, width, height, layer);

	if (!LoopInit(engine)) {
		return false;
	}

	IMGUI_CHECKVERSION();
	auto context = ImGui::CreateContext();
	ImGui::SetCurrentContext(context);

#if __APPLE__
	if (!ImGui_ImplSDL2_InitForMetal(window)) {
		return false;
	}
#elif defined __WIN32__
	if (!ImGui_ImplSDL2_InitForD3D(window)) {
		return false;
	}
#else
	if (!ImGui_ImplSDL2_InitForOther(window)) {
		return false;
	}
#endif

	ImGui_ImplWGPU_InitInfo imgui_info;
	imgui_info.Device = m_Device;
	imgui_info.NumFramesInFlight = 3;
	imgui_info.RenderTargetFormat = WGPUTextureFormat_BGRA8Unorm;
	imgui_info.DepthStencilFormat = WGPUTextureFormat_Depth24Plus;
	ImGui_ImplWGPU_Init(&imgui_info);

	SDL_SetRelativeMouseMode(SDL_FALSE);

	// Main loop
	DebugTiming::MainLoopTimer(); // init to zero
	m_SyncLock.lock();

	m_FrameTimer = std::chrono::steady_clock::now();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop([]() {
		Core::Singleton().SDLLoopInner();
	}, 0, 1);
#else
	while (!engine->ShouldExit()) {
		SDLLoopInner();
	}
#endif
	Core::Log("Finished");

	return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////
void Core::IRenderEditor(void) {

	ImGui::SetCursorPos(ImVec2(10.0, 10.0));

	const char* close_string = "(*)";
	const char* open_string = "( )";

#ifdef NESHNY_EDITOR_VIEWERS
	auto info_label = std::format("{0} [{1:.2f} FPS] Info###info_view", m_Interface.p_InfoView.p_Visible ? close_string : open_string, (double)ImGui::GetIO().Framerate);
	if (ImGui::Button(info_label.c_str(), ImVec2(250, 0))) {
		m_Interface.p_InfoView.p_Visible = !m_Interface.p_InfoView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button((std::format("{} Logs", m_Interface.p_LogView.p_Visible ? close_string : open_string)).c_str())) {
		m_Interface.p_LogView.p_Visible = !m_Interface.p_LogView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button((std::format("{} Buffers", m_Interface.p_BufferView.p_Visible ? close_string : open_string)).c_str())) {
		m_Interface.p_BufferView.p_Visible = !m_Interface.p_BufferView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button((std::format("{} Shaders", m_Interface.p_ShaderView.p_Visible ? close_string : open_string)).c_str())) {
		m_Interface.p_ShaderView.p_Visible = !m_Interface.p_ShaderView.p_Visible;
	}

#ifdef NESHNY_WEBGPU
	ImGui::SameLine();
	if (ImGui::Button((std::format("{} Pipelines", m_Interface.p_PipelineView.p_Visible ? close_string : open_string)).c_str())) {
		m_Interface.p_PipelineView.p_Visible = !m_Interface.p_PipelineView.p_Visible;
	}
#endif

	ImGui::SameLine();
	if (ImGui::Button((std::format("{} Resources", m_Interface.p_ResourceView.p_Visible ? close_string : open_string)).c_str())) {
		m_Interface.p_ResourceView.p_Visible = !m_Interface.p_ResourceView.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button((std::format("{} 2D Scrapbook", m_Interface.p_Scrapbook2D.p_Visible ? close_string : open_string)).c_str())) {
		m_Interface.p_Scrapbook2D.p_Visible = !m_Interface.p_Scrapbook2D.p_Visible;
	}
	ImGui::SameLine();
	if (ImGui::Button((std::format("{} 3D Scrapbook", m_Interface.p_Scrapbook3D.p_Visible ? close_string : open_string)).c_str())) {
		m_Interface.p_Scrapbook3D.p_Visible = !m_Interface.p_Scrapbook3D.p_Visible;
	}
	ImGui::SameLine();
#endif
	if (ImGui::Button((std::format("{} ImGUI Demo", m_Interface.p_ShowImGuiDemo ? close_string : open_string)).c_str())) {
		m_Interface.p_ShowImGuiDemo = !m_Interface.p_ShowImGuiDemo;
	}
	if (m_Interface.p_ShowImGuiDemo) {
		ImGui::ShowDemoWindow();
	}

#ifdef NESHNY_EDITOR_VIEWERS
	InfoViewer::RenderImGui(m_Interface.p_InfoView);
	LogViewer::Singleton().RenderImGui(m_Interface.p_LogView);
	BufferViewer::Singleton().RenderImGui(m_Interface.p_BufferView);
	ShaderViewer::RenderImGui(m_Interface.p_ShaderView);
#ifdef NESHNY_WEBGPU
	PipelineViewer::RenderImGui(m_Interface.p_PipelineView);
#endif
	ResourceViewer::RenderImGui(m_Interface.p_ResourceView);
	Scrapbook2D::RenderImGui(m_Interface.p_Scrapbook2D);
	Scrapbook3D::RenderImGui(m_Interface.p_Scrapbook3D);

	Scrapbook2D::Clear();
	Scrapbook3D::Clear();
#endif

#ifdef NESHNY_TESTING
	UnitTester::RenderImGui();
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool Core::IIsBufferEnabled(std::string_view name) {
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
void Core::EnsureEmbeddableLoaderInit(void) {
	if (m_EmbeddableLoader.has_value()) {
		return;
	}
	m_EmbeddableLoader = [this] (std::string_view path, std::string& err_msg) -> std::string {
		std::string path_str(path);
		auto found = m_EmbeddedFiles.find(path_str);
		if (found != m_EmbeddedFiles.end()) {
			return std::string((char*)found->second.data(), (int)found->second.size());
		}

		for (auto prefix : m_ResourceDirs) {
			std::string filename = std::format("{}/{}", prefix, path);
			std::ifstream file(filename, std::ios::in | std::ios::binary);
			if (file) {
				std::ostringstream data_stream;
				data_stream << file.rdbuf();
				std::string data = data_stream.str();
				file.close();
				return data;
			}
		}
		err_msg = "Could not open file";
		return std::string();
	};
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
GLShader* Core::IGetShader(std::string_view name, std::string_view insertion) {

	EnsureEmbeddableLoaderInit();

	ShaderGroup* found_group = nullptr;
	for (auto& group : m_ShaderGroups) {
		if (group.p_Name == name) {
			for (const auto& instance : group.p_Instances) {
				if (instance.m_StartInsert == insertion) {
					return instance.p_Shader;
				}
			}
			found_group = &group;
			break;
		}
	}
	if (!found_group) {
		m_ShaderGroups.push_back({ std::string(name), {} });
		found_group = &m_ShaderGroups.back();
	}

	std::string name_str(name);
	std::string vertex_name = name_str + ".vert", fragment_name = name_str + ".frag", geometry_name = "";

	GLShader* new_shader = new GLShader();
	found_group->p_Instances.push_back({ new_shader, std::string(insertion) });
	std::string err_msg = "";
	if (!new_shader->Init(err_msg, m_EmbeddableLoader.value(), vertex_name, fragment_name, geometry_name, insertion)) {
		Core::Log(err_msg);
		m_Interface.p_ShaderView.p_Visible = true;
	}
	return new_shader;
}

////////////////////////////////////////////////////////////////////////////////
GLShader* Core::IGetComputeShader(std::string_view name, std::string_view insertion) {

	EnsureEmbeddableLoaderInit();

	ShaderGroup* found_group = nullptr;
	for (auto& group : m_ComputeShaderGroups) {
		if (group.p_Name == name) {
			for (const auto& instance : group.p_Instances) {
				if (instance.m_StartInsert == insertion) {
					return instance.p_Shader;
				}
			}
			found_group = &group;
			break;
		}
	}
	if (!found_group) {
		m_ComputeShaderGroups.push_back({ std::string(name), {} });
		found_group = &m_ComputeShaderGroups.back();
	}

	std::string shader_name = std::format("{}.comp", name);

	GLShader* new_shader = new GLShader();
	found_group->p_Instances.push_back({ new_shader, std::string(insertion) });
	std::string err_msg;
	if (!new_shader->InitCompute(err_msg, m_EmbeddableLoader.value(), shader_name, insertion)) {
		m_Interface.p_ShaderView.p_Visible = true;
		Core::Log(err_msg);
	}
	return new_shader;
}

////////////////////////////////////////////////////////////////////////////////
GLBuffer* Core::IGetBuffer(std::string name) {

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
WebGPUShader* Core::IGetShader(std::string_view name, std::string_view start_insert, std::string_view end_insert) {

	EnsureEmbeddableLoaderInit();

	ShaderGroup* found_group = nullptr;
	for (auto& group : m_ShaderGroups) {
		if (group.p_Name == name) {
			for (const auto& instance : group.p_Instances) {
				if ((instance.m_StartInsert == start_insert) && (instance.m_EndInsert == end_insert)) {
					return instance.p_Shader;
				}
			}
			found_group = &group;
			break;
		}
	}
	if (!found_group) {
		m_ShaderGroups.push_back({ std::string(name), {} });
		found_group = &m_ShaderGroups.back();
	}
	WebGPUShader* new_shader = new WebGPUShader();
	found_group->p_Instances.push_back({ new_shader, std::string(start_insert), std::string(end_insert) });

	std::string wgsl_name = std::format("{}.wgsl", name);
	if (!new_shader->Init(m_EmbeddableLoader.value(), wgsl_name, start_insert, end_insert)) {
		for (auto err : new_shader->GetErrors()) {
			Core::Log(std::format("COMPILE ERROR on line {}: {}", err.m_LineNum, err.m_Message));
		}
		m_Interface.p_ShaderView.p_Visible = true;
	}
	return new_shader;
}

////////////////////////////////////////////////////////////////////////////////
WebGPURenderBuffer* Core::IGetBuffer(std::string name) {

	auto found = m_Buffers.find(name);
	if (found != m_Buffers.end()) {
		return found->second;
	}
	m_Buffers.insert_or_assign(name, nullptr);

	int vertex_counter = 0;
	WebGPURenderBuffer* new_buffer = new WebGPURenderBuffer();
	if(name == "Square") {
		std::vector<float> vertices = {
			-1.0, -1.0
			,1.0, 1.0
			,1.0, -1.0
			,-1.0, 1.0
		};
		new_buffer->Init(WGPUVertexFormat_Float32x2, WGPUPrimitiveTopology_TriangleList, (unsigned char*)vertices.data(), (int)vertices.size() * sizeof(float), {
			0, 1, 2,
			0, 3, 1
		});
	} else if(name == "Cube") {

		const float S = 0.5;
		std::vector<float> vertices = {
			-S, -S, -S
			,-S, S, -S
			,S, S, -S
			,S, -S, -S
			,-S, -S, S
			,-S, S, S
			,S, S, S
			,S, -S, S
		};
		new_buffer->Init(WGPUVertexFormat_Float32x3, WGPUPrimitiveTopology_TriangleList, (unsigned char*)vertices.data(), (int)vertices.size() * sizeof(float), {
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
		});
	} else if(name == "Circle") {
		
		constexpr int num_segments = 32;
		constexpr double seg_rads = (2 * PI) / num_segments;

		std::vector<float> vertices;
		double ang = 0;
		for (int i = 0; i <= num_segments; i++) {
			vertices.push_back(cos(ang));
			vertices.push_back(sin(ang));
			ang += seg_rads;
		}
		new_buffer->Init(WGPUVertexFormat_Float32x2, WGPUPrimitiveTopology_LineStrip, (unsigned char*)vertices.data(), (int)vertices.size() * sizeof(float));
	} else if(name == "SquareOutline") {

		std::vector<float> vertices = {
			-1.0, -1.0
			,1.0, -1.0
			,1.0, 1.0
			,-1.0, 1.0
			,-1.0, -1.0
		};
		new_buffer->Init(WGPUVertexFormat_Float32x2, WGPUPrimitiveTopology_LineStrip, (unsigned char*)vertices.data(), (int)vertices.size() * sizeof(float));
#pragma msg("why does this fail with indices? combo of WGPUPrimitiveTopology_LineStrip and indices causes issues")
		//new_buffer->Init(WGPUVertexFormat_Float32x2, WGPUPrimitiveTopology_LineStrip, (unsigned char*)vertices.data(), (int)vertices.size() * sizeof(float), {
		//	0, 1, 2, 3, 0
		//});
	} else if(name == "Cylinder") {
		/*
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
		*/
	} else if(name == "DebugSquare") {
		//new_buffer = new GLBuffer();
		//new_buffer->Init(2, GL_LINE_LOOP, std::vector<GLfloat>({0, 0, 0, 1, 1, 1, 1, 0 }));
	} else if(name == "DebugTriangle") {
		//new_buffer = new GLBuffer();
		//new_buffer->Init(2, GL_TRIANGLES, std::vector<GLfloat>({0, 0, 1, 0, 0, 1}));
	} else if(name == "Mess") {

		//std::vector<GLfloat> vertices;
		//for (int i = 0; i < 3 * 16; i++) {
		//	vertices.push_back(Random(-1, 1));
		//}
		//new_buffer = new GLBuffer();
		//new_buffer->Init(3, GL_TRIANGLE_STRIP, vertices);
	} else if(name == "TriangleBlob") {

		//new_buffer = new GLBuffer();

		//new_buffer->Init(3, GL_TRIANGLES
		//	, std::vector<GLfloat>({
		//		1.0f, 0.0f, 0.0f
		//		,-1.0f, 0.3f, 0.0f
		//		,-1.0f, -0.3f, -0.3f
		//		,-1.0f, -0.3f, 0.3f
		//		})
		//	, std::vector<GLuint>({ 0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 3, 2 }));

	} else {
		delete new_buffer;
		return nullptr;
	}
	m_Buffers.insert_or_assign(name, new_buffer);
	return new_buffer;
}

////////////////////////////////////////////////////////////////////////////////
WebGPUSampler* Core::IGetSampler(WGPUAddressMode mode, WGPUFilterMode filter, bool linear_mipmaps, unsigned int max_anisotropy) {
	for (auto existing : m_Samplers) {
		if (
			(mode == existing->GetMode()) &&
			(filter == existing->GetFilter()) &&
			(linear_mipmaps == existing->GetLinearMipMaps()) &&
			(max_anisotropy == existing->GetMaxAnisotropy())
		) {
			return existing;
		}
	}
	WebGPUSampler* new_sampler = new WebGPUSampler(mode, filter, linear_mipmaps, max_anisotropy);
	m_Samplers.push_back(new_sampler);
	return new_sampler;
}

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(void, await_device_queue, (), {
	const device = webgpu["preinitializedWebGPUDevice"];
	await device.queue.onSubmittedWorkDone();
});
#endif

////////////////////////////////////////////////////////////////////////////////
void Core::WaitForCommandsToFinish(void) {
	auto& self = Core::Singleton();

	DebugTiming debug_timing("Core::WaitForCommandsToFinish");

#ifndef __EMSCRIPTEN__
	std::optional<WGPUQueueWorkDoneStatus> status_result;
	wgpuQueueOnSubmittedWorkDone(self.m_Queue, [](WGPUQueueWorkDoneStatus status, void* user_data) {
		auto info = (std::optional<WGPUQueueWorkDoneStatus>*)user_data;
		*info = status;
		}, &status_result);

	while (!status_result.has_value()) {
		wgpuDeviceTick(Core::Singleton().GetWebGPUDevice());
	}
#else
	await_device_queue();
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Core::UnloadPipeline(std::string_view identifier) {
	for (auto iter = m_PreparedPipelines.begin(); iter != m_PreparedPipelines.end(); iter++) {
		if (iter->get()->GetIdentifier() == identifier) {
			m_PreparedPipelines.erase(iter); // not a big deal that it's linear time
			return;
		}
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
void Core::UnloadAllShaders(void) {

	for (auto& group : m_ShaderGroups) {
		for (auto& instance : group.p_Instances) {
			delete instance.p_Shader;
		}
	}
	m_ShaderGroups.clear();

#if defined NESHNY_GL
	for (auto& group : m_ComputeShaderGroups) {
		for (auto& instance : group.p_Instances) {
			delete instance.p_Shader;
		}
	}
	m_ComputeShaderGroups.clear();
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
void IEngine::ManageResources(ResourceManagementToken token, uint64_t allocated_ram, uint64_t allocated_gpu_ram) {

	int64_t excess_ram = (int64_t)allocated_ram - (int64_t)m_MaxMemory;
	int64_t excess_gpu_ram = (int64_t)allocated_gpu_ram - (int64_t)m_MaxGPUMemory;
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