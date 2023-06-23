////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_EDITOR_VIEWERS

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseDebugRender {

public:

    //static inline std::vector<QString>&	        GetStrings  	    ( void ) { return Singleton().m_Strings; }
	//static inline std::unordered_map<QString, QString>& GetPersistantStrings(void) { return Singleton().m_PersistStrings; }

    inline void									AddLine             ( Vec3 a, Vec3 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { m_Lines.push_back(DebugLine{a, b, color, on_top}); }
    inline void									AddPoint            ( Vec3 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { m_Points.push_back(DebugPoint{pos, std::string(""), color, on_top}); }
    inline void									AddPoint            ( Vec3 pos, std::string text, Vec4 color, bool on_top = true ) { m_Points.push_back(DebugPoint{pos, text, color, on_top}); }
    inline void									AddTriangle         ( Vec3 a, Vec3 b, Vec3 c, Vec4 color ) { m_Triangles.push_back(DebugTriangle{a, b, c, color}); }
    inline void									AddCircle			( Vec2 a, double radius, Vec4 color, bool filled = false ) { m_Circles.push_back(DebugCircle{a, radius, color, filled}); }
    inline void									AddSquare			( Vec2 min_pos, Vec2 max_pos, Vec4 color, bool filled = false ) { m_Squares.push_back(DebugSquare{min_pos, max_pos, color, filled}); }

    //static inline void                          AddString           ( const QString& str ) { Singleton().m_Strings.push_back(str); }
    //static inline void                          AddPersistString    ( QString key, QString val ) { Singleton().m_PersistStrings.insert_or_assign(key, val); }

protected:

	struct DebugPoint {
		Vec3 p_Pos;
		std::string p_Str;
		Vec4 p_Col;
		bool p_OnTop;
	};

	struct DebugLine {
		Vec3 p_A, p_B; Vec4 p_Col; bool p_OnTop;
	};

	struct DebugTriangle {
		Vec3 p_A, p_B, p_C;
		Vec4 p_Col;
	};

	struct DebugCircle {
		Vec2 p_Pos;
		double p_Radius;
		Vec4 p_Col;
		bool p_Filled = false;
	};

	struct DebugSquare {
		Vec2 p_MinPos;
		Vec2 p_MaxPos;
		Vec4 p_Col;
		bool p_Filled = false;
	};

	void										IRender3DDebug		( const fMatrix4& view_perspective, int width, int height, Vec3 offset, double scale, double point_size = 1.0 );
    inline void									IClear		        ( void ) { m_Lines.clear(); m_Points.clear(); m_Triangles.clear(); m_Circles.clear(); m_Squares.clear(); }

    std::vector<DebugLine>						m_Lines;
    std::vector<DebugPoint>						m_Points;
    std::vector<DebugTriangle>					m_Triangles;
	std::vector<DebugCircle>					m_Circles;
	std::vector<DebugSquare>					m_Squares;
    //std::vector<QString> 						m_Strings;
	//std::unordered_map<QString, QString>		m_PersistStrings;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class DebugRender : public BaseDebugRender {

public:

	static inline DebugRender&					Singleton			( void ) { static DebugRender instance; return instance; }

	static inline void							Render3DDebug		( const fMatrix4& view_perspective, int width, int height, Vec3 offset = Vec3(0, 0, 0), double scale = 1.0f) { Singleton().IRender3DDebug(view_perspective, width, height, offset, scale); }

    static inline void					        Clear		        ( void ) { Singleton().IClear(); }

	static inline void                          Line				( Vec3 a, Vec3 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a, b, color, on_top); }
	static inline void                          Point				( Vec3 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos, color, on_top); }
	static inline void                          Point				( Vec3 pos, std::string text, Vec4 color, bool on_top = true ) { Singleton().AddPoint(pos, text, color, on_top); }
	static inline void                          Triangle			( Vec3 a, Vec3 b, Vec3 c, Vec4 color ) { Singleton().AddTriangle(a, b, c, color); }
	static inline void                          Circle				( Vec2 pos, double radius, Vec4 color, bool filled = false ) { Singleton().AddCircle(pos, radius, color, filled); }
	static inline void                          Square				( Vec2 min_pos, Vec2 max_pos, Vec4 color, bool filled = false ) { Singleton().AddSquare(min_pos, max_pos, color, filled); }
protected:
												DebugRender			( void ) {}
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class InfoViewer {
public:

	inline static InfoViewer&					Singleton			( void ) { static InfoViewer instance; return instance; }

	inline static void							RenderImGui			( InterfaceInfoViewer& data ) { Singleton().IRenderImGui(data); }

	inline static void							LoopTime			( qint64 nanos ) { Singleton().ILoopTime(nanos); }
	inline static void							ClearLoopTime		( qint64 nanos ) { Singleton().IClearLoopTime(); }

protected:
												InfoViewer			( void ) { IClearLoopTime(); }

	void										ILoopTime			( qint64 nanos );
	void										IRenderImGui		( InterfaceInfoViewer& data );
	void										IClearLoopTime		( void );

	std::vector<std::pair<double, int>>			m_LoopHistogram;
	int											m_LoopHistogramOverflow;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BufferViewer {

public:

	inline static BufferViewer&	Singleton			( void ) { static BufferViewer instance; return instance; }

#if defined(NESHNY_GL)
	static inline void			Checkpoint			( QString name, QString stage, class GLSSBO& buffer, MemberSpec::Type type, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, nullptr, type); }
	static inline void			Checkpoint			( QString name, QString stage, class GLSSBO& buffer, const StructInfo& info, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, &info, MemberSpec::Type::T_UNKNOWN); }
	static inline void			Checkpoint			( QString stage, class GPUEntity& entity ) { Singleton().ICheckpoint(stage, entity); }
#elif defined(NESHNY_WEBGPU)
#endif

	void						RenderImGui			( InterfaceBufferViewer& data );

#if defined(NESHNY_GL)
	static inline std::shared_ptr<GLSSBO> GetStoredFrameAt	( QString name, int tick, int& count ) { return Singleton().IGetStoredFrameAt(name, tick, count); }
#elif defined(NESHNY_WEBGPU)
#endif

	static inline void			Highlight			( QString name, int id ) { Singleton().IHighlight(name, id); }
	static inline void			ClearHighlight		( void ) { Singleton().IHighlight(QString(), -1); }

protected:

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

#if defined(NESHNY_GL)
	void					ICheckpoint			( QString name, QString stage, class GLSSBO& buffer, int count, const StructInfo* info, MemberSpec::Type type );
	void					ICheckpoint			( QString stage, class GPUEntity& entity );
	std::shared_ptr<GLSSBO>	IGetStoredFrameAt	( QString name, int tick, int& count );
#elif defined(NESHNY_WEBGPU)
#endif

	void					IStoreCheckpoint	( QString name, CheckpointData data, const StructInfo* info, MemberSpec::Type type );
	void					IHighlight			( QString name, int id ) { m_HighlightName = name; m_HighlightID = id; }

	std::unordered_map<QString, CheckpointList>	m_Frames; // TODO: this doesn't really have to be a map, probably faster as a vector
	QString		m_HighlightName;
	int			m_HighlightID = -1;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class ShaderViewer {
public:
	static void						RenderImGui			( InterfaceShaderViewer& data );// { Singleton().IRenderImGui(data); }
#if defined(NESHNY_GL)
	static InterfaceCollapsible*	RenderShader		( InterfaceShaderViewer& data, QString name, GLShader* shader, bool is_compute, QString search );
#elif defined(NESHNY_WEBGPU)
	static InterfaceCollapsible*	RenderShader		( InterfaceShaderViewer& data, QString name, WebGPUShader* shader, bool is_compute, QString search );
#endif
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
class Scrapbook2D : private BaseDebugRender {
public:

	inline static Scrapbook2D&	Singleton				( void ) { static Scrapbook2D instance; return instance; }

    static inline void			Line					( Vec2 a, Vec2 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a.ToVec3(), b.ToVec3(), color, on_top); }
	static inline void			Point					( Vec2 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos.ToVec3(), color, on_top); }
	static inline void			Point					( Vec2 pos, std::string text, Vec4 color, bool on_top = true ) { Singleton().AddPoint(pos.ToVec3(), text, color, on_top); }
	static inline void			Triangle				( Vec2 a, Vec2 b, Vec2 c, Vec4 color ) { Singleton().AddTriangle(a.ToVec3(), b.ToVec3(), c.ToVec3(), color); }
	static inline void          Circle					( Vec2 pos, double radius, Vec4 color, bool filled = false ) { Singleton().AddCircle(pos, radius, color, filled); }
	static inline void          Square					( Vec2 min_pos, Vec2 max_pos, Vec4 color, bool filled = false ) { Singleton().AddSquare(min_pos, max_pos, color, filled); }
	static inline void			Controls				( std::function<void(int width, int height)> controls ) { Singleton().m_Controls.push_back(controls); }

	static inline std::optional<Vec2>	MouseWorldPos	( void ) { return Singleton().m_LastMousePos; }
	static Token				ActivateRTT				( void );
#if defined(NESHNY_WEBGPU)
	static void					RenderPipeline			( WebGPURenderPipeline* pipeline ) { Singleton().m_RTT.Render(pipeline); }
#endif

	static void					RenderImGui				( InterfaceScrapbook2D& data ) { Singleton().IRenderImGui(data); }

private:

	void						IRenderImGui			( InterfaceScrapbook2D& data );

	RTT							m_RTT;
	int							m_Width = 32;
	int							m_Height = 32;
	std::optional<Vec2>			m_LastMousePos = std::nullopt;
	bool						m_NeedsReset = true;

	fMatrix4					m_CachedViewPerspective;

	std::vector<std::function<void(int width, int height)>>		m_Controls;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Scrapbook3D : public BaseDebugRender {
public:

	inline static Scrapbook3D&	Singleton					( void ) { static Scrapbook3D instance; return instance; }

#if defined(NESHNY_GL)
	static auto					ActivateRTT					( void );
#elif defined(NESHNY_WEBGPU)
#endif
	static fMatrix4				GetViewPerspectiveMatrix	( void ) { auto& self = Singleton(); return self.m_CachedViewPerspective; }

	static inline void			Line						( Vec3 a, Vec3 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a, b, color, on_top); }
	static inline void			Point						( Vec3 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos, color, on_top); }
	static inline void			Point						( Vec3 pos, std::string text, Vec4 color, bool on_top = true ) { Singleton().AddPoint(pos, text, color, on_top); }
	static inline void			Triangle					( Vec3 a, Vec3 b, Vec3 c, Vec4 color ) { Singleton().AddTriangle(a, b, c, color); }
	static inline void			Controls					( std::function<void(int width, int height)> controls ) { Singleton().m_Controls.push_back(controls); }

	static void					RenderImGui					( InterfaceScrapbook3D& data ) { Singleton().IRenderImGui(data); }

private:

	void						IRenderImGui				( InterfaceScrapbook3D& data );

#if defined(NESHNY_GL)
	RTT							m_RTT;
#elif defined(NESHNY_WEBGPU)
#endif
	int							m_Width = 32;
	int							m_Height = 32;
	bool						m_NeedsReset = true;

	fMatrix4					m_CachedViewPerspective;

	std::vector<std::function<void(int width, int height)>>		m_Controls;
};

} // namespace Neshny