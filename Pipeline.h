////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class PipelineStage {

public:

	enum class RunType {
		ENTITY_PROCESS,
		ENTITY_RENDER,
		ENTITY_ITERATE,
		BASIC_RENDER,
		BASIC_COMPUTE
	};

#if defined(NESHNY_WEBGPU)
	class Prepared {
		RunType						m_RunType;
		WebGPUPipeline*				m_Pipeline = nullptr;
		WebGPUBuffer*				m_UniformBuffer = nullptr;
		int							m_EntityBufferIndex = -1;

		SSBO*						m_ControlSSBO = nullptr; // not owned, do not delete
		GPUEntity*					m_Entity = nullptr; // not owned, do not delete
		RenderableBuffer*			m_Buffer = nullptr; // not owned, do not delete
		QStringList					m_VarNames;
		std::vector<GPUEntity*>		m_Entities;
		bool						m_UsingRandom;

		struct DataVectorInfo {
			QString			m_Name;
			QString			m_CountVar;
			QString			m_OffsetVar;
			unsigned char*	m_Data = nullptr;
			int				m_SizeInts = 0;
		};
		std::vector<DataVectorInfo>	m_DataVectors;

		friend class PipelineStage;
	public:

							~Prepared	( void );

		template <class T>
		Prepared&			WithDataVector	( QString name, const std::vector<T>& items ) {
			for (auto& vect : m_DataVectors) {
				if (vect.m_Name == name) {
					vect.m_Data = (unsigned char*)items.data();
					vect.m_SizeInts = ((int)items.size() * sizeof(T)) / sizeof(int);
					break;
				}
			}
			return *this;
		}

		template <class UniformSpec>
		void				Render		( RTT& rtt, const UniformSpec& uniform ) { Run((unsigned char*)&uniform, sizeof(UniformSpec), {}, 1, &rtt); }
		template <class UniformSpec>
		void				Render		( RTT& rtt, const UniformSpec& uniform, int iterations ) { Run((unsigned char*)&uniform, sizeof(UniformSpec), {}, iterations, &rtt); }
		template <class UniformSpec>
		void				Render		( RTT& rtt, const UniformSpec& uniform, std::vector<std::pair<QString, int*>>&& variables ) { Run((unsigned char*)&uniform, sizeof(UniformSpec), std::forward<std::vector<std::pair<QString, int*>>>(variables), 1, &rtt); }

		template <class UniformSpec>
		void				Run			( const UniformSpec& uniform ) { Run((unsigned char*)&uniform, sizeof(UniformSpec), {}, 1, nullptr); }
		template <class UniformSpec>
		void				Run			( const UniformSpec& uniform, int iterations ) { Run((unsigned char*)&uniform, sizeof(UniformSpec), {}, iterations, nullptr); }
		template <class UniformSpec>
		void				Run			( const UniformSpec& uniform, std::vector<std::pair<QString, int*>>&& variables ) { Run((unsigned char*)&uniform, sizeof(UniformSpec), std::forward<std::vector<std::pair<QString, int*>>>(variables), 1, nullptr); }
		void				Run			( unsigned char* uniform, int uniform_bytes, std::vector<std::pair<QString, int*>>&& variables, int iterations, RTT* rtt );
	};
#endif

	struct AddedSSBO {
		SSBO& p_Buffer;
		QString					p_Name;
		MemberSpec::Type		p_Type;
#if defined(NESHNY_WEBGPU)
		WGPUShaderStageFlags	p_Flags;
#endif
		bool					p_ReadOnly = true;
	};

	static PipelineStage ModifyEntity(GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines = {}, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_PROCESS, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderEntity(GPUEntity& entity, QString shader_name, bool replace_main, RenderableBuffer* buffer, const std::vector<QString>& shader_defines = {}) {
		return PipelineStage(RunType::ENTITY_RENDER, &entity, buffer, nullptr, shader_name, replace_main, shader_defines);
	}
	static PipelineStage IterateEntity(GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines = {}, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_ITERATE, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderBuffer(QString shader_name, RenderableBuffer* buffer, const std::vector<QString>& shader_defines = {}, SSBO* control_ssbo = nullptr) {
		return PipelineStage(RunType::BASIC_RENDER, nullptr, buffer, nullptr, shader_name, false, shader_defines, control_ssbo);
	}
	static PipelineStage Compute(QString shader_name, int iterations, SSBO* control_ssbo, const std::vector<QString>& shader_defines = {}) {
		return PipelineStage(RunType::BASIC_COMPUTE, nullptr, nullptr, nullptr, shader_name, false, shader_defines, control_ssbo, iterations);
	}

								~PipelineStage		( void ) {}

	PipelineStage&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddCreatableEntity	( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddCode				( QString code ) { m_ExtraCode += code; return *this; }
#if defined(NESHNY_GL)
	PipelineStage&				AddInputOutputVar	( QString name, int* in_out ) { m_Vars.push_back({ name, in_out }); return *this; }
	PipelineStage&				AddSSBO				( QString name, SSBO& ssbo, MemberSpec::Type array_type, bool read_only = true ) { m_SSBOs.push_back({ ssbo, name, array_type, read_only }); return *this; }
	PipelineStage&				AddTexture			( QString name, GLuint texture ) { m_Textures.push_back({ name, texture }); return *this; }
	void						Run					( std::optional<std::function<void(Shader* program)>> pre_execute = std::nullopt );

	template <class T>
	PipelineStage& AddDataVector(QString name, const std::vector<T>& items) {
		AddDataVectorBase(name, items);
		return *this;
	}

#elif defined(NESHNY_WEBGPU)
	PipelineStage&				AddInputOutputVar	( QString name ) { m_Vars.push_back({ name }); return *this; }
	PipelineStage&				AddBuffer			( QString name, SSBO& ssbo, WGPUShaderStageFlags flags, MemberSpec::Type array_type, bool read_only = true ) { m_SSBOs.push_back({ ssbo, name, array_type, flags, read_only }); return *this; }
	PipelineStage&				AddTexture			( QString name, WebGPUTexture* texture ) { m_Textures.push_back({ name, texture }); return *this; }
	PipelineStage&				AddSampler			( QString name, WebGPUSampler* sampler ) { m_Samplers.push_back({ name, sampler }); return *this; }

	template <class UniformSpec>
	std::unique_ptr<Prepared>	Prepare				( void ) {
		std::vector<MemberSpec> uniform_members;
		Serialiser<UniformSpec> serializeFunc(uniform_members);
		meta::doForAllMembers<UniformSpec>(serializeFunc);
		return PrepareWithUniform(uniform_members);
	}

	template <class T>
	PipelineStage& AddDataVector(QString name) {
		int ints_per = sizeof(T) / sizeof(int);
		m_DataVectors.push_back({ name, ints_per });
		AddedDataVector& ref = m_DataVectors.back();
		Serialiser<T> serializeFunc(ref.p_Members);
		meta::doForAllMembers<T>(serializeFunc);
		return *this;
	}

#endif


	inline iVec3				GetLocalSize		( void ) const { return m_LocalSize; }
	inline void					SetLocalSize		( int x, int y, int z ) { m_LocalSize = iVec3(x, y, z); }

protected:

								PipelineStage		( RunType type, GPUEntity* entity, RenderableBuffer* buffer, class BaseCache* cache, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, SSBO* control_ssbo = nullptr, int iterations = 0);

#if defined(NESHNY_GL)
	struct AddedDataVector {
		QString					p_Name;
		int						p_NumItems;
		int						p_NumIntsPerItem;
		std::vector<MemberSpec> p_Members;
		std::vector<int>		p_Data;
	};
#elif defined(NESHNY_WEBGPU)
	struct AddedDataVector {
		QString					p_Name;
		int						p_NumIntsPerItem;
		std::vector<MemberSpec> p_Members;
	};
	std::unique_ptr<Prepared>	PrepareWithUniform	( const std::vector<MemberSpec>& unform_members );
#endif

	struct AddedEntity {
		GPUEntity&	p_Entity;
		bool		p_Creatable = false;
	};

	struct AddedInOut {
		QString		p_Name;
#if defined(NESHNY_GL)
		int*		p_Ptr;
#endif
	};

#if defined(NESHNY_GL)
	struct AddedTexture {
		QString				p_Name;
		GLuint				p_Tex;
	};
#elif defined(NESHNY_WEBGPU)
	struct AddedTexture {
		QString				p_Name;
		WebGPUTexture*		p_Tex;
	};
	struct AddedSampler {
		QString				p_Name;
		WebGPUSampler*		p_Sampler;
	};
#endif

#if defined(NESHNY_GL)
	template <class T>
	void						AddDataVectorBase(QString name, const std::vector<T>& items) {
		if (items.empty()) {
			return;
		}
		int ints_per = sizeof(T) / sizeof(int);
		m_DataVectors.push_back({ name, (int)items.size(), ints_per });
		AddedDataVector& ref = m_DataVectors.back();

		Serialiser<T> serializeFunc(ref.p_Members);
		meta::doForAllMembers<T>(serializeFunc);

		int num_ints = ref.p_NumIntsPerItem * ref.p_NumItems;
		ref.p_Data.resize(num_ints);
		memcpy((unsigned char*)&(ref.p_Data[0]), (unsigned char*)&(items[0]), sizeof(int) * num_ints);
	}

	static QString				GetDataVectorStructCode(
		const AddedDataVector& data_vect
		,QStringList& insertion_uniforms
		,std::vector<std::pair<QString, int>>& integer_vars
		,int offset
	);
#elif defined(NESHNY_WEBGPU)
	static QString				GetDataVectorStructCode	( const AddedDataVector& data_vect, QString& count_var, QString& offset_var );
#endif
	RunType						m_RunType;
	GPUEntity*					m_Entity = nullptr;
	RenderableBuffer*			m_Buffer = nullptr;
	QString						m_ShaderName;
	std::vector<QString>		m_ShaderDefines;
	bool						m_ReplaceMain = false;
	iVec3						m_LocalSize = iVec3(8, 8, 8);
	int							m_Iterations = 0;
	SSBO*						m_ControlSSBO = nullptr;
	QString						m_ExtraCode;

	std::vector<AddedEntity>	m_Entities;
	std::vector<AddedSSBO>		m_SSBOs;
	std::vector<AddedInOut>		m_Vars;
	std::vector<AddedTexture>	m_Textures;
#if defined(NESHNY_WEBGPU)
	std::vector<AddedSampler>	m_Samplers;
#endif

	std::vector<AddedDataVector>		m_DataVectors;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseCache {
public:
	virtual QString Bind(std::vector<PipelineStage::AddedSSBO>& ssbos ) = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class QueryEntities {

public:

								QueryEntities		( GPUEntity& entity );

	QueryEntities&				ByNearestPosition	( QString param_name, fVec2 pos );
	QueryEntities&				ByNearestPosition	( QString param_name, fVec3 pos );
	QueryEntities&				ById				( int id );

	std::optional<int>			Run					( void ) {
		int index = ExecuteQuery();
		if (index < 0) {
			return std::nullopt;
		}
		return index;
	}

	template<typename T>
	std::optional<T>			Run					( int* index_result = nullptr ) {
		int index = ExecuteQuery();
		if (index < 0) {
			return std::nullopt;
		}
		if (index_result) {
			*index_result = index;
		}
		return m_Entity.ExtractSingle<T>(index);
	}

private:

	int						ExecuteQuery		( void );

	enum class QueryType {
		Position2D
		,Position3D
		,ID
	};

	GPUEntity&							m_Entity;
	QueryType							m_Query;
	QString								m_ParamName;
	std::variant<fVec2, fVec3, int>		m_QueryParam;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Grid2DCache : public BaseCache {

public:

								Grid2DCache		( GPUEntity& entity, QString pos_name );
	void						GenerateCache	( iVec2 grid_size, Vec2 grid_min, Vec2 grid_max );

	virtual QString				Bind			( std::vector<PipelineStage::AddedSSBO>& ssbos ) override;

private:

	GPUEntity&					m_Entity;
	QString						m_PosName;

	SSBO						m_GridIndices;
	SSBO						m_GridItems;

	iVec2						m_GridSize;
	Vec2						m_GridMin;
	Vec2						m_GridMax;
};

} // namespace Neshny