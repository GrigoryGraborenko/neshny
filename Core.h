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
class Core {

public:

	inline static Core&					Singleton				( void ) { static Core core; return core; }

#ifdef SDL_h_
	bool								SDLLoop					( SDL_Window* window, IEngine* engine );
#endif

	GLShader*							GetShader				( QString name, QString insertion = QString() );
	GLShader*							GetComputeShader		( QString name, QString insertion = QString() );
	GLBuffer*							GetBuffer				( QString name );
	GLTexture*							GetTexture				( QString name, bool skybox = false );

	void								UnloadAllShaders		( void );

	static void							DispatchMultiple		( GLShader* prog, int count, bool mem_barrier = true );

	inline static void					RenderEditor			( void ) { Singleton().IRenderEditor(); }


	inline const std::map<QString, GLShader*>&	GetShaders				( void ) { return m_Shaders; }
	inline const std::map<QString, GLShader*>&	GetComputeShaders		( void ) { return m_ComputeShaders; }

private:

										Core					( void );
										~Core					( void );

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

	QFile								m_LogFile;

	InterfaceCore						m_Interface;
};
