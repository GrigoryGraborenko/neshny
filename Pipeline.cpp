////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
CommonPipeline::CommonPipeline(GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines) :
	m_Entity			( entity )
	,m_ShaderName		( shader_name )
	,m_ReplaceMain		( replace_main )
	,m_ShaderDefines	( shader_defines )
{
}

////////////////////////////////////////////////////////////////////////////////
QString CommonPipeline::GetUniformVectorStructCode(AddedUniformVector& uniform, QStringList& insertion_uniforms, std::vector<std::pair<QString, int>>& integer_vars, std::vector<std::pair<QString, std::vector<float>*>>& vector_vars) {

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
PipelineStage::PipelineStage(GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines) :
	CommonPipeline		( entity, shader_name, replace_main, shader_defines )
{
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ entity, false });
	if (cache) {
		cache->Bind(*this);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddCreatableEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ entity, true });
	if (cache) {
		cache->Bind(*this);
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
void PipelineStage::Run(std::optional<std::function<void(GLShader* program)>> pre_execute) {

	// TODO: investigate GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS
	// TODO: debug of SSBOs is optional

	QStringList insertion;
	int entity_deaths = 0;
	int entity_free_count = m_Entity.GetFreeCount(); // only used for DeleteMode::STABLE_WITH_GAPS

	QStringList insertion_images;
	QStringList insertion_buffers;
	QStringList insertion_uniforms;
	std::vector<std::pair<QString, int>> integer_vars;
	std::vector<std::pair<QString, std::vector<float>*>> vector_vars;
	std::vector<std::pair<GLSSBO*, int>> ssbo_binds;
	std::vector<std::pair<int, int*>> var_vals;

	insertion_uniforms += "uniform int uCount;";
	insertion_uniforms += "uniform int uOffset;";
	for (auto str : m_ShaderDefines) {
		insertion += QString("#define %1").arg(str);
	}
	insertion += QString("#define BUFFER_TEX_SIZE %1").arg(BUFFER_TEX_SIZE);
	if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
		var_vals.push_back({ 0, nullptr });
	} else if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
		var_vals.push_back({ 0, &entity_deaths });
		var_vals.push_back({ entity_free_count, &entity_free_count });
	}
	for (const auto& var : m_Vars) {
		insertion += QString("#define %1 (b_Control.i[%2])").arg(var.p_Name).arg((int)var_vals.size());
		var_vals.push_back({ var.p_Ptr ? *var.p_Ptr : 0, var.p_Ptr });
	}
	insertion_buffers += "layout(std430, binding = 0) buffer ControlBuffer { int i[]; } b_Control;";

	// inserts main entity code
	if (m_Entity.GetStoreMode() == GPUEntity::StoreMode::TEXTURE) {
		integer_vars.push_back({ QString("i_%1Tex").arg(m_Entity.GetName()), insertion_images.size() });
		glBindImageTexture(insertion_images.size(), m_Entity.GetTex(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		insertion_images += QString("layout(binding = 0, r32f) uniform image2D i_%2Tex;").arg(m_Entity.GetName());
	} else {
		int buffer_index = insertion_buffers.size();
		ssbo_binds.push_back({ m_Entity.GetSSBO(), buffer_index });
		insertion_buffers += QString("layout(std430, binding = %1) buffer MainEntityBuffer { float i[]; } b_%2;").arg(buffer_index).arg(m_Entity.GetName());
	}

	insertion += m_Entity.GetGPUInsertion();
	insertion += m_Entity.GetSpecs().p_GPUInsertion;

	if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
		int buffer_index = insertion_buffers.size();
		m_Entity.GetFreeListSSBO()->EnsureSize(m_Entity.GetMaxIndex() * sizeof(int));
		m_Entity.GetFreeListSSBO()->Bind(buffer_index);
		insertion_buffers += QString("layout(std430, binding = %1) writeonly buffer DeathBuffer { int i[]; } b_Death;").arg(buffer_index);
	} else if(m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
		int buffer_index = insertion_buffers.size();
		m_Entity.GetFreeListSSBO()->Bind(buffer_index);
		insertion_buffers += QString("layout(std430, binding = %1) writeonly buffer FreeBuffer { int i[]; } b_FreeList;").arg(buffer_index);
	}
	insertion += QString("void Destroy%1(int index) {").arg(m_Entity.GetName());

	if (m_Entity.GetStoreMode() == GPUEntity::StoreMode::TEXTURE) {
		insertion += QString("\tint y = int(floor(float(index) / float(BUFFER_TEX_SIZE)));\n\tivec2 base = ivec2(index - y * BUFFER_TEX_SIZE, y * FLOATS_PER_%1);").arg(m_Entity.GetName());
	} else {
		insertion += QString("\tint base = index * FLOATS_PER_%1;").arg(m_Entity.GetName());
	}
	insertion += QString("\t%1_SET(base, 0, intBitsToFloat(-1));").arg(m_Entity.GetName());

	if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
		insertion += QString("\tint death_index = atomicAdd(b_Control.i[%1], 1);").arg((int)var_vals.size());
		insertion += QString("\tb_Death.i[death_index] = index;");
		var_vals.push_back({ 0, &entity_deaths });
	} else if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
		insertion += "\tatomicAdd(b_Control.i[0], 1);"; // assume b_Control has 0 and 1 reserved for death
		insertion += "\tint free_index = atomicAdd(b_Control.i[1], 1);";
		insertion += "\tb_FreeList.i[free_index] = index;";
	}
	insertion += "}";

	for (int b = 0; b < m_SSBOs.size(); b++) {
		int buffer_index = insertion_buffers.size();
		auto& ssbo = m_SSBOs[b];
		ssbo_binds.push_back({ &ssbo.p_Buffer, buffer_index });
		insertion_buffers += QString("layout(std430, binding = %1) %4buffer GenericBuffer%1 { %2 i[]; } %3;").arg(buffer_index).arg(MemberSpec::GetGPUType(ssbo.p_Type)).arg(ssbo.p_Name).arg(ssbo.p_ReadOnly ? "readonly " : "");
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
		if (m_Entity.GetStoreMode() == GPUEntity::StoreMode::TEXTURE) {
			integer_vars.push_back({ QString("i_%1Tex").arg(name), insertion_images.size() });
			glBindImageTexture(insertion_images.size(), entity.GetTex(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
			insertion_images += QString("layout(binding = %1, r32f) uniform image2D i_%2Tex;").arg(e + 1).arg(name);
		} else {
			int buffer_index = insertion_buffers.size();
			ssbo_binds.push_back({ entity.GetSSBO(), buffer_index });
			insertion_buffers += QString("layout(std430, binding = %1) buffer EntityBuffer%1 { float i[]; } b_%2;").arg(buffer_index).arg(entity.GetName());
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
	if (m_ReplaceMain) {
		insertion +=
			"bool %1Main(int item_index, %1 item, inout %1 new_item); // forward declaration\n"
			"void main() {\n"
			"\tuvec3 global_id = gl_GlobalInvocationID;\n"
			"\tint item_index = int(global_id.x) + (int(global_id.y) + int(global_id.z) * 32) * 32 + uOffset;\n"
			"\tif (item_index >= uCount) return;\n"
			"\t%1 item = Get%1(item_index);";
		insertion += QString("\tif (item.%1 < 0) return;").arg(m_Entity.GetIDName());
		insertion +=
			"\t%1 new_item = item;\n"
			"\tbool should_destroy = %1Main(item_index, item, new_item);\n"
			"\tif(should_destroy) { Destroy%1(item_index); } else { Set%1(new_item, item_index); }\n"
			"}\n////////////////";
	}

	if (!var_vals.empty()) {

		m_Entity.GetControlSSBO()->EnsureSize((int)var_vals.size() * sizeof(int));
		m_Entity.GetControlSSBO()->Bind(0); // you can assume this is at index zero

		std::vector<int> values;
		for (auto var: var_vals) {
			values.push_back(var.first);
		}
		m_Entity.GetControlSSBO()->SetValues(values);
	}

	if (!m_ExtraCode.isNull()) {
		insertion += "////////////////";
		insertion += m_ExtraCode;
	}

	//DebugGPU::Checkpoint("PreRun", m_Entity);

	QString insertion_str = QString(insertion.join("\n")).arg(m_Entity.GetName());
	GLShader* prog = Neshny::GetComputeShader(m_ShaderName, insertion_str);
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

	if (pre_execute.has_value()) {
		pre_execute.value()(prog);
	}

	////////////////////////////////////////////////////
	Neshny::DispatchMultiple(prog, m_Entity.GetMaxIndex());
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	////////////////////////////////////////////////////

	if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
		//DebugGPU::Checkpoint(QString("%1 Death").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT);
	}

	std::vector<int> control_values;
	m_Entity.GetControlSSBO()->GetValues<int>(control_values, (int)var_vals.size());
	for(int v = 0; v < var_vals.size(); v++) {
		if (var_vals[v].second) {
			*var_vals[v].second = control_values[v];
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
	if ((m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) && (entity_deaths > 0)) {
		m_Entity.ProcessMoveDeaths(entity_deaths);
		if (true) {
			BufferViewer::Checkpoint(QString("%1 Death Control").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetControlSSBO(), MemberSpec::Type::T_INT);
		}
	} else if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
		m_Entity.ProcessStableDeaths(entity_deaths);
	}

	//DebugGPU::Checkpoint(QString("%1 Control").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetControlSSBO(), MemberSpec::Type::T_INT);

	if (m_Entity.GetStoreMode() == GPUEntity::StoreMode::SSBO) {
		//DebugGPU::Checkpoint("PostRun", m_Entity);
	}
	if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
		//DebugGPU::Checkpoint(QString("%1 Free").arg(m_Entity.GetName()), "PostRun", *m_Entity.GetFreeListSSBO(), MemberSpec::Type::T_INT, m_Entity.GetFreeCount());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
EntityRender::EntityRender(GPUEntity& entity, QString shader_name, const std::vector<QString>& shader_defines) :
	CommonPipeline		( entity, shader_name, false, shader_defines )
{
}

////////////////////////////////////////////////////////////////////////////////
EntityRender& EntityRender::AddSSBO(QString name, GLSSBO& ssbo, MemberSpec::Type array_type) {
	m_SSBOs.push_back({ ssbo, name, array_type });
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
void EntityRender::Render(GLBuffer* buffer, std::optional<std::function<void(GLShader* program)>> pre_execute) {

	QStringList insertion;
	QStringList insertion_images;
	QStringList insertion_buffers;
	QStringList insertion_uniforms;
	std::vector<std::pair<QString, int>> integer_vars;
	std::vector<std::pair<GLSSBO*, int>> ssbo_binds;
	std::vector<std::pair<QString, std::vector<float>*>> vector_vars;
	int time_slider = Neshny::GetInterfaceData().p_BufferView.p_TimeSlider;

	int num_entities = m_Entity.GetMaxIndex();
	std::shared_ptr<GLSSBO> replace = nullptr;

	if (time_slider > 0) {
		replace = BufferViewer::GetStoredFrameAt(m_Entity.GetName(), Neshny::GetTicks() - time_slider, num_entities);
	}

	for (auto str : m_ShaderDefines) {
		insertion += QString("#define %1").arg(str);
	}
	insertion += QString("#define BUFFER_TEX_SIZE %1").arg(BUFFER_TEX_SIZE);

	// inserts main entity code
	if (m_Entity.GetStoreMode() == GPUEntity::StoreMode::TEXTURE) {
		integer_vars.push_back({ QString("i_%1Tex").arg(m_Entity.GetName()), insertion_images.size() });
		glBindImageTexture(insertion_images.size(), m_Entity.GetTex(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		insertion_images += QString("layout(binding = 0, r32f) uniform image2D i_%2Tex;").arg(m_Entity.GetName());
	} else {
		int buffer_index = insertion_buffers.size();
		ssbo_binds.push_back({ replace.get() ? replace.get() : m_Entity.GetSSBO(), buffer_index });
		insertion_buffers += QString("layout(std430, binding = %1) buffer MainEntityBuffer { float i[]; } b_%2;").arg(buffer_index).arg(m_Entity.GetName());
	}
	insertion_uniforms += QString("uniform int u%1Count;").arg(m_Entity.GetName());
	integer_vars.push_back({ QString("u%1Count").arg(m_Entity.GetName()), num_entities });
	insertion += m_Entity.GetGPUInsertion();
	insertion += m_Entity.GetSpecs().p_GPUReadOnlyInsertion;

	for (int b = 0; b < m_SSBOs.size(); b++) {
		auto& ssbo = m_SSBOs[b];
		int buffer_index = insertion_buffers.size();
		ssbo_binds.push_back({ &ssbo.p_Buffer, buffer_index });
		insertion_buffers += QString("layout(std430, binding = %1) readonly buffer GenericBuffer%1 { %2 i[]; } %3;").arg(buffer_index).arg(MemberSpec::GetGPUType(ssbo.p_Type)).arg(ssbo.p_Name);
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

	if (!m_ExtraCode.isNull()) {
		insertion += "////////////////";
		insertion += m_ExtraCode;
	}

	QString insertion_str = QString(insertion.join("\n")).arg(m_Entity.GetName());
	GLShader* prog = Neshny::GetShader(m_ShaderName, insertion_str);
	prog->UseProgram();
	buffer->UseBuffer(prog);

	for (const auto& var: integer_vars) {
		glUniform1i(prog->GetUniform(var.first), var.second);
	}
	for (const auto& var : vector_vars) {
		glUniform1fv(prog->GetUniform(var.first), (int)var.second->size(), &((*var.second)[0]));
	}
	for (auto& ssbo : ssbo_binds) {
		ssbo.first->Bind(ssbo.second);
	}

	if (pre_execute.has_value()) {
		pre_execute.value()(prog);
	}

	////////////////////////////////////////////////////
	buffer->DrawInstanced(num_entities);
	////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////
Grid2DCache::Grid2DCache(GPUEntity& entity, QString pos_name) :
	m_Entity	( entity )
	,m_PosName	( pos_name )
{
	int brk = 0;
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::GenerateCache(IVec2 grid_size, Vec2 grid_min, Vec2 grid_max) {

	m_GridSize = grid_size;
	m_GridMin = grid_min;
	m_GridMax = grid_max;

	Vec2 range = grid_max - grid_min;
	Vec2 cell_size(range.x / grid_size.x, range.y / grid_size.y);
	m_GridIndices.EnsureSize(grid_size.x * grid_size.y * 3 * sizeof(int));
	m_GridItems.EnsureSize(m_Entity.GetCount() * sizeof(int));

	QString main_func =
		QString("void ItemMain(int item_index, vec2 pos);\nbool %1Main(int item_index, %1 item, inout %1 new_item) { ItemMain(item_index, item.%2); return false; }")
		.arg(m_Entity.GetName()).arg(m_PosName);

	PipelineStage(
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
	PipelineStage(
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

	PipelineStage(
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
void Grid2DCache::Bind(PipelineStage& stage) {

	auto name = m_Entity.GetName();
	stage.AddSSBO(QString("b_%1GridIndices").arg(name), m_GridIndices, MemberSpec::Type::T_INT);
	stage.AddSSBO(QString("b_%1GridItems").arg(name), m_GridItems, MemberSpec::Type::T_INT);

	stage.AddCode(
		QString(
			"ivec2 GetGridPos(vec2 pos, vec2 grid_min, vec2 grid_max, ivec2 grid_size); // forward declare \n"
			"ivec2 Get%1IndexRangeAt(ivec2 grid_pos) {\n"
			"\tint index = (grid_pos.x + grid_pos.y * %6) * 3;\n"
			"\tint start = b_%1GridIndices.i[index + 1];\n"
			"\tint count = b_%1GridIndices.i[index + 2];\n"
			"\treturn ivec2(start, start + count);\n"
			"}\n"
			"%1 Get%1AtCache(int index) {\n"
			"\treturn Get%1(b_%1GridItems.i[index]);"
			"}\n"
			"ivec2 Get%1GridPosAt(vec2 pos) {\n"
			"\treturn GetGridPos(pos, vec2(%2, %3), vec2(%4, %5), ivec2(%6, %7));\n"
			"}"
		)
		.arg(name)
		.arg(m_GridMin.x, 0, 'f', 6)
		.arg(m_GridMin.y, 0, 'f', 6)
		.arg(m_GridMax.x, 0, 'f', 6)
		.arg(m_GridMax.y, 0, 'f', 6)
		.arg(m_GridSize.x)
		.arg(m_GridSize.y)
	);
}