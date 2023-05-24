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
		T_IVEC4
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
		} else if (type == MemberSpec::T_IVEC2) {
			return "ivec2";
		} else if (type == MemberSpec::T_IVEC3) {
			return "ivec3";
		} else if (type == MemberSpec::T_IVEC4) {
			return "ivec4";
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
		} else if (type == MemberSpec::T_IVEC2) {
			return sizeof(int) * 2;
		} else if (type == MemberSpec::T_IVEC3) {
			return sizeof(int) * 3;
		} else if (type == MemberSpec::T_IVEC4) {
			return sizeof(int) * 4;
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
		} else if (type == MemberSpec::T_IVEC2) {
			return QString("ivec2(%3_LOOKUP(base, %1), %3_LOOKUP(base, %2))").arg(index).arg(index + 1).arg("%1");
		} else if (type == MemberSpec::T_IVEC3) {
			return QString("ivec3(%4_LOOKUP(base, %1), %4_LOOKUP(base, %2), %4_LOOKUP(base, %3))").arg(index).arg(index + 1).arg(index + 2).arg("%1");
		} else if (type == MemberSpec::T_IVEC4) {
			return QString("ivec4(%5_LOOKUP(base, %1), %5_LOOKUP(base, %2), %5_LOOKUP(base, %3), %5_LOOKUP(base, %4))").arg(index).arg(index + 1).arg(index + 2).arg(index + 3).arg("%1");
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

} // namespace Neshny