////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

struct InterfaceShaderViewer {
	bool	p_Visible = false;
};

struct InterfaceBufferViewer {
	bool	p_Visible = false;
};

struct InterfaceCore {
	InterfaceShaderViewer	p_ShaderView;
	InterfaceBufferViewer	p_BufferView;
};

class IEngine {
public:
	virtual							~IEngine() {}

	virtual void					MouseButton	( int button, bool is_down ) = 0;
	virtual void					MouseMove	( QVector2D delta ) = 0;
	virtual void					MouseWheel	( bool up ) = 0;
	virtual void					Key			( int key, bool is_down ) = 0;

	virtual void					Init		( void ) = 0;
	virtual void					Tick		( qint64 delta_nanoseconds ) = 0;
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
class Core {

public:

	enum class ResourceState {
		PENDING
		,DONE
		,ERROR
	};

	struct Resource {
		ResourceState	m_State = ResourceState::PENDING;
		unsigned char*	m_Data = nullptr;
		int				m_DataLength = 0;
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
	static const Resource&				GetResource				( QString path ) { return Singleton().IGetResource(path); }

	void								UnloadAllShaders		( void );

	static void							DispatchMultiple		( GLShader* prog, int count, bool mem_barrier = true );

	inline static void					RenderEditor			( void ) { Singleton().IRenderEditor(); }


	inline const std::map<QString, GLShader*>&	GetShaders				( void ) { return m_Shaders; }
	inline const std::map<QString, GLShader*>&	GetComputeShaders		( void ) { return m_ComputeShaders; }

private:

										Core					( void );
										~Core					( void );

	const Resource&						IGetResource			( QString path );
	void								IRenderEditor			( void );

#ifdef _DEBUG
	inline std::vector<QString>			GetShaderPrefixes		( void ) { return { "../src/Shaders/", "../src/Neshny/Shaders/" }; }
#else
	inline std::vector<QString>			GetShaderPrefixes		( void ) { return { ":/Core/src/Shaders/", ":/Core/src/Neshny/Shaders/" }; }
#endif

	std::map<QString, GLShader*>		m_Shaders;
	std::map<QString, GLBuffer*>		m_Buffers;
	std::map<QString, GLTexture*>		m_Textures;
	std::map<QString, GLShader*>		m_ComputeShaders;
	std::map<QString, Resource>			m_Resources;

	QFile								m_LogFile;

	InterfaceCore						m_Interface;
	WorkerThreadPool					m_ResourceThreads;
};
