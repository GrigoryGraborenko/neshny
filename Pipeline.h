////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define MINIMUM_UNIFORM_VECTOR_LENGTH 16

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class CommonPipeline {

public:

								CommonPipeline		( QString shader_name, bool replace_main, const std::vector<QString>& shader_defines );
								~CommonPipeline		( void ) {}

	struct AddedSSBO {
		GLSSBO& p_Buffer;
		QString					p_Name;
		MemberSpec::Type		p_Type;
		bool					p_ReadOnly = true;
	};

protected:

	struct AddedUniformVector {
		QString					p_Name;
		int						p_NumItems;
		int						p_NumFloatsPerItem;
		std::vector<MemberSpec> p_Members;
		std::vector<float>		p_Data;
	};

	struct AddedEntity {
		GPUEntity&	p_Entity;
		bool		p_Creatable = false;
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

	QString						m_ShaderName;
	std::vector<QString>		m_ShaderDefines;
	bool						m_ReplaceMain = false;
	QString						m_ExtraCode;

	std::vector<AddedUniformVector>		m_UniformVectors;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseCache {
public:
	virtual QString Bind(std::vector<CommonPipeline::AddedSSBO>& ssbos ) = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class PipelineStage : public CommonPipeline {

public:
								PipelineStage		( GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, BaseCache* cache = nullptr );
								~PipelineStage		( void ) {}

	PipelineStage&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddCreatableEntity	( GPUEntity& entity, BaseCache* cache = nullptr );
	PipelineStage&				AddInputOutputVar	( QString name, int* in_out );
	PipelineStage&				AddSSBO				( QString name, GLSSBO& ssbo, MemberSpec::Type array_type, bool read_only = true );
	PipelineStage&				AddCode				( QString code ) { m_ExtraCode += code; return *this; }
	void						Run					( std::optional<std::function<void(GLShader* program)>> pre_execute = std::nullopt );

	template <class T>
	PipelineStage&				AddUniformVector	( QString name, const std::vector<T>& items ) {
		AddUniformVectorBase(name, items);
		return *this;
	}

private:

	struct AddedInOut {
		QString					p_Name;
		int*					p_Ptr;
	};

	GPUEntity&					m_Entity;
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
	EntityRender&				AddCode				( QString code ) { m_ExtraCode += code; return *this; }
	void						Render				( GLBuffer* buffer, std::optional<std::function<void(GLShader* program)>> pre_execute = std::nullopt );

	template <class T>
	EntityRender& AddUniformVector(QString name, const std::vector<T>& items) {
		AddUniformVectorBase(name, items);
		return *this;
	}

private:

	GPUEntity&					m_Entity;
	std::vector<AddedSSBO>		m_SSBOs;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BasicRender : CommonPipeline {

public:

								BasicRender			( GLBuffer* buffer, QString shader_name, const std::vector<QString>& shader_defines = {} );
								~BasicRender		( void ) {}

	BasicRender&				AddSSBO				( QString name, GLSSBO& ssbo, MemberSpec::Type array_type );
	BasicRender&				AddEntity			( GPUEntity& entity, BaseCache* cache = nullptr );

	void						Render				( std::optional<std::function<void(GLShader* program)>> pre_execute = std::nullopt );

private:
	
	GLBuffer*					m_Buffer = nullptr;
	std::vector<AddedSSBO>		m_SSBOs;
	std::vector<AddedEntity>	m_Entities;

};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class QueryEntities : CommonPipeline {

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

	virtual QString				Bind			( std::vector<CommonPipeline::AddedSSBO>& ssbos ) override;

private:

	GPUEntity&					m_Entity;
	QString						m_PosName;

	GLSSBO						m_GridIndices;
	GLSSBO						m_GridItems;

	iVec2						m_GridSize;
	Vec2						m_GridMin;
	Vec2						m_GridMax;
};
