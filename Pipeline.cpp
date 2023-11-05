////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
PipelineStage::PipelineStage(RunType type, GPUEntity* entity, RenderableBuffer* buffer, BaseCache* cache, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, SSBO* control_ssbo, int iterations) :
	m_RunType			( type )
	,m_Entity			( entity )
	,m_Buffer			( buffer )
	,m_ShaderName		( shader_name )
	,m_ReplaceMain		( replace_main )
	,m_ShaderDefines	( shader_defines )
	,m_Iterations		( iterations )
	,m_ControlSSBO		( control_ssbo )
{
	if (cache) {
		m_ExtraCode += cache->Bind(m_SSBOs);
	}
#if defined(NESHNY_WEBGPU)
	const auto& limits = Core::Singleton().GetLimits();
	m_LocalSize = iVec3(limits.maxComputeInvocationsPerWorkgroup, 1, 1);
#endif
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ entity, false });
	if (cache) {
		m_ExtraCode += cache->Bind(m_SSBOs);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddCreatableEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ entity, true });
	if (cache) {
		m_ExtraCode += cache->Bind(m_SSBOs);
	}
	return *this;
}

#if defined(NESHNY_GL)
////////////////////////////////////////////////////////////////////////////////
QString PipelineStage::GetDataVectorStructCode(const AddedDataVector& data_vect, QStringList& insertion_uniforms, std::vector<std::pair<QString, int>>& integer_vars, int offset) {

	QStringList insertion;
	insertion += QString("struct %1 {").arg(data_vect.p_Name);

	QStringList function(QString("%1 Get%1(int index) {\n\t%1 item;").arg(data_vect.p_Name));
	function += QString("\tindex = u%1Offset + (index * %2);").arg(data_vect.p_Name).arg(data_vect.p_NumIntsPerItem);

	int pos_index = 0;
	for (const auto& member : data_vect.p_Members) {
		insertion += QString("\t%1 %2;").arg(MemberSpec::GetGPUType(member.p_Type)).arg(member.p_Name);

		QString name = member.p_Name;
		if (member.p_Type == MemberSpec::T_INT) {
			function += QString("\titem.%1 = b_Control.i[index + %2];").arg(name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_FLOAT) {
			function += QString("\titem.%1 = intBitsToFloat(b_Control.i[index + %2]);").arg(name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_VEC2) {
			function += QString("\titem.%1 = vec2(intBitsToFloat(b_Control.i[index + %2]), intBitsToFloat(b_Control.i[index + %2 + 1]));").arg(name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_VEC3) {
			function += QString("\titem.%1 = vec3(intBitsToFloat(b_Control.i[index + %2]), intBitsToFloat(b_Control.i[index + %2 + 1]), intBitsToFloat(b_Control.i[index + %2 + 2]));").arg(name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_VEC4) {
			function += QString("\titem.%1 = vec4(intBitsToFloat(b_Control.i[index + %2]), intBitsToFloat(b_Control.i[index + %2 + 1]), intBitsToFloat(b_Control.i[index + %2 + 2]), intBitsToFloat(b_Control.i[index + %2 + 3]));").arg(name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_IVEC2) {
			function += QString("\titem.%1 = ivec2(b_Control.i[index + %2], b_Control.i[index + %2 + 1]);").arg(name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_IVEC3) {
			function += QString("\titem.%1 = ivec3(b_Control.i[index + %2], b_Control.i[index + %2 + 1], b_Control.i[index + %2 + 2]);").arg(name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_IVEC4) {
			function += QString("\titem.%1 = ivec4(b_Control.i[index + %2], b_Control.i[index + %2 + 1], b_Control.i[index + %2 + 2], b_Control.i[index + %2 + 3]);").arg(name).arg(pos_index);
		}
		pos_index += member.p_Size / sizeof(float);
	}
	function += "\treturn item;\n};";

	insertion += "};";
	insertion += function.join("\n");

	insertion_uniforms += QString("uniform int u%1Count;").arg(data_vect.p_Name);
	integer_vars.emplace_back(QString("u%1Count").arg(data_vect.p_Name), data_vect.p_NumItems);

	insertion_uniforms += QString("uniform int u%1Offset;").arg(data_vect.p_Name);
	integer_vars.emplace_back(QString("u%1Offset").arg(data_vect.p_Name), offset);
	return insertion.join("\n");
}

////////////////////////////////////////////////////////////////////////////////
void PipelineStage::Run(std::optional<std::function<void(Shader* program)>> pre_execute) {

	// TODO: investigate GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS
	// TODO: debug of SSBOs is optional

	QStringList insertion;
	int entity_deaths = 0;
	int entity_free_count = m_Entity ? m_Entity->GetFreeCount() : 0; // only used for DeleteMode::STABLE_WITH_GAPS
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);
	bool is_render = ((m_RunType == RunType::ENTITY_RENDER) || (m_RunType == RunType::BASIC_RENDER));

	QStringList insertion_images;
	QStringList insertion_buffers;
	QStringList insertion_uniforms;
	std::vector<std::pair<QString, int>> integer_vars;
	std::vector<std::pair<GLSSBO*, int>> ssbo_binds;
	std::vector<std::pair<int, int*>> var_vals;

	if (!is_render) {
		insertion += QString("layout(local_size_x = %1, local_size_y = %2, local_size_z = %3) in;").arg(m_LocalSize.x).arg(m_LocalSize.y).arg(m_LocalSize.z);
	}

	std::shared_ptr<GLSSBO> replace = nullptr;
	int num_entities = m_Entity ? m_Entity->GetMaxIndex() : 0;
	if (is_render && m_Entity) {
		insertion_uniforms += "uniform int uCount;";
		integer_vars.push_back({ QString("uCount").arg(m_Entity->GetName()), num_entities });

		int time_slider = Core::GetInterfaceData().p_BufferView.p_TimeSlider;
		if (time_slider > 0) {
			replace = BufferViewer::GetStoredFrameAt(m_Entity->GetName(), Core::GetTicks() - time_slider, num_entities);
		}
	} else if(m_Entity || (m_RunType == RunType::BASIC_COMPUTE)) {
		insertion_uniforms += "uniform int uCount;";
		insertion_uniforms += "uniform int uOffset;";
	}
	for (auto str : m_ShaderDefines) {
		insertion += QString("#define %1").arg(str);
	}
	insertion_buffers += "layout(std430, binding = 0) buffer ControlBuffer { int i[]; } b_Control;";
	if(entity_processing) {
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			m_Vars.push_back({ "ioEntityDeaths", &entity_deaths });
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Vars.push_back({ "ioEntityDeaths", &entity_deaths });
			m_Vars.push_back({ "ioEntityFreeCount", &entity_free_count });
		}
	}
	for (const auto& var : m_Vars) {
		insertion += QString("#define %1 (b_Control.i[%2])").arg(var.p_Name).arg((int)var_vals.size());
		var_vals.push_back({ var.p_Ptr ? *var.p_Ptr : 0, var.p_Ptr });
	}
	const bool rand_gen = true; // add by default, why not
	if (rand_gen) {

		int control_ind = (int)var_vals.size();
		int seed = rand();
		var_vals.push_back({ seed, nullptr });

		insertion += "#include \"Random.glsl\"\n";
		insertion += QString("float Random(float min_val = 0.0, float max_val = 1.0) { return GetRandom(min_val, max_val, atomicAdd(b_Control.i[%1], 1)); }").arg(control_ind);
	}

	if(m_Entity) {
		int buffer_index = insertion_buffers.size();
		ssbo_binds.push_back({ replace.get() ? replace.get() : m_Entity->GetSSBO(), buffer_index });
		if (entity_processing && m_Entity->IsDoubleBuffering()) {
			insertion_buffers += QString("layout(std430, binding = %1) readonly buffer MainEntityBuffer { int i[]; } b_%2;").arg(buffer_index).arg(m_Entity->GetName());
			buffer_index++;
			ssbo_binds.push_back({ m_Entity->GetOuputSSBO(), buffer_index });
			insertion_buffers += QString("layout(std430, binding = %1) writeonly buffer MainEntityOutputBuffer { int i[]; } b_Output%2;").arg(buffer_index).arg(m_Entity->GetName());
			insertion += m_Entity->GetDoubleBufferGPUInsertion();
		} else {
			insertion_buffers += QString("layout(std430, binding = %1) buffer MainEntityBuffer { int i[]; } b_%2;").arg(buffer_index).arg(m_Entity->GetName());
			insertion += m_Entity->GetGPUInsertion();
		}
		if (entity_processing) {
			insertion += m_Entity->GetSpecs().p_GPUInsertion;
		} else {
			insertion += m_Entity->GetSpecs().p_GPUReadOnlyInsertion;
		}
	}

	if(entity_processing) {
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			int buffer_index = insertion_buffers.size();
			m_Entity->GetFreeListSSBO()->EnsureSizeBytes((num_entities + m_Entity->GetFreeCount()) * sizeof(int));
			m_Entity->GetFreeListSSBO()->Bind(buffer_index);
			insertion_buffers += QString("layout(std430, binding = %1) writeonly buffer DeathBuffer { int i[]; } b_Death;").arg(buffer_index);
		} else if(m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			int buffer_index = insertion_buffers.size();
			m_Entity->GetFreeListSSBO()->EnsureSizeBytes((num_entities + m_Entity->GetFreeCount()) * sizeof(int), false);
			m_Entity->GetFreeListSSBO()->Bind(buffer_index);
			insertion_buffers += QString("layout(std430, binding = %1) writeonly buffer FreeBuffer { int i[]; } b_FreeList;").arg(buffer_index);
		}
		insertion += QString("void Destroy%1(int index) {").arg(m_Entity->GetName());
		insertion += QString("\tint base = index * FLOATS_PER_%1;").arg(m_Entity->GetName());
		insertion += QString("\t%1_SET(base, 0, -1);").arg(m_Entity->GetName());

		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			insertion += "\tint death_index = atomicAdd(ioEntityDeaths, 1);";
			insertion += QString("\tb_Death.i[death_index] = index;");
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			insertion += "\tatomicAdd(ioEntityDeaths, 1);";
			insertion += "\tint free_index = atomicAdd(ioEntityFreeCount, 1);";
			insertion += "\tb_FreeList.i[free_index] = index;";
		}
		insertion += "}";
	}

	for (int b = 0; b < m_SSBOs.size(); b++) {
		int buffer_index = insertion_buffers.size();
		auto& ssbo = m_SSBOs[b];
		ssbo_binds.push_back({ &ssbo.p_Buffer, buffer_index });
		insertion_buffers += QString("layout(std430, binding = %1) %4buffer GenericBuffer%1 { %2 i[]; } %3;").arg(buffer_index).arg(MemberSpec::GetGPUType(ssbo.p_Type)).arg(ssbo.p_Name).arg(ssbo.p_ReadOnly ? "readonly " : "");
	}
	for (const auto& tex : m_Textures) {
		insertion_uniforms += QString("uniform sampler2D %1;").arg(tex.p_Name);
	}
	
	struct EntityControl {
		int		p_MaxIndex;
		int		p_NextId;
		int		p_FreeCount;
	};
	std::vector<EntityControl> entity_controls;
	entity_controls.reserve(m_Entities.size()); // reserve to avoid reallocations, so you can have pointers to it
	for (int e = 0; e < m_Entities.size(); e++) {
		GPUEntity& entity = m_Entities[e].p_Entity;
		QString name = entity.GetName();
		insertion += QString("//////////////// Entity %1").arg(name);
		integer_vars.push_back({ QString("u%1Count").arg(name), entity.GetMaxIndex() });
		insertion_uniforms += QString("uniform int u%1Count;").arg(name);
		{
			int buffer_index = insertion_buffers.size();
			ssbo_binds.push_back({ entity.GetSSBO(), buffer_index });
			insertion_buffers += QString("layout(std430, binding = %1) buffer EntityBuffer%1 { int i[]; } b_%2;").arg(buffer_index).arg(entity.GetName());
		}

		insertion += entity.GetGPUInsertion();
		insertion += QString(entity.GetSpecs().p_GPUInsertion).arg(name);

		entity_controls.push_back({ entity.GetMaxIndex(), entity.GetNextId(), entity.GetFreeCount() });
		if (m_Entities[e].p_Creatable) {
			int control_index = (int)var_vals.size();
			auto& curr = entity_controls.back();
			var_vals.push_back({ curr.p_NextId, &curr.p_NextId });
			var_vals.push_back({ curr.p_MaxIndex, &curr.p_MaxIndex });

			insertion += QString("void Create%1(%1 item) {").arg(name);
			insertion += QString("\tint item_id = atomicAdd(b_Control.i[%1], 1);").arg(control_index);
			if (entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
				int buffer_index = insertion_buffers.size();
				insertion_buffers += QString("layout(std430, binding = %1) buffer FreeEntityBuffer%1 { int i[]; } b_Free%2;").arg(buffer_index).arg(entity.GetName());
				ssbo_binds.push_back({ entity.GetFreeListSSBO(), buffer_index });
				var_vals.push_back({ curr.p_FreeCount, &curr.p_FreeCount });
				insertion += "\tint item_pos = 0;";
				insertion += QString("\tint free_count = atomicAdd(b_Control.i[%1], -1);").arg(control_index + 2);
				insertion += "\tif(free_count > 0) {";
					insertion += QString("\t\titem_pos = b_Free%1.i[free_count - 1];").arg(entity.GetName());
				insertion += "\t} else {";
					insertion += QString("\t\titem_pos = atomicAdd(b_Control.i[%1], 1);").arg(control_index + 1);
				insertion += "\t}";
			} else {
				insertion += QString("\tint item_pos = atomicAdd(b_Control.i[%1], 1);").arg(control_index + 1);
			}
			insertion += QString("\titem.%1 = item_id;").arg(entity.GetIDName());
			insertion += QString("\tSet%1(item, item_pos);\n}").arg(name);
		}
	}

	insertion += "////////////////";

	if (entity_processing && m_ReplaceMain) {
		insertion +=
			"bool %1Main(int item_index, %1 item, inout %1 new_item); // forward declaration\n"
			"void main() {\n"
			"\tuvec3 global_id = gl_GlobalInvocationID;\n"
			"\tint item_index = int(global_id.x) + (int(global_id.y) + int(global_id.z) * 32) * 32 + uOffset;\n"
			"\tif (item_index >= uCount) return;\n"
			"\t%1 item = Get%1(item_index);";
		if (m_Entity->IsDoubleBuffering()) {
			insertion += QString("\tif (item.%1 < 0) { Set%2(item, item_index); return; }").arg(m_Entity->GetIDName()).arg("%1");
		} else {
			insertion += QString("\tif (item.%1 < 0) return;").arg(m_Entity->GetIDName());
		}
		insertion +=
			"\t%1 new_item = item;\n"
			"\tbool should_destroy = %1Main(item_index, item, new_item);\n"
			"\tif(should_destroy) { Destroy%1(item_index); } else { Set%1(new_item, item_index); }\n"
			"}\n////////////////";
	} else if (m_Entity && m_ReplaceMain && (m_RunType == RunType::ENTITY_RENDER)) {
		insertion +=
			"#ifdef IS_VERTEX_SHADER\n"
			"void %1Main(int item_index, %1 item); // forward declaration\n"
			"void main() {\n"
			"\t%1 item = Get%1(gl_InstanceID);";
		insertion += QString("\tif (item.%1 < 0) { gl_Position = vec4(0.0, 0.0, 100.0, 0.0); return; }").arg(m_Entity->GetIDName());
		insertion +=
			"\t%1Main(gl_InstanceID, item);\n"
			"}\n#endif\n////////////////";
	} else if (m_Entity && m_ReplaceMain) {
		insertion +=
			"void %1Main(int item_index, %1 item); // forward declaration\n"
			"void main() {\n"
			"\tuvec3 global_id = gl_GlobalInvocationID;\n"
			"\tint item_index = int(global_id.x) + (int(global_id.y) + int(global_id.z) * 32) * 32 + uOffset;\n"
			"\tif (item_index >= uCount) return;\n"
			"\t%1 item = Get%1(item_index);";
		insertion += QString("\tif (item.%1 < 0) return;").arg(m_Entity->GetIDName());
		insertion +=
			"\t%1Main(item_index, item);\n"
			"}\n////////////////";
	}

	// WARNING: do NOT add to control buffer beyond this point

	GLSSBO* control_ssbo = m_ControlSSBO ? m_ControlSSBO : (m_Entity ? m_Entity->GetControlSSBO() : nullptr);
	if (control_ssbo && !var_vals.empty()) {

		std::vector<int> values;
		for (auto var : var_vals) {
			values.push_back(var.first);
		}
		int control_size = (int)var_vals.size();

		// allocate some space for vectors on the control buffer and copy data over
		if (!m_DataVectors.empty()) {
			insertion += "//////////////// Data vector helpers";
		}
		for (const auto& data_vect : m_DataVectors) {
			int data_size = data_vect.p_NumIntsPerItem * data_vect.p_NumItems;
			int offset = control_size;
			control_size += data_size;
			values.resize(control_size);
			memcpy((unsigned char*)&(values[offset]), (unsigned char*)&(data_vect.p_Data[0]), sizeof(int) * data_size);
			insertion += GetDataVectorStructCode(data_vect, insertion_uniforms, integer_vars, offset);
		}

		control_ssbo->EnsureSizeBytes(control_size * sizeof(int));
		control_ssbo->Bind(0); // you can assume this is at index zero
		control_ssbo->SetValues(values);
	}

	if (!m_ExtraCode.isNull()) {
		insertion += "////////////////";
		insertion += m_ExtraCode;
	}

	insertion.push_front(insertion_images.join("\n"));
	insertion.push_front(insertion_buffers.join("\n"));
	insertion.push_front(insertion_uniforms.join("\n"));

	//DebugGPU::Checkpoint("PreRun", m_Entity);

	QString insertion_str = m_Entity ? QString(insertion.join("\n")).arg(m_Entity->GetName()) : insertion.join("\n");
	GLShader* prog = is_render ? Core::GetShader(m_ShaderName, insertion_str) : Core::GetComputeShader(m_ShaderName, insertion_str);
	prog->UseProgram();

	//DebugGPU::Checkpoint("PostRun", m_Entity);

	//if (m_ControlBuffer) {
	//	DebugGPU::Checkpoint(QString("%1 Control").arg(m_Entity.GetName()), "PreRun", *m_ControlBuffer);
	//}

	for (const auto& var: integer_vars) {
		glUniform1i(prog->GetUniform(var.first), var.second);
	}
	for (auto& ssbo : ssbo_binds) {
		ssbo.first->Bind(ssbo.second);
	}
	for (int b = 0; b < m_Textures.size(); b++) {
		auto& tex = m_Textures[b];
		glUniform1i(prog->GetUniform(tex.p_Name), b);
		glActiveTexture(GL_TEXTURE0 + b);
		glBindTexture(GL_TEXTURE_2D, tex.p_Tex);
	}
	glActiveTexture(GL_TEXTURE0);

	if (pre_execute.has_value()) {
		pre_execute.value()(prog);
	}

	////////////////////////////////////////////////////
	if (m_RunType == RunType::BASIC_COMPUTE) {
		Core::DispatchMultiple(prog, m_Iterations, m_LocalSize.x * m_LocalSize.y * m_LocalSize.z);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	} else if (entity_processing || (m_RunType == RunType::ENTITY_ITERATE)) {
		Core::DispatchMultiple(prog, num_entities, m_LocalSize.x * m_LocalSize.y * m_LocalSize.z);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	} else if(is_render) {
		if (!m_Buffer) {
			throw std::invalid_argument("Render buffer not found");
		}
		m_Buffer->UseBuffer(prog);
		if (m_Entity) {
			m_Buffer->DrawInstanced(num_entities);
		} else {
			m_Buffer->Draw();
		}
	}
	if (entity_processing && m_Entity->IsDoubleBuffering()) {
		m_Entity->SwapInputOutputSSBOs();
	}
	////////////////////////////////////////////////////

	if (entity_processing && (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT)) {
		//DebugGPU::Checkpoint(QString("%1 Death").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT);
	}

	if (control_ssbo) {
		std::vector<int> control_values;
		control_ssbo->GetValues<int>(control_values, (int)var_vals.size());
		for (int v = 0; v < var_vals.size(); v++) {
			if (var_vals[v].second) {
				*var_vals[v].second = control_values[v];
			}
		}
	}

	for (int e = 0; e < m_Entities.size(); e++) {
		if (!m_Entities[e].p_Creatable) {
			continue;
		}
		GPUEntity& entity = m_Entities[e].p_Entity;
		if (entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			entity.ProcessStableCreates(entity_controls[e].p_MaxIndex, entity_controls[e].p_NextId, entity_controls[e].p_FreeCount);
		} else {
			entity.ProcessMoveCreates(entity_controls[e].p_MaxIndex, entity_controls[e].p_NextId);
		}
	}

	if (entity_processing) {
		if ((m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) && (entity_deaths > 0)) {
			m_Entity->ProcessMoveDeaths(entity_deaths);
			if (true) {
				BufferViewer::Checkpoint(QString("%1 Death Control").arg(m_Entity->GetName()), "PostRun", *control_ssbo, MemberSpec::Type::T_INT);
			}
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Entity->ProcessStableDeaths(entity_deaths);
		}

		//BufferViewer::Checkpoint(QString("%1 Control").arg(m_Entity.GetName()), "PostRun", *control_ssbo, MemberSpec::Type::T_INT);

		//BufferViewer::Checkpoint("PostRun", m_Entity);
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			//BufferViewer::Checkpoint(QString("%1 Free").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT, m_Entity.GetFreeCount());
		}
	}
}
#elif defined(NESHNY_WEBGPU)
////////////////////////////////////////////////////////////////////////////////
QString PipelineStage::GetDataVectorStructCode(const AddedDataVector& data_vect, QString& count_var, QString& offset_var) {

	QStringList insertion;
	insertion += QString("struct %1 {").arg(data_vect.p_Name);

	QStringList function(QString("fn Get%1(base_index: i32) -> %1 {\n\tvar item: %1;").arg(data_vect.p_Name));
	QStringList members;
	function += QString("\tlet index = Get_io%1Offset + (base_index * %2);").arg(data_vect.p_Name).arg(data_vect.p_NumIntsPerItem);

	int pos_index = 0;
	for (const auto& member : data_vect.p_Members) {
		members += QString("\t%1: %2").arg(member.p_Name).arg(MemberSpec::GetGPUType(member.p_Type));

		QString name = member.p_Name;
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
	result->m_Entities = {};
	for (auto& entity : m_Entities) {
		if (entity.p_Creatable) {
			result->m_Entities.push_back(&entity.p_Entity);
		}
	}

	result->m_Pipeline = new WebGPUPipeline();

	QStringList immediate_insertion;
	QStringList insertion;
	QStringList end_insertion;
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);
	bool is_render = ((m_RunType == RunType::ENTITY_RENDER) || (m_RunType == RunType::BASIC_RENDER));
	WGPUShaderStageFlags vis_flags = is_render ? WGPUShaderStage_Vertex | WGPUShaderStage_Fragment : WGPUShaderStage_Compute;

	QStringList insertion_buffers;

	std::shared_ptr<SSBO> replace = nullptr;
	int num_entities = m_Entity ? m_Entity->GetMaxIndex() : 0;
	//if (is_render && m_Entity) {
	//	int time_slider = Core::GetInterfaceData().p_BufferView.p_TimeSlider;
	//	if (time_slider > 0) {
	//		replace = BufferViewer::GetStoredFrameAt(m_Entity->GetName(), Core::GetTicks() - time_slider, num_entities);
	//	}
	//}

	for (auto str : m_ShaderDefines) {
		insertion += QString("#define %1").arg(str);
	}

	result->m_ControlSSBO = m_ControlSSBO ? m_ControlSSBO : (m_Entity ? m_Entity->GetControlSSBO() : nullptr);
	if (result->m_ControlSSBO) {
		if (is_render) {
			insertion_buffers += "@group(0) @binding(0) var<storage, read> b_Control: array<i32>;";
		} else {
			insertion_buffers += "@group(0) @binding(0) var<storage, read_write> b_Control: array<atomic<i32>>;";
		}
		result->m_Pipeline->AddBuffer(*result->m_ControlSSBO, vis_flags, is_render);
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

//TODO: add all the buffers here, and setup structs and various ways to access them

	if(entity_processing) {
		m_Vars.push_back({ "ioCount" });
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			m_Vars.push_back({ "ioEntityDeaths" });
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Vars.push_back({ "ioEntityDeaths" });
			m_Vars.push_back({ "ioEntityFreeCount" });
		}
	}
	
	if (result->m_ControlSSBO && (!m_DataVectors.empty())) {
		end_insertion += "//////////////// Data vector helpers";
		for (const auto& data_vect : m_DataVectors) {
			QString count_var;
			QString offset_var;
			end_insertion += GetDataVectorStructCode(data_vect, count_var, offset_var);
			m_Vars.push_back({ count_var });
			m_Vars.push_back({ offset_var });
			result->m_DataVectors.push_back({ data_vect.p_Name, count_var, offset_var, nullptr, 0 });
		}
	}

	/*
	const bool rand_gen = true; // add by default, why not
	if (rand_gen) {

		int control_ind = (int)var_vals.size();
		int seed = rand();
		var_vals.push_back({ seed, nullptr });

		insertion += "#include \"Random.glsl\"\n";
		insertion += QString("float Random(float min_val = 0.0, float max_val = 1.0) { return GetRandom(min_val, max_val, atomicAdd(b_Control.i[%1], 1)); }").arg(control_ind);
	}
	*/

	if(m_Entity) {
		bool input_read_only = (!entity_processing) || m_Entity->IsDoubleBuffering();
		result->m_Pipeline->AddBuffer(replace.get() ? *replace.get() : *m_Entity->GetSSBO(), vis_flags, input_read_only);
		result->m_EntityBufferIndex = insertion_buffers.size();
		insertion_buffers += QString("@group(0) @binding(%1) var<storage, %2> b_%3: array<i32>;").arg(insertion_buffers.size()).arg(input_read_only ? "read" : "read_write").arg(m_Entity->GetName());

		if (entity_processing && m_Entity->IsDoubleBuffering()) {
			result->m_Pipeline->AddBuffer(*m_Entity->GetOuputSSBO(), vis_flags, false);
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_Output%2: array<i32>;").arg(insertion_buffers.size()).arg(m_Entity->GetName());
			insertion += m_Entity->GetDoubleBufferGPUInsertion();
		} else {
			insertion += m_Entity->GetGPUInsertion();
		}
		if (entity_processing) {
			insertion += m_Entity->GetSpecs().p_GPUInsertion;
		} else {
			insertion += m_Entity->GetSpecs().p_GPUReadOnlyInsertion;
		}
	}

	if(entity_processing) {
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			m_Entity->GetFreeListSSBO()->EnsureSizeBytes((num_entities + m_Entity->GetFreeCount()) * sizeof(int));
			result->m_Pipeline->AddBuffer(*m_Entity->GetFreeListSSBO(), vis_flags, false);
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_Death: array<i32>;").arg(insertion_buffers.size());
		} else if(m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Entity->GetFreeListSSBO()->EnsureSizeBytes((num_entities + m_Entity->GetFreeCount()) * sizeof(int), false);
			result->m_Pipeline->AddBuffer(*m_Entity->GetFreeListSSBO(), vis_flags, false);
			insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_FreeList: array<i32>;").arg(insertion_buffers.size());
		}
		insertion += QString("fn Destroy%1(index: i32) {").arg(m_Entity->GetName());
		insertion += QString("\tlet base = index * FLOATS_PER_%1;").arg(m_Entity->GetName());
		insertion += QString("\t%1_SET(base, 0, -1);").arg(m_Entity->GetName());

		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			insertion += "\tlet death_index = atomicAdd(&ioEntityDeaths, 1);";
			insertion += QString("\tb_Death[death_index] = index;");
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			insertion += "\tatomicAdd(&ioEntityDeaths, 1);";
			insertion += "\tlet free_index = atomicAdd(&ioEntityFreeCount, 1);";
			insertion += "\tb_FreeList[free_index] = index;";
		}
		insertion += "}";
	}

	/*
	for (int b = 0; b < m_SSBOs.size(); b++) {
		int buffer_index = insertion_buffers.size();
		auto& ssbo = m_SSBOs[b];
		ssbo_binds.push_back({ &ssbo.p_Buffer, buffer_index });
		insertion_buffers += QString("layout(std430, binding = %1) %4buffer GenericBuffer%1 { %2 i[]; } %3;").arg(buffer_index).arg(MemberSpec::GetGPUType(ssbo.p_Type)).arg(ssbo.p_Name).arg(ssbo.p_ReadOnly ? "readonly " : "");
	}
	for (const auto& tex : m_Textures) {
		insertion_uniforms += QString("uniform sampler2D %1;").arg(tex.p_Name);
	}
	*/
	
	for (int e = 0; e < m_Entities.size(); e++) {
		GPUEntity& entity = m_Entities[e].p_Entity;
		QString name = entity.GetName();
		insertion += QString("//////////////// Entity %1").arg(name);

		m_Vars.push_back({ QString("io%1Count").arg(name) });

		insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_%2: array<i32>;").arg(insertion_buffers.size()).arg(entity.GetName());
		result->m_Pipeline->AddBuffer(*entity.GetSSBO(), vis_flags, false);

		insertion += entity.GetGPUInsertion();
		insertion += QString(entity.GetSpecs().p_GPUInsertion).arg(name);

		if (m_Entities[e].p_Creatable) {

			QString next_id = QString("io%1NextId").arg(name);
			QString max_index = QString("io%1MaxIndex").arg(name);
			m_Vars.push_back({ next_id });
			m_Vars.push_back({ max_index });

			insertion += QString("fn Create%1(input_item: %1) {").arg(name);
			insertion += "\tvar item = input_item;";
			insertion += QString("\tlet item_id: i32 = atomicAdd(&%1, 1);").arg(next_id);
			
			if (entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
				QString free_count = QString("io%1FreeCount").arg(name);
				m_Vars.push_back({ free_count });

				insertion_buffers += QString("@group(0) @binding(%1) var<storage, read_write> b_Free%2: array<i32>;").arg(insertion_buffers.size()).arg(entity.GetName());
				entity.GetFreeListSSBO()->EnsureSizeBytes((entity.GetCount() + entity.GetFreeCount() + 16) * sizeof(int), false);
				result->m_Pipeline->AddBuffer(*entity.GetFreeListSSBO(), vis_flags, false);

				insertion += "\tvar item_pos: i32 = 0;";
				insertion += QString("\tlet free_count: i32 = atomicAdd(&%1, -1);").arg(free_count);
				insertion += "\tif(free_count > 0) {";
					insertion += QString("\t\titem_pos = b_Free%1[free_count - 1];").arg(entity.GetName());
				insertion += "\t} else {";
					insertion += QString("\t\titem_pos = atomicAdd(&%1, 1);").arg(max_index);
				insertion += "\t}";
			} else {
				insertion += QString("\tlet item_pos: i32 = atomicAdd(&%1, 1);").arg(max_index);
			}
			insertion += QString("\titem.%1 = item_id;").arg(entity.GetIDName());
			insertion += QString("\tSet%1(item, item_pos);\n}").arg(name);
		}
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
			"\tif (item_index >= Get_ioCount) { return; }\n"
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
			"\tlet item_index = global_id.x;\n"
			"\tif (item_index >= ioCount) return;\n"
			"\tlet item: %1 = Get%1(item_index);";
		end_insertion += QString("\tif (item.%1 < 0) return;").arg(m_Entity->GetIDName());
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
		result->m_Pipeline->FinalizeRender(m_ShaderName, *m_Buffer, insertion_str, end_insertion_str);
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
void PipelineStage::Prepared::Run(unsigned char* uniform, int uniform_bytes, std::vector<std::pair<QString, int*>>&& variables, int iterations, RTT* rtt) {

	bool compute = m_Pipeline->GetType() == WebGPUPipeline::Type::COMPUTE;
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);

	if (!compute && !rtt) {
		throw std::invalid_argument("Render requires an RTT");
	}

	int entity_deaths = 0;
	int entity_free_count = m_Entity ? m_Entity->GetFreeCount() : 0; // only used for DeleteMode::STABLE_WITH_GAPS

	int run_count = m_Entity ? m_Entity->GetCount() : iterations;

	if (entity_processing) {
		variables.push_back({ "ioCount", &run_count });
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			variables.push_back({ "ioEntityDeaths", &entity_deaths });
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			variables.push_back({ "ioEntityDeaths", &entity_deaths });
			variables.push_back({ "ioEntityFreeCount", &entity_free_count });
		}
	}

	struct EntityControl {
		int		p_MaxIndex = 0;
		int		p_NextId = 0;
		int		p_FreeCount = 0;
	};
	std::vector<EntityControl> entity_controls;
	entity_controls.resize(m_Entities.size()); // resize to avoid reallocations, so you can have pointers to it

	for (int e = 0; e < m_Entities.size(); e++) {
		GPUEntity* entity = m_Entities[e];
		QString name = entity->GetName();
		entity_controls[e] = { entity->GetMaxIndex(), entity->GetNextId(), entity->GetFreeCount() };

		variables.push_back({ QString("io%1NextId").arg(name), &entity_controls[e].p_NextId });
		variables.push_back({ QString("io%1MaxIndex").arg(name), &entity_controls[e].p_MaxIndex });
		if (entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			variables.push_back({ QString("io%1FreeCount").arg(name), &entity_controls[e].p_FreeCount });
		}

		if (entity->IsDoubleBuffering()) {
			m_Pipeline->ReplaceBuffer(*entity->GetOuputSSBO(), *entity->GetSSBO());
		}

	}
	
	auto fill_var_data = [this, &variables] (QString name, std::pair<int, int*>& pair) {

		for (const auto& var: variables) {
			if (var.first == name) {
				pair = { var.second ? *var.second : 0, var.second };
				return;
			}
		}
		int offset = m_VarNames.size();
		for (const auto& vect : m_DataVectors) {
			if (vect.m_CountVar == name) {
				pair.first = vect.m_SizeInts;
				return;
			}
			if (vect.m_OffsetVar == name) {
				pair.first = offset;
				return;
			}
			offset += vect.m_SizeInts;
		}
	};

	std::vector<std::pair<int, int*>> var_vals(m_VarNames.size(), { 0, nullptr });
	for (int i = 0; i < m_VarNames.size(); i++) {

		fill_var_data(m_VarNames[i], var_vals[i]);
		/*
		for (const auto& var : variables) {
			if (var.first == m_VarNames[i]) {
				var_vals[i] = { var.second ? *var.second : 0, var.second };
				break;
			}
		}
		int offset = m_VarNames.size();
		for (const auto& vect : m_DataVectors) {
			if (vect.m_CountVar == m_VarNames[i]) {
				var_vals[i].first = vect.m_SizeInts;
				break;
			}
			if (vect.m_OffsetVar == m_VarNames[i]) {
				var_vals[i].first = offset;
				break;
			}
			offset += vect.m_SizeInts;
		}
		*/
	}

	if (m_ControlSSBO && !var_vals.empty()) {

		std::vector<int> values;
		for (auto var : var_vals) {
			values.push_back(var.first);
		}
		int control_size = (int)var_vals.size();

		// allocate some space for vectors on the control buffer and copy data over
		for (const auto& data_vect : m_DataVectors) {
			int data_size = data_vect.m_SizeInts;
			int offset = control_size;
			control_size += data_size;
			values.resize(control_size);
			memcpy((unsigned char*)&(values[offset]), data_vect.m_Data, sizeof(int) * data_size);
		}

		m_ControlSSBO->EnsureSizeBytes(control_size * sizeof(int));
		m_ControlSSBO->SetValues(values);
	}

	if (m_UniformBuffer && uniform && uniform_bytes) {
		m_UniformBuffer->EnsureSizeBytes(uniform_bytes, false);
		m_UniformBuffer->Write(uniform, 0, uniform_bytes);
	}

	if (compute) {
		m_Pipeline->Compute(run_count);
	} else {
		if (!m_Buffer) {
			throw std::invalid_argument("Render buffer not found");
		}
		if (m_Entity && m_Entity->IsDoubleBuffering()) {
			m_Pipeline->ReplaceBuffer(m_EntityBufferIndex, *m_Entity->GetSSBO());
		}

		rtt->Render(m_Pipeline, run_count);
	}

	if (entity_processing && m_Entity->IsDoubleBuffering()) {
		m_Pipeline->ReplaceBuffer(*m_Entity->GetOuputSSBO(), *m_Entity->GetSSBO());
		m_Pipeline->ReplaceBuffer(*m_Entity->GetSSBO(), *m_Entity->GetOuputSSBO());
		m_Entity->SwapInputOutputSSBOs();
	}

	/*
	////////////////////////////////////////////////////
	if (entity_processing && (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT)) {
		//DebugGPU::Checkpoint(QString("%1 Death").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT);
	}
	*/

	if (m_ControlSSBO) {
		std::vector<int> control_values;
		m_ControlSSBO->GetValues<int>(control_values, (int)var_vals.size());
		for (int v = 0; v < var_vals.size(); v++) {
			if (var_vals[v].second) {
				*var_vals[v].second = control_values[v];
			}
		}
	}

	for (int e = 0; e < m_Entities.size(); e++) {
		auto entity = m_Entities[e];
		if (entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			entity->ProcessStableCreates(entity_controls[e].p_MaxIndex, entity_controls[e].p_NextId, entity_controls[e].p_FreeCount);
		} else {
			entity->ProcessMoveCreates(entity_controls[e].p_MaxIndex, entity_controls[e].p_NextId);
		}
	}

	if (entity_processing) {
		if ((m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) && (entity_deaths > 0)) {
			m_Entity->ProcessMoveDeaths(entity_deaths);
			//if (true) {
			//	BufferViewer::Checkpoint(QString("%1 Death Control").arg(m_Entity->GetName()), "PostRun", *control_ssbo, MemberSpec::Type::T_INT);
			//}
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Entity->ProcessStableDeaths(entity_deaths);
		}

		//BufferViewer::Checkpoint(QString("%1 Control").arg(m_Entity.GetName()), "PostRun", *control_ssbo, MemberSpec::Type::T_INT);

		//BufferViewer::Checkpoint("PostRun", m_Entity);
		//if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			//BufferViewer::Checkpoint(QString("%1 Free").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT, m_Entity.GetFreeCount());
		//}
	}
}

#endif

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

#if defined(NESHNY_GL)
	QString main_name;
	if (m_Query == QueryType::Position2D) {
		main_name = "Nearest2D";
	} else if (m_Query == QueryType::Position3D) {
		main_name = "Nearest3D";
	} else if (m_Query == QueryType::ID) {
		main_name = "IsID";
	}

	QString main_func =
		QString("void ItemMain%3(int item_index, vec2 pos);\nvoid %1Main(int item_index, %1 item) { ItemMain%3(item_index, item.%2); }")
		.arg(m_Entity.GetName()).arg(m_ParamName).arg(main_name);

	int best_index = -1;
	int best_dist = std::numeric_limits<int>::max();
	PipelineStage::IterateEntity(
		m_Entity
		,"Query"
		,true
		,{}
	)
	.AddCode(main_func)
	.AddInputOutputVar("ioIndex", &best_index)
	.AddInputOutputVar("ioDistance", &best_dist)
	.Run([this](GLShader* prog) {
		if (std::holds_alternative<fVec2>(m_QueryParam)) {
			fVec2 v = std::get<fVec2>(m_QueryParam);
			glUniform2f(prog->GetUniform("uPos2D"), v.x, v.y);
		} else if (std::holds_alternative<fVec3>(m_QueryParam)) {
			fVec3 v = std::get<fVec3>(m_QueryParam);
			glUniform3f(prog->GetUniform("uPos3D"), v.x, v.y, v.z);
		} else if (std::holds_alternative<int>(m_QueryParam)) {
			int v = std::get<int>(m_QueryParam);
			glUniform1i(prog->GetUniform("uID"), v);
		}
	});
	return best_index;
#else
#pragma msg("finish webGPU version")
	return -1;
#endif
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
#endif
{
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::GenerateCache(iVec2 grid_size, Vec2 grid_min, Vec2 grid_max) {

#if defined(NESHNY_GL)

	m_GridSize = grid_size;
	m_GridMin = grid_min;
	m_GridMax = grid_max;

	Vec2 range = grid_max - grid_min;
	Vec2 cell_size(range.x / grid_size.x, range.y / grid_size.y);
	m_GridIndices.EnsureSizeBytes(grid_size.x * grid_size.y * 3 * sizeof(int));
	m_GridItems.EnsureSizeBytes(m_Entity.GetCount() * sizeof(int));

	QString main_func =
		QString("void ItemMain(int item_index, vec2 pos);\nvoid %1Main(int item_index, %1 item) { ItemMain(item_index, item.%2); }")
		.arg(m_Entity.GetName()).arg(m_PosName);

	PipelineStage::IterateEntity(
		m_Entity
		,"GridCache2D"
		,true
		,{ "PHASE_INDEX" }
	)
	.AddCode(main_func)
	.AddSSBO("b_Index", m_GridIndices, MemberSpec::Type::T_INT, false)
	.Run([grid_size, grid_min, grid_max](GLShader* prog) {
		glUniform2i(prog->GetUniform("uGridSize"), grid_size.x, grid_size.y);
		glUniform2f(prog->GetUniform("uGridMin"), grid_min.x, grid_min.y);
		glUniform2f(prog->GetUniform("uGridMax"), grid_max.x, grid_max.y);
	});

	int alloc_count = 0;
	PipelineStage::IterateEntity(
		m_Entity
		,"GridCache2D"
		,true
		,{ "PHASE_ALLOCATE" }
	)
	.AddCode(main_func)
	.AddSSBO("b_Index", m_GridIndices, MemberSpec::Type::T_INT, false)
	.AddInputOutputVar("AllocationCount", &alloc_count)
	.Run([grid_size, grid_min, grid_max](GLShader* prog) {
		glUniform2i(prog->GetUniform("uGridSize"), grid_size.x, grid_size.y);
		glUniform2f(prog->GetUniform("uGridMin"), grid_min.x, grid_min.y);
		glUniform2f(prog->GetUniform("uGridMax"), grid_max.x, grid_max.y);
	});

	//BufferViewer::Checkpoint(m_Entity.GetName() + "_Cache", "Gen", m_GridIndices, MemberSpec::Type::T_INT);

	PipelineStage::IterateEntity(
		m_Entity
		,"GridCache2D"
		,true
		,{ "PHASE_FILL" }
	)
	.AddCode(main_func)
	.AddSSBO("b_Index", m_GridIndices, MemberSpec::Type::T_INT, false)
	.AddSSBO("b_Cache", m_GridItems, MemberSpec::Type::T_INT, false)
	.Run([grid_size, grid_min, grid_max](GLShader* prog) {
		glUniform2i(prog->GetUniform("uGridSize"), grid_size.x, grid_size.y);
		glUniform2f(prog->GetUniform("uGridMin"), grid_min.x, grid_min.y);
		glUniform2f(prog->GetUniform("uGridMax"), grid_max.x, grid_max.y);
	});

	//BufferViewer::Checkpoint(m_Entity.GetName() + "_Cache_Items", "Gen", m_GridItems, MemberSpec::Type::T_INT);
#else
#pragma msg("finish webGPU version")
#endif

}

////////////////////////////////////////////////////////////////////////////////
QString Grid2DCache::Bind(std::vector<PipelineStage::AddedSSBO>& ssbos) {

	auto name = m_Entity.GetName();

	ssbos.push_back({ m_GridIndices, QString("b_%1GridIndices").arg(name), MemberSpec::Type::T_INT, true });
	ssbos.push_back({ m_GridItems, QString("b_%1GridItems").arg(name), MemberSpec::Type::T_INT, true });

	return QString(
			"ivec2 GetGridPos(vec2 pos, vec2 grid_min, vec2 grid_max, ivec2 grid_size); // forward declare \n"
			"ivec2 Get%1IndexRangeAt(ivec2 grid_pos) {\n"
			"\tint index = (grid_pos.x + grid_pos.y * %6) * 3;\n"
			"\tint start = b_%1GridIndices.i[index + 1];\n"
			"\tint count = b_%1GridIndices.i[index + 2];\n"
			"\treturn ivec2(start, start + count);\n"
			"}\n"
			"int Get%1IndexAtCache(int index) {\n"
			"\treturn b_%1GridItems.i[index];\n"
			"}\n"
			"ivec2 Get%1GridPosAt(vec2 pos) {\n"
			"\treturn GetGridPos(pos, vec2(%2, %3), vec2(%4, %5), ivec2(%6, %7));\n" // TODO: should you be hardcoding in the grid params?
			"}"
		)
		.arg(name)
		.arg(m_GridMin.x, 0, 'f', 6)
		.arg(m_GridMin.y, 0, 'f', 6)
		.arg(m_GridMax.x, 0, 'f', 6)
		.arg(m_GridMax.y, 0, 'f', 6)
		.arg(m_GridSize.x)
		.arg(m_GridSize.y)
	;
}

} // namespace Neshny