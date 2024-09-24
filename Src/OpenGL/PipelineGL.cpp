////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(NESHNY_GL)
namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
EntityPipeline::EntityPipeline(RunType type, GPUEntity* entity, RenderableBuffer* buffer, BaseCache* cache, std::string_view shader_name, bool replace_main, const std::vector<std::string>& shader_defines, SSBO* control_ssbo, int iterations) :
	m_RunType			( type )
	,m_Entity			( entity )
	,m_Buffer			( buffer )
	,m_Cache			( cache )
	,m_ShaderName		( shader_name )
	,m_ReplaceMain		( replace_main )
	,m_ShaderDefines	( shader_defines )
	,m_Iterations		( iterations )
	,m_ControlSSBO		( control_ssbo )
{
	if (cache) {
		cache->Bind(*this);
	}
}

////////////////////////////////////////////////////////////////////////////////
EntityPipeline& EntityPipeline::AddEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ &entity, false });
	if (cache) {
		cache->Bind(*this);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
EntityPipeline& EntityPipeline::AddCreatableEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ &entity, true });
	if (cache) {
		cache->Bind(*this);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
std::string EntityPipeline::GetDataVectorStructCode(const AddedDataVector& data_vect, std::vector<std::string>& insertion_uniforms, std::vector<std::pair<std::string, int>>& integer_vars, int offset) {

	std::vector<std::string> insertion;
	insertion.push_back(std::format("struct {0} {{", data_vect.p_Name));

	std::vector<std::string> function{ std::format("{0} Get{0}(int index) {{\n\t{0} item;", data_vect.p_Name) };
	function.push_back(std::format("\tindex = u{0}Offset + (index * {1});", data_vect.p_Name, data_vect.p_NumIntsPerItem));

	int pos_index = 0;
	for (const auto& member : data_vect.p_Members) {
		insertion.push_back(std::format("\t{} {};", MemberSpec::GetGPUType(member.p_Type), member.p_Name));

		if (member.p_Type == MemberSpec::T_INT) {
			function.push_back(std::format("\titem.{0} = b_Control.i[index + {1}];", member.p_Name, pos_index));
		} else if (member.p_Type == MemberSpec::T_FLOAT) {
			function.push_back(std::format("\titem.{0} = intBitsToFloat(b_Control.i[index + {1}]);", member.p_Name, pos_index));
		} else if (member.p_Type == MemberSpec::T_VEC2) {
			function.push_back(std::format("\titem.{0} = vec2(intBitsToFloat(b_Control.i[index + {1}]), intBitsToFloat(b_Control.i[index + {1} + 1]));", member.p_Name, pos_index));
		} else if (member.p_Type == MemberSpec::T_VEC3) {
			function.push_back(std::format("\titem.{0} = vec3(intBitsToFloat(b_Control.i[index + {1}]), intBitsToFloat(b_Control.i[index + {1} + 1]), intBitsToFloat(b_Control.i[index + {1} + 2]));", member.p_Name, pos_index));
		} else if (member.p_Type == MemberSpec::T_VEC4) {
			function.push_back(std::format("\titem.{0} = vec4(intBitsToFloat(b_Control.i[index + {1}]), intBitsToFloat(b_Control.i[index + {1} + 1]), intBitsToFloat(b_Control.i[index + {1} + 2]), intBitsToFloat(b_Control.i[index + {1} + 3]));", member.p_Name, pos_index));
		} else if (member.p_Type == MemberSpec::T_IVEC2) {
			function.push_back(std::format("\titem.{0} = ivec2(b_Control.i[index + {1}], b_Control.i[index + {1} + 1]);", member.p_Name, pos_index));
		} else if (member.p_Type == MemberSpec::T_IVEC3) {
			function.push_back(std::format("\titem.{0} = ivec3(b_Control.i[index + {1}], b_Control.i[index + {1} + 1], b_Control.i[index + {1} + 2]);", member.p_Name, pos_index));
		} else if (member.p_Type == MemberSpec::T_IVEC4) {
			function.push_back(std::format("\titem.{0} = ivec4(b_Control.i[index + {1}], b_Control.i[index + {1} + 1], b_Control.i[index + {1} + 2], b_Control.i[index + {1} + 3]);", member.p_Name, pos_index));
		}
		pos_index += member.p_Size / sizeof(float);
	}
	function.push_back("\treturn item;\n};");

	insertion.push_back("};");
	insertion.push_back(JoinStrings(function, "\n"));

	insertion_uniforms.push_back(std::format("uniform int u{0}Count;", data_vect.p_Name));
	integer_vars.emplace_back(std::format("u{0}Count", data_vect.p_Name), data_vect.p_NumItems);

	insertion_uniforms.push_back(std::format("uniform int u{0}Offset;", data_vect.p_Name));
	integer_vars.emplace_back(std::format("u{0}Offset", data_vect.p_Name), offset);
	return JoinStrings(insertion, "\n");
}

////////////////////////////////////////////////////////////////////////////////
void EntityPipeline::Run(std::optional<std::function<void(Shader* program)>> pre_execute) {

	// TODO: investigate GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS
	// TODO: debug of SSBOs is optional

	std::list<std::string> insertion;
	int entity_deaths = 0;
	int entity_free_count = m_Entity ? m_Entity->GetFreeCount() : 0; // only used for DeleteMode::STABLE_WITH_GAPS
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);
	bool is_render = ((m_RunType == RunType::ENTITY_RENDER) || (m_RunType == RunType::BASIC_RENDER));

	std::vector<std::string> insertion_images;
	std::vector<std::string> insertion_buffers;
	std::vector<std::string> insertion_uniforms;
	std::vector<std::pair<std::string, int>> integer_vars;
	std::vector<std::pair<GLSSBO*, int>> ssbo_binds;
	std::vector<std::pair<int, int*>> var_vals;

	if (!is_render) {
		insertion.push_back(std::format("layout(local_size_x = {0}, local_size_y = {1}, local_size_z = {2}) in;", m_LocalSize.x, m_LocalSize.y, m_LocalSize.z));
	}

	std::shared_ptr<GLSSBO> replace = nullptr;
	int num_entities = m_Entity ? m_Entity->GetMaxIndex() : 0;
	if (is_render && m_Entity) {
		insertion_uniforms.push_back("uniform int uCount;");
		integer_vars.push_back({ std::format("u{0}Count", m_Entity->GetName()), num_entities });

		int time_slider = Core::GetInterfaceData().p_BufferView.p_TimeSlider;
		if (time_slider > 0) {
			replace = BufferViewer::GetStoredFrameAt(m_Entity->GetName(), Core::GetTicks() - time_slider, num_entities);
		}
	} else if(m_Entity || (m_RunType == RunType::BASIC_COMPUTE)) {
		insertion_uniforms.push_back("uniform int uCount;");
		insertion_uniforms.push_back("uniform int uOffset;");
	}
	for (auto str : m_ShaderDefines) {
		insertion.push_back(std::format("#define {0}", str));
	}
	insertion_buffers.push_back("layout(std430, binding = 0) buffer ControlBuffer { int i[]; } b_Control;");
	if(entity_processing) {
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			m_Vars.push_back({ "ioEntityDeaths", &entity_deaths });
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Vars.push_back({ "ioEntityDeaths", &entity_deaths });
			m_Vars.push_back({ "ioEntityFreeCount", &entity_free_count });
		}
	}
	for (const auto& var : m_Vars) {
		insertion.push_back(std::format("#define {0} (b_Control.i[{1}])", var.p_Name, (int)var_vals.size()));
		var_vals.push_back({ var.p_Ptr ? *var.p_Ptr : 0, var.p_Ptr });
	}
	const bool rand_gen = true; // add by default, why not
	if (rand_gen) {

		int control_ind = (int)var_vals.size();
		int seed = rand();
		var_vals.push_back({ seed, nullptr });

		insertion.push_back("#include \"Random.glsl\"\n");
		insertion.push_back(std::format("float Random(float min_val = 0.0, float max_val = 1.0) {{ return GetRandom(min_val, max_val, atomicAdd(b_Control.i[{0}], 1)); }}", control_ind));
	}

	if(m_Entity) {
		int buffer_index = insertion_buffers.size();
		ssbo_binds.push_back({ replace.get() ? replace.get() : m_Entity->GetSSBO(), buffer_index });
		if (entity_processing && m_Entity->IsDoubleBuffering()) {
			insertion_buffers.push_back(std::format("layout(std430, binding = {0}) readonly buffer MainEntityBuffer {{ int i[]; }} b_{1};", buffer_index, m_Entity->GetName()));
			buffer_index++;
			ssbo_binds.push_back({ m_Entity->GetOuputSSBO(), buffer_index });
			insertion_buffers.push_back(std::format("layout(std430, binding = {0}) writeonly buffer MainEntityOutputBuffer {{ int i[]; }} b_Output{1};", buffer_index, m_Entity->GetName()));
			insertion.push_back(std::string(m_Entity->GetDoubleBufferGPUInsertion()));
		} else {
			insertion_buffers.push_back(std::format("layout(std430, binding = {0}) buffer MainEntityBuffer {{ int i[]; }} b_{1};", buffer_index, m_Entity->GetName()));
			insertion.push_back(std::string(m_Entity->GetGPUInsertion()));
		}
		if (entity_processing) {
			insertion.push_back(std::string(m_Entity->GetSpecs().p_GPUInsertion));
		} else {
			insertion.push_back(std::string(m_Entity->GetSpecs().p_GPUReadOnlyInsertion));
		}
	}

	if(entity_processing) {
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			int buffer_index = insertion_buffers.size();
			m_Entity->GetFreeListSSBO()->EnsureSizeBytes((num_entities + m_Entity->GetFreeCount()) * sizeof(int));
			m_Entity->GetFreeListSSBO()->Bind(buffer_index);
			insertion_buffers.push_back(std::format("layout(std430, binding = {0}) writeonly buffer DeathBuffer {{ int i[]; }} b_Death;", buffer_index));
		} else if(m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			int buffer_index = insertion_buffers.size();
			m_Entity->GetFreeListSSBO()->EnsureSizeBytes((num_entities + m_Entity->GetFreeCount()) * sizeof(int), false);
			m_Entity->GetFreeListSSBO()->Bind(buffer_index);
			insertion_buffers.push_back(std::format("layout(std430, binding = {0}) writeonly buffer FreeBuffer {{ int i[]; }} b_FreeList;", buffer_index));
		}
		insertion.push_back(std::format("void Destroy{0}(int index) {{", m_Entity->GetName()));
		insertion.push_back(std::format("\tint base = index * FLOATS_PER_{0};", m_Entity->GetName()));
		insertion.push_back(std::format("\t{0}_SET(base, 0, -1);", m_Entity->GetName()));

		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			insertion.push_back("\tint death_index = atomicAdd(ioEntityDeaths, 1);");
			insertion.push_back(std::format("\tb_Death.i[death_index] = index;"));
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			insertion.push_back("\tatomicAdd(ioEntityDeaths, 1);");
			insertion.push_back("\tint free_index = atomicAdd(ioEntityFreeCount, 1);");
			insertion.push_back("\tb_FreeList.i[free_index] = index;");
		}
		insertion.push_back("}");
	}

	for (int b = 0; b < m_SSBOs.size(); b++) {
		int buffer_index = insertion_buffers.size();
		auto& ssbo = m_SSBOs[b];
		ssbo_binds.push_back({ &ssbo.p_Buffer, buffer_index });
		insertion_buffers.push_back(std::format("layout(std430, binding = {0}) {3}buffer GenericBuffer{0} {{ {1} i[]; }} {2};", buffer_index, MemberSpec::GetGPUType(ssbo.p_Type), ssbo.p_Name, ssbo.p_Access == BufferAccess::READ_ONLY ? "readonly " : ""));
	}
	for (const auto& tex : m_Textures) {
		insertion_uniforms.push_back(std::format("uniform sampler2D {0};", tex.p_Name));
	}
	
	struct EntityControl {
		int		p_MaxIndex;
		int		p_NextId;
		int		p_FreeCount;
	};
	std::vector<EntityControl> entity_controls;
	entity_controls.reserve(m_Entities.size()); // reserve to avoid reallocations, so you can have pointers to it
	for (int e = 0; e < m_Entities.size(); e++) {
		GPUEntity& entity = *m_Entities[e].p_Entity; // assume this is set
		insertion.push_back(std::format("//////////////// Entity {0}", entity.GetName()));
		integer_vars.push_back({ std::format("u{0}Count", entity.GetName()), entity.GetMaxIndex() });
		insertion_uniforms.push_back(std::format("uniform int u{0}Count;", entity.GetName()));
		{
			int buffer_index = insertion_buffers.size();
			ssbo_binds.push_back({ entity.GetSSBO(), buffer_index });
			insertion_buffers.push_back(std::format("layout(std430, binding = {0}) buffer EntityBuffer{0} {{ int i[]; }} b_{1};", buffer_index, entity.GetName()));
		}

		insertion.push_back(std::string(entity.GetGPUInsertion()));
		insertion.push_back(std::string(entity.GetSpecs().p_GPUInsertion));

		entity_controls.push_back({ entity.GetMaxIndex(), entity.GetNextId(), entity.GetFreeCount() });
		if (m_Entities[e].p_Creatable) {
			int control_index = (int)var_vals.size();
			auto& curr = entity_controls.back();
			var_vals.push_back({ curr.p_NextId, &curr.p_NextId });
			var_vals.push_back({ curr.p_MaxIndex, &curr.p_MaxIndex });

			insertion.push_back(std::format("void Create{0}({0} item) {{", entity.GetName()));
			insertion.push_back(std::format("\tint item_id = atomicAdd(b_Control.i[{0}], 1);", control_index));
			if (entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
				int buffer_index = insertion_buffers.size();
				insertion_buffers.push_back(std::format("layout(std430, binding = {0}) buffer FreeEntityBuffer{0} {{ int i[]; }} b_Free{1};", buffer_index, entity.GetName()));
				ssbo_binds.push_back({ entity.GetFreeListSSBO(), buffer_index });
				var_vals.push_back({ curr.p_FreeCount, &curr.p_FreeCount });
				insertion.push_back("\tint item_pos = 0;");
				insertion.push_back(std::format("\tint free_count = atomicAdd(b_Control.i[{0}], -1);", control_index + 2));
				insertion.push_back("\tif(free_count > 0) {");
					insertion.push_back(std::format("\t\titem_pos = b_Free{0}.i[free_count - 1];", entity.GetName()));
				insertion.push_back("\t} else {");
					insertion.push_back(std::format("\t\titem_pos = atomicAdd(b_Control.i[{0}], 1);", control_index + 1));
				insertion.push_back("\t}");
			} else {
				insertion.push_back(std::format("\tint item_pos = atomicAdd(b_Control.i[{0}], 1);", control_index + 1));
			}
			insertion.push_back(std::format("\titem.{0} = item_id;", entity.GetIDName()));
			insertion.push_back(std::format("\tSet{0}(item, item_pos);\n}}", entity.GetName()));
		}
	}

	insertion.push_back("////////////////");

	if (entity_processing && m_ReplaceMain) {
		insertion.push_back(std::format(
			"bool {0}Main(int item_index, {0} item, inout {0} new_item); // forward declaration\n"
			"void main() {{\n"
			"\tuvec3 global_id = gl_GlobalInvocationID;\n"
			"\tint item_index = int(global_id.x) + (int(global_id.y) + int(global_id.z) * 32) * 32 + uOffset;\n"
			"\tif (item_index >= uCount) return;\n"
			"\t{0} item = Get{0}(item_index);", m_Entity->GetName()));
		if (m_Entity->IsDoubleBuffering()) {
			insertion.push_back(std::format("\tif (item.{0} < 0) {{ Set{1}(item, item_index); return; }}", m_Entity->GetIDName(), m_Entity->GetName()));
		} else {
			insertion.push_back(std::format("\tif (item.{0} < 0) return;", m_Entity->GetIDName()));
		}
		insertion.push_back(std::format(
			"\t{0} new_item = item;\n"
			"\tbool should_destroy = {0}Main(item_index, item, new_item);\n"
			"\tif(should_destroy) {{ Destroy{0}(item_index); }} else {{ Set{0}(new_item, item_index); }}\n"
			"}}\n////////////////", m_Entity->GetName()));
	} else if (m_Entity && m_ReplaceMain && (m_RunType == RunType::ENTITY_RENDER)) {
		insertion.push_back(std::format(
			"#ifdef IS_VERTEX_SHADER\n"
			"void {0}Main(int item_index, {0} item); // forward declaration\n"
			"void main() {{\n"
			"\t{0} item = Get{0}(gl_InstanceID);", m_Entity->GetName()));
		insertion.push_back(std::format("\tif (item.{0} < 0) {{ gl_Position = vec4(0.0, 0.0, 100.0, 0.0); return; }}", m_Entity->GetIDName()));
		insertion.push_back(std::format(
			"\t{0}Main(gl_InstanceID, item);\n"
			"}}\n#endif\n////////////////", m_Entity->GetName()));
	} else if (m_Entity && m_ReplaceMain) {
		insertion.push_back(std::format(
			"void {0}Main(int item_index, {0} item); // forward declaration\n"
			"void main() {{\n"
			"\tuvec3 global_id = gl_GlobalInvocationID;\n"
			"\tint item_index = int(global_id.x) + (int(global_id.y) + int(global_id.z) * 32) * 32 + uOffset;\n"
			"\tif (item_index >= uCount) return;\n"
			"\t{0} item = Get{0}(item_index);", m_Entity->GetName()));
		insertion.push_back(std::format("\tif (item.{0} < 0) return;", m_Entity->GetIDName()));
		insertion.push_back(std::format(
			"\t{0}Main(item_index, item);\n"
			"}}\n////////////////", m_Entity->GetName()));
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
			insertion.push_back("//////////////// Data vector helpers");
		}
		for (const auto& data_vect : m_DataVectors) {
			int data_size = data_vect.p_NumIntsPerItem * data_vect.p_NumItems;
			int offset = control_size;
			control_size += data_size;
			values.resize(control_size);
			memcpy((unsigned char*)&(values[offset]), (unsigned char*)&(data_vect.p_Data[0]), sizeof(int) * data_size);
			insertion.push_back(GetDataVectorStructCode(data_vect, insertion_uniforms, integer_vars, offset));
		}

		control_ssbo->EnsureSizeBytes(control_size * sizeof(int));
		control_ssbo->Bind(0); // you can assume this is at index zero
		control_ssbo->SetValues(values);
	}

	if (!m_ExtraCode.empty()) {
		insertion.push_back("////////////////");
		insertion.push_back(m_ExtraCode);
	}

	insertion.push_front(JoinStrings(insertion_images, "\n"));
	insertion.push_front(JoinStrings(insertion_buffers, "\n"));
	insertion.push_front(JoinStrings(insertion_uniforms, "\n"));

	//DebugGPU::Checkpoint("PreRun", m_Entity);

	std::string insertion_str = JoinStrings(insertion, "\n");
	GLShader* prog = is_render ? Core::GetShader(m_ShaderName, insertion_str) : Core::GetComputeShader(m_ShaderName, insertion_str);
	prog->UseProgram();

	//DebugGPU::Checkpoint("PostRun", m_Entity);

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
		//DebugGPU::Checkpoint(std::format("{0} Death", m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT);
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
		if ((!m_Entities[e].p_Creatable) || (!m_Entities[e].p_Entity)) {
			continue;
		}
		GPUEntity& entity = *m_Entities[e].p_Entity;
		if (entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			entity.ProcessStableCreates(entity_controls[e].p_MaxIndex, entity_controls[e].p_NextId, entity_controls[e].p_FreeCount);
		} else {
			entity.ProcessMoveCreates(entity_controls[e].p_MaxIndex, entity_controls[e].p_NextId);
		}
	}

	if (entity_processing) {
		if ((m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) && (entity_deaths > 0)) {
			m_Entity->ProcessMoveDeaths(entity_deaths);
			if (false) {
				BufferViewer::Checkpoint(std::format("{} Death Control", m_Entity->GetName()), "PostRun", *control_ssbo, MemberSpec::Type::T_INT);
			}
		} else if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Entity->ProcessStableDeaths(entity_deaths);
		}

		//BufferViewer::Checkpoint(std::format("{0} Control", m_Entity.GetName()), "PostRun", *control_ssbo, MemberSpec::Type::T_INT);

		//BufferViewer::Checkpoint("PostRun", m_Entity);
		if (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			//BufferViewer::Checkpoint(std::format("{0} Free", m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT, m_Entity.GetFreeCount());
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
QueryEntities& QueryEntities::ByNearestPosition(std::string_view param_name, fVec2 pos) {
	m_Query = QueryType::Position2D;
	m_QueryParam = pos;
	m_ParamName = param_name;
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ByNearestPosition(std::string_view param_name, fVec3 pos) {
	m_Query = QueryType::Position3D;
	m_QueryParam = pos;
	m_ParamName = param_name;
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ById(int id) {
	m_Query = QueryType::ID;
	m_QueryParam = id;
	m_ParamName = std::string(m_Entity.GetIDName());
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
int QueryEntities::ExecuteQuery(void) {

	std::string main_name;
	if (m_Query == QueryType::Position2D) {
		main_name = "Nearest2D";
	} else if (m_Query == QueryType::Position3D) {
		main_name = "Nearest3D";
	} else if (m_Query == QueryType::ID) {
		main_name = "IsID";
	}

	auto main_func = std::format("void ItemMain{2}(int item_index, vec2 pos);\nvoid {0}Main(int item_index, {0} item) {{ ItemMain{2}(item_index, item.{1}); }}", m_Entity.GetName(), m_ParamName, main_name);

	int best_index = -1;
	int best_dist = std::numeric_limits<int>::max();
	EntityPipeline::IterateEntity(
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
Grid2DCache::Grid2DCache(GPUEntity& entity, std::string_view pos_name) :
	m_Entity		( entity )
	,m_PosName		( pos_name )
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

	std::string main_func = std::format("void ItemMain(int item_index, vec2 pos);\nvoid {0}Main(int item_index, {0} item) {{ ItemMain(item_index, item.{1}); }}", m_Entity.GetName(), m_PosName);

	EntityPipeline::IterateEntity(
		m_Entity
		,"GridCache2D"
		,true
		,{ "PHASE_INDEX" }
	)
	.AddCode(main_func)
	.AddBuffer("b_Index", m_GridIndices, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_WRITE)
	.Run([grid_size, grid_min, grid_max](GLShader* prog) {
		glUniform2i(prog->GetUniform("uGridSize"), grid_size.x, grid_size.y);
		glUniform2f(prog->GetUniform("uGridMin"), grid_min.x, grid_min.y);
		glUniform2f(prog->GetUniform("uGridMax"), grid_max.x, grid_max.y);
	});

	int alloc_count = 0;
	EntityPipeline::IterateEntity(
		m_Entity
		,"GridCache2D"
		,true
		,{ "PHASE_ALLOCATE" }
	)
	.AddCode(main_func)
	.AddBuffer("b_Index", m_GridIndices, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_WRITE)
	.AddInputOutputVar("AllocationCount", &alloc_count)
	.Run([grid_size, grid_min, grid_max](GLShader* prog) {
		glUniform2i(prog->GetUniform("uGridSize"), grid_size.x, grid_size.y);
		glUniform2f(prog->GetUniform("uGridMin"), grid_min.x, grid_min.y);
		glUniform2f(prog->GetUniform("uGridMax"), grid_max.x, grid_max.y);
	});

	//BufferViewer::Checkpoint(m_Entity.GetName() + "_Cache", "Gen", m_GridIndices, MemberSpec::Type::T_INT);

	EntityPipeline::IterateEntity(
		m_Entity
		,"GridCache2D"
		,true
		,{ "PHASE_FILL" }
	)
	.AddCode(main_func)
	.AddBuffer("b_Index", m_GridIndices, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_WRITE)
	.AddBuffer("b_Cache", m_GridItems, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_WRITE)
	.Run([grid_size, grid_min, grid_max](GLShader* prog) {
		glUniform2i(prog->GetUniform("uGridSize"), grid_size.x, grid_size.y);
		glUniform2f(prog->GetUniform("uGridMin"), grid_min.x, grid_min.y);
		glUniform2f(prog->GetUniform("uGridMax"), grid_max.x, grid_max.y);
	});

	//BufferViewer::Checkpoint(m_Entity.GetName() + "_Cache_Items", "Gen", m_GridItems, MemberSpec::Type::T_INT);
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::Bind(EntityPipeline& target_stage) {

	auto name = m_Entity.GetName();

	target_stage.AddBuffer(std::format("b_{0}GridIndices", name), m_GridIndices, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_ONLY);
	target_stage.AddBuffer(std::format("b_{0}GridItems", name), m_GridItems, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_ONLY);

	target_stage.AddCode(std::format(
			"ivec2 GetGridPos(vec2 pos, vec2 grid_min, vec2 grid_max, ivec2 grid_size); // forward declare \n"
			"ivec2 Get{0}IndexRangeAt(ivec2 grid_pos) {{\n"
			"\tint index = (grid_pos.x + grid_pos.y * {5}) * 3;\n"
			"\tint start = b_{0}GridIndices.i[index + 1];\n"
			"\tint count = b_{0}GridIndices.i[index + 2];\n"
			"\treturn ivec2(start, start + count);\n"
			"}}\n"
			"int Get{0}IndexAtCache(int index) {{\n"
			"\treturn b_{0}GridItems.i[index];\n"
			"}}\n"
			"ivec2 Get{0}GridPosAt(vec2 pos) {{\n"
			"\treturn GetGridPos(pos, vec2({1:.6f}, {2:.6f}), vec2({3:.6f}, {4:.6f}), ivec2({5}, {6}));\n" // TODO: should you be hardcoding in the grid params?
			"}}"
		,name
		,m_GridMin.x
		,m_GridMin.y
		,m_GridMax.x
		,m_GridMax.y
		,m_GridSize.x
		,m_GridSize.y
	));
}

} // namespace Neshny
#endif