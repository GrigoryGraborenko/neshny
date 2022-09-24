////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define INTERFACE_SAVE_VERSION 1 // todo: start incrementing this on release
struct InterfaceCollapsible {
	QString		p_Name;
	bool		p_Open = false;
	bool		p_Enabled = true;
};

struct InterfaceShaderViewer {
	bool			p_Visible = false;
	std::string		p_Search = "";
	std::vector<InterfaceCollapsible> p_Items;
};

struct InterfaceBufferViewer {
	bool	p_Visible = false;
	bool	p_AllEnabled = false;
	int		p_MaxFrames = 100;
	int		p_TimeSlider = 0; // do not save this
	std::vector<InterfaceCollapsible> p_Items;
};

struct InterfaceCore {
	int						p_Version = INTERFACE_SAVE_VERSION;
	bool					p_ShowImGuiDemo = false;
	InterfaceShaderViewer	p_ShaderView;
	InterfaceBufferViewer	p_BufferView;
};

namespace meta {
	template<> inline auto registerMembers<InterfaceCore>() {
		return members(
			member("Version", &InterfaceCore::p_Version)
			,member("ShowImGuiDemo", &InterfaceCore::p_ShowImGuiDemo)
			,member("ShaderView", &InterfaceCore::p_ShaderView)
			,member("BufferView", &InterfaceCore::p_BufferView)
		);
	}
	template<> inline auto registerMembers<InterfaceShaderViewer>() {
		return members(
			member("Visible", &InterfaceShaderViewer::p_Visible)
			,member("Search", &InterfaceShaderViewer::p_Search)
			,member("Items", &InterfaceShaderViewer::p_Items)
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
	template<> inline auto registerMembers<InterfaceCollapsible>() {
		return members(
			member("Name", &InterfaceCollapsible::p_Name)
			,member("Open", &InterfaceCollapsible::p_Open)
			,member("Enabled", &InterfaceCollapsible::p_Enabled)
		);
	}
}

class IEngine {
public:
	virtual							~IEngine() {}

	virtual void					MouseButton	( int button, bool is_down ) = 0;
	virtual void					MouseMove	( QVector2D delta ) = 0;
	virtual void					MouseWheel	( bool up ) = 0;
	virtual void					Key			( int key, bool is_down ) = 0;

	virtual void					Init		( void ) = 0;
	virtual bool					Tick		( qint64 delta_nanoseconds, int tick ) = 0;
	virtual void					Render		( int width, int height ) = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Token {
	Token(std::function<void()> destruction, bool valid = true) : p_DestructFunc(destruction) {}
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
	virtual bool		Init			( unsigned char* data, int length, QString& err ) = 0;
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
		,ERROR
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
		ResourceState	m_State = ResourceState::PENDING;
		T*				m_Resource = nullptr;
		QString			m_Error;
	};

	inline static Core&					Singleton				( void ) { static Core core; return core; }

#ifdef SDL_h_
	bool								SDLLoop					( SDL_Window* window, IEngine* engine );
#endif

	GLShader*							GetShader				( QString name, QString insertion = QString() );
	GLShader*							GetComputeShader		( QString name, QString insertion = QString() );
	GLBuffer*							GetBuffer				( QString name );
	GLTexture*							GetTexture				( QString name, bool skybox = false );
	template<class T, typename = typename std::enable_if<std::is_base_of<Resource, T>::value>::type>
	static inline const ResourceResult<T> GetResource			( QString path ) { return Singleton().IGetResource<T>(path); }
	static inline bool					IsBufferEnabled			( QString name ) { return Singleton().IIsBufferEnabled(name); }
	static inline const InterfaceCore&	GetInterfaceData		( void ) { return Singleton().m_Interface; }

	void								UnloadAllShaders		( void );

	static void							DispatchMultiple		( GLShader* prog, int count, bool mem_barrier = true );

	inline static void					RenderEditor			( void ) { Singleton().IRenderEditor(); }
	inline static int					GetTicks				( void ) { return Singleton().m_Ticks; }


	inline const std::map<QString, GLShader*>&	GetShaders				( void ) { return m_Shaders; }
	inline const std::map<QString, GLShader*>&	GetComputeShaders		( void ) { return m_ComputeShaders; }

private:

										Core					( void );
										~Core					( void );

	template<class T>
	const ResourceResult<T>				IGetResource			( QString path );
	void								IRenderEditor			( void );
	bool								IIsBufferEnabled		( QString name );

#ifdef _DEBUG
	inline std::vector<QString>			GetShaderPrefixes		( void ) { return { "../src/Shaders/", "../src/Neshny/Shaders/" }; }
#else
	inline std::vector<QString>			GetShaderPrefixes		( void ) { return { ":/Core/src/Shaders/", ":/Core/src/Neshny/Shaders/" }; }
#endif

	std::map<QString, GLShader*>		m_Shaders;
	std::map<QString, GLBuffer*>		m_Buffers;
	std::map<QString, GLTexture*>		m_Textures;
	std::map<QString, GLShader*>		m_ComputeShaders;
	std::map<QString, ResourceContainer> m_Resources;

	int									m_Ticks = 0;

	QFile								m_LogFile;

	InterfaceCore						m_Interface;
	WorkerThreadPool					m_ResourceThreads;
};
