////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

struct EntityInfo {
	int	p_Count = 0;
	int p_FreeCount = 0;
	int p_NextId = 0;
	int p_MaxIndex = 0;
};

const int ENTITY_OFFSET_INTS = 0;

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
void SerializeStructInfo(StructInfo& info, std::string get_base_str, std::string_view entity_name) {
	Serialiser<T> serializeFunc(info.p_Members);
	meta::doForAllMembers<T>(serializeFunc);

	std::vector<std::string> read_only_lines;
	std::list<std::string> lines;

	read_only_lines.push_back(std::format("struct {0} {{", entity_name));
	for (auto member : info.p_Members) {
		read_only_lines.push_back(std::format("\t{} {};", MemberSpec::GetGPUType(member.p_Type), member.p_Name));
	}
	read_only_lines.push_back("};");

	read_only_lines.push_back(std::format("{0} Get{0}(int index) {{", entity_name));
	read_only_lines.push_back(get_base_str);
	read_only_lines.push_back(std::format("\t{0} result;", entity_name));
	int pos_index = 0;
	std::vector<std::string> functions;
	for (auto member : info.p_Members) {
		std::string get_syntax = MemberSpec::GetGPUGetSyntax(member.p_Type, pos_index, std::string(entity_name));
		read_only_lines.push_back(std::format("\tresult.{0} = {1};", member.p_Name, get_syntax));
		functions.push_back(std::format("{0} Get{2}{1}(int index) {{\n", MemberSpec::GetGPUType(member.p_Type), member.p_Name, entity_name) + get_base_str + std::format("\n\treturn {0};\n}}", get_syntax));
		if (member.p_Type == MemberSpec::Type::T_INT) {
			functions.push_back(std::format("#define Access{2}{0}(index) (b_{2}.i[(index) * FLOATS_PER_{2} + {1}])\n", member.p_Name, pos_index, entity_name));
		} else if ((member.p_Type == MemberSpec::Type::T_IVEC2) || (member.p_Type == MemberSpec::Type::T_IVEC3) || (member.p_Type == MemberSpec::Type::T_IVEC4)) {
			functions.push_back(std::format("#define Access{2}{0}_X(index) (b_{2}.i[(index) * FLOATS_PER_{2} + {1}])\n", member.p_Name, pos_index, entity_name));
			functions.push_back(std::format("#define Access{2}{0}_Y(index) (b_{2}.i[(index) * FLOATS_PER_{2} + {1}])\n", member.p_Name, pos_index + 1, entity_name));
			if (member.p_Type != MemberSpec::Type::T_IVEC2) {
				functions.push_back(std::format("#define Access{2}{0}_Z(index) (b_{2}.i[(index) * FLOATS_PER_{2} + {1}])\n", member.p_Name, pos_index + 2, entity_name));
				if (member.p_Type == MemberSpec::Type::T_IVEC4) {
					functions.push_back(std::format("#define Access{2}{0}_W(index) (b_{2}.i[(index) * FLOATS_PER_{2} + {1}])\n", member.p_Name, pos_index + 3, entity_name));
				}
			}
		}

		pos_index += member.p_Size / sizeof(float);
	}
	read_only_lines.push_back("\treturn result;\n}");
	read_only_lines.push_back(JoinStrings(functions, "\n"));

	lines.push_back(std::format("void Set{0}({0} item, int index) {{", entity_name));
	lines.push_back(get_base_str);
	pos_index = 0;
	functions = {};
	for (auto member : info.p_Members) {
		std::string mod_str;
		std::string value_mod_str;
		if (member.p_Type == MemberSpec::T_INT) {
			mod_str = std::format("\t{2}_SET(base, {0}, item.{1});", pos_index, member.p_Name, entity_name);
			value_mod_str = std::format("\t{1}_SET(base, {0}, value);", pos_index, entity_name);
		} else if (member.p_Type == MemberSpec::T_FLOAT) {
			mod_str = std::format("\t{2}_SET(base, {0}, floatBitsToInt(item.{1}));", pos_index, member.p_Name, entity_name);
			value_mod_str = std::format("\t{1}_SET(base, {0}, floatBitsToInt(value));", pos_index, entity_name);
		} else if (member.p_Type == MemberSpec::T_VEC2) {
			mod_str = std::format("\t{3}_SET(base, {0}, floatBitsToInt(item.{2}.x)); {3}_SET(base, {1}, floatBitsToInt(item.{2}.y));", pos_index, pos_index + 1, member.p_Name, entity_name);
			value_mod_str = std::format("\t{2}_SET(base, {0}, floatBitsToInt(value.x)); {2}_SET(base, {1}, floatBitsToInt(value.y));", pos_index, pos_index + 1, entity_name);
		} else if (member.p_Type == MemberSpec::T_VEC3) {
			mod_str = std::format("\t{4}_SET(base, {0}, floatBitsToInt(item.{3}.x)); {4}_SET(base, {1}, floatBitsToInt(item.{3}.y)); {4}_SET(base, {2}, floatBitsToInt(item.{3}.z));", pos_index, pos_index + 1, pos_index + 2, member.p_Name, entity_name);
			value_mod_str = std::format("\t{3}_SET(base, {0}, floatBitsToInt(value.x)); {3}_SET(base, {1}, floatBitsToInt(value.y)); {3}_SET(base, {2}, floatBitsToInt(value.z));", pos_index, pos_index + 1, pos_index + 2, entity_name);
		} else if (member.p_Type == MemberSpec::T_VEC4) {
			mod_str = std::format("\t{5}_SET(base, {0}, floatBitsToInt(item.{4}.x)); {5}_SET(base, {1}, floatBitsToInt(item.{4}.y)); {5}_SET(base, {2}, floatBitsToInt(item.{4}.z)); {5}_SET(base, {3}, floatBitsToInt(item.{4}.w));", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, member.p_Name, entity_name);
			value_mod_str = std::format("\t{4}_SET(base, {0}, floatBitsToInt(value.x)); {4}_SET(base, {1}, floatBitsToInt(value.y)); {4}_SET(base, {2}, floatBitsToInt(value.z)); {4}_SET(base, {3}, floatBitsToInt(value.w));", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, entity_name);
		} else if (member.p_Type == MemberSpec::T_IVEC2) {
			mod_str = std::format("\t{3}_SET(base, {0}, item.{2}.x); {3}_SET(base, {1}, item.{2}.y);", pos_index, pos_index + 1, member.p_Name, entity_name);
			value_mod_str = std::format("\t{2}_SET(base, {0}, value.x); {2}_SET(base, {1}, value.y);", pos_index, pos_index + 1, entity_name);
		} else if (member.p_Type == MemberSpec::T_IVEC3) {
			mod_str = std::format("\t{4}_SET(base, {0}, item.{3}.x); {4}_SET(base, {1}, item.{3}.y); {4}_SET(base, {2}, item.{3}.z);", pos_index, pos_index + 1, pos_index + 2, member.p_Name, entity_name);
			value_mod_str = std::format("\t{3}_SET(base, {0}, value.x); {3}_SET(base, {1}, value.y); {3}_SET(base, {2}, value.z);", pos_index, pos_index + 1, pos_index + 2, entity_name);
		} else if (member.p_Type == MemberSpec::T_IVEC4) {
			mod_str = std::format("\t{5}_SET(base, {0}, item.{4}.x); {5}_SET(base, {1}, item.{4}.y); {5}_SET(base, {2}, item.{4}.z); {5}_SET(base, {3}, item.{4}.w);", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, member.p_Name, entity_name);
			value_mod_str = std::format("\t{4}_SET(base, {0}, value.x); {4}_SET(base, {1}, value.y); {4}_SET(base, {2}, value.z); {4}_SET(base, {3}, value.w);", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, entity_name);
		}
		lines.push_back(mod_str);
		functions.push_back(std::format("void Set{2}{0}(int index, {1} value) {{\n", member.p_Name, MemberSpec::GetGPUType(member.p_Type), entity_name) + get_base_str + std::format("\n{0}\n}}", value_mod_str));

		pos_index += member.p_Size / sizeof(float);
	}

	lines.push_back("}");
	lines.push_back(JoinStrings(functions, "\n"));

	info.p_GPUReadOnlyInsertion = JoinStrings(read_only_lines, "\n");
	lines.push_front(info.p_GPUReadOnlyInsertion);
	info.p_GPUInsertion = JoinStrings(lines, "\n");
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

	template <typename T> GPUEntity(std::string_view name, DeleteMode delete_mode, int T::* id_ptr, std::string_view id_name, bool double_buffer = true) :
			m_DeleteMode(delete_mode)
			,m_Name(name)
			,m_NumDataFloats(sizeof(T) / sizeof(float))
			,m_IDName(id_name)
			,m_DoubleBuffering(double_buffer)
		{
		std::string get_base_str = std::format("\tint base = index * FLOATS_PER_{0};", m_Name);

		SerializeStructInfo<T>(m_Specs, get_base_str, name);

		std::vector<std::string> insertion;
		insertion.push_back(std::format("#define FLOATS_PER_{0} {1}", m_Name, m_NumDataFloats));
		insertion.push_back(std::format("#define {0}_LOOKUP(base, index) (b_{0}.i[(base) + (index)])", m_Name));
		std::vector<std::string> insertion_double_buffer = insertion;
		insertion.push_back(std::format("#define {0}_SET(base, index, value) (b_{0}.i[(base) + (index)] = (value))", m_Name));
		insertion_double_buffer.push_back(std::format("#define {0}_SET(base, index, value) (b_Output{0}.i[(base) + (index)] = (value))", m_Name));
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
		int total_size = size_offset + size_item * m_Info.p_MaxIndex;
		unsigned char* buffer = new unsigned char[total_size];
		MakeCopyIn(buffer, 0, total_size);
		items.resize(m_Info.p_MaxIndex);
		memcpy(items.data(), buffer + size_offset, m_Info.p_MaxIndex * size_item);
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
	int							AddInstance				( void* data, int* index = nullptr );
	void						DeleteInstance			( int index );

	std::string					GetDebugInfo			( void );
	std::shared_ptr<unsigned char[]> MakeCopySync		( void );

	inline std::string_view		GetName					( void ) const { return m_Name; }
	inline SSBO*				GetSSBO					( void ) const { return m_SSBO; }
	inline SSBO*				GetOuputSSBO			( void ) const { return m_OutputSSBO; }
	inline SSBO*				GetControlSSBO			( void ) const { return m_ControlSSBO; }
	inline SSBO*				GetFreeListSSBO			( void ) const { return m_FreeList; }
	inline int					GetMaxCount				( void ) const { return m_MaxItems; }
	inline int					GetCount				( void ) const { return m_Info.p_Count; }
	inline int					GetMaxIndex				( void ) const { return m_Info.p_MaxIndex; }
	inline int					GetNextId				( void ) const { return m_Info.p_NextId; }
	inline int					GetFreeCount			( void ) const { return m_Info.p_FreeCount; }

	inline int					GetFloatsPer			( void ) const { return m_NumDataFloats; }
	inline const StructInfo&	GetSpecs				( void ) const { return m_Specs; }
	inline std::string_view		GetGPUInsertion			( void ) const { return m_GPUInsertion; }
	inline std::string_view		GetDoubleBufferGPUInsertion	( void ) const { return m_GPUInsertionDoubleBuffer; }
	inline std::string_view		GetIDName				( void ) const { return m_IDName; }

	inline bool					IsDoubleBuffering		( void ) const { return m_DoubleBuffering; };

	inline DeleteMode			GetDeleteMode			( void ) const { return m_DeleteMode; }
	void						ProcessMoveDeaths		( int death_count );
	void						ProcessMoveCreates		( int new_count, int new_next_id );
	void						ProcessStableDeaths		( int death_count );
	void						ProcessStableCreates	( int new_max_id, int new_next_id, int new_free_count );

	void						SwapInputOutputSSBOs	( void );

protected:

	void						AddInstancesInternal	( unsigned char* data, int item_count, int item_size );
	void						MakeCopyIn				( unsigned char* ptr, int offset, int size );
	void						Destroy					( void );

	DeleteMode					m_DeleteMode;
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

	EntityInfo					m_Info;
};

} // namespace Neshny