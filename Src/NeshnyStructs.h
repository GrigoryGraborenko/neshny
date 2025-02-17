////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

struct MemberSpec {

	enum Type {
		T_UNKNOWN,
		T_INT,
		T_UINT,
		T_FLOAT,
		T_VEC2,
		T_VEC3,
		T_VEC4,
		T_IVEC2,
		T_IVEC3,
		T_IVEC4,
		T_MAT3,
		T_MAT4
	};

	template<typename T>
	static Type GetTypeOf(void) {
		if constexpr (std::is_same<T, int>::value) {
			return MemberSpec::T_INT;
		} else if constexpr (std::is_same<T, unsigned int>::value) {
			return MemberSpec::T_UINT;
		} else if constexpr (std::is_same<T, float>::value) {
			return MemberSpec::T_FLOAT;
		} else if constexpr (std::is_same<T, fVec2>::value) {
			return MemberSpec::T_VEC2;
		} else if constexpr (std::is_same<T, fVec3>::value) {
			return MemberSpec::T_VEC3;
		} else if constexpr (std::is_same<T, fVec4>::value) {
			return MemberSpec::T_VEC4;
		} else if constexpr (std::is_same<T, iVec2>::value) {
			return MemberSpec::T_IVEC2;
		} else if constexpr (std::is_same<T, iVec3>::value) {
			return MemberSpec::T_IVEC3;
		} else if constexpr (std::is_same<T, iVec4>::value) {
			return MemberSpec::T_IVEC4;
		} else if constexpr (std::is_same<T, fMatrix3>::value) {
			return MemberSpec::T_MAT3;
		} else if constexpr (std::is_same<T, fMatrix4>::value) {
			return MemberSpec::T_MAT4;
		}
		return MemberSpec::T_UNKNOWN;
	}

	static std::string GetGPUType(Type type) {
#if defined(NESHNY_GL)
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
		} else if (type == MemberSpec::T_IVEC2) {
			return "ivec2";
		} else if (type == MemberSpec::T_IVEC3) {
			return "ivec3";
		} else if (type == MemberSpec::T_IVEC4) {
			return "ivec4";
		} else if (type == MemberSpec::T_MAT3) {
			return "mat3";
		} else if (type == MemberSpec::T_MAT4) {
			return "mat4";
		}
#elif defined(NESHNY_WEBGPU)
		if (type == MemberSpec::T_INT) {
			return "i32";
		} else if (type == MemberSpec::T_UINT) {
			return "u32";
		} else if (type == MemberSpec::T_FLOAT) {
			return "f32";
		} else if (type == MemberSpec::T_VEC2) {
			return "vec2f";
		} else if (type == MemberSpec::T_VEC3) {
			return "vec3f";
		} else if (type == MemberSpec::T_VEC4) {
			return "vec4f";
		} else if (type == MemberSpec::T_IVEC2) {
			return "vec2i";
		} else if (type == MemberSpec::T_IVEC3) {
			return "vec3i";
		} else if (type == MemberSpec::T_IVEC4) {
			return "vec4i";
		} else if (type == MemberSpec::T_MAT3) {
			return "mat3x3f";
		} else if (type == MemberSpec::T_MAT4) {
			return "mat4x4f";
		}
#endif
		return std::string();
	}

	static int GetGPUTypeSizeBytes(Type type) {
		if (type == MemberSpec::T_VEC2) {
			return sizeof(float) * 2;
		} else if (type == MemberSpec::T_VEC3) {
			return sizeof(float) * 3;
		} else if (type == MemberSpec::T_VEC4) {
			return sizeof(float) * 4;
		} else if (type == MemberSpec::T_IVEC2) {
			return sizeof(int) * 2;
		} else if (type == MemberSpec::T_IVEC3) {
			return sizeof(int) * 3;
		} else if (type == MemberSpec::T_IVEC4) {
			return sizeof(int) * 4;
		} else if (type == MemberSpec::T_MAT3) {
			return sizeof(float) * 9;
		} else if (type == MemberSpec::T_MAT4) {
			return sizeof(float) * 16;
		}
		return sizeof(int);
	}

	static std::string GetGPUGetSyntax(Type type, int index, std::string entity_name) {
#if defined(NESHNY_GL)
		if (type == MemberSpec::T_INT) {
			return std::format("{}_LOOKUP(base, {})", entity_name, index);
		} else if (type == MemberSpec::T_UINT) {
			return std::format("uint({}_LOOKUP(base, {}))", entity_name, index);
		} else if (type == MemberSpec::T_FLOAT) {
			return std::format("intBitsToFloat({}_LOOKUP(base, {}))", entity_name, index);
		} else if (type == MemberSpec::T_VEC2) {
			return std::format("vec2(intBitsToFloat({2}_LOOKUP(base, {0})), intBitsToFloat({2}_LOOKUP(base, {1})))", index, index + 1, entity_name);
		} else if (type == MemberSpec::T_VEC3) {
			return std::format("vec3(intBitsToFloat({3}_LOOKUP(base, {0})), intBitsToFloat({3}_LOOKUP(base, {1})), intBitsToFloat({3}_LOOKUP(base, {2})))", index, index + 1, index + 2, entity_name);
		} else if (type == MemberSpec::T_VEC4) {
			return std::format("vec4(intBitsToFloat({4}_LOOKUP(base, {0})), intBitsToFloat({4}_LOOKUP(base, {1})), intBitsToFloat({4}_LOOKUP(base, {2})), intBitsToFloat({4}_LOOKUP(base, {3})))", index, index + 1, index + 2, index + 3, entity_name);
		} else if (type == MemberSpec::T_IVEC2) {
			return std::format("ivec2({2}_LOOKUP(base, {0}), {2}_LOOKUP(base, {1}))", index, index + 1, entity_name);
		} else if (type == MemberSpec::T_IVEC3) {
			return std::format("ivec3({3}_LOOKUP(base, {0}), {3}_LOOKUP(base, {1}), {3}_LOOKUP(base, {2}))", index, index + 1, index + 2, entity_name);
		} else if (type == MemberSpec::T_IVEC4) {
			return std::format("ivec4({4}_LOOKUP(base, {0}), {4}_LOOKUP(base, {1}), {4}_LOOKUP(base, {2}), {4}_LOOKUP(base, {3}))", index, index + 1, index + 2, index + 3, entity_name);
		} else if (type == MemberSpec::T_MAT3) {
			return std::format("mat3({9}_LOOKUP(base, {0}), {9}_LOOKUP(base, {1}), {9}_LOOKUP(base, {2}), {9}_LOOKUP(base, {3}), {9}_LOOKUP(base, {4}), {9}_LOOKUP(base, {5}), {9}_LOOKUP(base, {6}), {9}_LOOKUP(base, {7}), {9}_LOOKUP(base, {8}))"
				,index, index + 1, index + 2
				,index + 3, index + 4, index + 5
				,index + 6, index + 7, index + 8, entity_name);
		} else if (type == MemberSpec::T_MAT4) {
			return std::format("mat4({16}_LOOKUP(base, {0}), {16}_LOOKUP(base, {1}), {16}_LOOKUP(base, {2}), {16}_LOOKUP(base, {3}), {16}_LOOKUP(base, {4}), {16}_LOOKUP(base, {5}), {16}_LOOKUP(base, {6}), {16}_LOOKUP(base, {7}), {16}_LOOKUP(base, {8}), {16}_LOOKUP(base, {9}), {16}_LOOKUP(base, {10}), {16}_LOOKUP(base, {11}), {16}_LOOKUP(base, {12}), {16}_LOOKUP(base, {13}), {16}_LOOKUP(base, {14}), {16}_LOOKUP(base, {15}))"
				,index, index + 1, index + 2, index + 3
				,index + 4, index + 5, index + 6, index + 7
				,index + 8, index + 9, index + 10, index + 11
				,index + 12, index + 13, index + 14, index + 15, entity_name);
		}
#elif defined(NESHNY_WEBGPU)
		if (type == MemberSpec::T_INT) {
			return std::format("{}_LOOKUP(base, {})", entity_name, index);
		} else if (type == MemberSpec::T_UINT) {
			return std::format("u32({}_LOOKUP(base, {}))", entity_name, index);
		} else if (type == MemberSpec::T_FLOAT) {
			return std::format("bitcast<f32>({}_LOOKUP(base, {}))", entity_name, index);
		} else if (type == MemberSpec::T_VEC2) {
			return std::format("vec2f(bitcast<f32>({2}_LOOKUP(base, {0})), bitcast<f32>({2}_LOOKUP(base, {1})))", index, index + 1, entity_name);
		} else if (type == MemberSpec::T_VEC3) {
			return std::format("vec3f(bitcast<f32>({3}_LOOKUP(base, {0})), bitcast<f32>({3}_LOOKUP(base, {1})), bitcast<f32>({3}_LOOKUP(base, {2})))", index, index + 1, index + 2, entity_name);
		} else if (type == MemberSpec::T_VEC4) {
			return std::format("vec4f(bitcast<f32>({4}_LOOKUP(base, {0})), bitcast<f32>({4}_LOOKUP(base, {1})), bitcast<f32>({4}_LOOKUP(base, {2})), bitcast<f32>({4}_LOOKUP(base, {3})))", index, index + 1, index + 2, index + 3, entity_name);
		} else if (type == MemberSpec::T_IVEC2) {
			return std::format("vec2i({2}_LOOKUP(base, {0}), {2}_LOOKUP(base, {1}))", index, index + 1, entity_name);
		} else if (type == MemberSpec::T_IVEC3) {
			return std::format("vec3i({3}_LOOKUP(base, {0}), {3}_LOOKUP(base, {1}), {3}_LOOKUP(base, {2}))", index, index + 1, index + 2, entity_name);
		} else if (type == MemberSpec::T_IVEC4) {
			return std::format("vec4i({4}_LOOKUP(base, {0}), {4}_LOOKUP(base, {1}), {4}_LOOKUP(base, {2}), {4}_LOOKUP(base, {3}))", index, index + 1, index + 2, index + 3, entity_name);
		} else if (type == MemberSpec::T_MAT3) {
			return std::format("mat3x3f({9}_LOOKUP(base, {0}), {9}_LOOKUP(base, {1}), {9}_LOOKUP(base, {2}), {9}_LOOKUP(base, {3}), {9}_LOOKUP(base, {4}), {9}_LOOKUP(base, {5}), {9}_LOOKUP(base, {6}), {9}_LOOKUP(base, {7}), {9}_LOOKUP(base, {8}))"
				,index, index + 1, index + 2
				,index + 3, index + 4, index + 5
				,index + 6, index + 7, index + 8, entity_name);
		} else if (type == MemberSpec::T_MAT4) {
			return std::format("mat4x4f({16}_LOOKUP(base, {0}), {16}_LOOKUP(base, {1}), {16}_LOOKUP(base, {2}), {16}_LOOKUP(base, {3}), {16}_LOOKUP(base, {4}), {16}_LOOKUP(base, {5}), {16}_LOOKUP(base, {6}), {16}_LOOKUP(base, {7}), {16}_LOOKUP(base, {8}), {16}_LOOKUP(base, {9}), {16}_LOOKUP(base, {10}), {16}_LOOKUP(base, {11}), {16}_LOOKUP(base, {12}), {16}_LOOKUP(base, {13}), {16}_LOOKUP(base, {14}), {16}_LOOKUP(base, {15}))"
				,index, index + 1, index + 2, index + 3
				,index + 4, index + 5, index + 6, index + 7
				,index + 8, index + 9, index + 10, index + 11
				,index + 12, index + 13, index + 14, index + 15, entity_name);
		}
#endif

		return std::string();
	}

	static std::pair<std::string, std::string> GetGPUSetSyntax(Type type, int pos_index, std::string entity_name, std::string name) {
		std::string mod_str;
		std::string value_mod_str;

#if defined(NESHNY_GL)
		// TODO: fill this in and use it
#elif defined(NESHNY_WEBGPU)
		if (type == MemberSpec::T_INT) {
			mod_str = std::format("\t{2}_SET(base, {0}, item.{1});", pos_index, name, entity_name);
			value_mod_str = std::format("\t{1}_SET(base, {0}, value);", pos_index, entity_name);
		} else if (type == MemberSpec::T_UINT) {
			mod_str = std::format("\t{2}_SET(base, {0}, bitcast<i32>(item.{1}));", pos_index, name, entity_name);
			value_mod_str = std::format("\t{1}_SET(base, {0}, bitcast<i32>(value));", pos_index, entity_name);
		} else if (type == MemberSpec::T_FLOAT) {
			mod_str = std::format("\t{2}_SET(base, {0}, bitcast<i32>(item.{1}));", pos_index, name, entity_name);
			value_mod_str = std::format("\t{1}_SET(base, {0}, bitcast<i32>(value));", pos_index, entity_name);
		} else if (type == MemberSpec::T_VEC2) {
			mod_str = std::format("\t{3}_SET(base, {0}, bitcast<i32>(item.{2}.x)); {3}_SET(base, {1}, bitcast<i32>(item.{2}.y));", pos_index, pos_index + 1, name, entity_name);
			value_mod_str = std::format("\t{2}_SET(base, {0}, bitcast<i32>(value.x)); {2}_SET(base, {1}, bitcast<i32>(value.y));", pos_index, pos_index + 1, entity_name);
		} else if (type == MemberSpec::T_VEC3) {
			mod_str = std::format("\t{4}_SET(base, {0}, bitcast<i32>(item.{3}.x)); {4}_SET(base, {1}, bitcast<i32>(item.{3}.y)); {4}_SET(base, {2}, bitcast<i32>(item.{3}.z));", pos_index, pos_index + 1, pos_index + 2, name, entity_name);
			value_mod_str = std::format("\t{3}_SET(base, {0}, bitcast<i32>(value.x)); {3}_SET(base, {1}, bitcast<i32>(value.y)); {3}_SET(base, {2}, bitcast<i32>(value.z));", pos_index, pos_index + 1, pos_index + 2, entity_name);
		} else if (type == MemberSpec::T_VEC4) {
			mod_str = std::format("\t{5}_SET(base, {0}, bitcast<i32>(item.{4}.x)); {5}_SET(base, {1}, bitcast<i32>(item.{4}.y)); {5}_SET(base, {2}, bitcast<i32>(item.{4}.z)); {5}_SET(base, {3}, bitcast<i32>(item.{4}.w));", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, name, entity_name);
			value_mod_str = std::format("\t{4}_SET(base, {0}, bitcast<i32>(value.x)); {4}_SET(base, {1}, bitcast<i32>(value.y)); {4}_SET(base, {2}, bitcast<i32>(value.z)); {4}_SET(base, {3}, bitcast<i32>(value.w));", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, entity_name);
		} else if (type == MemberSpec::T_IVEC2) {
			mod_str = std::format("\t{3}_SET(base, {0}, item.{2}.x); {3}_SET(base, {1}, item.{2}.y);", pos_index, pos_index + 1, name, entity_name);
			value_mod_str = std::format("\t{2}_SET(base, {0}, value.x); {2}_SET(base, {1}, value.y);", pos_index, pos_index + 1, entity_name);
		} else if (type == MemberSpec::T_IVEC3) {
			mod_str = std::format("\t{4}_SET(base, {0}, item.{3}.x); {4}_SET(base, {1}, item.{3}.y); {4}_SET(base, {2}, item.{3}.z);", pos_index, pos_index + 1, pos_index + 2, name, entity_name);
			value_mod_str = std::format("\t{3}_SET(base, {0}, value.x); {3}_SET(base, {1}, value.y); {3}_SET(base, {2}, value.z);", pos_index, pos_index + 1, pos_index + 2, entity_name);
		} else if (type == MemberSpec::T_IVEC4) {
			mod_str = std::format("\t{5}_SET(base, {0}, item.{4}.x); {5}_SET(base, {1}, item.{4}.y); {5}_SET(base, {2}, item.{4}.z); {5}_SET(base, {3}, item.{4}.w);", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, name, entity_name);
			value_mod_str = std::format("\t{4}_SET(base, {0}, value.x); {4}_SET(base, {1}, value.y); {4}_SET(base, {2}, value.z); {4}_SET(base, {3}, value.w);", pos_index, pos_index + 1, pos_index + 2, pos_index + 3, entity_name);
		}
		// TODO: fill in mat3x3 and mat4x4
#endif
		return { mod_str, value_mod_str };
	}


	std::string					p_Name;
	Type						p_Type;
	int							p_Size;
	bool						p_IsID = false;
	std::optional<std::size_t>	p_ArrayCount;
};

struct StructInfo {
	std::vector<MemberSpec>	p_Members;
	std::string				p_GPUInsertion;
	std::string				p_GPUReadOnlyInsertion;
};

} // namespace Neshny