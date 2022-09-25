////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BufferViewer {

public:

	inline static BufferViewer&	Singleton			( void ) { static BufferViewer debug; return debug; }

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
class Scrapbook {
public:
	static void						RenderImGui			( InterfaceScrapbook2D& data );
	static void						RenderImGui			( InterfaceScrapbook3D& data );
};
