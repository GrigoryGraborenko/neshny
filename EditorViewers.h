////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

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

										DebugTiming		( const char* label );
										~DebugTiming	( void );

	static QStringList					Report			( void );

	static std::vector<TimingInfo>&		GetTimings		( void ) { static std::vector<TimingInfo> timings = {}; return timings; }

	static qint64						MainLoopTimer	( void ) { static QElapsedTimer timer; qint64 time = timer.nsecsElapsed(); timer.restart(); return time; }

private:

	QElapsedTimer		m_Timer;
	const char*			m_Label;
};

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

	void										IRender3DDebug		( const QMatrix4x4& view_perspective, int width, int height, Triple offset, double scale, double point_size = 1.0 );
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

	static inline void			Checkpoint			( QString name, QString stage, class GLSSBO& buffer, MemberSpec::Type type, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, nullptr, type); }
	static inline void			Checkpoint			( QString name, QString stage, class GLSSBO& buffer, const StructInfo& info, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, &info, MemberSpec::Type::T_UNKNOWN); }
	static inline void			Checkpoint			( QString stage, class GPUEntity& entity ) { Singleton().ICheckpoint(stage, entity); }

	void						RenderImGui			( InterfaceBufferViewer& data );

	static inline std::shared_ptr<GLSSBO> GetStoredFrameAt	( QString name, int tick, int& count ) { return Singleton().IGetStoredFrameAt(name, tick, count); }

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
class Scrapbook2D : private BaseDebugRender {
public:

	inline static Scrapbook2D&	Singleton				( void ) { static Scrapbook2D instance; return instance; }

    static inline void			Line					( Vec2 a, Vec2 b, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a.ToTriple(), b.ToTriple(), color, on_top); }
	static inline void			Point					( Vec2 pos, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos.ToTriple(), color, on_top); }
	static inline void			Point					( Vec2 pos, std::string text, QVector4D color, bool on_top = true ) { Singleton().AddPoint(pos.ToTriple(), text, color, on_top); }
	static inline void			Triangle				( Vec2 a, Vec2 b, Vec2 c, QVector4D color ) { Singleton().AddTriangle(a.ToTriple(), b.ToTriple(), c.ToTriple(), color); }
	static inline void			Controls				( std::function<void(int width, int height)> controls ) { Singleton().m_Controls.push_back(controls); }

	static inline std::optional<Vec2>	MouseWorldPos	( void ) { return Singleton().m_LastMousePos; }
	static auto					ActivateRTT				( void );

	static void					RenderImGui				( InterfaceScrapbook2D& data ) { Singleton().IRenderImGui(data); }

private:

	void						IRenderImGui			( InterfaceScrapbook2D& data );

	RTT							m_RTT;
	int							m_Width = 32;
	int							m_Height = 32;
	std::optional<Vec2>			m_LastMousePos = std::nullopt;
	bool						m_NeedsReset = true;

	QMatrix4x4					m_CachedViewPerspective;

	std::vector<std::function<void(int width, int height)>>		m_Controls;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Scrapbook3D : public BaseDebugRender {
public:

	inline static Scrapbook3D&	Singleton					( void ) { static Scrapbook3D instance; return instance; }

	static auto					ActivateRTT					( void );
	static QMatrix4x4			GetViewPerspectiveMatrix	( void ) { auto& self = Singleton(); return self.m_CachedViewPerspective; }

	static inline void			Line						( Triple a, Triple b, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddLine(a, b, color, on_top); }
	static inline void			Point						( Triple pos, QVector4D color = QVector4D(1.0, 1.0, 1.0, 1.0), bool on_top = false ) { Singleton().AddPoint(pos, color, on_top); }
	static inline void			Point						( Triple pos, std::string text, QVector4D color, bool on_top = true ) { Singleton().AddPoint(pos, text, color, on_top); }
	static inline void			Triangle					( Triple a, Triple b, Triple c, QVector4D color ) { Singleton().AddTriangle(a, b, c, color); }
	static inline void			Controls					( std::function<void(int width, int height)> controls ) { Singleton().m_Controls.push_back(controls); }

	static void					RenderImGui					( InterfaceScrapbook3D& data ) { Singleton().IRenderImGui(data); }

private:

	void						IRenderImGui				( InterfaceScrapbook3D& data );

	RTT							m_RTT;
	int							m_Width = 32;
	int							m_Height = 32;
	bool						m_NeedsReset = true;

	QMatrix4x4					m_CachedViewPerspective;

	std::vector<std::function<void(int width, int height)>>		m_Controls;
};
