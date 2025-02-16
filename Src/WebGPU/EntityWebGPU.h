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

template<typename>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template <class T>
using element_type = typename std::conditional<
	std::is_array<T>::value,
	typename std::remove_all_extents<T>::type,
	typename T::value_type
>::type;

template<typename T>
class Serialiser {
public:
	Serialiser(std::vector<MemberSpec>& specs/*, int T::* id_ref*/) : m_Specs(specs)/*, m_IdRef(id_ref)*/ {}

	template<typename Member>
	void operator()(Member& member) {
		std::string name = member.getName();
		using CurrentMemberType = meta::get_member_type<decltype(member)>;

		MemberSpec::Type type = MemberSpec::T_UNKNOWN;
		bool is_id = false;
		std::optional<std::size_t> array_count;

		//if constexpr (std::is_same<decltype(member.getPtr()), decltype(m_IdRef)>::value) { // TODO: automate ID name
		auto byte_size = sizeof(CurrentMemberType);
		if constexpr (is_std_array<CurrentMemberType>::value) {
			type = MemberSpec::GetTypeOf<element_type<CurrentMemberType>>();
			array_count = std::tuple_size<CurrentMemberType>::value;
			byte_size = sizeof(element_type<CurrentMemberType>);
		} else {
			type = MemberSpec::GetTypeOf<CurrentMemberType>();
		}
		m_Specs.push_back({ name, type, (int)byte_size, is_id, array_count });
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
			auto type_str = MemberSpec::GetGPUType(member.p_Type);
			member_vars.push_back(std::format("\t{}: {}", member.p_Name, member.p_ArrayCount.has_value() ? std::format("array<{}, {}>", type_str, *member.p_ArrayCount) : type_str));
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

		if (member.p_ArrayCount.has_value()) {
			std::size_t num = *member.p_ArrayCount;
			for (std::size_t i = 0; i < num; i++) {
				std::string get_syntax = MemberSpec::GetGPUGetSyntax(member.p_Type, pos_index, entity_name);
				pos_index += member.p_Size / sizeof(float);
				read_only_lines.push_back(std::format("\tresult.{}[{}] = {};", member_name, i, get_syntax));
			}
			continue;
		}

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

		if (member.p_ArrayCount.has_value()) {
			std::size_t num = *member.p_ArrayCount;
			for (std::size_t i = 0; i < num; i++) {
				auto [mod_str, value_mod_str] = MemberSpec::GetGPUSetSyntax(member.p_Type, pos_index, entity_name, std::format("{}[{}]", member.p_Name, i));
				lines.push_back(mod_str);
				pos_index += member.p_Size / sizeof(float);
			}
			continue;
		}

		auto [mod_str, value_mod_str] = MemberSpec::GetGPUSetSyntax(member.p_Type, pos_index, entity_name, member.p_Name);
		lines.push_back(mod_str);
		functions.push_back(std::format("fn Set{2}{0}(index: i32, value: {1}) {{\n", member.p_Name, MemberSpec::GetGPUType(member.p_Type), entity_name) + get_base_str + std::format("\n{}\n}}", value_mod_str));

		pos_index += member.p_Size / sizeof(float);
	}

	lines.push_back("}");
	lines.push_back(JoinStrings(functions, "\n"));

	info.p_GPUReadOnlyInsertion = JoinStrings(read_only_lines, "\n");
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
		// TODO: does this still make sense? should there be an offset?
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

	// will add instances in random order, no need to set IDs, they will be overwritten
	template <typename T> void	AddInstances			( std::vector<T>& items ) { AddInstancesInternal((unsigned char*)items.data(), items.size(), sizeof(T) ); }

	// this version preserves the ordering, but you must set the IDs correctly yourself, and will delete all prior instances
	template <typename T> void	SetInstances			( std::vector<T>& items ) { SetInstancesInternal((unsigned char*)items.data(), items.size(), sizeof(T) ); }

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
	void						SetInstancesInternal	( unsigned char* data, int item_count, int item_size );
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