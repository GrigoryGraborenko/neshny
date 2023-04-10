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

	static PipelineStage ModifyEntity(GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_PROCESS, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderEntity(GPUEntity& entity, QString shader_name, bool replace_main, GLBuffer* buffer, const std::vector<QString>& shader_defines) {
		return PipelineStage(RunType::ENTITY_RENDER, &entity, buffer, nullptr, shader_name, replace_main, shader_defines);
	}
	static PipelineStage IterateEntity(GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, class BaseCache* cache = nullptr) {
		return PipelineStage(RunType::ENTITY_ITERATE, &entity, nullptr, cache, shader_name, replace_main, shader_defines);
	}
	static PipelineStage RenderBuffer(QString shader_name, GLBuffer* buffer, const std::vector<QString>& shader_defines) {
		return PipelineStage(RunType::BASIC_RENDER, nullptr, buffer, nullptr, shader_name, false, shader_defines);
	}
	static PipelineStage Compute(QString shader_name, int iterations, GLSSBO* control_ssbo, const std::vector<QString>& shader_defines) {
		return PipelineStage(RunType::BASIC_COMPUTE, nullptr, nullptr, nullptr, shader_name, false, shader_defines, iterations, control_ssbo);
	}

								~PipelineStage		( void ) {}

	PipelineStage&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddCreatableEntity	( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddInputOutputVar	( QString name, int* in_out );
	PipelineStage&				AddSSBO				( QString name, GLSSBO& ssbo, MemberSpec::Type array_type, bool read_only = true );
	PipelineStage&				AddTexture			( QString name, GLuint texture ) { m_Textures.push_back({ name, texture }); return *this; }
	PipelineStage&				AddCode				( QString code ) { m_ExtraCode += code; return *this; }
	void						Run					( std::optional<std::function<void(GLShader* program)>> pre_execute = std::nullopt );

	template <class T>
	PipelineStage&				AddDataVector		( QString name, const std::vector<T>& items ) {
		AddDataVectorBase(name, items);
		return *this;
	}

	inline int					GetLocalSizeX(void) const { return m_LocalSizeX; }
	inline int					GetLocalSizeY(void) const { return m_LocalSizeY; }
	inline int					GetLocalSizeZ(void) const { return m_LocalSizeZ; }
	inline void					SetLocalSizes(int x, int y, int z) { m_LocalSizeX = x; m_LocalSizeY = y; m_LocalSizeZ = z; }

	struct AddedSSBO {
		GLSSBO& p_Buffer;
		QString					p_Name;
		MemberSpec::Type		p_Type;
		bool					p_ReadOnly = true;
	};

protected:

								PipelineStage		( RunType type, GPUEntity* entity, GLBuffer* buffer, class BaseCache* cache, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, int iterations = 0, GLSSBO* control_ssbo = nullptr);

	struct AddedDataVector {
		QString					p_Name;
		int						p_NumItems;
		int						p_NumIntsPerItem;
		std::vector<MemberSpec> p_Members;
		std::vector<int>		p_Data;
	};

	struct AddedEntity {
		GPUEntity&	p_Entity;
		bool		p_Creatable = false;
	};

	struct AddedInOut {
		QString		p_Name;
		int*		p_Ptr;
	};

	struct AddedTexture {
		QString		p_Name;
		GLuint		p_Tex;
	};

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
		const AddedDataVector& uniform
		,QStringList& insertion_uniforms
		,std::vector<std::pair<QString, int>>& integer_vars
		,int offset
	);

	RunType						m_RunType;
	GPUEntity*					m_Entity = nullptr;
	GLBuffer*					m_Buffer = nullptr;
	QString						m_ShaderName;
	std::vector<QString>		m_ShaderDefines;
	bool						m_ReplaceMain = false;
	int							m_LocalSizeX = 8;
	int							m_LocalSizeY = 8;
	int							m_LocalSizeZ = 8;
	int							m_Iterations = 0;
	GLSSBO*						m_ControlSSBO = nullptr;
	QString						m_ExtraCode;

	std::vector<AddedEntity>	m_Entities;
	std::vector<AddedSSBO>		m_SSBOs;
	std::vector<AddedInOut>		m_Vars;
	std::vector<AddedTexture>	m_Textures;

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

	GLSSBO						m_GridIndices;
	GLSSBO						m_GridItems;

	iVec2						m_GridSize;
	Vec2						m_GridMin;
	Vec2						m_GridMax;
};

} // namespace Neshny