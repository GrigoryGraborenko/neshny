////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(NESHNY_WEBGPU)
namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
PipelineStage::PipelineStage(RunType type, GPUEntity* entity, RenderableBuffer* buffer, BaseCache* cache, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, SSBO* control_ssbo, int iterations, WebGPUPipeline::RenderParams render_params) :
	m_RunType			( type )
	,m_Entity			( entity )
	,m_Buffer			( buffer )
	,m_Cache			( cache )
	,m_ShaderName		( shader_name )
	,m_ReplaceMain		( replace_main )
	,m_ShaderDefines	( shader_defines )
	,m_Iterations		( iterations )
	,m_ControlSSBO		( control_ssbo )
	,m_RenderParams		( render_params )
{
	if (cache) {
		cache->Bind(*this);
	}
	const auto& limits = Core::Singleton().GetLimits();
	m_LocalSize = iVec3(limits.maxComputeInvocationsPerWorkgroup, 1, 1);
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ &entity, false });
	if (cache) {
		cache->Bind(*this);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddCreatableEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ &entity, true });
	if (cache) {
		cache->Bind(*this);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool PipelineStage::OutputResults::GetValue(QString name, int& value) const {
	for (const auto& result : p_Results) {
		if (result.first == name) {
			value = result.second;
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
QString PipelineStage::GetDataVectorStructCode(const AddedDataVector& data_vect, QString& count_var, QString& offset_var, bool read_only) {

	QStringList insertion;
	insertion += QString("struct %1 {").arg(data_vect.p_Name);

	QStringList function(QString("fn Get%1(base_index: i32) -> %1 {\n\tvar item: %1;").arg(data_vect.p_Name));
	QStringList members;
	if (read_only) {
		function += QString("\tlet index = io%1Offset + (base_index * %2);").arg(data_vect.p_Name).arg(data_vect.p_NumIntsPerItem);
	} else {
		function += QString("\tlet index = Get_io%1Offset + (base_index * %2);").arg(data_vect.p_Name).arg(data_vect.p_NumIntsPerItem);
	}

	int pos_index = 0;
	for (const auto& member : data_vect.p_Members) {
		members += QString("\t%1: %2").arg(member.p_Name).arg(MemberSpec::GetGPUType(member.p_Type));

		QString name = member.p_Name;
		if (read_only) {

			if (member.p_Type == MemberSpec::T_INT) {
				function += QString("\titem.%1 = b_Control[index + %2];").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_FLOAT) {
				function += QString("\titem.%1 = bitcast<f32>(b_Control[index + %2]);").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_VEC2) {
				function += QString("\titem.%1 = vec2f(bitcast<f32>(b_Control[index + %2]), bitcast<f32>(b_Control[index + %2 + 1]));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_VEC3) {
				function += QString("\titem.%1 = vec3f(bitcast<f32>(b_Control[index + %2]), bitcast<f32>(b_Control[index + %2 + 1]), bitcast<f32>(b_Control[index + %2 + 2]));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_VEC4) {
				function += QString("\titem.%1 = vec4f(bitcast<f32>(b_Control[index + %2]), bitcast<f32>(b_Control[index + %2 + 1]), bitcast<f32>(b_Control[index + %2 + 2]), bitcast<f32>(b_Control[index + %2 + 3]));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_IVEC2) {
				function += QString("\titem.%1 = vec2i(b_Control[index + %2], b_Control[index + %2 + 1]);").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_IVEC3) {
				function += QString("\titem.%1 = vec3i(b_Control[index + %2], b_Control[index + %2 + 1], b_Control[index + %2 + 2]);").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_IVEC4) {
				function += QString("\titem.%1 = vec4i(b_Control[index + %2], b_Control[index + %2 + 1], b_Control[index + %2 + 2], b_Control[index + %2 + 3]);").arg(name).arg(pos_index);
			}

		} else {
			if (member.p_Type == MemberSpec::T_INT) {
				function += QString("\titem.%1 = atomicLoad(&b_Control[index + %2]);").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_FLOAT) {
				function += QString("\titem.%1 = bitcast<f32>(atomicLoad(&b_Control[index + %2]));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_VEC2) {
				function += QString("\titem.%1 = vec2f(bitcast<f32>(atomicLoad(&b_Control[index + %2])), bitcast<f32>(atomicLoad(&b_Control[index + %2 + 1])));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_VEC3) {
				function += QString("\titem.%1 = vec3f(bitcast<f32>(atomicLoad(&b_Control[index + %2])), bitcast<f32>(atomicLoad(&b_Control[index + %2 + 1])), bitcast<f32>(atomicLoad(&b_Control[index + %2 + 2])));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_VEC4) {
				function += QString("\titem.%1 = vec4f(bitcast<f32>(atomicLoad(&b_Control[index + %2])), bitcast<f32>(atomicLoad(&b_Control[index + %2 + 1])), bitcast<f32>(atomicLoad(&b_Control[index + %2 + 2])), bitcast<f32>(atomicLoad(&b_Control[index + %2 + 3])));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_IVEC2) {
				function += QString("\titem.%1 = vec2i(atomicLoad(&b_Control[index + %2]), atomicLoad(&b_Control[index + %2 + 1]));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_IVEC3) {
				function += QString("\titem.%1 = vec3i(atomicLoad(&b_Control[index + %2]), atomicLoad(&b_Control[index + %2 + 1]), atomicLoad(&b_Control[index + %2 + 2]));").arg(name).arg(pos_index);
			} else if (member.p_Type == MemberSpec::T_IVEC4) {
				function += QString("\titem.%1 = vec4i(atomicLoad(&b_Control[index + %2]), atomicLoad(&b_Control[index + %2 + 1]), atomicLoad(&b_Control[index + %2 + 2]), atomicLoad(&b_Control[index + %2 + 3]));").arg(name).arg(pos_index);
			}
		}
		pos_index += member.p_Size / sizeof(int);
	}
	function += "\treturn item;\n};";

	insertion += members.join(",\n");
	insertion += "};";
	insertion += function.join("\n");

	count_var = QString("io%1Count").arg(data_vect.p_Name);
	offset_var = QString("io%1Offset").arg(data_vect.p_Name);
	return insertion.join("\n");
}

////////////////////////////////////////////////////////////////////////////////
std::unique_ptr<PipelineStage::Prepared> PipelineStage::PrepareWithUniform(const std::vector<MemberSpec>& unform_members) {

	std::unique_ptr<PipelineStage::Prepared> result = std::make_unique<Prepared>();
	result->m_RunType = m_RunType;
	result->m_Entity = m_Entity;
	result->m_Buffer = m_Buffer;
	result->m_Cache = m_Cache;
	result->m_Entities = m_Entities;

	result->m_Pipeline = new WebGPUPipeline();

	QStringList immediate_insertion;
	QStringList insertion;
	QStringList end_insertion;
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);
	bool is_render = ((m_RunType == RunType::ENTITY_RENDER) || (m_RunType == RunType::BASIC_RENDER));
	WGPUShaderStageFlags vis_flags = is_render ? WGPUShaderStage_Vertex | WGPUShaderStage_Fragment : WGPUShaderStage_Compute;

	for (auto str : m_ShaderDefines) {
		immediate_insertion += QString("#define %1").arg(str);
	}
	immediate_insertion += QString("#define ENTITY_OFFSET_INTS %1").arg(ENTITY_OFFSET_INTS);

	QStringList insertion_buffers;
	result->m_ControlSSBO = m_ControlSSBO ? m_ControlSSBO : (m_Entity ? m_Entity->GetControlSSBO() : nullptr);
	if (result->m_ControlSSBO) {
		if (is_render) {
			insertion_buffers += "@group(0) @binding(0) var<storage, read> b_Control: array<i32>;";
		} else {
			insertion_buffers += "@group(0) @binding(0) var<storage, read_write> b_Control: array<atomic<i32>>;";
		}
		result->m_Pipeline->AddBuffer(*result->m_ControlSSBO, vis_flags, is_render);
	}
	result->m_ReadRequired = false;
	for (const auto& var: m_Vars) {
		if (var.p_ReadBack) {
			result->m_ReadRequired = result->m_ControlSSBO;
			break;
		}
	}

	if(!unform_members.empty()) { // uniform
		QStringList members;
		int size = 0;
		for (const auto& member : unform_members) {
			size += member.p_Size;
			members += QString("\t%1: %2").arg(member.p_Name).arg(MemberSpec::GetGPUType(member.p_Type));
		}
		immediate_insertion += "struct UniformStruct {";
		immediate_insertion += members.join(",\n");
		immediate_insertion += "};";

		insertion_buffers += QString("@group(0) @binding(%1) var<uniform> Uniform: UniformStruct;").arg(insertion_buffers.size());
		result->m_UniformBuffer = new WebGPUBuffer(WGPUBufferUsage_Uniform, size);
		result->m_Pipeline->AddBuffer(*result->m_UniformBuffer, vis_flags, true);
	}

	if (m_Entity && is_render) {
		insertion += QString("#define Get_ioMaxIndex (b_%1[3])").arg(m_Entity->GetName());
	} else if (m_Entity) {
		insertion += QString("#define Get_ioMaxIndex (atomicLoad(&b_%1[3]))").arg(m_Entity->GetName());
	}
	if (entity_processing) {
		m_Vars.push_back({ "ioEntityDeaths" });
		insertion += QString("#define ioEntityCount b_%1[0]").arg(m_Entity->GetName());
		insertion += QString("#define ioEntityFreeCount b_%1[1]").arg(m_Entity->GetName());
	}
	
	if (result->m_ControlSSBO && (!m_DataVectors.empty())) {
		end_insertion += "//////////////// Data vector helpers";
		for (const auto& data_vect : m_DataVectors) {
			QString count_var;
			QString offset_var;
			end_insertion += GetDataVectorStructCode(data_vect, count_var, offset_var, is_render);
			m_Vars.push_back({ count_var });
			m_Vars.push_back({ offset_var });
			result->m_DataVectors.push_back({ data_vect.p_Name, count_var, offset_var, nullptr, 0 });
		}
	}

	result->m_UsingRandom = !is_render;
	if (result->m_UsingRandom) {
		m_Vars.push_back({ "ioRandSeed" });
		insertion += "#include \"Random.wgsl\"\n";
		insertion += QString("fn RandomRange(min_val: f32, max_val: f32) -> f32 { return GetRandom(min_val, max_val, u32(atomicAdd(&ioRandSeed, 1))); }");
		insertion += QString("fn Random() -> f32 { return GetRandom(0.0, 1.0, u32(atomicAdd(&ioRandSeed, 1))); }");
	}

	if(m_Entity) {
		bool input_read_only = is_render;
		result->m_Pipeline->AddBuffer(*m_Entity->GetSSBO(), vis_flags, input_read_only);
		result->m_EntityBufferIndex = insertion_buffers.size();
		insertion_buffers += QString("@group(0) @binding(%1) var<storage, %2> b_%3: array<%4>;").arg(insertion_buffers.size()).arg(input_read_only ? "read" : "read_write").arg(m_Entity->GetName()).arg(input_read_only ? "i32" : "atomic<i32>");

		if (entity_processing && m_Entity->IsDoubleBuffering()) {
			result->m_Pipeline->AddBuffer(*m_Entity->GetOuputSSBO(), vis_flags, false);
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_Output%2: array<i32>;").arg(insertion_buffers.size()).arg(m_Entity->GetName());
			insertion += m_Entity->GetDoubleBufferGPUInsertion();
		} else {
			insertion += m_Entity->GetGPUInsertion();
		}
		if (!input_read_only) {
			insertion += QString("#define %1_LOOKUP(base, index) (atomicLoad(&b_%1[(base) + (index)]))").arg(m_Entity->GetName());
		}

		if (entity_processing) {
			insertion += m_Entity->GetSpecs().p_GPUInsertion;
		} else {
			insertion += m_Entity->GetSpecs().p_GPUReadOnlyInsertion;
		}
	}

	if(entity_processing) {
		result->m_Pipeline->AddBuffer(*m_Entity->GetFreeListSSBO(), vis_flags, false);
		insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_FreeList: array<i32>;").arg(insertion_buffers.size());

		insertion += QString("fn Destroy%1(index: i32) {").arg(m_Entity->GetName());
		insertion += QString("\tlet base = index * FLOATS_PER_%1 + ENTITY_OFFSET_INTS;").arg(m_Entity->GetName());
		insertion += QString("\t%1_SET(base, 0, -1);").arg(m_Entity->GetName());
		insertion += "\tatomicAdd(&ioEntityDeaths, 1);";
		insertion += "\tatomicAdd(&ioEntityCount, -1);";
		insertion += "\tlet free_index = atomicAdd(&ioEntityFreeCount, 1);";
		insertion += "\tb_FreeList[free_index] = index;";
		insertion += "}";
	}
	
	for (const auto& ssbo: m_SSBOs) {
		int buffer_index = insertion_buffers.size();
		bool read_only = ssbo.p_Access == BufferAccess::READ_ONLY;
		auto type_str = MemberSpec::GetGPUType(ssbo.p_Type);
		if (ssbo.p_Access == BufferAccess::READ_WRITE_ATOMIC) {
			type_str = QString("atomic<%1>").arg(type_str);
		}
		insertion_buffers += QString("@group(0) @binding(%1) var<storage, %2> %3: array<%4>;")
			.arg(insertion_buffers.size())
			.arg(read_only ? "read" : "read_write")
			.arg(ssbo.p_Name)
			.arg(type_str);
		result->m_Pipeline->AddBuffer(ssbo.p_Buffer, vis_flags, read_only);
	}

	for (const auto& ssbo_spec : m_StructBuffers) {

		QStringList members;
		for (const auto& member : ssbo_spec.p_Members) {
			members += QString("\t%1: %2").arg(member.p_Name).arg(MemberSpec::GetGPUType(member.p_Type));
		}
		immediate_insertion += QString("struct %1 {").arg(ssbo_spec.p_StructName);
		immediate_insertion += members.join(",\n");
		immediate_insertion += "};";

		int buffer_index = insertion_buffers.size();
		bool read_only = ssbo_spec.p_Access == BufferAccess::READ_ONLY;
		auto type_str = ssbo_spec.p_StructName;
		if (ssbo_spec.p_Access == BufferAccess::READ_WRITE_ATOMIC) {
			type_str = QString("atomic<%1>").arg(type_str);
		}
		if (ssbo_spec.p_IsArray) {
			type_str = QString("array<%1>").arg(type_str);
		}
		if (ssbo_spec.p_Buffer.GetUsageFlags() & WGPUBufferUsage_Uniform) {
			read_only = true;
			insertion_buffers += QString("@group(0) @binding(%1) var<uniform> %2: %3;")
				.arg(insertion_buffers.size())
				.arg(ssbo_spec.p_Name)
				.arg(type_str);
		} else {
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, %2> %3: %4;")
				.arg(insertion_buffers.size())
				.arg(read_only ? "read" : "read_write")
				.arg(ssbo_spec.p_Name)
				.arg(type_str);
		}

		result->m_Pipeline->AddBuffer(ssbo_spec.p_Buffer, vis_flags, read_only);
	}

	for (auto& entity_spec : m_Entities) {
		if (!entity_spec.p_Entity) {
			continue;
		}
		GPUEntity& entity = *entity_spec.p_Entity;
		QString name = entity.GetName();
		insertion += QString("//////////////// Entity %1").arg(name);

		insertion += QString("#define io%1Count b_%1[0]").arg(entity.GetName());

		bool read_only_entity = is_render;
		if (read_only_entity) {
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, read> b_%2: array<i32>;").arg(insertion_buffers.size()).arg(entity.GetName());
			result->m_Pipeline->AddBuffer(*entity.GetSSBO(), vis_flags, true);

			insertion += entity.GetGPUInsertion();
			insertion += QString(entity.GetSpecs().p_GPUReadOnlyInsertion).arg(name);

		} else {
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_%2: array<atomic<i32>>;").arg(insertion_buffers.size()).arg(entity.GetName());
			result->m_Pipeline->AddBuffer(*entity.GetSSBO(), vis_flags, false);

			insertion += entity.GetGPUInsertion();
			// replace non-atomic getters and setters with atomic ones
			insertion += QString("#define %1_LOOKUP(base, index) (atomicLoad(&b_%1[(base) + (index)]))").arg(name);
			insertion += QString("#define %1_SET(base, index, value) atomicStore(&b_%1[(base) + (index)], (value))").arg(name);
			insertion += QString(entity.GetSpecs().p_GPUInsertion).arg(name);
		}

		if (entity_spec.p_Creatable) {

			insertion += QString("#define io%1FreeCount b_%1[1]").arg(entity.GetName());
			insertion += QString("#define io%1NextId b_%1[2]").arg(entity.GetName());
			insertion += QString("#define io%1MaxIndex b_%1[3]").arg(entity.GetName());

			QString next_id = QString("io%1NextId").arg(name);
			QString max_index = QString("io%1MaxIndex").arg(name);
			QString free_count = QString("io%1FreeCount").arg(name);

			insertion += QString("fn Create%1(input_item: %1) {").arg(name);
			insertion += "\tvar item = input_item;";
			insertion += QString("\tlet item_id: i32 = atomicAdd(&%1, 1);").arg(next_id);
			
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_Free%2: array<i32>;").arg(insertion_buffers.size()).arg(entity.GetName());
			result->m_Pipeline->AddBuffer(*entity.GetFreeListSSBO(), vis_flags, false);

			insertion += "\tvar item_pos: i32 = 0;";
			insertion += QString("\tatomicAdd(&io%1Count, 1);").arg(entity.GetName());
			insertion += QString("\tlet free_count: i32 = atomicAdd(&%1, -1);").arg(free_count);
			insertion += QString("\tatomicMax(&%1, 0);").arg(free_count); // ensure this is never negative after completion
			insertion += "\tif(free_count > 0) {";
				insertion += QString("\t\titem_pos = b_Free%1[free_count - 1];").arg(entity.GetName());
			insertion += "\t} else {";
				insertion += QString("\t\titem_pos = atomicAdd(&%1, 1);").arg(max_index);
			insertion += "\t}";

			insertion += QString("\titem.%1 = item_id;").arg(entity.GetIDName());
			insertion += QString("\tSet%1(item, item_pos);\n}").arg(name);
		}
	}
	
	for (const auto& tex : m_Textures) {
		auto dim = tex.p_Tex->GetViewDimension();
		QString tex_spec;
		if (dim == WGPUTextureViewDimension_1D) {
			tex_spec = "texture_1d<f32>";
		} else if (dim == WGPUTextureViewDimension_2D) {
			tex_spec = "texture_2d<f32>";
		} else if (dim == WGPUTextureViewDimension_2DArray) {
			tex_spec = "texture_2d_array<f32>";
		} else if (dim == WGPUTextureViewDimension_Cube) {
			tex_spec = "texture_cube<f32>";
		} else if (dim == WGPUTextureViewDimension_CubeArray) {
			tex_spec = "texture_cube_array<f32>";
		} else if (dim == WGPUTextureViewDimension_3D) {
			tex_spec = "texture_3d<f32>";
		}
		insertion_buffers += QString("@group(0) @binding(%1) var %2: %3;").arg(insertion_buffers.size()).arg(tex.p_Name).arg(tex_spec);
		result->m_Pipeline->AddTexture(*tex.p_Tex);
	}
	for (const auto& sampler : m_Samplers) {
		insertion_buffers += QString("@group(0) @binding(%1) var %2: sampler;").arg(insertion_buffers.size()).arg(sampler.p_Name);
		result->m_Pipeline->AddSampler(*sampler.p_Sampler);
	}

	if (result->m_ControlSSBO) { // avoid initial validation error for zero-sized buffers
		result->m_ControlSSBO->EnsureSizeBytes(std::max(int(m_Vars.size()), 1) * sizeof(int));
	}
	for (int i = 0; i < m_Vars.size(); i++) {
		const auto& var = m_Vars[i];
		insertion.push_front(QString("#define Get_%1 (atomicLoad(&b_Control[%2]))").arg(var.p_Name).arg(i));
		insertion.push_front(QString("#define %1 (b_Control[%2])").arg(var.p_Name).arg(i));
		result->m_VarNames += var.p_Name;
	}

	insertion += "////////////////";

	if (entity_processing && m_ReplaceMain) {
		end_insertion += QString("@compute @workgroup_size(%1, %2, %3)").arg(m_LocalSize.x).arg(m_LocalSize.y).arg(m_LocalSize.z);
		end_insertion +=
			"fn main(@builtin(global_invocation_id) global_id: vec3u) {\n"
			"\tlet item_index = i32(global_id.x);\n"
			"\tif (item_index >= Get_ioMaxIndex) { return; }\n"
			"\tlet item: %1 = Get%1(item_index);";
		if (m_Entity->IsDoubleBuffering()) {
			end_insertion += QString("\tif (item.%1 < 0) { Set%2(item, item_index); return; }").arg(m_Entity->GetIDName()).arg("%1");
		} else {
			end_insertion += QString("\tif (item.%1 < 0) { return; }").arg(m_Entity->GetIDName());
		}
		end_insertion +=
			"\tvar new_item: %1 = item;\n"
			"\tlet should_destroy: bool = %1Main(item_index, item, &new_item);\n"
			"\tif(should_destroy) { Destroy%1(item_index); } else { Set%1(new_item, item_index); }\n"
			"}";
	} else if (m_Entity && m_ReplaceMain && (m_RunType == RunType::ENTITY_RENDER)) {

		/*
		end_insertion +=
			"@vertex\n"
			"void %1Main(int item_index, %1 item); // forward declaration\n"
			"void main() {\n"
			"\t%1 item = Get%1(gl_InstanceID);";
		end_insertion += QString("\tif (item.%1 < 0) { gl_Position = vec4(0.0, 0.0, 100.0, 0.0); return; }").arg(m_Entity->GetIDName());
		end_insertion +=
			"\t%1Main(gl_InstanceID, item);\n"
			"}\n#endif\n////////////////";
			*/
	} else if (m_Entity && m_ReplaceMain) {
		end_insertion += QString("@compute @workgroup_size(%1, %2, %3)").arg(m_LocalSize.x).arg(m_LocalSize.y).arg(m_LocalSize.z);
		end_insertion +=
			"fn main(@builtin(global_invocation_id) global_id: vec3u) {\n"
			"\tlet item_index = i32(global_id.x);\n"
			"\tif (item_index >= Get_ioMaxIndex) { return; }\n"
			"\tlet item: %1 = Get%1(item_index);";
		end_insertion += QString("\tif (item.%1 < 0) { return; }").arg(m_Entity->GetIDName());
		end_insertion +=
			"\t%1Main(item_index, item);\n"
			"}\n////////////////";
	}

	// WARNING: do NOT add to control buffer beyond this point

	if (!m_ExtraCode.isNull()) {
		insertion += "////////////////";
		insertion += m_ExtraCode;
	}

	insertion.push_front(insertion_buffers.join("\n"));
	insertion.push_front(immediate_insertion.join("\n"));

	//DebugGPU::Checkpoint("PreRun", m_Entity);

	QByteArray insertion_str = (m_Entity ? QString(insertion.join("\n")).arg(m_Entity->GetName()) : insertion.join("\n")).toLocal8Bit();
	QByteArray end_insertion_str;
	if (!end_insertion.empty()) {
		end_insertion_str = "\n//////////\n" + (m_Entity ? QString(end_insertion.join("\n")).arg(m_Entity->GetName()) : end_insertion.join("\n")).toLocal8Bit();
	}

	if (is_render) {
		result->m_Pipeline->FinalizeRender(m_ShaderName, *m_Buffer, m_RenderParams, insertion_str, end_insertion_str);
	} else {
		result->m_Pipeline->FinalizeCompute(m_ShaderName, insertion_str, end_insertion_str);
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage::Prepared::~Prepared(void) {
	delete m_Pipeline;
	delete m_UniformBuffer;
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage::AsyncOutputResults PipelineStage::Prepared::RunInternal(unsigned char* uniform, int uniform_bytes, std::vector<std::pair<QString, int>>&& variables, int iterations, RTT* rtt, std::optional<std::function<void(const OutputResults& results)>>&& callback) {

	bool compute = m_Pipeline->GetType() == WebGPUPipeline::Type::COMPUTE;
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);

	if (!compute && !rtt) {
		throw std::invalid_argument("Render requires an RTT");
	}

	if (m_Entity) {
		iterations = m_Entity->GetMaxCount();
	}
	if (entity_processing) {
		variables.push_back({ "ioEntityDeaths", 0 });
	}

	int rand_seed_val = 0;
	if (m_UsingRandom) {
		rand_seed_val = rand();
		variables.push_back({ "ioRandSeed", rand_seed_val });
	}

	bool previously_using_temp_frame = m_TemporaryFrame.get();
	m_TemporaryFrame = nullptr;
	if (!compute && m_Entity) {
		int time_slider = Core::GetInterfaceData().p_BufferView.p_TimeSlider;
		if (time_slider > 0) {
			m_TemporaryFrame = BufferViewer::GetStoredFrameAt(m_Entity->GetName(), Core::GetTicks() - time_slider, iterations);
		}
	}

	for(auto& pair: m_Entities) {
		if (pair.p_Entity && pair.p_Entity->IsDoubleBuffering()) {
			m_Pipeline->ReplaceBuffer(*pair.p_Entity->GetOuputSSBO(), *pair.p_Entity->GetSSBO());
		}
	}
	
	auto fill_var_data = [this, &variables] (QString name, int& value) {
		for (const auto& var: variables) {
			if (var.first == name) {
				value = var.second;
				return;
			}
		}
		int offset = m_VarNames.size();
		for (const auto& vect : m_DataVectors) {
			if (vect.m_CountVar == name) {
				value = vect.m_SizeInts;
				return;
			}
			if (vect.m_OffsetVar == name) {
				value = offset;
				return;
			}
			offset += vect.m_SizeInts;
		}
	};

	int death_count_index = -1;
	std::vector<int> var_vals(m_VarNames.size(), 0);
	for (int i = 0; i < m_VarNames.size(); i++) {
		if (m_VarNames[i] == "ioEntityDeaths") {
			death_count_index = i;
		}
		fill_var_data(m_VarNames[i], var_vals[i]);
	}

	if (m_ControlSSBO && !var_vals.empty()) {

		std::vector<int> values = var_vals;
		int control_size = (int)var_vals.size();

		// allocate some space for vectors on the control buffer and copy data over
		for (const auto& data_vect : m_DataVectors) {
			int data_size = data_vect.m_SizeInts;
			int offset = control_size;
			control_size += data_size;
			values.resize(control_size);
			memcpy((unsigned char*)&(values[offset]), data_vect.m_Data, sizeof(int) * data_size);
		}

		m_ControlSSBO->EnsureSizeBytes(control_size * sizeof(int), false);
		m_ControlSSBO->SetValues(values);
	}

	if (m_UniformBuffer && uniform && uniform_bytes) {
		m_UniformBuffer->EnsureSizeBytes(uniform_bytes, false);
		m_UniformBuffer->Write(uniform, 0, uniform_bytes);
	}

	if (compute) {
		m_Pipeline->Compute(iterations);
	} else {
		if (!m_Buffer) {
			throw std::invalid_argument("Render buffer not found");
		}
		if (m_TemporaryFrame.get()) {
			m_Pipeline->ReplaceBuffer(m_EntityBufferIndex, *m_TemporaryFrame.get());
		} else if (m_Entity && (m_Entity->IsDoubleBuffering() || previously_using_temp_frame)) {
			m_Pipeline->ReplaceBuffer(m_EntityBufferIndex, *m_Entity->GetSSBO());
		}
		rtt->Render(m_Pipeline, iterations);
	}
	if (entity_processing) {
		m_Entity->QueueInfoRead();
	}
	for (auto& pair: m_Entities) {
		if (pair.p_Entity && pair.p_Creatable) {
			pair.p_Entity->QueueInfoRead();
		}
	}

	////////////////////////////////////////////////////

	if (entity_processing && m_Entity->IsDoubleBuffering()) {
		WebGPUBuffer::CopyBufferToBuffer(m_Entity->GetSSBO()->Get(), m_Entity->GetOuputSSBO()->Get(), 0, 0, sizeof(EntityInfo));
		m_Pipeline->ReplaceBuffer(*m_Entity->GetOuputSSBO(), *m_Entity->GetSSBO());
		m_Pipeline->ReplaceBuffer(*m_Entity->GetSSBO(), *m_Entity->GetOuputSSBO());
		m_Entity->SwapInputOutputSSBOs();
	}

#ifdef NESHNY_ENTITY_DEBUG
	if (entity_processing && (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT)) {
		BufferViewer::Checkpoint(QString("%1 Death").arg(m_Entity->GetName()), "PostRun", *m_Entity->GetFreeListSSBO(), MemberSpec::Type::T_INT);
	}
	if (entity_processing) {
		BufferViewer::Checkpoint(QString("%1 Control").arg(m_Entity->GetName()), "PostRun", *m_ControlSSBO, MemberSpec::Type::T_INT);
		BufferViewer::Checkpoint(QString("%1 Free").arg(m_Entity->GetName()), "PostRun", *m_Entity->GetFreeListSSBO(), MemberSpec::Type::T_INT, m_Entity->GetFreeCount());
	}
#endif

	if (m_ControlSSBO && m_ReadRequired) {
		m_ControlSSBO->Read<OutputResults>(0, (int)(var_vals.size() * sizeof(int)), [var_names = m_VarNames, result_callback = std::move(callback)](unsigned char* data, int size, AsyncOutputResults token) -> std::shared_ptr<OutputResults> {
			OutputResults* results = new OutputResults();
			int* int_data = (int*)data;
			for (int i = 0; i < var_names.size(); i++) {
				results->p_Results.push_back({ var_names[i], int_data[i] });
			}
			if (result_callback.has_value()) {
				result_callback.value()(*results);
			}
			return std::shared_ptr<OutputResults>(results);
		});
	}

	return AsyncOutputResults::Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
QueryEntities::QueryEntities(GPUEntity& entity) :
	m_Entity(entity)
{
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ByNearestPosition(QString param_name, fVec2 pos) {
	m_Query = QueryType::Position2D;
	m_QueryParam = pos;
	m_ParamName = param_name;
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ByNearestPosition(QString param_name, fVec3 pos) {
	m_Query = QueryType::Position3D;
	m_QueryParam = pos;
	m_ParamName = param_name;
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ById(int id) {
	m_Query = QueryType::ID;
	m_QueryParam = id;
	m_ParamName = m_Entity.GetIDName();
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
int QueryEntities::ExecuteQuery(void) {

#pragma msg("finish webGPU version")
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Grid2DCache::Grid2DCache(GPUEntity& entity, QString pos_name) :
	m_Entity		( entity )
	,m_PosName		( pos_name )
#if defined(NESHNY_WEBGPU)
	,m_GridIndices	( WGPUBufferUsage_Storage )
	,m_GridItems	( WGPUBufferUsage_Storage )
	,m_Uniform		( WGPUBufferUsage_Uniform, sizeof(Grid2DCacheUniform) )
#endif
{
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::GenerateCache(iVec2 grid_size, Vec2 grid_min, Vec2 grid_max) {

	m_GridSize = grid_size;
	m_GridMin = grid_min;
	m_GridMax = grid_max;

	Vec2 range = grid_max - grid_min;
	Vec2 cell_size(range.x / grid_size.x, range.y / grid_size.y);
	m_GridIndices.EnsureSizeBytes(grid_size.x * grid_size.y * 3 * sizeof(int), true);
	m_GridItems.EnsureSizeBytes(m_Entity.GetMaxCount() * sizeof(int), false);

	if (!m_CacheIndex) {
		QString main_func =
			QString("fn %1Main(item_index: i32, item: %1) { ItemMain(item_index, item.%2); }")
			.arg(m_Entity.GetName()).arg(m_PosName);

		m_CacheIndex = Neshny::PipelineStage::IterateEntity(m_Entity, "GridCache2D", true, { "PHASE_INDEX" })
			.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, PipelineStage::BufferAccess::READ_WRITE_ATOMIC)
			.AddCode(main_func)
			.Prepare<Grid2DCacheUniform>();

		m_CacheAlloc = Neshny::PipelineStage::IterateEntity(m_Entity, "GridCache2D", true, { "PHASE_ALLOCATE" })
			.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, PipelineStage::BufferAccess::READ_WRITE_ATOMIC)
			.AddCode(main_func)
			.AddInputVar("AllocationCount")
			.Prepare<Grid2DCacheUniform>();

		m_CacheFill = Neshny::PipelineStage::IterateEntity(m_Entity, "GridCache2D", true, { "PHASE_FILL" })
			.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, PipelineStage::BufferAccess::READ_WRITE_ATOMIC)
			.AddBuffer("b_Cache", m_GridItems, MemberSpec::T_INT, PipelineStage::BufferAccess::READ_WRITE)
			.AddCode(main_func)
			.Prepare<Grid2DCacheUniform>();
	}

	Grid2DCacheUniform uniform{
		grid_size
		,grid_min
		,grid_max
	};

	m_CacheIndex->Run(uniform);
	m_CacheAlloc->Run(uniform, { { "AllocationCount", 0 } }, std::nullopt);
	m_CacheAlloc->Run(uniform);
	m_CacheFill->Run(uniform);

	// gets used by the entity run that uses the buffer
	m_Uniform.SetSingleValue(0, uniform);
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::Bind(PipelineStage& target_stage) {

	auto name = m_Entity.GetName();

	target_stage.AddBuffer(QString("b_%1GridIndices").arg(name), m_GridIndices, MemberSpec::Type::T_INT, PipelineStage::BufferAccess::READ_ONLY);
	target_stage.AddBuffer(QString("b_%1GridItems").arg(name), m_GridItems, MemberSpec::Type::T_INT, PipelineStage::BufferAccess::READ_ONLY);

	target_stage.AddStructBuffer<Grid2DCacheUniform>(QString("b_%1GridUniform").arg(name), QString("%1GridUniformStruct").arg(name), m_Uniform, PipelineStage::BufferAccess::READ_ONLY, false);

	QString err_msg;
	auto utils_file = Core::Singleton().LoadEmbedded("GridCache2D.wgsl.template", err_msg);
	if (utils_file.isNull()) {
		qWarning() << "could not open 'GridCache2D.wgsl.template'" << err_msg;
		target_stage.AddCode("#error could not open 'GridCache2D.wgsl.template'");
		return;
	}
	target_stage.AddCode(
		QString(utils_file)
		.arg(name)
		.arg(m_Entity.GetIDName())
	);
}

} // namespace Neshny
#endif