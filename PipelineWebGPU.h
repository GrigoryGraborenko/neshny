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
		bool GetValue(std::string_view name, int& value) const;
		std::vector<std::pair<std::string, int>> p_Results;
	};

	typedef WebGPUBuffer::AsyncToken<OutputResults> AsyncOutputResults;

	class Prepared {
		RunType						m_RunType;
		std::string					m_Identifier;
		WebGPUPipeline*				m_Pipeline = nullptr;
		WebGPUBuffer*				m_UniformBuffer = nullptr;
		int							m_EntityBufferIndex = -1;

		SSBO*						m_ControlSSBO = nullptr; // not owned, do not delete
		GPUEntity*					m_Entity = nullptr; // not owned, do not delete
		RenderableBuffer*			m_Buffer = nullptr; // not owned, do not delete
		class BaseCache*			m_Cache = nullptr; // not owned, do not delete
		std::vector<std::string>	m_VarNames;
		std::vector<AddedEntity>	m_Entities;
		bool						m_UsingRandom;
		bool						m_ReadRequired = false;
		std::shared_ptr<SSBO>		m_TemporaryFrame; // used for time travel feature

		struct DataVectorInfo {
			std::string		m_Name;
			std::string		m_CountVar;
			std::string		m_OffsetVar;
			std::string		m_NumVar;
			unsigned char*	m_Data = nullptr;
			int				m_SizeInts = 0;
			int				m_NumItems = 0;
		};
		std::vector<DataVectorInfo>	m_DataVectors;

		friend class PipelineStage;
	public:

							~Prepared	( void );

		template <class T>
		Prepared&			WithDataVector	( std::string_view name, const std::vector<T>& items ) {
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
		// todo: create WithTexture and WithBuffer functions for swapping out textures and buffers after prep

		std::string_view			GetIdentifier ( void) const { return m_Identifier; }

		template <class UniformSpec>
		void						Render			( RTT& rtt, const UniformSpec& uniform ) { RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, 1, &rtt, std::nullopt); }
		template <class UniformSpec>
		void						Render			( RTT& rtt, const UniformSpec& uniform, int iterations ) { RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, iterations, &rtt, std::nullopt); }
		template <class UniformSpec>
		void						Render			( RTT& rtt, const UniformSpec& uniform, std::vector<std::pair<std::string, int>>&& variables ) { RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), std::forward<std::vector<std::pair<std::string, int>>>(variables), 1, &rtt, std::nullopt); }

		template <class UniformSpec>
		AsyncOutputResults			Run				( const UniformSpec& uniform ) { return RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, 1, nullptr, std::nullopt); }
		template <class UniformSpec>
		AsyncOutputResults			Run				( const UniformSpec& uniform, int iterations ) { return RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), {}, iterations, nullptr, std::nullopt); }
		template <class UniformSpec>
		AsyncOutputResults			Run				( const UniformSpec& uniform, std::vector<std::pair<std::string, int>>&& variables, std::optional<std::function<void(const OutputResults& results)>>&& callback ) { return RunInternal((unsigned char*)&uniform, sizeof(UniformSpec), std::forward<std::vector<std::pair<std::string, int>>>(variables), 1, nullptr, std::move(callback)); }
	private:
		AsyncOutputResults			RunInternal		( unsigned char* uniform, int uniform_bytes, std::vector<std::pair<std::string, int>>&& variables, int iterations, RTT* rtt, std::optional<std::function<void(const OutputResults& results)>>&& callback );
	};

	static PipelineStage ModifyEntity(std::string_view identifier, GPUEntity& entity, std::string_view shader_name, bool replace_main, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_PROCESS, &entity, nullptr, cache, shader_name, replace_main, identifier);
	}
	static PipelineStage RenderEntity(std::string_view identifier, GPUEntity& entity, std::string_view shader_name, bool replace_main, RenderableBuffer* buffer, WebGPUPipeline::RenderParams render_params) {
		return PipelineStage(RunType::ENTITY_RENDER, &entity, buffer, nullptr, shader_name, replace_main, identifier, nullptr, 0, render_params);
	}
	static PipelineStage IterateEntity(std::string_view identifier, GPUEntity& entity, std::string_view shader_name, bool replace_main, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_ITERATE, &entity, nullptr, cache, shader_name, replace_main, identifier);
	}
	static PipelineStage RenderBuffer(std::string_view identifier, std::string_view shader_name, RenderableBuffer* buffer, WebGPUPipeline::RenderParams render_params, SSBO* control_ssbo = nullptr) {
		return PipelineStage(RunType::BASIC_RENDER, nullptr, buffer, nullptr, shader_name, false, identifier, control_ssbo, 0, render_params);
	}
	static PipelineStage Compute(std::string_view identifier, std::string_view shader_name, int iterations, SSBO* control_ssbo) {
		return PipelineStage(RunType::BASIC_COMPUTE, nullptr, nullptr, nullptr, shader_name, false, identifier, control_ssbo, iterations);
	}

								~PipelineStage		( void ) {}

	PipelineStage&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddCreatableEntity	( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddBuffer			( std::string_view name, SSBO& ssbo, MemberSpec::Type array_type, BufferAccess access ) { m_SSBOs.push_back({ ssbo, std::string(name), array_type, access }); return *this; }
	PipelineStage&				AddCode				( std::string_view code ) { m_ExtraCode = std::format("{}{}\n", m_ExtraCode, code); return *this; }
	PipelineStage&				AddCodeAtStart		( std::string_view code ) { m_ImmediateExtraCode = std::format("{}{}\n", m_ImmediateExtraCode, code); return *this; }

	PipelineStage&				AddInputOutputVar	( std::string_view name ) { m_Vars.push_back({ std::string(name), true }); return *this; }
	PipelineStage&				AddInputVar			( std::string_view name ) { m_Vars.push_back({ std::string(name), false }); return *this; }
	PipelineStage&				AddTexture			( std::string_view name, const WebGPUTexture* texture ) { m_Textures.push_back({ std::string(name), texture }); return *this; }
	PipelineStage&				AddSampler			( std::string_view name, const WebGPUSampler* sampler ) { m_Samplers.push_back({ std::string(name), sampler }); return *this; }

	template <class T>
	PipelineStage&				AddStructBuffer		( std::string_view name, std::string_view struct_name, SSBO& ssbo, BufferAccess access, bool is_array ) {
		std::vector<MemberSpec> members;
		Serialiser<T> serializeFunc(members);
		meta::doForAllMembers<T>(serializeFunc);
		m_StructBuffers.push_back({ ssbo, std::string(name), std::string(struct_name), std::move(members), access, is_array });
		return *this;
	}

	template <class UniformSpec>
	Prepared* Prepare				( void ) {
		std::vector<MemberSpec> uniform_members;
		Serialiser<UniformSpec> serializeFunc(uniform_members);
		meta::doForAllMembers<UniformSpec>(serializeFunc);
		return PrepareWithUniform(uniform_members);
	}

	template <class T>
	PipelineStage& AddDataVector(std::string_view name) {
		int ints_per = sizeof(T) / sizeof(int);
		m_DataVectors.push_back({ std::string(name), ints_per });
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
														std::string_view shader_name, bool replace_main,
														std::string_view identifer,
														SSBO* control_ssbo = nullptr, int iterations = 0,
														WebGPUPipeline::RenderParams render_params = {} );

	struct AddedSSBO {
		SSBO& p_Buffer;
		std::string				p_Name;
		MemberSpec::Type		p_Type;
		BufferAccess			p_Access;
	};
	struct AddedDataVector {
		std::string				p_Name;
		int						p_NumIntsPerItem;
		std::vector<MemberSpec> p_Members;
	};
	struct AddedStructBuffer {
		SSBO&					p_Buffer;
		std::string				p_Name;
		std::string				p_StructName;
		std::vector<MemberSpec> p_Members;
		BufferAccess			p_Access;
		bool					p_IsArray;
	};

	static std::string			GetDataVectorStructCode	( const AddedDataVector& data_vect, bool read_only );
	Prepared*					GetCachedPipeline		( void );
	Prepared*					PrepareWithUniform		( const std::vector<MemberSpec>& unform_members );

	struct AddedInOut {
		std::string	p_Name;
		bool		p_ReadBack = true;
	};

	struct AddedTexture {
		std::string				p_Name;
		const WebGPUTexture*	p_Tex;
	};
	struct AddedSampler {
		std::string				p_Name;
		const WebGPUSampler*	p_Sampler;
	};

	std::string						m_Identifier;
	RunType							m_RunType;
	GPUEntity*						m_Entity = nullptr;
	RenderableBuffer*				m_Buffer = nullptr;
	BaseCache*						m_Cache = nullptr;
	std::string						m_ShaderName;
	bool							m_ReplaceMain = false;
	iVec3							m_LocalSize = iVec3(8, 8, 8);
	int								m_Iterations = 0;
	SSBO*							m_ControlSSBO = nullptr;
	std::string						m_ImmediateExtraCode;
	std::string						m_ExtraCode;
	WebGPUPipeline::RenderParams	m_RenderParams;

	std::vector<BaseCache*>			m_CachesToBind;
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
	virtual void Bind(PipelineStage& target_stage, bool initial_creation) = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class QueryEntities {

public:

								QueryEntities		( GPUEntity& entity );

	QueryEntities&				ByNearestPosition	( std::string_view param_name, fVec2 pos );
	QueryEntities&				ByNearestPosition	( std::string_view param_name, fVec3 pos );
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
	std::string							m_ParamName;
	std::variant<fVec2, fVec3, int>		m_QueryParam;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Grid2DCache : public BaseCache {

public:

								Grid2DCache		( GPUEntity& entity, std::string_view pos_name );
	void						GenerateCache	( iVec2 grid_size, Vec2 grid_min, Vec2 grid_max );

	virtual void				Bind			( PipelineStage& target_stage, bool initial_creation ) override;

private:

	GPUEntity&					m_Entity;
	std::string					m_PosName;

	SSBO						m_GridIndices;
	SSBO						m_GridItems;
	SSBO						m_Uniform;

	iVec2						m_GridSize;
	Vec2						m_GridMin;
	Vec2						m_GridMax;
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
