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

	static QString GetGPUType(Type type) {
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
		return QString();
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

	static QString GetGPUGetSyntax(Type type, int index) {
#if defined(NESHNY_GL)
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
		} else if (type == MemberSpec::T_IVEC2) {
			return QString("ivec2(%3_LOOKUP(base, %1), %3_LOOKUP(base, %2))").arg(index).arg(index + 1).arg("%1");
		} else if (type == MemberSpec::T_IVEC3) {
			return QString("ivec3(%4_LOOKUP(base, %1), %4_LOOKUP(base, %2), %4_LOOKUP(base, %3))").arg(index).arg(index + 1).arg(index + 2).arg("%1");
		} else if (type == MemberSpec::T_IVEC4) {
			return QString("ivec4(%5_LOOKUP(base, %1), %5_LOOKUP(base, %2), %5_LOOKUP(base, %3), %5_LOOKUP(base, %4))").arg(index).arg(index + 1).arg(index + 2).arg(index + 3).arg("%1");
		} else if (type == MemberSpec::T_MAT3) {
			return QString("mat3(%10_LOOKUP(base, %1), %10_LOOKUP(base, %2), %10_LOOKUP(base, %3), %10_LOOKUP(base, %4), %10_LOOKUP(base, %5), %10_LOOKUP(base, %6), %10_LOOKUP(base, %7), %10_LOOKUP(base, %8), %10_LOOKUP(base, %9))")
				.arg(index).arg(index + 1).arg(index + 2)
				.arg(index + 3).arg(index + 4).arg(index + 5)
				.arg(index + 6).arg(index + 7).arg(index + 8).arg("%1");
		} else if (type == MemberSpec::T_MAT4) {
			return QString("mat4(%17_LOOKUP(base, %1), %17_LOOKUP(base, %2), %17_LOOKUP(base, %3), %17_LOOKUP(base, %4), %17_LOOKUP(base, %5), %17_LOOKUP(base, %6), %17_LOOKUP(base, %7), %17_LOOKUP(base, %8), %17_LOOKUP(base, %9), %17_LOOKUP(base, %10), %17_LOOKUP(base, %11), %17_LOOKUP(base, %12), %17_LOOKUP(base, %13), %17_LOOKUP(base, %14), %17_LOOKUP(base, %15), %17_LOOKUP(base, %16))")
				.arg(index).arg(index + 1).arg(index + 2).arg(index + 3)
				.arg(index + 4).arg(index + 5).arg(index + 6).arg(index + 7)
				.arg(index + 8).arg(index + 9).arg(index + 10).arg(index + 11)
				.arg(index + 12).arg(index + 13).arg(index + 14).arg(index + 15).arg("%1");
		}
#elif defined(NESHNY_WEBGPU)
		if (type == MemberSpec::T_INT) {
			return QString("%2_LOOKUP(base, %1)").arg(index).arg("%1");
		} else if (type == MemberSpec::T_UINT) {
			return QString("u32(%2_LOOKUP(base, %1))").arg(index).arg("%1");
		} else if (type == MemberSpec::T_FLOAT) {
			return QString("bitcast<f32>(%2_LOOKUP(base, %1))").arg(index).arg("%1");
		} else if (type == MemberSpec::T_VEC2) {
			return QString("vec2f(bitcast<f32>(%3_LOOKUP(base, %1)), bitcast<f32>(%3_LOOKUP(base, %2)))").arg(index).arg(index + 1).arg("%1");
		} else if (type == MemberSpec::T_VEC3) {
			return QString("vec3f(bitcast<f32>(%4_LOOKUP(base, %1)), bitcast<f32>(%4_LOOKUP(base, %2)), bitcast<f32>(%4_LOOKUP(base, %3)))").arg(index).arg(index + 1).arg(index + 2).arg("%1");
		} else if (type == MemberSpec::T_VEC4) {
			return QString("vec4f(bitcast<f32>(%5_LOOKUP(base, %1)), bitcast<f32>(%5_LOOKUP(base, %2)), bitcast<f32>(%5_LOOKUP(base, %3)), bitcast<f32>(%5_LOOKUP(base, %4)))").arg(index).arg(index + 1).arg(index + 2).arg(index + 3).arg("%1");
		} else if (type == MemberSpec::T_IVEC2) {
			return QString("vec2i(%3_LOOKUP(base, %1), %3_LOOKUP(base, %2))").arg(index).arg(index + 1).arg("%1");
		} else if (type == MemberSpec::T_IVEC3) {
			return QString("vec3i(%4_LOOKUP(base, %1), %4_LOOKUP(base, %2), %4_LOOKUP(base, %3))").arg(index).arg(index + 1).arg(index + 2).arg("%1");
		} else if (type == MemberSpec::T_IVEC4) {
			return QString("vec4i(%5_LOOKUP(base, %1), %5_LOOKUP(base, %2), %5_LOOKUP(base, %3), %5_LOOKUP(base, %4))").arg(index).arg(index + 1).arg(index + 2).arg(index + 3).arg("%1");
		} else if (type == MemberSpec::T_MAT3) {
			return QString("mat3x3f(%10_LOOKUP(base, %1), %10_LOOKUP(base, %2), %10_LOOKUP(base, %3), %10_LOOKUP(base, %4), %10_LOOKUP(base, %5), %10_LOOKUP(base, %6), %10_LOOKUP(base, %7), %10_LOOKUP(base, %8), %10_LOOKUP(base, %9))")
				.arg(index).arg(index + 1).arg(index + 2)
				.arg(index + 3).arg(index + 4).arg(index + 5)
				.arg(index + 6).arg(index + 7).arg(index + 8).arg("%1");
		} else if (type == MemberSpec::T_MAT4) {
			return QString("mat4x4f(%17_LOOKUP(base, %1), %17_LOOKUP(base, %2), %17_LOOKUP(base, %3), %17_LOOKUP(base, %4), %17_LOOKUP(base, %5), %17_LOOKUP(base, %6), %17_LOOKUP(base, %7), %17_LOOKUP(base, %8), %17_LOOKUP(base, %9), %17_LOOKUP(base, %10), %17_LOOKUP(base, %11), %17_LOOKUP(base, %12), %17_LOOKUP(base, %13), %17_LOOKUP(base, %14), %17_LOOKUP(base, %15), %17_LOOKUP(base, %16))")
				.arg(index).arg(index + 1).arg(index + 2).arg(index + 3)
				.arg(index + 4).arg(index + 5).arg(index + 6).arg(index + 7)
				.arg(index + 8).arg(index + 9).arg(index + 10).arg(index + 11)
				.arg(index + 12).arg(index + 13).arg(index + 14).arg(index + 15).arg("%1");
		}
#endif

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

} // namespace Neshny