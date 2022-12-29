////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

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

	QMatrix4x4				GetViewMatrix				( void ) const {
		QMatrix4x4 viewMatrix;
		viewMatrix.translate(0, 0, -p_Zoom);
		viewMatrix.rotate(p_VerticalDegrees, 1, 0, 0);
		viewMatrix.rotate(p_HorizontalDegrees, 0, 1, 0);
		viewMatrix.translate(float(-p_Pos.x), float(-p_Pos.y), float(-p_Pos.z));
		return viewMatrix;
	}
	QMatrix4x4				GetViewPerspectiveMatrix	( int width, int height ) const {
		QMatrix4x4 perspectiveMatrix;
		perspectiveMatrix.perspective(p_FovDegrees, (float)width / height, p_NearPlane, p_FarPlane);
		return perspectiveMatrix * GetViewMatrix();
	}
	Vec3				GetCamRealPos					( void ) const {
		return GetViewMatrix().inverted() * QVector4D(0, 0, 0, 1);
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
	QMatrix4x4 GetViewMatrix(void) const {
		QMatrix4x4 viewMatrix;
		viewMatrix.rotate(p_Direction);
		viewMatrix.translate(float(-p_Pos.x), float(-p_Pos.y), float(-p_Pos.z));
		return viewMatrix;
	}
	QMatrix4x4 GetViewPerspectiveMatrix(int width, int height) const {
		QMatrix4x4 perspectiveMatrix;
		perspectiveMatrix.perspective(p_FovDegrees, (float)width / height, p_NearPlane, p_FarPlane);
		return perspectiveMatrix * GetViewMatrix();
	}
	void GetDirections(Vec3* forward = nullptr, Vec3* up = nullptr, Vec3* side = nullptr) {
		auto inv = p_Direction.inverted();
		if (forward) { *forward = inv * QVector3D(0, 0, -1); forward->Normalize(); }
		if (up) { *up = inv * QVector3D(0, 1, 0); up->Normalize(); }
		if (side) { *side = inv * QVector3D(1, 0, 0); side->Normalize(); }
	}
	Vec3		p_Pos;
	QQuaternion	p_Direction = QQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
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

namespace meta {
	template<> inline auto registerMembers<InterfaceCore>() {
		return members(
			member("Version", &InterfaceCore::p_Version)
			,member("ShowImGuiDemo", &InterfaceCore::p_ShowImGuiDemo)
			,member("InfoView", &InterfaceCore::p_InfoView)
			,member("BufferView", &InterfaceCore::p_BufferView)
			,member("ShaderView", &InterfaceCore::p_ShaderView)
			,member("ResourceView", &InterfaceCore::p_ResourceView)
			,member("Scrapbook2D", &InterfaceCore::p_Scrapbook2D)
			,member("Scrapbook3D", &InterfaceCore::p_Scrapbook3D)
		);
	}
	template<> inline auto registerMembers<InterfaceInfoViewer>() {
		return members(
			member("Visible", &InterfaceInfoViewer::p_Visible)
		);
	}
	template<> inline auto registerMembers<InterfaceBufferViewer>() {
		return members(
			member("Visible", &InterfaceBufferViewer::p_Visible)
			,member("AllEnabled", &InterfaceBufferViewer::p_AllEnabled)
			,member("MaxFrames", &InterfaceBufferViewer::p_MaxFrames)
			,member("Items", &InterfaceBufferViewer::p_Items)
		);
	}
	template<> inline auto registerMembers<InterfaceShaderViewer>() {
		return members(
			member("Visible", &InterfaceShaderViewer::p_Visible)
			,member("Search", &InterfaceShaderViewer::p_Search)
			,member("Items", &InterfaceShaderViewer::p_Items)
		);
	}

	template<> inline auto registerMembers<InterfaceResourceViewer>() {
		return members(
			member("Visible", &InterfaceResourceViewer::p_Visible)
		);
	}
	template<> inline auto registerMembers<InterfaceScrapbook2D>() {
		return members(
			member("Visible", &InterfaceScrapbook2D::p_Visible)
			,member("Cam", &InterfaceScrapbook2D::p_Cam)
		);
	}
	template<> inline auto registerMembers<InterfaceScrapbook3D>() {
		return members(
			member("Visible", &InterfaceScrapbook3D::p_Visible)
			,member("Cam", &InterfaceScrapbook3D::p_Cam)
		);
	}

	template<> inline auto registerMembers<InterfaceCollapsible>() {
		return members(
			member("Name", &InterfaceCollapsible::p_Name)
			,member("Open", &InterfaceCollapsible::p_Open)
			,member("Enabled", &InterfaceCollapsible::p_Enabled)
		);
	}

	template<> inline auto registerMembers<Camera3DOrbit>() {
		return members(
			member("Pos", &Camera3DOrbit::p_Pos)
			,member("Zoom", &Camera3DOrbit::p_Zoom)
			,member("HorizontalDegrees", &Camera3DOrbit::p_HorizontalDegrees)
			,member("VerticalDegrees", &Camera3DOrbit::p_VerticalDegrees)
			,member("FovDegrees", &Camera3DOrbit::p_FovDegrees)
			,member("NearPlane", &Camera3DOrbit::p_NearPlane)
			,member("FarPlane", &Camera3DOrbit::p_FarPlane)
		);
	}
	template<> inline auto registerMembers<Camera2D>() {
		return members(
			member("Pos", &Camera2D::p_Pos)
			,member("Zoom", &Camera2D::p_Zoom)
			,member("RotationAngle", &Camera2D::p_RotationAngle)
		);
	}

}

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
	virtual bool		Init			( QString path, QString& err ) = 0;
protected:
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Neshny {

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

	inline static Neshny&				Singleton				( void ) { static Neshny core; return core; }

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
	template<class T, typename = typename std::enable_if<std::is_base_of<Resource, T>::value>::type>
	static inline const ResourceResult<T> GetResource			( QString path ) { return Singleton().IGetResource<T>(path); }
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

	inline static void					RenderEditor			( void ) { Singleton().IRenderEditor(); }
	inline static int					GetTicks				( void ) { return Singleton().m_Ticks; }

	inline const std::map<QString, GLShader*>&	GetShaders				( void ) { return m_Shaders; }
	inline const std::map<QString, GLShader*>&	GetComputeShaders		( void ) { return m_ComputeShaders; }

	int									CreateGLContext			( void );
	bool 								ActivateGLContext		( int index );
	void 								DeleteGLContext			( int index );
	static void							OpenGLSync				( void );

private:

										Neshny					( void );
										~Neshny					( void );

	GLShader*							IGetShader				( QString name, QString insertion );
	GLBuffer*							IGetBuffer				( QString name );
	GLShader*							IGetComputeShader		( QString name, QString insertion );

	template<class T>
	inline const ResourceResult<T>		IGetResource			( QString path ) {
		auto found = m_Resources.find(path);
		if (found != m_Resources.end()) {
			return ResourceResult<T>(found->second);
		}
		ResourceContainer& resource = m_Resources.insert_or_assign(path, ResourceContainer{}).first->second;

		m_ResourceThreads.DoTask([path]() -> void* {
			T* result = new T();
			QString err;
			bool valid = result->Init(path, err);
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
