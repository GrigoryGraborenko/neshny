////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_EDITOR_VIEWERS

namespace Neshny {

using GPUVariable = std::variant<int, unsigned int, float, fVec2, fVec3, fVec4, fMatrix3, fMatrix4>;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseSimpleRender {

public:

    inline void									AddLine             ( Vec3 a, Vec3 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { m_Lines.push_back(SimpleLine{a, b, color, on_top}); }
    inline void									AddPoint            ( Vec3 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { m_Points.push_back(SimplePoint{pos, std::string(""), color, on_top}); }
    inline void									AddPoint            ( Vec3 pos, std::string_view text, Vec4 color, bool on_top = true ) { m_Points.push_back(SimplePoint{pos, std::string(text), color, on_top}); }
    inline void									AddTriangle         ( Vec3 a, Vec3 b, Vec3 c, Vec4 color ) { m_Triangles.push_back(SimpleTriangle{a, b, c, color}); }
    inline void									AddCircle			( Vec3 a, double radius, Vec4 color, bool filled = false ) { m_Circles.push_back(SimpleCircle{a, radius, color, filled}); }
    inline void									AddSquare			( Vec3 min_pos, Vec3 max_pos, Vec4 color, bool filled = false ) { m_Squares.push_back(SimpleSquare{min_pos, max_pos, color, filled}); }
    inline void									AddTexture			( Vec3 min_pos, Vec3 max_pos, std::string_view filename ) { m_Textures.push_back(SimpleTexture{min_pos, max_pos, std::string(filename) }); }

protected:

	struct SimplePoint {
		Vec3 p_Pos;
		std::string p_Str;
		Vec4 p_Col;
		bool p_OnTop;
	};

	struct SimpleLine {
		Vec3 p_A, p_B; Vec4 p_Col; bool p_OnTop;
	};

	struct SimpleTriangle {
		Vec3 p_A, p_B, p_C;
		Vec4 p_Col;
	};

	struct SimpleCircle {
		Vec3 p_Pos;
		double p_Radius;
		Vec4 p_Col;
		bool p_Filled = false;
	};

	struct SimpleSquare {
		Vec3 p_MinPos;
		Vec3 p_MaxPos;
		Vec4 p_Col;
		bool p_Filled = false;
	};

	struct SimpleText {
		std::string p_Text;
		Vec2 p_Pos;
		Vec4 p_Col;
	};

	struct SimpleTexture {
		Vec3 p_MinPos;
		Vec3 p_MaxPos;
		std::string p_Filename;
	};

#if defined(NESHNY_WEBGPU)
	void										IRender				( WebGPURTT& rtt, const Matrix4& view_perspective, int width, int height, Vec3 offset, double scale, double point_size = 1.0 );
#elif defined(NESHNY_GL)
	void										IRender				( const Matrix4& view_perspective, int width, int height, Vec3 offset, double scale, double point_size = 1.0 );
#endif
	inline void									IClear		        ( void ) { m_Lines.clear(); m_Points.clear(); m_Triangles.clear(); m_Circles.clear(); m_Squares.clear(); m_Textures.clear(); }

												BaseSimpleRender	( void );
												~BaseSimpleRender	( void );

    std::vector<SimpleLine>						m_Lines;
    std::vector<SimplePoint>					m_Points;
    std::vector<SimpleTriangle>					m_Triangles;
	std::vector<SimpleCircle>					m_Circles;
	std::vector<SimpleSquare>					m_Squares;
	std::vector<SimpleTexture>					m_Textures;
	
#if defined(NESHNY_WEBGPU)
	WebGPUBuffer*								m_Uniforms = nullptr;
	WebGPURenderBuffer							m_LineBuffer;
	WebGPURenderBuffer							m_TriangleBuffer;
	WebGPUPipeline								m_LinePipline;
	WebGPUPipeline								m_TrianglePipline;

	WebGPUPipeline								m_CirclePipline;
	WebGPUBuffer*								m_CircleBuffer = nullptr;

	WebGPUPipeline								m_TexturePipline;
	std::vector<WebGPUBuffer*>					m_PreviousFrameBuffers;

#endif

};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class SimpleRender2D : private BaseSimpleRender {

public:

	static inline SimpleRender2D&				Singleton			( void ) { static SimpleRender2D instance; return instance; }

#if defined(NESHNY_WEBGPU)
	static inline void							Render				( WebGPURTT& rtt, const Matrix4& view_perspective, int width, int height, Vec2 offset = Vec2(0, 0), double scale = 1.0f) { Singleton().IRender(rtt, view_perspective, width, height, offset.ToVec3(), scale); }
#else
	static inline void							Render				( const Matrix4& view_perspective, int width, int height, Vec3 offset = Vec3(0, 0, 0), double scale = 1.0f) { Singleton().IRender(view_perspective, width, height, offset, scale); }
#endif

    static inline void					        Clear		        ( void ) { Singleton().IClear(); }

	static inline void                          Line				( Vec2 a, Vec2 b, Vec4 color, double z_order = 0.0 ) { Singleton().AddLine(a.ToVec3(z_order), b.ToVec3(z_order), color); }
	static inline void                          Point				( Vec2 pos, Vec4 color, double z_order = 0.0 ) { Singleton().AddPoint(pos.ToVec3(z_order), color); }
	static inline void                          Point				( Vec2 pos, std::string text, Vec4 color, double z_order = 0.0 ) { Singleton().AddPoint(pos.ToVec3(z_order), text, color); }
	static inline void                          Triangle			( Vec2 a, Vec2 b, Vec2 c, Vec4 color, double z_order = 0.0 ) { Singleton().AddTriangle(a.ToVec3(z_order), b.ToVec3(z_order), c.ToVec3(z_order), color); }
	static inline void                          Circle				( Vec2 pos, double radius, Vec4 color, bool filled = false, double z_order = 0.0 ) { Singleton().AddCircle(pos.ToVec3(z_order), radius, color, filled); }
	static inline void                          Square				( Vec2 min_pos, Vec2 max_pos, Vec4 color, bool filled = false, double z_order = 0.0 ) { Singleton().AddSquare(min_pos.ToVec3(z_order), max_pos.ToVec3(z_order), color, filled); }
	static inline void                          Texture				( Vec2 min_pos, Vec2 max_pos, std::string_view filename, double z_order = 0.0 ) { Singleton().AddTexture(min_pos.ToVec3(z_order), max_pos.ToVec3(z_order), filename); }
protected:
												SimpleRender2D		( void ) {}
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class SimpleRender3D : private BaseSimpleRender {

public:

	static inline SimpleRender3D&				Singleton			( void ) { static SimpleRender3D instance; return instance; }

#if defined(NESHNY_WEBGPU)
	static inline void							Render				( WebGPURTT& rtt, const Matrix4& view_perspective, int width, int height, Vec3 offset = Vec3(0, 0, 0), double scale = 1.0f) { Singleton().IRender(rtt, view_perspective, width, height, offset, scale); }
#else
	static inline void							Render				( const Matrix4& view_perspective, int width, int height, Vec3 offset = Vec3(0, 0, 0), double scale = 1.0f) { Singleton().IRender(view_perspective, width, height, offset, scale); }
#endif

    static inline void					        Clear		        ( void ) { Singleton().IClear(); }

	static inline void                          Line				( Vec3 a, Vec3 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a, b, color, on_top); }
	static inline void                          Point				( Vec3 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos, color, on_top); }
	static inline void                          Point				( Vec3 pos, std::string text, Vec4 color, bool on_top = true ) { Singleton().AddPoint(pos, text, color, on_top); }
	static inline void                          Triangle			( Vec3 a, Vec3 b, Vec3 c, Vec4 color ) { Singleton().AddTriangle(a, b, c, color); }
protected:
												SimpleRender3D		( void ) {}
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class InfoViewer {
public:

	inline static InfoViewer&					Singleton			( void ) { static InfoViewer instance; return instance; }

	inline static void							RenderImGui			( InterfaceInfoViewer& data ) { Singleton().IRenderImGui(data); }

	inline static void							LoopTime			( TimerNanos nanos ) { Singleton().ILoopTime(nanos); }
	inline static void							ClearLoopTime		( void ) { Singleton().IClearLoopTime(); }

protected:
												InfoViewer			( void ) { IClearLoopTime(); }

	void										ILoopTime			( TimerNanos nanos );
	void										IRenderImGui		( InterfaceInfoViewer& data );
	void										IClearLoopTime		( void );

	std::vector<std::pair<double, int>>			m_LoopHistogram;
	int											m_LoopHistogramOverflow;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class LogViewer {
public:

	struct LogEntry {
		std::string	p_TimeStr;
		std::string	p_Message;
		ImVec4		p_Color;
	};

	inline static LogViewer&					Singleton			( void ) { static LogViewer instance; return instance; }

	inline static void							RenderImGui			( InterfaceLogViewer& data ) { Singleton().IRenderImGui(data); }

	inline void									Clear				( void ) { m_Logs.clear(); };
	inline void									Log					( LogEntry&& entry ) { std::lock_guard<std::mutex> lock(m_Lock); m_Logs.emplace_back(entry); }

protected:

												LogViewer			( void ) {}

	void										IRenderImGui		( InterfaceLogViewer& data );

	std::mutex									m_Lock;
	std::vector<LogEntry>						m_Logs;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BufferViewer {

public:

	inline static BufferViewer&	Singleton			( void ) { static BufferViewer instance; return instance; }

	static inline void					Checkpoint			( std::string_view name, std::string_view stage, SSBO& buffer, MemberSpec::Type type, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, nullptr, type); }
	static inline void					Checkpoint			( std::string_view name, std::string_view stage, SSBO& buffer, const StructInfo& info, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, &info, MemberSpec::Type::T_UNKNOWN); }
	static inline void					Checkpoint			( std::string_view stage, GPUEntity& entity ) { Singleton().ICheckpoint(stage, entity); }

	void								RenderImGui			( InterfaceBufferViewer& data );

	static inline std::shared_ptr<SSBO> GetStoredFrameAt	( std::string_view name, int tick, int& count ) { return Singleton().IGetStoredFrameAt(name, tick, count); }

	static inline void					Highlight			( std::string_view name, int id ) { Singleton().IHighlight(name, id); }
	static inline void					ClearHighlight		( void ) { Singleton().IHighlight(std::string(), -1); }

	static std::optional<GPUVariable>	GetHoveredValue		( void ) { return Singleton().m_Hovered; }

protected:

	struct CheckpointData {
		std::string							p_Stage;
		std::string							p_Info;
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

	void					ICheckpoint			( std::string_view name, std::string_view stage, SSBO& buffer, int count, const StructInfo* info, MemberSpec::Type type );
	void					ICheckpoint			( std::string_view stage, GPUEntity& entity );
	std::shared_ptr<SSBO>	IGetStoredFrameAt	( std::string_view name, int tick, int& count );

	void					IStoreCheckpoint	( std::string name, CheckpointData data, const StructInfo* info, MemberSpec::Type type );
	void					IHighlight			( std::string_view name, int id ) { m_HighlightName = name; m_HighlightID = id; }
	void					UpdateCheckpoint	( std::string_view name, int tick, int new_count, std::string_view new_info, std::shared_ptr<unsigned char[]> new_data );

	std::unordered_map<std::string, CheckpointList>	m_Frames; // TODO: this doesn't really have to be a map, probably faster as a vector
	std::string									m_HighlightName;
	int											m_HighlightID = -1;
	std::optional<GPUVariable>					m_Hovered = std::nullopt;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class ShaderViewer {
public:
	static void						RenderImGui			( InterfaceShaderViewer& data );// { Singleton().IRenderImGui(data); }
#if defined(NESHNY_GL)
	static InterfaceCollapsible*	RenderShader		( InterfaceShaderViewer& data, std::string_view name, GLShader* shader, bool is_compute, std::string_view search );
#elif defined(NESHNY_WEBGPU)
	static InterfaceCollapsible*	RenderShader		( InterfaceShaderViewer& data, std::string_view name, WebGPUShader* shader, bool is_compute, std::string_view search );
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
class Scrapbook2D : private BaseSimpleRender {
public:

	inline static Scrapbook2D&	Singleton				( void ) { static Scrapbook2D instance; return instance; }

    static inline void			Line					( Vec2 a, Vec2 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), double z_order = 0.0 ) { Singleton().AddLine(a.ToVec3(z_order), b.ToVec3(z_order), color, false); }
	static inline void			Point					( Vec2 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), double z_order = 0.0 ) { Singleton().AddPoint(pos.ToVec3(z_order), color, false); }
	static inline void			Point					( Vec2 pos, std::string text, Vec4 color, double z_order = 0.0 ) { Singleton().AddPoint(pos.ToVec3(), text, color, false); }
	static inline void			Triangle				( Vec2 a, Vec2 b, Vec2 c, Vec4 color, double z_order = 0.0 ) { Singleton().AddTriangle(a.ToVec3(z_order), b.ToVec3(z_order), c.ToVec3(z_order), color); }
	static inline void          Circle					( Vec2 pos, double radius, Vec4 color, bool filled = false, double z_order = 0.0 ) { Singleton().AddCircle(pos.ToVec3(z_order), radius, color, filled); }
	static inline void          Square					( Vec2 min_pos, Vec2 max_pos, Vec4 color, bool filled = false, double z_order = 0.0) { Singleton().AddSquare(min_pos.ToVec3(z_order), max_pos.ToVec3(z_order), color, filled); }
	static inline void          Text					( std::string_view text, Vec2 pos, Vec4 color ) { Singleton().AddText(text, pos, color); }
	static inline void			Controls				( std::function<void(int width, int height)> controls ) { Singleton().m_Controls.push_back(controls); }

	static inline std::optional<Vec2>	MouseWorldPos	( void ) { return Singleton().m_LastMousePos; }
	static Token				ActivateRTT				( void );
#if defined(NESHNY_WEBGPU)
	static void					RenderPipeline			( WebGPUPipeline* pipeline ) { Singleton().m_RTT.Render(pipeline); }
#endif

	static void					RenderImGui				( InterfaceScrapbook2D& data ) { Singleton().IRenderImGui(data); }

    static inline void			Clear					( void ) { Singleton().IClear(); Singleton().m_Texts.clear(); }

private:

	void						AddText					( std::string_view text, Vec2 pos, Vec4 color ) { m_Texts.push_back(SimpleText{ std::string(text), pos, color }); }
	void						IRenderImGui			( InterfaceScrapbook2D& data );

	RTT							m_RTT;
	int							m_Width = 32;
	int							m_Height = 32;
	std::optional<Vec2>			m_LastMousePos = std::nullopt;
	bool						m_NeedsReset = true;

	Matrix4						m_CachedViewPerspective;

	std::vector<std::function<void(int width, int height)>>		m_Controls;
	std::vector<SimpleText>										m_Texts;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Scrapbook3D : public BaseSimpleRender {
public:

	inline static Scrapbook3D&	Singleton					( void ) { static Scrapbook3D instance; return instance; }

	static Token				ActivateRTT					( void );
	static Matrix4				GetViewPerspectiveMatrix	( void ) { auto& self = Singleton(); return self.m_CachedViewPerspective; }

	static inline void			Line						( Vec3 a, Vec3 b, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a, b, color, on_top); }
	static inline void			Point						( Vec3 pos, Vec4 color = Vec4(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos, color, on_top); }
	static inline void			Point						( Vec3 pos, std::string text, Vec4 color, bool on_top = true ) { Singleton().AddPoint(pos, text, color, on_top); }
	static inline void			Triangle					( Vec3 a, Vec3 b, Vec3 c, Vec4 color ) { Singleton().AddTriangle(a, b, c, color); }
	static inline void			Controls					( std::function<void(int width, int height)> controls ) { Singleton().m_Controls.push_back(controls); }

	static void					RenderImGui					( InterfaceScrapbook3D& data ) { Singleton().IRenderImGui(data); }

    static inline void			Clear						( void ) { Singleton().IClear(); }

private:

	void						IRenderImGui				( InterfaceScrapbook3D& data );

	RTT							m_RTT;
	int							m_Width = 32;
	int							m_Height = 32;
	bool						m_NeedsReset = true;

	Matrix4						m_CachedViewPerspective;

	std::vector<std::function<void(int width, int height)>>		m_Controls;
};

} // namespace Neshny