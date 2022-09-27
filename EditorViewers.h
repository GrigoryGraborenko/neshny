////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define DEBUG_POINT_SIZE 1.0

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseDebugRender {

public:

    //static inline std::vector<QString>&	        GetStrings  	    ( void ) { return Singleton().m_Strings; }
	//static inline std::unordered_map<QString, QString>& GetPersistantStrings(void) { return Singleton().m_PersistStrings; }

    inline void									AddLine             ( Triple a, Triple b, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { m_Lines.push_back(DebugLine{a, b, color, on_top}); }
    inline void									AddPoint            ( Triple pos, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { m_Points.push_back(DebugPoint{pos, std::string(""), color, on_top}); }
    inline void									AddPoint            ( Triple pos, std::string text, QVector4D color, bool on_top = true ) { m_Points.push_back(DebugPoint{pos, text, color, on_top}); }
    inline void									AddTriangle         ( Triple a, Triple b, Triple c, QVector4D color ) { m_Triangles.push_back(DebugTriangle{a, b, c, color}); }

    //static inline void                          AddString           ( const QString& str ) { Singleton().m_Strings.push_back(str); }
    //static inline void                          AddPersistString    ( QString key, QString val ) { Singleton().m_PersistStrings.insert_or_assign(key, val); }

protected:

	struct DebugPoint {
		Triple p_Pos;
		std::string p_Str;
		QVector4D p_Col;
		bool p_OnTop;
	};

	struct DebugLine {
		Triple p_A, p_B; QVector4D p_Col; bool p_OnTop;
	};

	struct DebugTriangle {
		Triple p_A, p_B, p_C;
		QVector4D p_Col;
	};

	void										IRender3DDebug		( const QMatrix4x4& view_perspective, int width, int height, Triple offset, double scale );
    inline void									IClear		        ( void ) { m_Lines.clear(); m_Points.clear(); m_Triangles.clear(); }

    std::vector<DebugLine>						m_Lines;
    std::vector<DebugPoint>						m_Points;
    std::vector<DebugTriangle>					m_Triangles;
    //std::vector<QString> 						m_Strings;
	//std::unordered_map<QString, QString>		m_PersistStrings;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class DebugRender : public BaseDebugRender {

public:

	static inline DebugRender&					Singleton			( void ) { static DebugRender instance; return instance; }

	static inline void							Render3DDebug		( const QMatrix4x4& view_perspective, int width, int height, Triple offset = Triple(0, 0, 0), double scale = 1.0f) { Singleton().IRender3DDebug(view_perspective, width, height, offset, scale); }

    static inline void					        Clear		        ( void ) { Singleton().IClear(); }

	static inline void                          Line				( Triple a, Triple b, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a, b, color, on_top); }
	static inline void                          Point				( Triple pos, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos, color, on_top); }
	static inline void                          Point				( Triple pos, std::string text, QVector4D color, bool on_top = true ) { Singleton().AddPoint(pos, text, color, on_top); }
	static inline void                          Triangle			( Triple a, Triple b, Triple c, QVector4D color ) { Singleton().AddTriangle(a, b, c, color); }
};


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BufferViewer {

public:

	inline static BufferViewer&	Singleton			( void ) { static BufferViewer instance; return instance; }

	static inline void			Checkpoint			( QString name, QString stage, class GLSSBO& buffer, MemberSpec::Type type, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, nullptr, type); }
	static inline void			Checkpoint			( QString name, QString stage, class GLSSBO& buffer, const StructInfo& info, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, &info, MemberSpec::Type::T_UNKNOWN); }
	static inline void			Checkpoint			( QString stage, class GPUEntity& entity ) { Singleton().ICheckpoint(stage, entity); }

	void						RenderImGui			( InterfaceBufferViewer& data );

	static inline std::shared_ptr<GLSSBO> GetStoredFrameAt	( QString name, int tick, int& count ) { return Singleton().IGetStoredFrameAt(name, tick, count); }

private:

	struct CheckpointData {
		QString								p_Stage;
		QString								p_Info;
		int									p_Count = 0;
		int									p_Tick = -1;
		bool								p_UsingFreeList = false;
		std::shared_ptr<unsigned char[]>	p_Data;
	};

	struct CheckpointList {
		std::vector<MemberSpec>		p_Members;
		std::deque<CheckpointData>	p_Frames;
		int							p_StructSize = 0;
	};

							BufferViewer		( void ) {}
							~BufferViewer		( void ) {}

	void					ICheckpoint			( QString name, QString stage, class GLSSBO& buffer, int count, const StructInfo* info, MemberSpec::Type type );
	void					ICheckpoint			( QString stage, class GPUEntity& entity );
	void					IStoreCheckpoint	( QString name, CheckpointData data, const StructInfo* info, MemberSpec::Type type );
	std::shared_ptr<GLSSBO>	IGetStoredFrameAt	( QString name, int tick, int& count );

	std::unordered_map<QString, CheckpointList>	m_Frames; // TODO: this doesn't really have to be a map, probably faster as a vector
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class ShaderViewer {
public:
	static void						RenderImGui			( InterfaceShaderViewer& data );// { Singleton().IRenderImGui(data); }
	static InterfaceCollapsible*	RenderShader		( InterfaceShaderViewer& data, QString name, GLShader* shader, bool is_compute, QString search );
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class ResourceViewer {
public:
	static void						RenderImGui			( InterfaceResourceViewer& data );
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Scrapbook2D {
public:

	inline static Scrapbook2D&	Singleton			( void ) { static Scrapbook2D instance; return instance; }

	auto						ActivateRTT			( void ) { return Singleton().m_RTT.Activate(RTT::Mode::RGBA_DEPTH_STENCIL, m_Width, m_Height); }

	static void					RenderImGui			( InterfaceScrapbook2D& data );

private:

	RTT							m_RTT;
	int							m_Width = 32;
	int							m_Height = 32;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Scrapbook3D : public BaseDebugRender {
public:

	inline static Scrapbook3D&	Singleton					( void ) { static Scrapbook3D instance; return instance; }

	auto						ActivateRTT					( void ) { return Singleton().m_RTT.Activate(RTT::Mode::RGBA_DEPTH_STENCIL, m_Width, m_Height); }
	QMatrix4x4					GetViewPerspectiveMatrix	( void ) { return Singleton().m_Cam.GetViewPerspectiveMatrix(m_Width, m_Height); }

	static void					RenderImGui					( InterfaceScrapbook3D& data ) { Singleton().IRenderImGui(data); }

private:

	void						IRenderImGui				( InterfaceScrapbook3D& data );

	RTT							m_RTT;
	int							m_Width = 32;
	int							m_Height = 32;

	Camera3DOrbit				m_Cam = Camera3DOrbit{ Triple(), 100, 30, 30 };
};
