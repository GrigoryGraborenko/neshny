////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define MINIMUM_UNIFORM_VECTOR_LENGTH 16

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class DebugGPU {

public:

	inline static DebugGPU&	Singleton			( void ) { static DebugGPU debug; return debug; }

	static inline int		GetMaxFrames		( void ) { return Singleton().m_MaxFrames; }
	static inline void		Checkpoint			( QString name, QString stage, class GLSSBO& buffer, MemberSpec::Type type, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, nullptr, type); }
	static inline void		Checkpoint			( QString name, QString stage, class GLSSBO& buffer, const StructInfo& info, int count = -1 ) { Singleton().ICheckpoint(name, stage, buffer, count, &info, MemberSpec::Type::T_UNKNOWN); }
	static inline void		Checkpoint			( QString stage, class GPUEntity& entity ) { Singleton().ICheckpoint(stage, entity); }

	void					RenderImGui			( InterfaceBufferViewer& data );

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

							DebugGPU			( void ) {}
							~DebugGPU			( void ) {}

	void					ICheckpoint			( QString name, QString stage, class GLSSBO& buffer, int count, const StructInfo* info, MemberSpec::Type type );
	void					ICheckpoint			( QString stage, class GPUEntity& entity );
	void					IStoreCheckpoint	( QString name, CheckpointData data, const StructInfo* info, MemberSpec::Type type );
	std::shared_ptr<GLSSBO>	IGetStoredFrameAt	( QString name, int tick, int& count );


	int						m_MaxFrames = 100;
	std::unordered_map<QString, CheckpointList>	m_Frames; // TODO: this doesn't really have to be a map, probably faster as a vector
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class ShaderViewEditor {
public:
	static void				RenderImGui			( InterfaceShaderViewer& data );// { Singleton().IRenderImGui(data); }
	static void				RenderShader		( InterfaceShaderViewer& data, QString name, GLShader* shader, bool is_compute );
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class CommonPipeline {

public:

								CommonPipeline		( GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines );
								~CommonPipeline		( void ) {}

protected:

	struct AddedUniformVector {
		QString					p_Name;
		int						p_NumItems;
		int						p_NumFloatsPerItem;
		std::vector<MemberSpec> p_Members;
		std::vector<float>		p_Data;
	};

	template <class T>
	void						AddUniformVectorBase( QString name, const std::vector<T>& items ) {
		
		int floats_per = sizeof(T) / sizeof(float);
		m_UniformVectors.push_back({ name, (int)items.size(), floats_per });
		AddedUniformVector& ref = m_UniformVectors.back();

		Serialiser<T> serializeFunc(ref.p_Members);
		meta::doForAllMembers<T>(serializeFunc);

		int num_floats = ref.p_NumFloatsPerItem * ref.p_NumItems;
		ref.p_Data.resize(num_floats);
		memcpy((unsigned char*)&(ref.p_Data[0]), (unsigned char*)&(items[0]), sizeof(float) * num_floats);
	}

	static QString				GetUniformVectorStructCode(
		AddedUniformVector& uniform
		,QStringList& insertion_uniforms
		,std::vector<std::pair<QString, int>>& integer_vars
		,std::vector<std::pair<QString, std::vector<float>*>>& vector_vars
	);

	GPUEntity&					m_Entity;
	QString						m_ShaderName;
	std::vector<QString>		m_ShaderDefines;
	bool						m_ReplaceMain = false;

	std::vector<AddedUniformVector>		m_UniformVectors;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class PipelineStage : public CommonPipeline {

public:
								PipelineStage		( GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, GLSSBO* control_buffer, GLSSBO* destruction_buffer = nullptr );
								~PipelineStage		( void ) {}

	PipelineStage&				AddEntity			( GPUEntity& entity );
	PipelineStage&				AddCreatableEntity	( GPUEntity& entity );
	PipelineStage&				AddInputOutputVar	( QString name, int* in_out );
	PipelineStage&				AddSSBO				( QString name, GLSSBO& ssbo, MemberSpec::Type array_type, bool read_only = true );
	void						Run					( std::optional<std::function<void(GLShader* program)>> pre_execute = std::nullopt );

	template <class T>
	PipelineStage&				AddUniformVector	( QString name, const std::vector<T>& items ) {
		AddUniformVectorBase(name, items);
		return *this;
	}

private:

	struct AddedEntity {
		GPUEntity&	p_Entity;
		bool		p_Creatable;
	};

	struct AddedSSBO {
		GLSSBO&					p_Buffer;
		QString					p_Name;
		MemberSpec::Type		p_Type;
		bool					p_ReadOnly;
	};

	struct AddedInOut {
		QString					p_Name;
		int*					p_Ptr;
	};

	GLSSBO*						m_ControlBuffer = nullptr;
	GLSSBO*						m_DestroyBuffer = nullptr;

	std::vector<AddedEntity>	m_Entities;
	std::vector<AddedSSBO>		m_SSBOs;
	std::vector<AddedInOut>		m_Vars;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class EntityRender : public CommonPipeline {

public:

								EntityRender		( GPUEntity& entity, QString shader_name, const std::vector<QString>& shader_defines = {} );
								~EntityRender		( void ) {}

	EntityRender&				AddSSBO				( QString name, GLSSBO& ssbo, MemberSpec::Type array_type );
	void						Render				( GLBuffer* buffer, std::optional<std::function<void(GLShader* program)>> pre_execute = std::nullopt );

	template <class T>
	EntityRender& AddUniformVector(QString name, const std::vector<T>& items) {
		AddUniformVectorBase(name, items);
		return *this;
	}

private:

	struct AddedSSBO {
		GLSSBO&					p_Buffer;
		QString					p_Name;
		MemberSpec::Type		p_Type;
	};

	std::vector<AddedSSBO>		m_SSBOs;
};