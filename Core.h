////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef QT_LOOP
	class QOpenGLWindow;
#endif

namespace Neshny {

struct Camera2D {

	inline Matrix4 Get4x4Matrix(int width, int height) const {
		float aspect = (float)height / width;
		float view_rad_x = p_Zoom;
		float view_rad_y = p_Zoom * aspect;
		Matrix4 viewMatrix = Matrix4::Ortho(p_Pos.x - view_rad_x, p_Pos.x + view_rad_x, p_Pos.y - view_rad_y, p_Pos.y + view_rad_y, 1.0, -1.0);
		if (p_RotationAngle != 0.0) {
			viewMatrix *= Matrix4::Rotation(p_RotationAngle, Vec3(0.0, 0.0, 1.0));
		}
		return viewMatrix;
	}

	// TODO: get 3x3 matrix here

	inline void Pan(int viewport_width, int delta_pixels_x, int delta_pixels_y) {
		double pan_mult = 2.0 * p_Zoom / float(viewport_width);
		p_Pos.x -= pan_mult * delta_pixels_x;
#if defined(NESHNY_GL)
		p_Pos.y -= pan_mult * delta_pixels_y;
#else
		p_Pos.y += pan_mult * delta_pixels_y;
#endif
	}

	inline Vec2 ScreenToWorld(Vec2 pos, int width, int height) {
		float aspect = (float)height / width;
#if defined(NESHNY_GL)
		double fx = pos.x / width - 0.5, fy = pos.y / height - 0.5;
#else
		double fx = pos.x / width - 0.5, fy = 0.5 - pos.y / height;
#endif
		// TODO: account for rotation here
		return Vec2(
			fx * p_Zoom * 2.0 + p_Pos.x
			,fy * p_Zoom * aspect * 2.0 + p_Pos.y
		);
	}

	inline Vec2 WorldToScreen(Vec2 pos, int width, int height) {
		auto vp = Get4x4Matrix(width, height);
		Vec4 res = vp * Vec4(pos.x, pos.y, 0.0, 1.0);
		double inv_w = 1.0 / res.w;
#if defined(NESHNY_GL)
		return Vec2((res.x * inv_w * 0.5 + 0.5) * width, (res.y * inv_w * 0.5 + 0.5) * height);
#else
		return Vec2((res.x * inv_w * 0.5 + 0.5) * width, (res.y * inv_w * -0.5 + 0.5) * height);
#endif
	}

	inline void Zoom(double new_zoom, Vec2 mouse_world_pos, int width, int height) {

		// TODO: more efficient method that also takes rotation into account?
		Vec2 screen = WorldToScreen(mouse_world_pos, width, height);

		p_Zoom = new_zoom;

		Vec2 new_world = ScreenToWorld(screen, width, height);
		Vec2 delta = mouse_world_pos - new_world;
		p_Pos += delta;
	}

	Vec2		p_Pos = Vec2(0, 0);
	double		p_Zoom = 10.0;
	float		p_RotationAngle = 0.0f;
};

struct Camera3DOrbit {

	Matrix4 GetViewMatrix(void) const {
		Matrix4 viewMatrix = Matrix4::Identity();
		viewMatrix.Translate(Vec3(0, 0, -p_Zoom));
		viewMatrix *= Matrix4::Rotation(p_VerticalDegrees, Vec3(1, 0, 0));
		viewMatrix *= Matrix4::Rotation(p_HorizontalDegrees, Vec3(0, 1, 0));
		viewMatrix.Translate(p_Pos * -1);
		return viewMatrix;
	}
	Matrix4 GetViewPerspectiveMatrix(int width, int height) const {
		Matrix4 perspectiveMatrix = Matrix4::Perspective(p_FovDegrees, (float)width / height, p_NearPlane, p_FarPlane);
		return perspectiveMatrix * GetViewMatrix();
	}
	Vec3 GetCamRealPos(void) const {
		return (GetViewMatrix().Inverse() * Vec4(0, 0, 0, 1)).ToVec3();
	}

	Vec3		p_Pos;
	double		p_Zoom = 100.0;
	float		p_HorizontalDegrees = 0.0f;
	float		p_VerticalDegrees = 0.0f;
	float		p_FovDegrees = 60.0f;
	float		p_NearPlane = 0.1f;
	float		p_FarPlane = 1000.0f;
};

struct Camera3DFPS {
	Matrix4 GetViewMatrix(void) const {
		Matrix4 viewMatrix = Matrix4::Identity();
		viewMatrix *= Matrix4::Rotation(p_Direction);
		viewMatrix.Translate(p_Pos * -1);
		return viewMatrix;
	}
	Matrix4 GetViewPerspectiveMatrix(int width, int height) const {
		Matrix4 perspectiveMatrix = Matrix4::Perspective(p_FovDegrees, (float)width / height, p_NearPlane, p_FarPlane);
		return perspectiveMatrix * GetViewMatrix();
	}
	void GetDirections(Vec3* forward = nullptr, Vec3* up = nullptr, Vec3* side = nullptr) {
		auto inv = p_Direction.Inverse();
		if (forward) { *forward = inv * Vec3(0, 0, -1); forward->Normalize(); }
		if (up) { *up = inv * Vec3(0, -1, 0); up->Normalize(); }
		if (side) { *side = inv * Vec3(-1, 0, 0); side->Normalize(); }
	}
	Vec3		p_Pos;
	Quat		p_Direction = Quat(0.0, 0.0, 0.0, 1.0);
	float		p_FovDegrees = 60;
	float		p_NearPlane = 0.1f;
	float		p_FarPlane = 1000.0f;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class DebugTiming {
public:

	struct TimingInfo {
		TimingInfo(const char* label) : p_Label(label) {}
		TimingInfo(const char* label, qint64 nanos) : p_Label(label) { Add(nanos); }
		void Add(qint64 nanos) {
			p_Nanos += nanos;
			p_RecentNanos += nanos;
			p_MinNanos = (p_MinNanos < 0) ? nanos : std::min(p_MinNanos, nanos);
			p_MaxNanos = std::max(p_MaxNanos, nanos);
			p_NumCalls++;
			p_RecentNumCalls++;
			if (p_RecentNumCalls >= 16) {
				double av_secs = ((double)p_RecentNanos * NANO_CONVERT) / (double)p_RecentNumCalls;
				p_RecentNanos = 0;
				p_RecentNumCalls = 0;
				const double roll_frac = 0.95;
				p_RollingAvSeconds = p_RollingAvSeconds ? p_RollingAvSeconds * roll_frac + av_secs * (1.0 - roll_frac) : av_secs;
			}
		}
		QString Report(qint64 total_global_nanos) {
			double total_av_secs = ((double)p_Nanos * NANO_CONVERT) / (double)p_NumCalls;
			double percent = 100.0 * (double)p_Nanos / (double)total_global_nanos;
			double max_secs = (double)p_MaxNanos * NANO_CONVERT;
			return QString("%1: %2 sec [%3 sec av %4 calls, %5 %%] max %6").arg(p_Label).arg(p_RollingAvSeconds, 0, 'f', 9).arg(total_av_secs, 0, 'f', 9).arg(p_NumCalls).arg(percent, 0, 'f', 6).arg(max_secs, 0, 'f', 9);
		}
		const char* p_Label;
		qint64 p_Nanos = 0;
		qint64 p_NumCalls = 0;
		qint64 p_MinNanos = -1;
		qint64 p_MaxNanos = 0;

		qint64 p_RecentNanos = 0;
		qint64 p_RecentNumCalls = 0;

		double p_RollingAvSeconds = 0;
	};

	DebugTiming(const char* label);
	~DebugTiming(void);

	void								Finish			( void );
	static QStringList					Report			( void );

	static std::vector<TimingInfo>&		GetTimings		( void ) { static std::vector<TimingInfo> timings = {}; return timings; }

	static qint64						MainLoopTimer	( void ) { static QElapsedTimer timer; qint64 time = timer.nsecsElapsed(); timer.restart(); return time; }

private:

	QElapsedTimer		m_Timer;
	const char*			m_Label;
	bool				m_Finished = false;
};

#define INTERFACE_SAVE_VERSION 1 // todo: start incrementing this on release
struct InterfaceCollapsible {
	QString		p_Name;
	bool		p_Open = false;
	bool		p_Enabled = true;
};

struct InterfaceInfoViewer {
	bool	p_Visible = false;
};

struct InterfaceLogViewer {
	bool		p_Visible = false;
	bool		p_AutoScroll = true;
	std::string	p_Search = "";
};

struct InterfaceBufferViewer {
	bool	p_Visible = false;
	bool	p_AllEnabled = false;
	int		p_MaxFrames = 100;
	int		p_TimeSlider = 0; // do not save this
	std::vector<InterfaceCollapsible> p_Items;
};

struct InterfaceShaderViewer {
	bool			p_Visible = false;
	bool			p_ShowPreprocessed = true;
	std::string		p_Search = "";
	std::vector<InterfaceCollapsible> p_Items;
};

struct InterfaceResourceViewer {
	bool			p_Visible = false;
};

struct InterfaceScrapbook2D {
	bool			p_Visible = false;
	Camera2D		p_Cam = Camera2D{};
};

struct InterfaceScrapbook3D {
	bool			p_Visible = false;
	Camera3DOrbit	p_Cam = Camera3DOrbit{ Vec3(), 100, 30, 30 };
};

struct InterfaceCore {
	int						p_Version = INTERFACE_SAVE_VERSION;
	bool					p_ShowImGuiDemo = false;
	InterfaceInfoViewer		p_InfoView;
	InterfaceLogViewer		p_LogView;
	InterfaceBufferViewer	p_BufferView;
	InterfaceShaderViewer	p_ShaderView;
	InterfaceResourceViewer	p_ResourceView;
	InterfaceScrapbook2D	p_Scrapbook2D;
	InterfaceScrapbook3D	p_Scrapbook3D;
};

} // namespace Neshny

namespace meta {
	template<> inline auto registerMembers<Neshny::InterfaceCore>() {
		return members(
			member("Version", &Neshny::InterfaceCore::p_Version)
			,member("ShowImGuiDemo", &Neshny::InterfaceCore::p_ShowImGuiDemo)
			,member("InfoView", &Neshny::InterfaceCore::p_InfoView)
			,member("LogView", &Neshny::InterfaceCore::p_LogView)
			,member("BufferView", &Neshny::InterfaceCore::p_BufferView)
			,member("ShaderView", &Neshny::InterfaceCore::p_ShaderView)
			,member("ResourceView", &Neshny::InterfaceCore::p_ResourceView)
			,member("Scrapbook2D", &Neshny::InterfaceCore::p_Scrapbook2D)
			,member("Scrapbook3D", &Neshny::InterfaceCore::p_Scrapbook3D)
		);
	}
	template<> inline auto registerMembers<Neshny::InterfaceInfoViewer>() {
		return members(
			member("Visible", &Neshny::InterfaceInfoViewer::p_Visible)
		);
	}
	template<> inline auto registerMembers<Neshny::InterfaceLogViewer>() {
		return members(
			member("Visible", &Neshny::InterfaceLogViewer::p_Visible)
			,member("AutoScroll", &Neshny::InterfaceLogViewer::p_AutoScroll)
			,member("Search", &Neshny::InterfaceLogViewer::p_Search)
		);
	}
	template<> inline auto registerMembers<Neshny::InterfaceBufferViewer>() {
		return members(
			member("Visible", &Neshny::InterfaceBufferViewer::p_Visible)
			,member("AllEnabled", &Neshny::InterfaceBufferViewer::p_AllEnabled)
			,member("MaxFrames", &Neshny::InterfaceBufferViewer::p_MaxFrames)
			,member("Items", &Neshny::InterfaceBufferViewer::p_Items)
		);
	}
	template<> inline auto registerMembers<Neshny::InterfaceShaderViewer>() {
		return members(
			member("Visible", &Neshny::InterfaceShaderViewer::p_Visible)
			,member("ShowPreprocessed", &Neshny::InterfaceShaderViewer::p_ShowPreprocessed)
			,member("Search", &Neshny::InterfaceShaderViewer::p_Search)
			,member("Items", &Neshny::InterfaceShaderViewer::p_Items)
		);
	}

	template<> inline auto registerMembers<Neshny::InterfaceResourceViewer>() {
		return members(
			member("Visible", &Neshny::InterfaceResourceViewer::p_Visible)
		);
	}
	template<> inline auto registerMembers<Neshny::InterfaceScrapbook2D>() {
		return members(
			member("Visible", &Neshny::InterfaceScrapbook2D::p_Visible)
			,member("Cam", &Neshny::InterfaceScrapbook2D::p_Cam)
		);
	}
	template<> inline auto registerMembers<Neshny::InterfaceScrapbook3D>() {
		return members(
			member("Visible", &Neshny::InterfaceScrapbook3D::p_Visible)
			,member("Cam", &Neshny::InterfaceScrapbook3D::p_Cam)
		);
	}

	template<> inline auto registerMembers<Neshny::InterfaceCollapsible>() {
		return members(
			member("Name", &Neshny::InterfaceCollapsible::p_Name)
			,member("Open", &Neshny::InterfaceCollapsible::p_Open)
			,member("Enabled", &Neshny::InterfaceCollapsible::p_Enabled)
		);
	}

	template<> inline auto registerMembers<Neshny::Camera3DOrbit>() {
		return members(
			member("Pos", &Neshny::Camera3DOrbit::p_Pos)
			,member("Zoom", &Neshny::Camera3DOrbit::p_Zoom)
			,member("HorizontalDegrees", &Neshny::Camera3DOrbit::p_HorizontalDegrees)
			,member("VerticalDegrees", &Neshny::Camera3DOrbit::p_VerticalDegrees)
			,member("FovDegrees", &Neshny::Camera3DOrbit::p_FovDegrees)
			,member("NearPlane", &Neshny::Camera3DOrbit::p_NearPlane)
			,member("FarPlane", &Neshny::Camera3DOrbit::p_FarPlane)
		);
	}
	template<> inline auto registerMembers<Neshny::Camera2D>() {
		return members(
			member("Pos", &Neshny::Camera2D::p_Pos)
			,member("Zoom", &Neshny::Camera2D::p_Zoom)
			,member("RotationAngle", &Neshny::Camera2D::p_RotationAngle)
		);
	}
}

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WorkerThreadPool {
public:

					WorkerThreadPool	( void ) {}
	void			Start				( int thread_count = 1 );
	void			Stop				( void );

	void			DoTask				( std::function<void*()> task, std::function<void(void* result)> callback );
	void			Sync				( void );

private:

	struct ThreadInfo {
		std::thread*	m_Thread;
#ifdef NESHNY_GL
		int				m_GLContext = -1;
#endif
		bool			m_StopRequested = false;
	};

	struct Task {
		std::function<void*()>				p_Task;
		std::function<void(void* result)>	p_Callback;
		void*								p_Result = nullptr;
	};

	std::mutex				m_Lock;
	std::vector<ThreadInfo>	m_Threads;
	std::deque<Task*>		m_Tasks;
	std::vector<Task*>		m_FinishedTasks;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Resource {
public:
	virtual				~Resource				( void ) {}
	virtual qint64		GetMemoryEstimate		( void ) const = 0;
	virtual qint64		GetGPUMemoryEstimate	( void ) const = 0;
protected:
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct ResourceManagementToken {
	struct ResourceEntry {
		ResourceEntry(Resource const* resource, QString id, qint64 memory, qint64 gpu_memory, int ticks_since_access) : p_Resource(resource), p_Id(id), p_Memory(memory), p_GPUMemory(gpu_memory), p_TicksSinceAccess(ticks_since_access), p_FlagForDeletion(false), p_Score(0.0) {}
		void FlagForDeletion(void) { p_FlagForDeletion = true; }
		Resource const*		p_Resource;
		QString				p_Id;
		qint64				p_Memory;
		qint64				p_GPUMemory;
		int					p_TicksSinceAccess;
		bool				p_FlagForDeletion;
		double				p_Score;
	};

	ResourceManagementToken(std::function<void(std::vector<ResourceManagementToken::ResourceEntry>&)> population, std::function<void(const std::vector<ResourceManagementToken::ResourceEntry>&)> destruction) : p_PopulateFunc(population), p_DestructFunc(destruction) {}
	~ResourceManagementToken(void) { p_DestructFunc(p_Entries); p_Entries = {}; }

	inline void				Populate	( void ) { p_PopulateFunc(p_Entries); }

	std::vector<ResourceEntry>	p_Entries;
private:
	std::function<void(std::vector<ResourceManagementToken::ResourceEntry>&)>		p_PopulateFunc;
	std::function<void(const std::vector<ResourceManagementToken::ResourceEntry>&)>	p_DestructFunc;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class IEngine {
public:
	virtual							~IEngine() {}

	virtual void					MouseButton	( int button, bool is_down ) = 0;
	virtual void					MouseMove	( Vec2 delta, bool occluded ) = 0;
	virtual void					MouseWheel	( bool up ) = 0;
	virtual void					Key			( int key, bool is_down ) = 0;

	virtual bool					Init		( void ) = 0;
	virtual void					ExitSignal	( void ) = 0;
	virtual bool					ShouldExit	( void ) = 0;
	virtual bool					Tick		( qint64 delta_nanoseconds, int tick ) = 0;
	virtual void					Render		( int width, int height ) = 0;

	// replace with your own custom resource/memory management system if required
	virtual void					ManageResources	( ResourceManagementToken token, qint64 allocated_ram, qint64 allocated_gpu_ram );

protected:
	qint64							m_MaxMemory = 1024ll * 1024ll * 1024ll * 2ll; // 2 gigs
	qint64							m_MaxGPUMemory = 1024ll * 1024ll * 1024ll * 1ll; // 1 gig
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Core {

public:

	enum class ResourceState {
		PENDING
		,DONE
		,IN_ERROR
	};

	struct ResourceContainer {
		ResourceState	m_State = ResourceState::PENDING;
		Resource*		m_Resource = nullptr;
		QString			m_Error;
		qint64			m_Memory = 0;
		qint64			m_GPUMemory = 0;
		int				m_LastTickAccessed = 0;
	};

	template<class T, typename = typename std::enable_if<std::is_base_of<Resource, T>::value>::type>
	struct ResourceResult {
		ResourceResult(const ResourceContainer& container) : m_State(container.m_State), m_Resource((T*)container.m_Resource), m_Error(container.m_Error) {}
		T* operator->() const { return m_Resource; }
		T* Ptr() const { return m_Resource; }
		bool IsValid(void) const { return m_State == ResourceState::DONE; }
		ResourceState	m_State = ResourceState::PENDING;
		T*				m_Resource = nullptr;
		QString			m_Error;
	};

	inline static Core&					Singleton					( void ) { static Core core; return core; }

	void								SetEmbeddableFileLoader		( std::function<QByteArray(QString, QString&)> loader ) { m_EmbeddableLoader = loader; }
	QByteArray							LoadEmbedded				( QString filename, QString& err_msg );

	inline void							SetTicksOverride			( int ticks ) { m_Ticks = ticks; }
	Token								SyncWithMainThread			( void );

	bool								LoopInit					( IEngine* engine );
	void								LoopInner					( IEngine* engine, int width, int height );
	void								LoopFinishImGui				( IEngine* engine, int width, int height );

#ifdef SDL_OPENGL_LOOP
	bool								SDLLoop						( SDL_Window* window, IEngine* engine );
#endif
#ifdef QT_LOOP
	bool								QTLoop						( QOpenGLWindow* window, IEngine* engine );
#endif
#ifdef NESHNY_WEBGPU
	enum class WebGPUNativeBackend {
		D3D12, Metal, Vulkan, OpenGL, OpenGLES
	};
	void								SDLLoopInner				( void );
	bool								WebGPUSDLLoop				( WebGPUNativeBackend backend, SDL_Window* window, IEngine* engine, int width, int height, void* layer = nullptr );
	void								SetResolution				( int width, int height ) { m_RequestedWidth = width; m_RequestedHeight = height; };
	void								SyncResolution				( void );
#endif

#if defined(NESHNY_GL)
	static GLShader*					GetShader					( QString name, QString insertion = QString() ) { return Singleton().IGetShader(name, insertion); }
	static GLShader*					GetComputeShader			( QString name, QString insertion = QString() ) { return Singleton().IGetComputeShader(name, insertion); }
	static GLBuffer*					GetBuffer					( QString name ) { return Singleton().IGetBuffer(name); }
#elif defined(NESHNY_WEBGPU)
	void								InitWebGPU					( WebGPUNativeBackend backend, SDL_Window* window, int width, int height, void* layer );
	inline void							SetWebGPU					( WGPUDevice device, WGPUQueue queue, WGPUSurface surface, WGPUSwapChain chain ) { m_Device = device; m_Queue = queue; m_Surface = surface; m_SwapChain = chain; }
	inline WGPUDevice					GetWebGPUDevice				( void ) { return m_Device; }
	inline WGPUQueue					GetWebGPUQueue				( void ) { return m_Queue; }
	inline WGPUSurface					GetWebGPUSurface			( void ) { return m_Surface; }
	inline WGPUSwapChain				GetWebGPUSwapChain			( void ) { return m_SwapChain; }
	inline const WGPULimits&			GetLimits					( void ) { return m_Limits; }
	WGPULimits							GetDefaultLimits			( void );
	inline WGPUTextureView				GetCurrentSwapTextureView	( void ) { return wgpuSwapChainGetCurrentTextureView(m_SwapChain); }
	static WebGPUShader*				GetShader					( QString name, QByteArray start_insert = QByteArray(), QByteArray end_insert = QByteArray()) { return Singleton().IGetShader(name, start_insert, end_insert); }
	static WebGPURenderBuffer*			GetBuffer					( QString name ) { return Singleton().IGetBuffer(name); }
	static WebGPUSampler*				GetSampler					( WGPUAddressMode mode, WGPUFilterMode filter = WGPUFilterMode_Linear, bool linear_mipmaps = true, unsigned int max_anisotropy = 1 ) { return Singleton().IGetSampler(mode, filter, linear_mipmaps, max_anisotropy); }
	static void							WaitForCommandsToFinish		( void );

#endif

	template<class T, typename P = typename T::Params, typename = typename std::enable_if<std::is_base_of<Resource, T>::value>::type>
	static inline const ResourceResult<T> GetResource(QString path, P params = {}) { static_assert(std::is_pod<P>::value, "Plain old data expected for Params"); return Singleton().IGetResource<T>(path, params); }

	static inline bool					IsBufferEnabled				( QString name ) { return Singleton().IIsBufferEnabled(name); }
	static inline const InterfaceCore&	GetInterfaceData			( void ) { return Singleton().m_Interface; }
	static inline const std::map<QString, ResourceContainer> GetResources	( void ) { return Singleton().m_Resources; }

	void								UnloadAllShaders			( void );
	void								UnloadAllResources			( void );

#ifdef NESHNY_GL
	static void							DispatchMultiple			( GLShader* prog, int count, int total_local_groups, bool mem_barrier = true );
#endif
	template <class T>
	static bool							SaveBinary					( const T& item, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::WriteOnly)) {
			return false;
		}
		QDataStream out(&file);
		out << item;
		return true;
	}
	template <class T>
	static bool							LoadBinary					( T& item, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}
		QDataStream in(&file);
		in >> item;
		return true;
	}

	template <class T>
	static bool							LoadJSON					( std::vector<T>& items, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}
		Json::ParseError err;
		Json::FromJson<T>(file.readAll(), items, err);
		return !err;
	}
	template <class T>
	static bool							LoadJSON					( T& item, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}
		Json::ParseError err;
		Json::FromJson<T>(file.readAll(), item, err);
		return !err;
	}

	template <class T>
	static bool							SaveJSON					( std::vector<T>& items, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::WriteOnly)) {
			return false;
		}
		Json::ParseError err;
		auto data = Json::ToJson(items, err);
		if (err) {
			return false;
		}
		return file.write(data) == data.size();
	}
	template <class T>
	static bool							SaveJSON					( T& item, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::WriteOnly)) {
			return false;
		}
		Json::ParseError err;
		auto data = Json::ToJson(item, err);
		if (err) {
			return false;
		}
		return file.write(data) == data.size();
	}

	inline static void					RenderEditor				( void ) { Singleton().IRenderEditor(); }
	inline static int					GetTicks					( void ) { return Singleton().m_Ticks; }

#if defined(NESHNY_GL)
	inline const std::map<QString, GLShader*>&	GetShaders			( void ) { return m_Shaders; }
	inline const std::map<QString, GLShader*>&	GetComputeShaders	( void ) { return m_ComputeShaders; }

	int									CreateGLContext				( void );
	bool 								ActivateGLContext			( int index );
	void 								DeleteGLContext				( int index );
	static void							OpenGLSync					( void );
#elif defined(NESHNY_WEBGPU)
	inline const std::map<QString, WebGPUShader*>& GetShaders		( void ) { return m_Shaders; }
#endif

	inline qint64						GetMemoryAllocated			( void ) { return m_MemoryAllocated; }
	inline qint64						GetGPUMemoryAllocated		( void ) { return m_GPUMemoryAllocated; }

	ResourceManagementToken				GetResourceManagementToken ( void );

#ifdef SDL_h_
	SDL_Window*							GetSDLWindow				( void ) { return m_Window; }
#endif

private:

										Core						( void );
										~Core						( void );

#if defined(NESHNY_GL)
	GLShader*							IGetShader					( QString name, QString insertion );
	GLBuffer*							IGetBuffer					( QString name );
	GLShader*							IGetComputeShader			( QString name, QString insertion );
#elif defined(NESHNY_WEBGPU)
	WebGPUShader*						IGetShader					( QString name, QByteArray start_insert, QByteArray end_insert );
	WebGPURenderBuffer*					IGetBuffer					( QString name );
	WebGPUSampler*						IGetSampler					( WGPUAddressMode mode, WGPUFilterMode filter, bool linear_mipmaps, unsigned int max_anisotropy );
	static void							WebGPUErrorCallbackStatic	( WGPUErrorType type, char const* message, void* userdata ) { ((Core*)userdata)->WebGPUErrorCallback(type, message); }
	void								WebGPUErrorCallback			( WGPUErrorType type, char const* message );
#endif

	template<class T, typename P = typename T::Params>
	inline const ResourceResult<T>		IGetResource				( QString path, const P& params ) {

		QString key = path;
		int size_params = sizeof(P);
		if (size_params > 1) {
			size_t hash = HashMemory((unsigned char*)&params, sizeof(P));
			key += QString(":%2").arg(hash);
		}

		auto found = m_Resources.find(key);
		if (found != m_Resources.end()) {
			found->second.m_LastTickAccessed = m_Ticks;
			return ResourceResult<T>(found->second);
		}
		ResourceContainer& resource = m_Resources.insert_or_assign(key, ResourceContainer{}).first->second;

		m_ResourceThreads.DoTask([path, params, ticks=m_Ticks]() -> void* {
			T* result = new T();
			QString err;
			bool valid = result->Init(path, params, err);
			if (!valid) {
				delete result;
				return new ResourceContainer{ ResourceState::IN_ERROR, nullptr, err };
			}
			return new ResourceContainer{ ResourceState::DONE, (Resource*)result, QString(), result->GetMemoryEstimate(), result->GetGPUMemoryEstimate(), ticks };
		}, [this, &resource](void* ptr) { // uses temporary resource to transfer across thread divide
			ResourceContainer* tmp_resource = (ResourceContainer*)ptr;
			resource = *tmp_resource;
			m_MemoryAllocated += resource.m_Memory;
			m_GPUMemoryAllocated += resource.m_GPUMemory;
			delete tmp_resource;
		});
		return ResourceResult<T>(resource);
	}
	void								IRenderEditor				( void );
	bool								IIsBufferEnabled			( QString name );

#if defined(NESHNY_GL)
	std::map<QString, GLShader*>		m_Shaders;
	std::map<QString, GLBuffer*>		m_Buffers;
	std::map<QString, GLShader*>		m_ComputeShaders;
#elif defined(NESHNY_WEBGPU)
	std::map<QString, WebGPUShader*>		m_Shaders;
	std::map<QString, WebGPURenderBuffer*>	m_Buffers;
	std::vector<WebGPUSampler*>				m_Samplers;
#endif
	std::map<QString, ResourceContainer>	m_Resources;
	qint64									m_MemoryAllocated = 0;
	qint64									m_GPUMemoryAllocated = 0;

	int									m_Ticks = 0;
	QElapsedTimer						m_FrameTimer;
	bool								m_FullScreenHover = true;
	std::thread::id						m_MainThreadId;
	std::mutex							m_SyncLock;
	std::atomic_int						m_SyncsRequested = 0;

	std::ofstream						m_LogFile;

	InterfaceCore						m_Interface;
	WorkerThreadPool					m_ResourceThreads;
	std::optional<std::function<QByteArray(QString, QString&)>>	m_EmbeddableLoader;

#ifdef SDL_h_
	SDL_Window*							m_Window;
#ifdef NESHNY_GL
	std::vector<SDL_GLContext>			m_Contexts;
#endif
#endif

#ifdef NESHNY_WEBGPU
	IEngine*							m_Engine = nullptr;
	WGPUDevice							m_Device = nullptr;
	WGPUQueue							m_Queue = nullptr;
	WGPUSurface							m_Surface = nullptr;
	WGPUSwapChain						m_SwapChain = nullptr;
	WGPULimits							m_Limits;
	int									m_CurrentWidth = -1;
	int									m_CurrentHeight = -1;
	int									m_RequestedWidth = -1;
	int									m_RequestedHeight = -1;
	WebGPUTexture*						m_DepthTex = nullptr;
#endif

};

} // namespace Neshny