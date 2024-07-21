////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(NESHNY_WEBGPU)
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

	enum class BufferAccess {
		READ_ONLY
		,READ_WRITE
		,READ_WRITE_ATOMIC
	};

	struct AddedEntity {
		GPUEntity*	p_Entity;
		bool		p_Creatable = false;
	};

	struct OutputResults {
		bool GetValue(QString name, int& value) const;
		std::vector<std::pair<QString, int>> p_Results;
	};

	typedef WebGPUBuffer::AsyncToken<OutputResults> AsyncOutputResults;

	class Prepared {
		RunType						m_RunType;
		WebGPUPipeline*				m_Pipeline = nullptr;
		WebGPUBuffer*				m_UniformBuffer = nullptr;
		int							m_EntityBufferIndex = -1;

		SSBO*						m_ControlSSBO = nullptr; // not owned, do not delete
		GPUEntity*					m_Entity = nullptr; // not owned, do not delete
		RenderableBuffer*			m_Buffer = nullptr; // not owned, do not delete
		class BaseCache*			m_Cache = nullptr; // not owned, do not delete
		QStringList					m_VarNames;
		std::vector<AddedEntity>	m_Entities;
		bool						m_UsingRandom;
		bool						m_ReadRequired = false;
		std::shared_ptr<SSBO>		m_TemporaryFrame; // used for time travel feature

		struct DataVectorInfo {
			QString			m_Name;
			QString			m_CountVar;
			QString			m_OffsetVar;
			QString			m_NumVar;
			unsigned char*	m_Data = nullptr;
			int				m_SizeInts = 0;
			int				m_NumItems = 0;
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
					vect.m_NumItems = (int)items.size();
					break;
				}
			}
			return *this;
		}

		template <class UniformSpec>
		void						Render		( RTT& rtt, const UniformSpec& uniform ) { RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, 1, &rtt, std::nullopt); }
		template <class UniformSpec>
		void						Render		( RTT& rtt, const UniformSpec& uniform, int iterations ) { RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, iterations, &rtt, std::nullopt); }
		template <class UniformSpec>
		void						Render		( RTT& rtt, const UniformSpec& uniform, std::vector<std::pair<QString, int>>&& variables ) { RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), std::forward<std::vector<std::pair<QString, int>>>(variables), 1, &rtt, std::nullopt); }

		template <class UniformSpec>
		AsyncOutputResults			Run			( const UniformSpec& uniform ) { return RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, 1, nullptr, std::nullopt); }
		template <class UniformSpec>
		AsyncOutputResults			Run			( const UniformSpec& uniform, int iterations ) { return RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, iterations, nullptr, std::nullopt); }
		template <class UniformSpec>
		AsyncOutputResults			Run			( const UniformSpec& uniform, std::vector<std::pair<QString, int>>&& variables, std::optional<std::function<void(const OutputResults& results)>>&& callback ) { return RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), std::forward<std::vector<std::pair<QString, int>>>(variables), 1, nullptr, std::move(callback)); }
	private:
		AsyncOutputResults			RunInternal	( unsigned char* uniform, int uniform_bytes, std::vector<std::pair<QString, int>>&& variables, int iterations, RTT* rtt, std::optional<std::function<void(const OutputResults& results)>>&& callback );
	};

	struct AddedSSBO {
		SSBO&					p_Buffer;
		QString					p_Name;
		MemberSpec::Type		p_Type;
		BufferAccess			p_Access;
	};

	static PipelineStage ModifyEntity(GPUEntity& entity, QString shader_name, bool replace_main, std::vector<QString>&& shader_defines = {}, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_PROCESS, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderEntity(GPUEntity& entity, QString shader_name, bool replace_main, RenderableBuffer* buffer, WebGPUPipeline::RenderParams render_params, std::vector<QString>&& shader_defines = {}) {
		return PipelineStage(RunType::ENTITY_RENDER, &entity, buffer, nullptr, shader_name, replace_main, shader_defines, nullptr, 0, render_params);
	}
	static PipelineStage IterateEntity(GPUEntity& entity, QString shader_name, bool replace_main, std::vector<QString>&& shader_defines = {}, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_ITERATE, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderBuffer(QString shader_name, RenderableBuffer* buffer, WebGPUPipeline::RenderParams render_params, std::vector<QString>&& shader_defines = {}, SSBO* control_ssbo = nullptr) {
		return PipelineStage(RunType::BASIC_RENDER, nullptr, buffer, nullptr, shader_name, false, shader_defines, control_ssbo, 0, render_params);
	}
	static PipelineStage Compute(QString shader_name, int iterations, SSBO* control_ssbo, std::vector<QString>&& shader_defines = {}) {
		return PipelineStage(RunType::BASIC_COMPUTE, nullptr, nullptr, nullptr, shader_name, false, shader_defines, control_ssbo, iterations);
	}

								~PipelineStage		( void ) {}

	PipelineStage&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddCreatableEntity	( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddBuffer			( QString name, SSBO& ssbo, MemberSpec::Type array_type, BufferAccess access ) { m_SSBOs.push_back({ ssbo, name, array_type, access }); return *this; }
	PipelineStage&				AddCode				( QString code ) { m_ExtraCode += code; return *this; }

	PipelineStage&				AddInputOutputVar	( QString name ) { m_Vars.push_back({ name, true }); return *this; }
	PipelineStage&				AddInputVar			( QString name ) { m_Vars.push_back({ name, false }); return *this; }
	PipelineStage&				AddTexture			( QString name, const WebGPUTexture* texture ) { m_Textures.push_back({ name, texture }); return *this; }
	PipelineStage&				AddSampler			( QString name, const WebGPUSampler* sampler ) { m_Samplers.push_back({ name, sampler }); return *this; }

	template <class T>
	PipelineStage&				AddStructBuffer		( QString name, QString struct_name, SSBO& ssbo, BufferAccess access, bool is_array ) {
		std::vector<MemberSpec> members;
		Serialiser<T> serializeFunc(members);
		meta::doForAllMembers<T>(serializeFunc);
		m_StructBuffers.push_back({ ssbo, name, struct_name, std::move(members), access, is_array });
		return *this;
	}

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

	inline iVec3				GetLocalSize		( void ) const { return m_LocalSize; }
	inline void					SetLocalSize		( int x, int y, int z ) { m_LocalSize = iVec3(x, y, z); }

protected:

								PipelineStage		(	RunType type,
														GPUEntity* entity,
														RenderableBuffer* buffer,
														class BaseCache* cache,
														QString shader_name, bool replace_main,
														const std::vector<QString>& shader_defines,
														SSBO* control_ssbo = nullptr, int iterations = 0,
														WebGPUPipeline::RenderParams render_params = {} );

	struct AddedDataVector {
		QString					p_Name;
		int						p_NumIntsPerItem;
		std::vector<MemberSpec> p_Members;
	};
	struct AddedStructBuffer {
		SSBO&					p_Buffer;
		QString					p_Name;
		QString					p_StructName;
		std::vector<MemberSpec> p_Members;
		BufferAccess			p_Access;
		bool					p_IsArray;
	};

	std::unique_ptr<Prepared>	PrepareWithUniform	( const std::vector<MemberSpec>& unform_members );

	struct AddedInOut {
		QString		p_Name;
		bool		p_ReadBack = true;
	};

	struct AddedTexture {
		QString					p_Name;
		const WebGPUTexture*	p_Tex;
	};
	struct AddedSampler {
		QString					p_Name;
		const WebGPUSampler*	p_Sampler;
	};

	static QString				GetDataVectorStructCode	( const AddedDataVector& data_vect, bool read_only );
	RunType							m_RunType;
	GPUEntity*						m_Entity = nullptr;
	RenderableBuffer*				m_Buffer = nullptr;
	BaseCache*						m_Cache = nullptr;
	QString							m_ShaderName;
	std::vector<QString>			m_ShaderDefines;
	bool							m_ReplaceMain = false;
	iVec3							m_LocalSize = iVec3(8, 8, 8);
	int								m_Iterations = 0;
	SSBO*							m_ControlSSBO = nullptr;
	QString							m_ExtraCode;
	WebGPUPipeline::RenderParams	m_RenderParams;

	std::vector<AddedEntity>		m_Entities;
	std::vector<AddedSSBO>			m_SSBOs;
	std::vector<AddedInOut>			m_Vars;
	std::vector<AddedTexture>		m_Textures;
	std::vector<AddedSampler>		m_Samplers;
	std::vector<AddedStructBuffer>	m_StructBuffers;
	std::vector<AddedDataVector>	m_DataVectors;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseCache {
public:
	virtual void Bind(PipelineStage& target_stage) = 0;
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

	virtual void				Bind			( PipelineStage& target_stage ) override;

private:

	GPUEntity&					m_Entity;
	QString						m_PosName;

	SSBO						m_GridIndices;
	SSBO						m_GridItems;
	SSBO						m_Uniform;

	iVec2						m_GridSize;
	Vec2						m_GridMin;
	Vec2						m_GridMax;

	std::unique_ptr<PipelineStage::Prepared> m_CacheIndex;
	std::unique_ptr<PipelineStage::Prepared> m_CacheAlloc;
	std::unique_ptr<PipelineStage::Prepared> m_CacheFill;
};

struct Grid2DCacheUniform {
	iVec2	p_GridSize;
	fVec2	p_GridMin;
	fVec2	p_GridMax;
};

} // namespace Neshny

namespace meta {
	template<> inline auto registerMembers<Neshny::Grid2DCacheUniform>() {
		return members(
			member("GridSize", &Neshny::Grid2DCacheUniform::p_GridSize),
			member("GridMin", &Neshny::Grid2DCacheUniform::p_GridMin),
			member("GridMax", &Neshny::Grid2DCacheUniform::p_GridMax)
		);
	}
}
#endif
