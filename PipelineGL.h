////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(NESHNY_GL)
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
	};

	struct AddedEntity {
		GPUEntity*	p_Entity;
		bool		p_Creatable = false;
	};

	struct AddedSSBO {
		SSBO&					p_Buffer;
		std::string				p_Name;
		MemberSpec::Type		p_Type;
		BufferAccess			p_Access;
	};

	static PipelineStage ModifyEntity(GPUEntity& entity, std::string_view shader_name, bool replace_main, const std::vector<std::string>& shader_defines = {}, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_PROCESS, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderEntity(GPUEntity& entity, std::string_view shader_name, bool replace_main, RenderableBuffer* buffer, const std::vector<std::string>& shader_defines = {}) {
		return PipelineStage(RunType::ENTITY_RENDER, &entity, buffer, nullptr, shader_name, replace_main, shader_defines);
	}
	static PipelineStage IterateEntity(GPUEntity& entity, std::string_view shader_name, bool replace_main, const std::vector<std::string>& shader_defines = {}, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_ITERATE, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderBuffer(std::string_view shader_name, RenderableBuffer* buffer, const std::vector<std::string>& shader_defines = {}, SSBO* control_ssbo = nullptr) {
		return PipelineStage(RunType::BASIC_RENDER, nullptr, buffer, nullptr, shader_name, false, shader_defines, control_ssbo);
	}
	static PipelineStage Compute(std::string_view shader_name, int iterations, SSBO* control_ssbo, const std::vector<std::string>& shader_defines = {}) {
		return PipelineStage(RunType::BASIC_COMPUTE, nullptr, nullptr, nullptr, shader_name, false, shader_defines, control_ssbo, iterations);
	}

								~PipelineStage		( void ) {}

	PipelineStage&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddCreatableEntity	( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddBuffer			( std::string name, SSBO& ssbo, MemberSpec::Type array_type, BufferAccess access ) { m_SSBOs.push_back({ ssbo, name, array_type, access }); return *this; }
	PipelineStage&				AddCode				( std::string_view code ) { m_ExtraCode += code; return *this; }
	PipelineStage&				AddInputOutputVar	( std::string name, int* in_out ) { m_Vars.push_back({ name, in_out }); return *this; }
	PipelineStage&				AddTexture			( std::string name, GLuint texture ) { m_Textures.push_back({ name, texture }); return *this; }
	void						Run					( std::optional<std::function<void(Shader* program)>> pre_execute = std::nullopt );

	template <class T>
	PipelineStage& AddDataVector(std::string_view name, const std::vector<T>& items) {
		AddDataVectorBase(name, items);
		return *this;
	}

	inline iVec3				GetLocalSize		( void ) const { return m_LocalSize; }
	inline void					SetLocalSize		( int x, int y, int z ) { m_LocalSize = iVec3(x, y, z); }

protected:

								PipelineStage		( RunType type, GPUEntity* entity, RenderableBuffer* buffer, class BaseCache* cache, std::string_view shader_name, bool replace_main, const std::vector<std::string>& shader_defines, SSBO* control_ssbo = nullptr, int iterations = 0);

	struct AddedDataVector {
		std::string				p_Name;
		int						p_NumItems;
		int						p_NumIntsPerItem;
		std::vector<MemberSpec> p_Members;
		std::vector<int>		p_Data;
	};

	struct AddedInOut {
		std::string	p_Name;
		int*		p_Ptr;
	};

	struct AddedTexture {
		std::string			p_Name;
		GLuint				p_Tex;
	};

	template <class T>
	void						AddDataVectorBase(std::string_view name, const std::vector<T>& items) {
		if (items.empty()) {
			return;
		}
		int ints_per = sizeof(T) / sizeof(int);
		m_DataVectors.push_back({ std::string(name), (int)items.size(), ints_per });
		AddedDataVector& ref = m_DataVectors.back();

		Serialiser<T> serializeFunc(ref.p_Members);
		meta::doForAllMembers<T>(serializeFunc);

		int num_ints = ref.p_NumIntsPerItem * ref.p_NumItems;
		ref.p_Data.resize(num_ints);
		memcpy((unsigned char*)&(ref.p_Data[0]), (unsigned char*)&(items[0]), sizeof(int) * num_ints);
	}

	static std::string			GetDataVectorStructCode(
		const AddedDataVector& data_vect
		,std::vector<std::string>& insertion_uniforms
		,std::vector<std::pair<std::string, int>>& integer_vars
		,int offset
	);

	RunType							m_RunType;
	GPUEntity*						m_Entity = nullptr;
	RenderableBuffer*				m_Buffer = nullptr;
	BaseCache*						m_Cache = nullptr;
	std::string						m_ShaderName;
	std::vector<std::string>		m_ShaderDefines;
	bool							m_ReplaceMain = false;
	iVec3							m_LocalSize = iVec3(8, 8, 8);
	int								m_Iterations = 0;
	SSBO*							m_ControlSSBO = nullptr;
	std::string						m_ExtraCode;

	std::vector<AddedEntity>		m_Entities;
	std::vector<AddedSSBO>			m_SSBOs;
	std::vector<AddedInOut>			m_Vars;
	std::vector<AddedTexture>		m_Textures;

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

	virtual void				Bind			( PipelineStage& target_stage ) override;

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

} // namespace Neshny
#endif