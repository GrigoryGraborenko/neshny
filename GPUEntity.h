////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define BUFFER_TEX_SIZE 4096

struct MemberSpec {

	enum Type {
		T_UNKNOWN,
		T_INT,
		T_UINT,
		T_FLOAT,
		T_VEC2,
		T_VEC3,
		T_VEC4
	};

	static QString GetGPUType(Type type) {
		if (type == MemberSpec::T_INT) {
			return "int";
		} else if (type == MemberSpec::T_UINT) {
			return "uint";
		} else if (type == MemberSpec::T_FLOAT) {
			return "float";
		} else if (type == MemberSpec::T_VEC2) {
			return "vec2";
		} else if (type == MemberSpec::T_VEC3) {
			return "vec3";
		} else if (type == MemberSpec::T_VEC4) {
			return "vec4";
		}
		return QString();
	}

	static int GetGPUTypeSizeBytes(Type type) {
		if (type == MemberSpec::T_VEC2) {
			return sizeof(float) * 2;
		} else if (type == MemberSpec::T_VEC3) {
			return sizeof(float) * 3;
		} else if (type == MemberSpec::T_VEC4) {
			return sizeof(float) * 4;
		}
		return sizeof(int);
	}

	static QString GetGPUGetSyntax(Type type, int index) {
		if (type == MemberSpec::T_INT) {
			return QString("%2_LOOKUP(base, %1)").arg(index).arg("%1");
		} else if (type == MemberSpec::T_UINT) {
			return QString("uint(%2_LOOKUP(base, %1))").arg(index).arg("%1");
		} else if (type == MemberSpec::T_FLOAT) {
			return QString("intBitsToFloat(%2_LOOKUP(base, %1))").arg(index).arg("%1");
		} else if (type == MemberSpec::T_VEC2) {
			return QString("vec2(intBitsToFloat(%3_LOOKUP(base, %1)), intBitsToFloat(%3_LOOKUP(base, %2)))").arg(index).arg(index + 1).arg("%1");
		} else if (type == MemberSpec::T_VEC3) {
			return QString("vec3(intBitsToFloat(%4_LOOKUP(base, %1)), intBitsToFloat(%4_LOOKUP(base, %2)), intBitsToFloat(%4_LOOKUP(base, %3)))").arg(index).arg(index + 1).arg(index + 2).arg("%1");
		} else if (type == MemberSpec::T_VEC4) {
			return QString("vec4(intBitsToFloat(%5_LOOKUP(base, %1)), intBitsToFloat(%5_LOOKUP(base, %2)), intBitsToFloat(%5_LOOKUP(base, %3)), intBitsToFloat(%5_LOOKUP(base, %4)))").arg(index).arg(index + 1).arg(index + 2).arg(index + 3).arg("%1");
		}
		return QString();
	}

	QString		p_Name;
	Type		p_Type;
	int			p_Size;
	bool		p_IsID = false;
};

struct StructInfo {
	std::vector<MemberSpec>	p_Members;
	QString					p_GPUInsertion;
	QString					p_GPUReadOnlyInsertion;
};

namespace meta {
	template<>
	inline auto registerMembers<QVector3D>() {
		return members(
			member("x", &QVector3D::x),
			member("y", &QVector3D::y),
			member("z", &QVector3D::z)
		);
	}
}

template<typename T>
class Serialiser {
public:
	Serialiser(std::vector<MemberSpec>& specs/*, int T::* id_ref*/) : m_Specs(specs)/*, m_IdRef(id_ref)*/ {}

	template<typename Member>
	void operator()(Member& member) {
		//member.getPtr();
		QString name = member.getName();
		using CurrentMemberType = meta::get_member_type<decltype(member)>;

		MemberSpec::Type type = MemberSpec::T_UNKNOWN;
		bool is_id = false;
		if constexpr (std::is_same<CurrentMemberType, int>::value) {
			type = MemberSpec::T_INT;
			//auto mem_ptr = &
			//auto thing = member.getPtr();
			//if constexpr (std::is_same<decltype(member.getPtr()), decltype(m_IdRef)>::value) {
				//member.getPtr() == m_IdRef;
				//is_id = member.getPtr() == m_IdRef;
			//}
			//int brk = 0;
			//is_id = member.getPtr() == m_IdRef;
		} else if constexpr (std::is_same<CurrentMemberType, unsigned int>::value) {
			type = MemberSpec::T_UINT;
		} else if constexpr (std::is_same<CurrentMemberType, float>::value) {
			type = MemberSpec::T_FLOAT;
		} else if constexpr (std::is_same<CurrentMemberType, fVec2>::value) {
			type = MemberSpec::T_VEC2;
		} else if constexpr (std::is_same<CurrentMemberType, fVec3>::value) {
			type = MemberSpec::T_VEC3;
		//} else if constexpr (std::is_same<CurrentMemberType, fVec4>::value) {
		//	type = MemberSpec::T_VEC4;
		} else if constexpr (std::is_same<CurrentMemberType, QVector2D>::value) {
			type = MemberSpec::T_VEC2;
		} else if constexpr (std::is_same<CurrentMemberType, QVector3D>::value) {
			type = MemberSpec::T_VEC3;
		} else if constexpr (std::is_same<CurrentMemberType, QVector4D>::value) {
			type = MemberSpec::T_VEC4;
		}
		m_Specs.push_back({ name, type, sizeof(CurrentMemberType), is_id });
	}

private:
	std::vector<MemberSpec>&	m_Specs;
};

template<typename T>
void SerializeStructInfo(StructInfo& info, QString get_base_str) {
	Serialiser<T> serializeFunc(info.p_Members);
	meta::doForAllMembers<T>(serializeFunc);

	// %1 is entity name
	QStringList read_only_lines;
	QStringList lines;
	read_only_lines += "struct %1 {";
	for (auto member : info.p_Members) {
		read_only_lines += QString("\t%1 %2;").arg(MemberSpec::GetGPUType(member.p_Type)).arg(member.p_Name);
	}
	read_only_lines += "};";

	read_only_lines += "%1 Get%1(int index) {";
	read_only_lines += get_base_str;
	read_only_lines += "\t%1 result;";
	int pos_index = 0;
	QStringList functions;
	for (auto member : info.p_Members) {
		QString get_syntax = MemberSpec::GetGPUGetSyntax(member.p_Type, pos_index);
		read_only_lines += QString("\tresult.%1 = %2;").arg(member.p_Name).arg(get_syntax);
		functions += QString("%1 Get%3%2(int index) {\n").arg(MemberSpec::GetGPUType(member.p_Type)).arg(member.p_Name).arg("%1") + get_base_str + QString("\n\treturn %1;\n}").arg(get_syntax);
		if (member.p_Type == MemberSpec::Type::T_INT) {
			functions += QString("#define Access%3%1(index) (b_%3.i[(index) * FLOATS_PER_%3 + %2])\n").arg(member.p_Name).arg(pos_index).arg("%1");
		}
		pos_index += member.p_Size / sizeof(float);
	}
	read_only_lines += "\treturn result;\n}";
	read_only_lines += functions.join("\n");

	lines += "void Set%1(%1 item, int index) {";
	lines += get_base_str;
	pos_index = 0;
	functions = QStringList();
	for (auto member : info.p_Members) {
		QString name = member.p_Name;
		QString mod_str;
		QString value_mod_str;
		if (member.p_Type == MemberSpec::T_INT) {
			mod_str = QString("\t%3_SET(base, %1, item.%2);").arg(pos_index).arg(name).arg("%1");
			value_mod_str = QString("\t%2_SET(base, %1, value);").arg(pos_index).arg("%1");
		} else if (member.p_Type == MemberSpec::T_FLOAT) {
			mod_str = QString("\t%3_SET(base, %1, floatBitsToInt(item.%2));").arg(pos_index).arg(name).arg("%1");
			value_mod_str = QString("\t%2_SET(base, %1, floatBitsToInt(value));").arg(pos_index).arg("%1");
		} else if (member.p_Type == MemberSpec::T_VEC2) {
			mod_str = QString("\t%4_SET(base, %1, floatBitsToInt(item.%3.x)); %4_SET(base, %2, floatBitsToInt(item.%3.y));").arg(pos_index).arg(pos_index + 1).arg(name).arg("%1");
			value_mod_str = QString("\t%3_SET(base, %1, floatBitsToInt(value.x)); %3_SET(base, %2, floatBitsToInt(value.y));").arg(pos_index).arg(pos_index + 1).arg("%1");
		} else if (member.p_Type == MemberSpec::T_VEC3) {
			mod_str = QString("\t%5_SET(base, %1, floatBitsToInt(item.%4.x)); %5_SET(base, %2, floatBitsToInt(item.%4.y)); %5_SET(base, %3, floatBitsToInt(item.%4.z));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(name).arg("%1");
			value_mod_str = QString("\t%4_SET(base, %1, floatBitsToInt(value.x)); %4_SET(base, %2, floatBitsToInt(value.y)); %4_SET(base, %3, floatBitsToInt(value.z));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg("%1");
		} else if (member.p_Type == MemberSpec::T_VEC4) {
			mod_str = QString("\t%6_SET(base, %1, floatBitsToInt(item.%5.x)); %6_SET(base, %2, floatBitsToInt(item.%5.y)); %6_SET(base, %3, floatBitsToInt(item.%5.z)); %6_SET(base, %4, floatBitsToInt(item.%5.z));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(pos_index + 3).arg(name).arg("%1");
			value_mod_str = QString("\t%5_SET(base, %1, floatBitsToInt(value.x)); %5_SET(base, %2, floatBitsToInt(value.y)); %5_SET(base, %3, floatBitsToInt(value.z)); %5_SET(base, %4, floatBitsToInt(value.z));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(pos_index + 3).arg("%1");
		}
		lines += mod_str;
		functions += QString("void Set%3%1(int index, %2 value) {\n").arg(member.p_Name).arg(MemberSpec::GetGPUType(member.p_Type)).arg("%1") + get_base_str + QString("\n%1\n}").arg(value_mod_str);

		pos_index += member.p_Size / sizeof(float);
	}
	lines += "}";
	lines += functions.join("\n");

	info.p_GPUReadOnlyInsertion = read_only_lines.join("\n");
	lines.push_front(info.p_GPUReadOnlyInsertion);
	info.p_GPUInsertion = lines.join("\n");
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class GPUEntity {

public:

	enum class DeleteMode {
		MOVING_COMPACT
		,STABLE_WITH_GAPS
	};

	// TODO: figure out better way of passing in T, perhaps template entire class
	template <typename T> GPUEntity(QString name, DeleteMode delete_mode, int T::* id_ptr, QString id_name, bool double_buffer = true) :
			m_DeleteMode(delete_mode)
			,m_Name(name)
			,m_NumDataFloats(sizeof(T) / sizeof(float))
			,m_IDName(id_name)
			,m_DoubleBuffering(double_buffer)
		{
		QString get_base_str = QString("\tint base = index * FLOATS_PER_%1;").arg(m_Name);

		SerializeStructInfo<T>(m_Specs, get_base_str);

		QStringList insertion;
		insertion += QString("#define FLOATS_PER_%1 %2").arg(m_Name).arg(m_NumDataFloats);
		insertion += QString("#define %1_LOOKUP(base, index) (b_%1.i[(base) + (index)])").arg(m_Name);
		QStringList insertion_double_buffer = insertion;
		insertion += QString("#define %1_SET(base, index, value) (b_%1.i[(base) + (index)] = (value))").arg(m_Name);
		insertion_double_buffer += QString("#define %1_SET(base, index, value) (b_Output%1.i[(base) + (index)] = (value))").arg(m_Name);
		m_GPUInsertion = insertion.join("\n");
		m_GPUInsertionDoubleBuffer = insertion_double_buffer.join("\n");
	}
	~GPUEntity(void) { Destroy(); }

	template <typename T> void ExtractAll(std::vector<T>& items) {
		if (m_MaxIndex <= 0) {
			return;
		}
		items.resize(m_MaxIndex);
		unsigned char* ptr = (unsigned char*)&(items[0]);
		MakeCopyIn(ptr, 0, m_MaxIndex * m_NumDataFloats * sizeof(float));
	}

	template <typename T> T ExtractSingle(int index) {
		T item;
		int size_item = m_NumDataFloats * sizeof(float);
		MakeCopyIn((unsigned char*)&item, index * size_item, size_item);
		return item;
	}


	bool						Init					( int expected_max_count = 100000 );
	void						Clear					( void );

	int							AddInstance				( void* data );
	void						DeleteInstance			( int index );

	QString						GetDebugInfo			( void );
	std::shared_ptr<unsigned char[]> MakeCopy			( void );

	inline DeleteMode			GetDeleteMode			( void ) const { return m_DeleteMode; }
	inline QString				GetName					( void ) const { return m_Name; }
	inline GLSSBO*				GetSSBO					( void ) const { return m_SSBO; }
	inline GLSSBO*				GetOuputSSBO			( void ) const { return m_OutputSSBO; }
	inline GLSSBO*				GetControlSSBO			( void ) const { return m_ControlSSBO; }
	inline GLSSBO*				GetFreeListSSBO			( void ) const { return m_FreeList; }
	inline int					GetCount				( void ) const { return m_CurrentCount; }
	inline int					GetMaxIndex				( void ) const { return m_MaxIndex; }
	inline int					GetNextId				( void ) const { return m_NextId; }
	inline int					GetFreeCount			( void ) const { return m_FreeCount; }
	inline int					GetFloatsPer			( void ) const { return m_NumDataFloats; }
	inline const StructInfo&	GetSpecs				( void ) const { return m_Specs; }
	inline QString				GetGPUInsertion			( void ) const { return m_GPUInsertion; }
	inline QString				GetDoubleBufferGPUInsertion	( void ) const { return m_GPUInsertionDoubleBuffer; }
	inline QString				GetIDName				( void ) const { return m_IDName; }
	inline bool					IsDoubleBuffering		( void ) const { return m_DoubleBuffering; };

	void						ProcessMoveDeaths		( int death_count );
	void						ProcessStableDeaths		( int death_count );
	void						ProcessMoveCreates		( int new_count, int new_next_id );
	void						ProcessStableCreates	( int new_max_id, int new_next_id, int new_free_count );
	void						SwapInputOutputSSBOs	( void );

protected:

	void						MakeCopyIn				( unsigned char* ptr, int offset, int size );

	void						Destroy					( void );

	DeleteMode					m_DeleteMode;
	QString						m_Name;
	StructInfo					m_Specs;
	QString						m_GPUInsertion;
	QString						m_GPUInsertionDoubleBuffer;
	QString						m_IDName;
	int							m_NumDataFloats = 0;

	bool						m_DoubleBuffering = true;
	GLSSBO*						m_SSBO = nullptr;
	GLSSBO*						m_OutputSSBO = nullptr;

	GLSSBO*						m_ControlSSBO = nullptr;
	GLSSBO*						m_FreeList = nullptr;
	GLSSBO*						m_CopyBuffer = nullptr;

	int							m_CurrentCount = 0;
	int							m_MaxIndex = 0;
	int							m_NextId = 0;
	int							m_FreeCount = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class RTT {

public:

	enum class Mode {
		RGBA
		,RGBA_FLOAT32
		,RGBA_FLOAT16
	};

								RTT			( void ) {}
								~RTT		( void ) { Destroy(); }

	Token						Activate	( std::vector<Mode> color_attachments, bool capture_depth_stencil, int width, int height, bool clear = true );

	inline GLuint				GetColorTex	( int index ) { return index >= m_ColorTextures.size() ? 0 : m_ColorTextures[index]; }
	inline GLuint				GetDepthTex	( void ) { return m_DepthTex; }

private:

	void						Destroy		( void );

	std::vector<Mode>			m_Modes = {};
	bool						m_CaptureDepthStencil = false;
	int							m_Width = 0;
	int							m_Height = 0;
	GLuint						m_FrameBuffer = 0;
	std::vector<GLuint>			m_ColorTextures;
	GLuint						m_DepthTex = 0;
	GLuint						m_DepthBuffer = 0;
};