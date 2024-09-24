////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(NESHNY_WEBGPU)
namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class EntityPipeline {

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

	struct OutputResults {
		bool GetValue(std::string_view name, int& value) const;
		std::vector<std::pair<std::string, int>> p_Results;
	};

	typedef WebGPUBuffer::AsyncToken<OutputResults> AsyncOutputResults;

	static EntityPipeline ModifyEntity(std::string_view identifier, GPUEntity& entity, std::string_view shader_name, bool replace_main, class BaseCache* cache = nullptr) {
		return EntityPipeline(RunType::ENTITY_PROCESS, &entity, nullptr, cache, shader_name, replace_main, identifier);
	}
	static EntityPipeline RenderEntity(std::string_view identifier, GPUEntity& entity, std::string_view shader_name, bool replace_main, RenderableBuffer* buffer, WebGPUPipeline::RenderParams render_params) {
		return EntityPipeline(RunType::ENTITY_RENDER, &entity, buffer, nullptr, shader_name, replace_main, identifier, nullptr, 0, render_params);
	}
	static EntityPipeline IterateEntity(std::string_view identifier, GPUEntity& entity, std::string_view shader_name, bool replace_main, class BaseCache* cache = nullptr) {
		return EntityPipeline(RunType::ENTITY_ITERATE, &entity, nullptr, cache, shader_name, replace_main, identifier);
	}
	static EntityPipeline RenderBuffer(std::string_view identifier, std::string_view shader_name, RenderableBuffer* buffer, WebGPUPipeline::RenderParams render_params, SSBO* control_ssbo = nullptr) {
		return EntityPipeline(RunType::BASIC_RENDER, nullptr, buffer, nullptr, shader_name, false, identifier, control_ssbo, 0, render_params);
	}
	static EntityPipeline Compute(std::string_view identifier, std::string_view shader_name, int iterations, SSBO* control_ssbo) {
		return EntityPipeline(RunType::BASIC_COMPUTE, nullptr, nullptr, nullptr, shader_name, false, identifier, control_ssbo, iterations);
	}

	EntityPipeline&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );
	EntityPipeline&				AddCreatableEntity	( GPUEntity& entity, BaseCache* cache = nullptr );
	EntityPipeline&				AddBuffer			( std::string_view name, SSBO& ssbo, MemberSpec::Type array_type, BufferAccess access ) { m_SSBOs.push_back({ ssbo, std::string(name), array_type, access }); return *this; }
	EntityPipeline&				AddCode				( std::string_view code ) { m_ExtraCode = std::format("{}{}\n", m_ExtraCode, code); return *this; }
	EntityPipeline&				AddCodeAtStart		( std::string_view code ) { m_ImmediateExtraCode = std::format("{}{}\n", m_ImmediateExtraCode, code); return *this; }

	EntityPipeline&				AddInputOutputVar	( std::string_view name, int value ) { m_Vars.push_back({ std::string(name), true, value }); return *this; }
	EntityPipeline&				AddInputVar			( std::string_view name, int value ) { m_Vars.push_back({ std::string(name), false, value }); return *this; }
	EntityPipeline&				AddTexture			( std::string_view name, const WebGPUTexture* texture ) { m_Textures.push_back({ std::string(name), texture }); return *this; }
	EntityPipeline&				AddSampler			( std::string_view name, const WebGPUSampler* sampler ) { m_Samplers.push_back({ std::string(name), sampler }); return *this; }

	template <class T>
	EntityPipeline&				AddStructBuffer		( std::string_view name, std::string_view struct_name, SSBO& ssbo, BufferAccess access, bool is_array ) {

		std::vector<MemberSpec> members;
		Serialiser<T> serializeFunc(members);
		meta::doForAllMembers<T>(serializeFunc);
		m_StructBuffers.push_back({ ssbo, std::string(name), std::string(struct_name), std::move(members), access, is_array });
		return *this;
	}

	template <class T>
	EntityPipeline&				SetUniform			( const T& uniform ) {

		m_Uniform.p_Spec.clear();
		Serialiser<T> serializeFunc(m_Uniform.p_Spec);
		meta::doForAllMembers<T>(serializeFunc);
		m_Uniform.p_Data = { (unsigned char*)&uniform, sizeof(uniform) };
		return *this;
	}

	template <class T>
	EntityPipeline&				AddDataVector		( std::string_view name, const std::vector<T>& items ) {

		int ints_per = sizeof(T) / sizeof(int);
		m_DataVectors.push_back({
			std::string(name), ints_per, (unsigned char*)items.data(), (int)items.size(),
			std::format("io{0}Count", name), std::format("io{0}Offset", name), std::format("io{0}Num", name)
		});
		AddedDataVector& ref = m_DataVectors.back();
		Serialiser<T> serializeFunc(ref.p_Members);
		meta::doForAllMembers<T>(serializeFunc);
		return *this;
	}

	void						Render			( RTT* rtt, int iterations = 1 ) { RunInternal(iterations, rtt, std::nullopt); }
	AsyncOutputResults			Run				( std::optional<std::function<void(const OutputResults& results)>>&& callback = std::nullopt, int iterations = 1 ) { return RunInternal(iterations, nullptr, std::move(callback)); }

	inline iVec3				GetLocalSize	( void ) const { return m_LocalSize; }
	inline void					SetLocalSize	( int x, int y, int z ) { m_LocalSize = iVec3(x, y, z); }

protected:

	struct AddedEntity {
		GPUEntity*					p_Entity;
		bool						p_Creatable = false;
	};
	struct AddedSSBO {
		SSBO&						p_Buffer;
		std::string					p_Name;
		MemberSpec::Type			p_Type;
		BufferAccess				p_Access;
	};
	struct AddedStructBuffer {
		SSBO&						p_Buffer;
		std::string					p_Name;
		std::string					p_StructName;
		std::vector<MemberSpec>		p_Members;
		BufferAccess				p_Access;
		bool						p_IsArray;
	};
	struct AddedInOut {
		std::string					p_Name;
		bool						p_ReadBack = true;
		int							p_Input;
	};
	struct AddedTexture {
		std::string					p_Name;
		const WebGPUTexture*		p_Tex;
	};
	struct AddedSampler {
		std::string					p_Name;
		const WebGPUSampler*		p_Sampler;
	};
	struct AddedUniform {
		std::vector<MemberSpec>		p_Spec;
		std::span<unsigned char>	p_Data;
	};
	struct AddedDataVector {
		std::string				p_Name;
		int						p_NumIntsPerItem = 0;
		unsigned char*			p_Data = nullptr;
		int						p_NumItems = 0;
		std::string				p_CountVar;
		std::string				p_OffsetVar;
		std::string				p_NumVar;
		std::vector<MemberSpec>	p_Members;
	};

											EntityPipeline			(	RunType type,
																		GPUEntity* entity,
																		RenderableBuffer* buffer, class BaseCache* cache,
																		std::string_view shader_name, bool replace_main,
																		std::string_view identifer,
																		SSBO* control_ssbo = nullptr, int iterations = 0,
																		WebGPUPipeline::RenderParams render_params = {} );

	static std::string						GetDataVectorStructCode	( const AddedDataVector& data_vect, bool read_only );
	std::shared_ptr<Core::CachedPipeline>	GetCachedPipeline		( void );
	std::shared_ptr<Core::CachedPipeline>	Prepare					( void );
	AsyncOutputResults						RunInternal				( int iterations, RTT* rtt, std::optional<std::function<void(const OutputResults& results)>>&& callback );


	std::string								m_Identifier;
	RunType									m_RunType;
	GPUEntity*								m_Entity = nullptr;
	RenderableBuffer*						m_Buffer = nullptr;
	BaseCache*								m_Cache = nullptr;
	std::string								m_ShaderName;
	bool									m_ReplaceMain = false;
	iVec3									m_LocalSize = iVec3(8, 8, 8);
	int										m_Iterations = 0;
	SSBO*									m_ControlSSBO = nullptr;
	std::string								m_ImmediateExtraCode;
	std::string								m_ExtraCode;
	WebGPUPipeline::RenderParams			m_RenderParams;

	AddedUniform							m_Uniform;
	std::vector<BaseCache*>					m_CachesToBind;
	std::vector<AddedEntity>				m_Entities;
	std::vector<AddedSSBO>					m_SSBOs;
	std::vector<AddedInOut>					m_Vars;
	std::vector<AddedTexture>				m_Textures;
	std::vector<AddedSampler>				m_Samplers;
	std::vector<AddedStructBuffer>			m_StructBuffers;
	std::vector<AddedDataVector>			m_DataVectors;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseCache {
public:
	virtual void Bind(EntityPipeline& target_stage, bool initial_creation) = 0;
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

	virtual void				Bind			( EntityPipeline& target_stage, bool initial_creation ) override;

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
