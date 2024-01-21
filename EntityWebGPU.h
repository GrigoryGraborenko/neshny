////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

struct EntityInfo {
	int	p_Count = 0;
	int p_FreeCount = 0;
	int p_NextId = 0;
	int p_MaxIndex = 0;
};

const int ENTITY_OFFSET_INTS = 4;

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
		} else if constexpr (std::is_same<CurrentMemberType, fVec4>::value) {
			type = MemberSpec::T_VEC4;
		} else if constexpr (std::is_same<CurrentMemberType, iVec2>::value) {
			type = MemberSpec::T_IVEC2;
		} else if constexpr (std::is_same<CurrentMemberType, iVec3>::value) {
			type = MemberSpec::T_IVEC3;
		} else if constexpr (std::is_same<CurrentMemberType, iVec4>::value) {
			type = MemberSpec::T_IVEC4;
		} else if constexpr (std::is_same<CurrentMemberType, fMatrix3>::value) {
			type = MemberSpec::T_MAT3;
		} else if constexpr (std::is_same<CurrentMemberType, fMatrix4>::value) {
			type = MemberSpec::T_MAT4;
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
	{
		QStringList member_vars;
		for (auto member : info.p_Members) {
			member_vars += QString("\t%2: %1").arg(MemberSpec::GetGPUType(member.p_Type)).arg(member.p_Name);
		}
		read_only_lines += member_vars.join(",\n");
	}
	read_only_lines += "}";

	read_only_lines += "fn Get%1(index: i32) -> %1 {";
	read_only_lines += get_base_str;
	read_only_lines += "\tvar result: %1;";
	int pos_index = 0;
	QStringList functions;
	for (auto member : info.p_Members) {
		QString get_syntax = MemberSpec::GetGPUGetSyntax(member.p_Type, pos_index);
		read_only_lines += QString("\tresult.%1 = %2;").arg(member.p_Name).arg(get_syntax);
		functions += QString("fn Get%3%2(index: i32) -> %1 {\n").arg(MemberSpec::GetGPUType(member.p_Type)).arg(member.p_Name).arg("%1") + get_base_str + QString("\n\treturn %1;\n}").arg(get_syntax);
		if (member.p_Type == MemberSpec::Type::T_INT) {
			functions += QString("#define Access%3%1(index) (b_%3[(index) * FLOATS_PER_%3 + %2 + ENTITY_OFFSET_INTS])\n").arg(member.p_Name).arg(pos_index).arg("%1");
		} else if ((member.p_Type == MemberSpec::Type::T_IVEC2) || (member.p_Type == MemberSpec::Type::T_IVEC3) || (member.p_Type == MemberSpec::Type::T_IVEC4)) {
			functions += QString("#define Access%3%1_X(index) (b_%3[(index) * FLOATS_PER_%3 + %2 + ENTITY_OFFSET_INTS])\n").arg(member.p_Name).arg(pos_index).arg("%1");
			functions += QString("#define Access%3%1_Y(index) (b_%3[(index) * FLOATS_PER_%3 + %2 + ENTITY_OFFSET_INTS])\n").arg(member.p_Name).arg(pos_index + 1).arg("%1");
			if (member.p_Type != MemberSpec::Type::T_IVEC2) {
				functions += QString("#define Access%3%1_Z(index) (b_%3[(index) * FLOATS_PER_%3 + %2 + ENTITY_OFFSET_INTS])\n").arg(member.p_Name).arg(pos_index + 2).arg("%1");
				if (member.p_Type == MemberSpec::Type::T_IVEC4) {
					functions += QString("#define Access%3%1_W(index) (b_%3[(index) * FLOATS_PER_%3 + %2 + ENTITY_OFFSET_INTS])\n").arg(member.p_Name).arg(pos_index + 3).arg("%1");
				}
			}
		}

		pos_index += member.p_Size / sizeof(float);
	}
	read_only_lines += "\treturn result;\n}";
	read_only_lines += functions.join("\n");

	lines += "fn Set%1(item: %1, index: i32) {"; // TODO: make this a ref?
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
			mod_str = QString("\t%3_SET(base, %1, bitcast<i32>(item.%2));").arg(pos_index).arg(name).arg("%1");
			value_mod_str = QString("\t%2_SET(base, %1, bitcast<i32>(value));").arg(pos_index).arg("%1");
		} else if (member.p_Type == MemberSpec::T_VEC2) {
			mod_str = QString("\t%4_SET(base, %1, bitcast<i32>(item.%3.x)); %4_SET(base, %2, bitcast<i32>(item.%3.y));").arg(pos_index).arg(pos_index + 1).arg(name).arg("%1");
			value_mod_str = QString("\t%3_SET(base, %1, bitcast<i32>(value.x)); %3_SET(base, %2, bitcast<i32>(value.y));").arg(pos_index).arg(pos_index + 1).arg("%1");
		} else if (member.p_Type == MemberSpec::T_VEC3) {
			mod_str = QString("\t%5_SET(base, %1, bitcast<i32>(item.%4.x)); %5_SET(base, %2, bitcast<i32>(item.%4.y)); %5_SET(base, %3, bitcast<i32>(item.%4.z));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(name).arg("%1");
			value_mod_str = QString("\t%4_SET(base, %1, bitcast<i32>(value.x)); %4_SET(base, %2, bitcast<i32>(value.y)); %4_SET(base, %3, bitcast<i32>(value.z));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg("%1");
		} else if (member.p_Type == MemberSpec::T_VEC4) {
			mod_str = QString("\t%6_SET(base, %1, bitcast<i32>(item.%5.x)); %6_SET(base, %2, bitcast<i32>(item.%5.y)); %6_SET(base, %3, bitcast<i32>(item.%5.z)); %6_SET(base, %4, bitcast<i32>(item.%5.w));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(pos_index + 3).arg(name).arg("%1");
			value_mod_str = QString("\t%5_SET(base, %1, bitcast<i32>(value.x)); %5_SET(base, %2, bitcast<i32>(value.y)); %5_SET(base, %3, bitcast<i32>(value.z)); %5_SET(base, %4, bitcast<i32>(value.w));").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(pos_index + 3).arg("%1");
		} else if (member.p_Type == MemberSpec::T_IVEC2) {
			mod_str = QString("\t%4_SET(base, %1, item.%3.x); %4_SET(base, %2, item.%3.y);").arg(pos_index).arg(pos_index + 1).arg(name).arg("%1");
			value_mod_str = QString("\t%3_SET(base, %1, value.x); %3_SET(base, %2, value.y);").arg(pos_index).arg(pos_index + 1).arg("%1");
		} else if (member.p_Type == MemberSpec::T_IVEC3) {
			mod_str = QString("\t%5_SET(base, %1, item.%4.x); %5_SET(base, %2, item.%4.y); %5_SET(base, %3, item.%4.z);").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(name).arg("%1");
			value_mod_str = QString("\t%4_SET(base, %1, value.x); %4_SET(base, %2, value.y); %4_SET(base, %3, value.z);").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg("%1");
		} else if (member.p_Type == MemberSpec::T_IVEC4) {
			mod_str = QString("\t%6_SET(base, %1, item.%5.x); %6_SET(base, %2, item.%5.y); %6_SET(base, %3, item.%5.z); %6_SET(base, %4, item.%5.w);").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(pos_index + 3).arg(name).arg("%1");
			value_mod_str = QString("\t%5_SET(base, %1, value.x); %5_SET(base, %2, value.y); %5_SET(base, %3, value.z); %5_SET(base, %4, value.w);").arg(pos_index).arg(pos_index + 1).arg(pos_index + 2).arg(pos_index + 3).arg("%1");
		}
		lines += mod_str;
		functions += QString("fn Set%3%1(index: i32, value: %2) {\n").arg(member.p_Name).arg(MemberSpec::GetGPUType(member.p_Type)).arg("%1") + get_base_str + QString("\n%1\n}").arg(value_mod_str);

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

	// TODO: figure out better way of passing in T, perhaps template entire class

	template <typename T> GPUEntity(QString name, int T::* id_ptr, QString id_name, bool double_buffer = true) :
			m_Name(name)
			,m_NumDataFloats(sizeof(T) / sizeof(float))
			,m_IDName(id_name)
			,m_DoubleBuffering(double_buffer)
		{
		QString get_base_str = QString("\tlet base = index * FLOATS_PER_%1 + ENTITY_OFFSET_INTS;").arg(m_Name);

		SerializeStructInfo<T>(m_Specs, get_base_str);

		int pos_index = 0;
		for (const auto& member : m_Specs.p_Members) {
			if (member.p_Name == m_IDName) {
				m_IdOffset = pos_index;
				break;
			}
			pos_index += member.p_Size;
		}

		QStringList insertion;
		insertion += QString("#define FLOATS_PER_%1 %2").arg(m_Name).arg(m_NumDataFloats);
		insertion += QString("#define %1_LOOKUP(base, index) (b_%1[(base) + (index)])").arg(m_Name);
		QStringList insertion_double_buffer = insertion;
		insertion += QString("#define %1_SET(base, index, value) atomicStore(&b_%1[(base) + (index)], value)").arg(m_Name);
		insertion_double_buffer += QString("#define %1_SET(base, index, value) b_Output%1[(base) + (index)] = (value)").arg(m_Name);
		m_GPUInsertion = insertion.join("\n");
		m_GPUInsertionDoubleBuffer = insertion_double_buffer.join("\n");
	}

	~GPUEntity(void) { Destroy(); }

	template <typename T> void ExtractMultiple(std::vector<T>& items, int count) {
		// does this still make sense? should there be an offset?
		if (count <= 0) {
			return;
		}
		items.resize(count);
		MakeCopyIn((unsigned char*)items.data(), ENTITY_OFFSET_INTS * sizeof(int), count * m_NumDataFloats * sizeof(int));
	}

	template <typename T> void ExtractAll(std::vector<T>& items) {
		int size_item = m_NumDataFloats * sizeof(int);
		int size_offset = ENTITY_OFFSET_INTS * sizeof(int);
		int total_size = size_offset + size_item * m_MaxItems;
		unsigned char* buffer = new unsigned char[total_size];
		MakeCopyIn(buffer, 0, total_size);
		int max_index = ((int*)buffer)[3];
		items.resize(max_index);
		memcpy(items.data(), buffer + size_offset, max_index * size_item);
		delete[] buffer;
	}

	template <typename T> T ExtractSingle(int index) {
		T item;
		int size_item = m_NumDataFloats * sizeof(float);
		MakeCopyIn((unsigned char*)&item, index * size_item + ENTITY_OFFSET_INTS * sizeof(int), size_item);
		return item;
	}

	template <typename T> void PlaceAll(const std::vector<T>& items, int offset = 0) {
		m_SSBO->SetValues<T>(items, offset);
	}

	template <typename T> void PlaceSingle(int index, T item) {
		m_SSBO->SetSingleValue<T>(index, item);
	}

	bool						Init					( int expected_max_count = 100000 );
	void						Clear					( void );

	template <typename T> void	AddInstances			( std::vector<T>& items ) { AddInstancesInternal((unsigned char*)items.data(), items.size(), sizeof(T) ); }
	void						DeleteInstance			( int index );

	std::shared_ptr<unsigned char[]> MakeCopySync		( void );
	void						AccessData				( std::function<void(unsigned char* data, int size_bytes, EntityInfo item_info)>&& callback);
	void						QueueInfoRead			( void );
	inline int					GetLastKnownCount		( void ) const { return m_LastKnownInfo.p_Count; }
	inline int					GetCountSync			( void ) { SyncInfo(); return m_LastKnownInfo.p_Count; }

	inline QString				GetName					( void ) const { return m_Name; }
	inline SSBO*				GetSSBO					( void ) const { return m_SSBO; }
	inline SSBO*				GetOuputSSBO			( void ) const { return m_OutputSSBO; }
	inline SSBO*				GetControlSSBO			( void ) const { return m_ControlSSBO; }
	inline SSBO*				GetFreeListSSBO			( void ) const { return m_FreeList; }
	inline int					GetMaxCount				( void ) const { return m_MaxItems; }

	inline int					GetFloatsPer			( void ) const { return m_NumDataFloats; }
	inline const StructInfo&	GetSpecs				( void ) const { return m_Specs; }
	inline QString				GetGPUInsertion			( void ) const { return m_GPUInsertion; }
	inline QString				GetDoubleBufferGPUInsertion	( void ) const { return m_GPUInsertionDoubleBuffer; }
	inline QString				GetIDName				( void ) const { return m_IDName; }
	inline bool					IsDoubleBuffering		( void ) const { return m_DoubleBuffering; };

	void						SwapInputOutputSSBOs	( void );

protected:

	void						AddInstancesInternal	( unsigned char* data, int item_count, int item_size );
	void						MakeCopyIn				( unsigned char* ptr, int offset, int size );
	void						Destroy					( void );

	void						SyncInfo				( void );

	QString						m_Name;
	StructInfo					m_Specs;
	QString						m_GPUInsertion;
	QString						m_GPUInsertionDoubleBuffer;
	QString						m_IDName;
	int							m_MaxItems;
	int							m_IdOffset = -1;
	int							m_NumDataFloats = 0;

	bool						m_DoubleBuffering = true;
	SSBO*						m_SSBO = nullptr;
	SSBO*						m_OutputSSBO = nullptr;

	SSBO*						m_ControlSSBO = nullptr;
	SSBO*						m_FreeList = nullptr;
	SSBO*						m_CopyBuffer = nullptr;

	EntityInfo					m_LastKnownInfo;
	std::deque<WebGPUBuffer::AsyncToken<EntityInfo>> m_Pending;
};

} // namespace Neshny