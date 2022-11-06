////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define MINIMUM_UNIFORM_VECTOR_LENGTH 16

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class BaseCache {
public:
	virtual void Bind(class PipelineStage& stage) = 0;
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
	QString						m_ExtraCode;

	std::vector<AddedUniformVector>		m_UniformVectors;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class PipelineStage : public CommonPipeline {

public:
								PipelineStage		( GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines );
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

	struct AddedEntity {
		GPUEntity&	p_Entity;
		bool		p_Creatable = false;
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

	struct AddedSSBO {
		GLSSBO&					p_Buffer;
		QString					p_Name;
		MemberSpec::Type		p_Type;
	};

	std::vector<AddedSSBO>		m_SSBOs;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Grid2DCache : public BaseCache {

public:

								Grid2DCache		( GPUEntity& entity, QString pos_name );
	void						GenerateCache	( IVec2 grid_size, Vec2 grid_min, Vec2 grid_max );

	virtual void				Bind			( class PipelineStage& stage ) override;

private:

	GPUEntity&					m_Entity;
	QString						m_PosName;

	GLSSBO						m_GridIndices;
	GLSSBO						m_GridItems;

	IVec2						m_GridSize;
	Vec2						m_GridMin;
	Vec2						m_GridMax;
};
