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
		std::string name = member.getName();
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
void SerializeStructInfo(StructInfo& info, std::string get_base_str, std::string entity_name) {
	Serialiser<T> serializeFunc(info.p_Members);
	meta::doForAllMembers<T>(serializeFunc);

	std::vector<std::string> read_only_lines;
	std::vector<std::string> lines;

	read_only_lines.push_back(std::format("struct {} {{", entity_name));
	{
		std::vector<std::string> member_vars;
		for (auto member : info.p_Members) {
			member_vars.push_back(std::format("\t{}: {}", member.p_Name, MemberSpec::GetGPUType(member.p_Type)));
		}
		read_only_lines.push_back(JoinStrings(member_vars, ",\n"));
	}
	read_only_lines.push_back("}");

	read_only_lines.push_back(std::format("fn Get{0}(index: i32) -> {0} {{", entity_name));
	read_only_lines.push_back(get_base_str);
	read_only_lines.push_back(std::format("\tvar result: {};", entity_name));
	int pos_index = 0;
	std::vector<std::string> functions;
	for (auto member : info.p_Members) {
		auto member_name = member.p_Name;
		std::string get_syntax = MemberSpec::GetGPUGetSyntax(member.p_Type, pos_index, entity_name);
		read_only_lines.push_back(std::format("\tresult.{} = {};", member_name, get_syntax));
		functions.push_back(std::format("fn Get{2}{1}(index: i32) -> {0} {{\n", MemberSpec::GetGPUType(member.p_Type), member_name, entity_name) + get_base_str + std::format("\n\treturn {};\n}}", get_syntax));
		if (member.p_Type == MemberSpec::Type::T_INT) {
			functions.push_back(std::format("// defined macro Access{}{}(index)\n", entity_name, member_name));
			functions.push_back(std::format("#define Access{2}{0}(index) (b_{2}[(index) * FLOATS_PER_{2} + {1} + ENTITY_OFFSET_INTS])\n", member_name, pos_index, entity_name));
		} else if ((member.p_Type == MemberSpec::Type::T_IVEC2) || (member.p_Type == MemberSpec::Type::T_IVEC3) || (member.p_Type == MemberSpec::Type::T_IVEC4)) {
			functions.push_back(std::format("// defined macro Access{}{}_X/Y/Z/W(index)\n", entity_name, member_name));
			functions.push_back(std::format("#define Access{2}{0}_X(index) (b_{2}[(index) * FLOATS_PER_{2} + {1} + ENTITY_OFFSET_INTS])\n", member_name, pos_index, entity_name));
			functions.push_back(std::format("#define Access{2}{0}_Y(index) (b_{2}[(index) * FLOATS_PER_{2} + {1} + ENTITY_OFFSET_INTS])\n", member_name, pos_index + 1, entity_name));
			if (member.p_Type != MemberSpec::Type::T_IVEC2) {
				functions.push_back(std::format("#define Access{2}{0}_Z(index) (b_{2}[(index) * FLOATS_PER_{2} + {1} + ENTITY_OFFSET_INTS])\n", member_name, pos_index + 2, entity_name));
				if (member.p_Type == MemberSpec::Type::T_IVEC4) {
					functions.push_back(std::format("#define Access{2}{0}_W(index) (b_{2}[(index) * FLOATS_PER_{2} + {1} + ENTITY_OFFSET_INTS])\n", member_name, pos_index + 3, entity_name));
				}
			}
		}

		pos_index += member.p_Size / sizeof(float);
	}
	read_only_lines.push_back("\treturn result;\n}");
	read_only_lines.push_back(JoinStrings(functions, "\n"));

	lines.push_back(std::format("fn Set{0}(item: {0}, index: i32) {{", entity_name)); // TODO: make this a ref?
	lines.push_back(get_base_str);
	pos_index = 0;
	functions = {};
	for (auto member : info.p_Members) {
		std::string name = member.p_Name;
		std::string mod_str;
		std::string value_mod_str;
		if (member.p_Type == MemberSpec::T_INT) {
			mod_str = std::format("\t{2}_SET(base, {0}, item.{1});", pos_index, name, entity_name);
			value_mod_str = std::format("\t{1}_SET(base, {0}, value);", pos_index, entity_name);
		} else if (member.p_Type == MemberSpec::T_FLOAT) {
			mod_str = std::format("\t{2}_SET(base, {0}, bitcast<i32>(item.{1}));", pos_index, name, entity_name);
			value_mod_str = std::format("\t{1}_SET(base, {0}, bitcast<i32>(value));", pos_index, entity_name);
		} else if (member.p_Type == MemberSpec::T_VEC2) {
			mod_str = std::format("\t{3}_SET(base, {0}, bitcast<i32>(item.{2}.x)); {3}_SET(base, {1}, bitcast<i32>(item.{2}.y));", pos_index, pos_index + 1, name, entity_name);
			value_mod_str = std::format("\t{2}_SET(base, {0}, bitcast<i32>(value.x)); {2}_SET(base, {1}, bitcast<i32>(value.y));", pos_index, pos_index + 1, entity_name);
		} else if (member.p_Type == MemberSpec::T_VEC3) {
			mod_str = std::format("\t{4}_SET(base, {0}, bitcast<i32>(item.{3}.x)); {4}_SET(base, {1}, bitcast<i32>(item.{3}.y)); {4}_SET(base, {2}, bitcast<i32>(item.{3}.z));", pos_index, pos_index + 1, pos_index + 2, name, entity_name);
			value_mod_str = std::format("\t{3}_SET(base, {0}, bitcast<i32>(value.x)); {3}_SET(base, {1}, bitcast<i32>(value.y)); {3}_SET(base, {2}, bitcast<i32>(value.z));", pos_index, pos_index + 1, pos_index + 2, entity_name);
		} else if (member.p_Type == MemberSpec::T_VEC4) {
			mod_str = std::format("\t{5}_SET(base, {0}, bitcast<i32>(item.{4}.x)); {5}_SET(base, {1}, bitcast<i32>(item.{4}.y)); {5}_SET(base, {2}, bitcast<i32>(item.{4}.z)); {5}_SET(base, {3}, bitcast<i32>(item.{4}.w));", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, name, entity_name);
			value_mod_str = std::format("\t{4}_SET(base, {0}, bitcast<i32>(value.x)); {4}_SET(base, {1}, bitcast<i32>(value.y)); {4}_SET(base, {2}, bitcast<i32>(value.z)); {4}_SET(base, {3}, bitcast<i32>(value.w));", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, entity_name);
		} else if (member.p_Type == MemberSpec::T_IVEC2) {
			mod_str = std::format("\t{3}_SET(base, {0}, item.{2}.x); {3}_SET(base, {1}, item.{2}.y);", pos_index, pos_index + 1, name, entity_name);
			value_mod_str = std::format("\t{2}_SET(base, {0}, value.x); {2}_SET(base, {1}, value.y);", pos_index, pos_index + 1, entity_name);
		} else if (member.p_Type == MemberSpec::T_IVEC3) {
			mod_str = std::format("\t{4}_SET(base, {0}, item.{3}.x); {4}_SET(base, {1}, item.{3}.y); {4}_SET(base, {2}, item.{3}.z);", pos_index, pos_index + 1, pos_index + 2, name, entity_name);
			value_mod_str = std::format("\t{3}_SET(base, {0}, value.x); {3}_SET(base, {1}, value.y); {3}_SET(base, {2}, value.z);", pos_index, pos_index + 1, pos_index + 2, entity_name);
		} else if (member.p_Type == MemberSpec::T_IVEC4) {
			mod_str = std::format("\t{5}_SET(base, {0}, item.{4}.x); {5}_SET(base, {1}, item.{4}.y); {5}_SET(base, {2}, item.{4}.z); {5}_SET(base, {3}, item.{4}.w);", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, name, entity_name);
			value_mod_str = std::format("\t{4}_SET(base, {0}, value.x); {4}_SET(base, {1}, value.y); {4}_SET(base, {2}, value.z); {4}_SET(base, {3}, value.w);", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, entity_name);
		}
		lines.push_back(mod_str);
		functions.push_back(std::format("fn Set{2}{0}(index: i32, value: {1}) {{\n", name, MemberSpec::GetGPUType(member.p_Type), entity_name) + get_base_str + std::format("\n{}\n}}", value_mod_str));

		pos_index += member.p_Size / sizeof(float);
	}

	lines.push_back("}");
	lines.push_back(JoinStrings(functions, "\n"));

	info.p_GPUReadOnlyInsertion = JoinStrings(read_only_lines, "\n");
	//lines.push_front(info.p_GPUReadOnlyInsertion);
	//info.p_GPUInsertion = JoinStrings(lines, "\n");
	info.p_GPUInsertion = std::format("{}{}", info.p_GPUReadOnlyInsertion, JoinStrings(lines, "\n"));
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class GPUEntity {

public:

	// TODO: figure out better way of passing in T, perhaps template entire class

	template <typename T> GPUEntity(std::string name, int T::* id_ptr, std::string id_name, bool double_buffer = true) :
			m_Name(name)
			,m_NumDataFloats(sizeof(T) / sizeof(float))
			,m_IDName(id_name)
			,m_DoubleBuffering(double_buffer)
		{
		auto get_base_str = std::format("\tlet base = index * FLOATS_PER_{} + ENTITY_OFFSET_INTS;", m_Name);

		SerializeStructInfo<T>(m_Specs, get_base_str, m_Name);

		int pos_index = 0;
		for (const auto& member : m_Specs.p_Members) {
			if (member.p_Name == m_IDName) {
				m_IdOffset = pos_index;
				break;
			}
			pos_index += member.p_Size;
		}

		std::vector<std::string> insertion;
		insertion.push_back(std::format("#define FLOATS_PER_{} {}", m_Name, m_NumDataFloats));
		insertion.push_back(std::format("#define {0}_LOOKUP(base, index) (b_{0}[(base) + (index)])", m_Name));
		std::vector<std::string> insertion_double_buffer = insertion;
		insertion.push_back(std::format("#define {0}_SET(base, index, value) atomicStore(&b_{0}[(base) + (index)], value)", m_Name));
		insertion_double_buffer.push_back(std::format("#define {0}_SET(base, index, value) b_Output{0}[(base) + (index)] = (value)", m_Name));
		m_GPUInsertion = JoinStrings(insertion, "\n");
		m_GPUInsertionDoubleBuffer = JoinStrings(insertion_double_buffer, "\n");
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

	inline std::string_view		GetName					( void ) const { return m_Name; }
	inline SSBO*				GetSSBO					( void ) const { return m_SSBO; }
	inline SSBO*				GetOuputSSBO			( void ) const { return m_OutputSSBO; }
	inline SSBO*				GetControlSSBO			( void ) const { return m_ControlSSBO; }
	inline SSBO*				GetFreeListSSBO			( void ) const { return m_FreeList; }
	inline int					GetMaxCount				( void ) const { return m_MaxItems; }

	inline int					GetFloatsPer			( void ) const { return m_NumDataFloats; }
	inline const StructInfo&	GetSpecs				( void ) const { return m_Specs; }
	inline std::string_view		GetGPUInsertion			( void ) const { return m_GPUInsertion; }
	inline std::string_view		GetDoubleBufferGPUInsertion	( void ) const { return m_GPUInsertionDoubleBuffer; }
	inline std::string_view		GetIDName				( void ) const { return m_IDName; }
	inline bool					IsDoubleBuffering		( void ) const { return m_DoubleBuffering; };

	void						SwapInputOutputSSBOs	( void );

protected:

	void						AddInstancesInternal	( unsigned char* data, int item_count, int item_size );
	void						MakeCopyIn				( unsigned char* ptr, int offset, int size );
	void						Destroy					( void );

	void						SyncInfo				( void );

	std::string					m_Name;
	StructInfo					m_Specs;
	std::string					m_GPUInsertion;
	std::string					m_GPUInsertionDoubleBuffer;
	std::string					m_IDName;
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