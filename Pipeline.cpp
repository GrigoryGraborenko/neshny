////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
PipelineStage::PipelineStage(RunType type, GPUEntity* entity, GLBuffer* buffer, BaseCache* cache, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines) :
	m_RunType			( type )
	,m_Entity			( entity )
	,m_Buffer			( buffer )
	,m_ShaderName		( shader_name )
	,m_ReplaceMain		( replace_main )
	,m_ShaderDefines	( shader_defines )
{
	if (cache) {
		m_ExtraCode += cache->Bind(m_SSBOs);
	}
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

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddInputOutputVar(QString name, int* in_out) {
	m_Vars.push_back({ name, in_out });
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddSSBO(QString name, GLSSBO& ssbo, MemberSpec::Type array_type, bool read_only) {
	m_SSBOs.push_back({ ssbo, name, array_type, read_only });
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
QString PipelineStage::GetUniformVectorStructCode(AddedUniformVector& uniform, QStringList& insertion_uniforms, std::vector<std::pair<QString, int>>& integer_vars, std::vector<std::pair<QString, std::vector<float>*>>& vector_vars) {

	QStringList insertion;

	insertion += QString("struct %1 {").arg(uniform.p_Name);

	QStringList function(QString("%1 Get%1(int index) {\n\t%1 item;").arg(uniform.p_Name));
	function += QString("\tindex *= %1;").arg(uniform.p_NumFloatsPerItem);

	int pos_index = 0;
	for (const auto& member : uniform.p_Members) {
		insertion += QString("\t%1 %2;").arg(MemberSpec::GetGPUType(member.p_Type)).arg(member.p_Name);

		QString name = member.p_Name;
		if (member.p_Type == MemberSpec::T_INT) {
			function += QString("\titem.%1 = floatBitsToInt(u%2[index + %3]);").arg(name).arg(uniform.p_Name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_FLOAT) {
			function += QString("\titem.%1 = u%2[index + %3];").arg(name).arg(uniform.p_Name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_VEC2) {
			function += QString("\titem.%1 = vec2(u%2[index + %3], u%2[index + %3 + 1]);").arg(name).arg(uniform.p_Name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_VEC3) {
			function += QString("\titem.%1 = vec3(u%2[index + %3], u%2[index + %3 + 1], u%2[index + %3 + 2]);").arg(name).arg(uniform.p_Name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_VEC4) {
			function += QString("\titem.%1 = vec4(u%2[index + %3], u%2[index + %3 + 1], u%2[index + %3 + 2], u%2[index + %3 + 3]);").arg(name).arg(uniform.p_Name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_IVEC2) {
			function += QString("\titem.%1 = ivec2(u%2[index + %3], u%2[index + %3 + 1]);").arg(name).arg(uniform.p_Name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_IVEC3) {
			function += QString("\titem.%1 = ivec3(u%2[index + %3], u%2[index + %3 + 1], u%2[index + %3 + 2]);").arg(name).arg(uniform.p_Name).arg(pos_index);
		} else if (member.p_Type == MemberSpec::T_IVEC4) {
			function += QString("\titem.%1 = ivec4(u%2[index + %3], u%2[index + %3 + 1], u%2[index + %3 + 2], u%2[index + %3 + 3]);").arg(name).arg(uniform.p_Name).arg(pos_index);
		}
		pos_index += member.p_Size / sizeof(float);

	}
	function += "\treturn item;\n};";

	insertion += "};";
	insertion += function.join("\n");

	int array_size = MINIMUM_UNIFORM_VECTOR_LENGTH;
	while (array_size < uniform.p_NumItems) {
		array_size *= 2;
	}
	int float_count = uniform.p_NumFloatsPerItem * array_size;

	insertion_uniforms += QString("uniform int u%1Count;").arg(uniform.p_Name);
	insertion_uniforms += QString("uniform float u%1[%2];").arg(uniform.p_Name).arg(float_count);

	integer_vars.emplace_back(QString("u%1Count").arg(uniform.p_Name), uniform.p_NumItems);
	vector_vars.push_back({ QString("u%1").arg(uniform.p_Name), &uniform.p_Data });

	return insertion.join("\n");
}

////////////////////////////////////////////////////////////////////////////////
void PipelineStage::Run(std::optional<std::function<void(GLShader* program)>> pre_execute) {

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
	std::vector<std::pair<QString, std::vector<float>*>> vector_vars;
	std::vector<std::pair<GLSSBO*, int>> ssbo_binds;
	std::vector<std::pair<int, int*>> var_vals;

	if (!is_render) {
		insertion += QString("layout(local_size_x = %1, local_size_y = %2, local_size_z = %3) in;").arg(m_LocalSizeX).arg(m_LocalSizeY).arg(m_LocalSizeZ);
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
	} else if(m_Entity) {
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
		insertion += QString("float Random(float min_val = 0.0, float max_val = 1.0) { return GetRandom(min_val, max_val, float(atomicAdd(b_Control.i[%1], 1))); }").arg(control_ind);
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
			m_Entity->GetFreeListSSBO()->EnsureSize(num_entities * sizeof(int));
			m_Entity->GetFreeListSSBO()->Bind(buffer_index);
			insertion_buffers += QString("layout(std430, binding = %1) writeonly buffer DeathBuffer { int i[]; } b_Death;").arg(buffer_index);
		} else if(m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			int buffer_index = insertion_buffers.size();
			m_Entity->GetFreeListSSBO()->EnsureSize(num_entities * sizeof(int), false);
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

	if (!m_UniformVectors.empty()) {
		insertion += "//////////////// Uniform vector helpers";
	}
	for (auto& uniform : m_UniformVectors) {
		insertion += GetUniformVectorStructCode(uniform, insertion_uniforms, integer_vars, vector_vars);
	}

	insertion += "////////////////";
	insertion.push_front(insertion_uniforms.join("\n"));
	insertion.push_front(insertion_buffers.join("\n"));
	insertion.push_front(insertion_images.join("\n"));

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

	// TODO control SSBO should be created if it doesn't exist
	GLSSBO* control_ssbo = m_Entity ? m_Entity->GetControlSSBO() : nullptr;
	if (control_ssbo && !var_vals.empty()) {

		control_ssbo->EnsureSize((int)var_vals.size() * sizeof(int));
		control_ssbo->Bind(0); // you can assume this is at index zero

		std::vector<int> values;
		for (auto var: var_vals) {
			values.push_back(var.first);
		}
		control_ssbo->SetValues(values);
	}

	if (!m_ExtraCode.isNull()) {
		insertion += "////////////////";
		insertion += m_ExtraCode;
	}

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
	for (const auto& var : vector_vars) {
		glUniform1fv(prog->GetUniform(var.first), (int)var.second->size(), &((*var.second)[0]));
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
	if (entity_processing || (m_RunType == RunType::ENTITY_ITERATE)) {
		Core::DispatchMultiple(prog, num_entities, m_LocalSizeX * m_LocalSizeY * m_LocalSizeZ);
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Grid2DCache::Grid2DCache(GPUEntity& entity, QString pos_name) :
	m_Entity	( entity )
	,m_PosName	( pos_name )
{
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::GenerateCache(iVec2 grid_size, Vec2 grid_min, Vec2 grid_max) {

	m_GridSize = grid_size;
	m_GridMin = grid_min;
	m_GridMax = grid_max;

	Vec2 range = grid_max - grid_min;
	Vec2 cell_size(range.x / grid_size.x, range.y / grid_size.y);
	m_GridIndices.EnsureSize(grid_size.x * grid_size.y * 3 * sizeof(int));
	m_GridItems.EnsureSize(m_Entity.GetCount() * sizeof(int));

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