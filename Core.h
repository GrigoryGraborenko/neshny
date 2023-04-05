////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

struct Camera2D {

	// TODO: replace with double-based matrix
	inline QMatrix4x4		Get4x4Matrix			( int width, int height ) const {
		float aspect = (float)height / width;
		float view_rad_x = p_Zoom;
		float view_rad_y = p_Zoom * aspect;
		QMatrix4x4 viewMatrix;
		viewMatrix.ortho(p_Pos.x - view_rad_x, p_Pos.x + view_rad_x, p_Pos.y - view_rad_y, p_Pos.y + view_rad_y, 1.0, -1.0);
		if (p_RotationAngle != 0.0) {
			viewMatrix.rotate(p_RotationAngle, 0.0, 0.0, 1.0);
		}
		return viewMatrix;
	}

	// TODO: get 3x3 matrix here

	inline void Pan(int viewport_width, int delta_pixels_x, int delta_pixels_y) {
		double pan_mult = 2.0 * p_Zoom / float(viewport_width);
		p_Pos.x -= pan_mult * delta_pixels_x;
		p_Pos.y += pan_mult * delta_pixels_y;
	}

	inline Vec2 ScreenToWorld(Vec2 pos, int width, int height) {
		float aspect = (float)height / width;
		double fx = pos.x / width - 0.5, fy = 0.5 - pos.y / height;
		// TODO: account for rotation here
		return Vec2(
			fx * p_Zoom * 2.0 + p_Pos.x
			,fy * p_Zoom * aspect * 2.0 + p_Pos.y
		);
	}

	inline Vec2 WorldToScreen(Vec2 pos, int width, int height) {
		auto vp = Get4x4Matrix(width, height);
		QVector4D res = vp * QVector4D(pos.x, pos.y, 0.0, 1.0);
		double inv_w = 1.0 / res.w();
		return Vec2((res.x() * inv_w * 0.5 + 0.5) * width, (res.y() * inv_w * -0.5 + 0.5) * height);
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
	QMatrix4x4 GetViewPerspectiveMatrix(int width, int height) const {
		Matrix4 perspectiveMatrix = Matrix4::Perspective(p_FovDegrees, (float)width / height, p_NearPlane, p_FarPlane);
		return (perspectiveMatrix * GetViewMatrix()).toQMatrix4x4();
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
	QMatrix4x4 GetViewPerspectiveMatrix(int width, int height) const {
		Matrix4 perspectiveMatrix = Matrix4::Perspective(p_FovDegrees, (float)width / height, p_NearPlane, p_FarPlane);
		return (perspectiveMatrix * GetViewMatrix()).toQMatrix4x4();
	}
	void GetDirections(Vec3* forward = nullptr, Vec3* up = nullptr, Vec3* side = nullptr) {
		auto inv = p_Direction.Inverse();
		if (forward) { *forward = inv * Vec3(0, 0, -1); forward->Normalize(); }
		if (up) { *up = inv * Vec3(0, 1, 0); up->Normalize(); }
		if (side) { *side = inv * Vec3(1, 0, 0); side->Normalize(); }
	}
	Vec3		p_Pos;
	Quat		p_Direction = Quat(0.0, 0.0, 0.0, 1.0);
	float		p_FovDegrees = 60;
	float		p_NearPlane = 0.1f;
	float		p_FarPlane = 1000.0f;
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

struct InterfaceBufferViewer {
	bool	p_Visible = false;
	bool	p_AllEnabled = false;
	int		p_MaxFrames = 100;
	int		p_TimeSlider = 0; // do not save this
	std::vector<InterfaceCollapsible> p_Items;
};

struct InterfaceShaderViewer {
	bool			p_Visible = false;
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
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Token {
	Token(std::function<void()> destruction, bool valid = true) : p_DestructFunc(destruction), p_Valid(valid) {}
	Token() : p_DestructFunc(), p_Valid(false) {}
	~Token(void) { p_DestructFunc(); }
	inline bool IsValid(void) { return p_Valid; }
private:
	std::function<void()>	p_DestructFunc;
	bool					p_Valid;
};

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
		int				m_GLContext = -1;
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
	virtual				~Resource		( void ) {}
protected:
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
		Resource* m_Resource = nullptr;
		QString			m_Error;
	};

	template<class T, typename = typename std::enable_if<std::is_base_of<Resource, T>::value>::type>
	struct ResourceResult {
		ResourceResult(const ResourceContainer& container) : m_State(container.m_State), m_Resource((T*)container.m_Resource), m_Error(container.m_Error) {}
		T* operator->() const { return m_Resource; }
		bool IsValid(void) const { return m_State == ResourceState::DONE; }
		ResourceState	m_State = ResourceState::PENDING;
		T*				m_Resource = nullptr;
		QString			m_Error;
	};

	inline static Core&					Singleton				( void ) { static Core core; return core; }

	void								SetEmbeddableFileLoader	( std::function<QByteArray(QString, QString&)> loader ) { m_EmbeddableLoader = loader; }
	inline void							SetTicksOverride		( int ticks ) { m_Ticks = ticks; }

	bool								LoopInit				( IEngine* engine );
	void								LoopInner				( IEngine* engine, int width, int height, bool& fullscreen_hover );

#ifdef SDL_h_
	bool								SDLLoop					( SDL_Window* window, IEngine* engine );
#endif
#ifdef QT_LOOP
	bool								QTLoop					( class QOpenGLWindow* window, IEngine* engine );
#endif
	static GLShader*					GetShader				( QString name, QString insertion = QString() ) { return Singleton().IGetShader(name, insertion); }
	static GLShader*					GetComputeShader		( QString name, QString insertion = QString() ) { return Singleton().IGetComputeShader(name, insertion); }
	static GLBuffer*					GetBuffer				( QString name ) { return Singleton().IGetBuffer(name); }

	template<class T, typename P = T::Params, typename = typename std::enable_if<std::is_base_of<Resource, T>::value>::type>
	static inline const ResourceResult<T> GetResource(QString path, P params = {}) { static_assert(std::is_pod<P>::value, "Plain old data expected for Params"); return Singleton().IGetResource<T>(path, params); }

	static inline bool					IsBufferEnabled			( QString name ) { return Singleton().IIsBufferEnabled(name); }
	static inline const InterfaceCore&	GetInterfaceData		( void ) { return Singleton().m_Interface; }
	static inline const std::map<QString, ResourceContainer>	GetResources	( void ) { return Singleton().m_Resources; }

	void								UnloadAllShaders		( void );
	void								UnloadAllResources		( void );

	static void							DispatchMultiple		( GLShader* prog, int count, bool mem_barrier = true );
	template <class T>
	static bool							SaveBinary				( const T& item, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::WriteOnly)) {
			return false;
		}
		QDataStream out(&file);
		out << item;
		return true;
	}
	template <class T>
	static bool							LoadBinary				( T& item, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}
		QDataStream in(&file);
		in >> item;
		return true;
	}

	template <class T>
	static bool							LoadJSON				( std::vector<T>& items, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}
		Json::ParseError err;
		Json::FromJson<T>(file.readAll(), items, err);
		return !err;
	}
	template <class T>
	static bool							LoadJSON				( T& item, QString filename ) {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}
		Json::ParseError err;
		Json::FromJson<T>(file.readAll(), item, err);
		return !err;
	}

	template <class T>
	static bool							SaveJSON				( std::vector<T>& items, QString filename ) {
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
	static bool							SaveJSON				( T& item, QString filename ) {
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


	inline static void					RenderEditor			( void ) { Singleton().IRenderEditor(); }
	inline static int					GetTicks				( void ) { return Singleton().m_Ticks; }

	inline const std::map<QString, GLShader*>&	GetShaders				( void ) { return m_Shaders; }
	inline const std::map<QString, GLShader*>&	GetComputeShaders		( void ) { return m_ComputeShaders; }

	int									CreateGLContext			( void );
	bool 								ActivateGLContext		( int index );
	void 								DeleteGLContext			( int index );
	static void							OpenGLSync				( void );

private:

										Core					( void );
										~Core					( void );

	GLShader*							IGetShader				( QString name, QString insertion );
	GLBuffer*							IGetBuffer				( QString name );
	GLShader*							IGetComputeShader		( QString name, QString insertion );

	template<class T, typename P = T::Params>
	inline const ResourceResult<T>		IGetResource			( QString path, const P& params ) {

		QString key = path;
		int size_params = sizeof(P);
		if (size_params > 1) {
			size_t hash = HashMemory((unsigned char*)&params, sizeof(P));
			key += QString(":%2").arg(hash);
		}

		auto found = m_Resources.find(key);
		if (found != m_Resources.end()) {
			return ResourceResult<T>(found->second);
		}
		ResourceContainer& resource = m_Resources.insert_or_assign(key, ResourceContainer{}).first->second;

		m_ResourceThreads.DoTask([path, params]() -> void* {
			T* result = new T();
			QString err;
			bool valid = result->Init(path, params, err);
			if (!valid) {
				delete result;
				return new ResourceContainer{ ResourceState::IN_ERROR, nullptr, err };
			}
			return new ResourceContainer{ ResourceState::DONE, (Resource*)result, QString() };
		}, [&resource](void* ptr) { // uses temporary resource to transfer across thread divide
			ResourceContainer* tmp_resource = (ResourceContainer*)ptr;
			resource = *tmp_resource;
			delete tmp_resource;
		});
		return ResourceResult<T>(resource);
	}
	void								IRenderEditor			( void );
	bool								IIsBufferEnabled		( QString name );

	std::map<QString, GLShader*>		m_Shaders;
	std::map<QString, GLBuffer*>		m_Buffers;
	std::map<QString, GLShader*>		m_ComputeShaders;
	std::map<QString, ResourceContainer> m_Resources;

	int									m_Ticks = 0;

	QFile								m_LogFile;

	InterfaceCore						m_Interface;
	WorkerThreadPool					m_ResourceThreads;
	std::optional<std::function<QByteArray(QString, QString&)>>	m_EmbeddableLoader;

#ifdef SDL_h_
	SDL_Window*							m_Window;
	std::vector<SDL_GLContext>			m_Contexts;
#endif
};

} // namespace Neshny