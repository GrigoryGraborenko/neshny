////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void DebugGPU::ICheckpoint(QString name, QString stage, class GLSSBO& buffer, int count, const StructInfo* info, MemberSpec::Type type) {
	int item_size = MemberSpec::GetGPUTypeSizeBytes(type);
	count = count >= 0 ? count : buffer.GetSizeBytes() / item_size;

	std::shared_ptr<unsigned char[]> mem = nullptr;
	if (Core::IsBufferEnabled(name)) {
		mem = buffer.MakeCopy(count * item_size);
	}
	IStoreCheckpoint(name, { stage, "", count, Core::GetTicks(), false, mem }, info, type);
}

////////////////////////////////////////////////////////////////////////////////
void DebugGPU::ICheckpoint(QString stage, GPUEntity& buffer) {
	std::shared_ptr<unsigned char[]> mem = nullptr;
	if (Core::IsBufferEnabled(buffer.GetName())) {
		mem = buffer.MakeCopy();
	}
	IStoreCheckpoint(buffer.GetName(), { stage, buffer.GetDebugInfo(), buffer.GetMaxIndex(), Core::GetTicks(), buffer.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS, mem }, &buffer.GetSpecs(), MemberSpec::Type::T_UNKNOWN);
}

////////////////////////////////////////////////////////////////////////////////
void DebugGPU::IStoreCheckpoint(QString name, CheckpointData data, const StructInfo* info, MemberSpec::Type type) {
	auto existing = m_Frames.find(name);
	if (existing == m_Frames.end()) {
		existing = m_Frames.insert_or_assign(name, info ? CheckpointList{ info->p_Members } : CheckpointList{ { MemberSpec{ "", type, MemberSpec::GetGPUTypeSizeBytes(type) } } }).first;
		existing->second.p_StructSize = 0;
		for (const auto& member : existing->second.p_Members) {
			existing->second.p_StructSize += member.p_Size;
		}
	}
	existing->second.p_Frames.push_front(data);
	while (existing->second.p_Frames.size() > m_MaxFrames) {
		existing->second.p_Frames.pop_back();
	}
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<GLSSBO> DebugGPU::IGetStoredFrameAt(QString name, int tick, int& count) {

	auto existing = m_Frames.find(name);
	if (existing == m_Frames.end()) {
		return nullptr;
	}
	for (auto& frame : existing->second.p_Frames) {
		if (frame.p_Tick == tick) {
			count = frame.p_Count;
			return std::make_shared<GLSSBO>(existing->second.p_StructSize * frame.p_Count, frame.p_Data.get()); // TODO: cache this if it's not performant
		}
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
void DebugGPU::RenderImGui(InterfaceBufferViewer& data) {

	if (!data.p_Visible) {
		return;
	}

	int curr_tick = Core::GetTicks();
	int min_stored_tick = curr_tick;
	for (const auto& buffer: m_Frames) {
		for (const auto& frame : buffer.second.p_Frames) {
			if (frame.p_Data) {
				min_stored_tick = std::min(frame.p_Tick, min_stored_tick);
			}
		}
	}

	ImGui::Begin("DebugGPUWindow", &data.p_Visible, ImGuiWindowFlags_NoCollapse);

	ImGui::Checkbox("All", &data.p_AllEnabled);
	ImGui::SameLine();

	int max_rewind = curr_tick - min_stored_tick;
	ImGui::SliderInt("Rewind", &data.p_TimeSlider, 0, max_rewind);
	ImGui::SameLine();
	ImGui::Text("%i", max_rewind);

	ImVec2 space_available = ImGui::GetWindowContentRegionMax();

	const int size_banner = 50;
	ImGui::SetCursorPos(ImVec2(2, size_banner));
	ImGui::BeginChild("BufferList", ImVec2(space_available.x - 4, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(255, 0, 0, 200));

	ImGuiTableFlags table_flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
	const int MAX_COLS = 50;
	for (auto& buffer : m_Frames) {
		const auto& frames = buffer.second.p_Frames;
		auto header = (buffer.first + (frames.empty() ? "" : frames.begin()->p_Info) + "###" + buffer.first).toLocal8Bit();

		InterfaceCollapsible* found = nullptr;
		for (auto& item: data.p_Items) {
			if (item.p_Name == buffer.first) {
				found = &item;
				break;
			}
		}
		if (!found) {
			data.p_Items.push_back({ buffer.first, false, false });
			found = &(data.p_Items.back());
		}

		auto check_header = ("###" + buffer.first).toLocal8Bit();

		if (data.p_AllEnabled) {
			ImGui::BeginDisabled();
		}
		ImGui::Checkbox(check_header, &found->p_Enabled);
		if (data.p_AllEnabled) {
			ImGui::EndDisabled();
		}
		ImGui::SameLine();

		ImGui::SetNextItemOpen(found->p_Open);
		if (found->p_Open = ImGui::CollapsingHeader(header.data())) {

			int max_frames = std::min((int)frames.size(), MAX_COLS);
			int max_count = 0;
			for (auto frame : frames) {
				max_count = std::max(max_count, frame.p_Count);
			}

			if (ImGui::BeginTable("##Table", max_frames + 1, table_flags)) {
				ImGui::TableSetupScrollFreeze(1, 1); // Make top row + left col always visible

				std::vector<std::pair<QString, MemberSpec::Type>> struct_types;
				for (const auto& member : buffer.second.p_Members) {
					if (member.p_Type == MemberSpec::T_VEC2) {
						struct_types.push_back({ member.p_Name + ".x", MemberSpec::T_FLOAT }); struct_types.push_back({ member.p_Name + ".y", MemberSpec::T_FLOAT });
					} else if (member.p_Type == MemberSpec::T_VEC3) {
						struct_types.push_back({ member.p_Name + ".x", MemberSpec::T_FLOAT }); struct_types.push_back({ member.p_Name + ".y", MemberSpec::T_FLOAT }); struct_types.push_back({ member.p_Name + ".z", MemberSpec::T_FLOAT });
					} else if (member.p_Type == MemberSpec::T_VEC4) {
						struct_types.push_back({ member.p_Name + ".x", MemberSpec::T_FLOAT }); struct_types.push_back({ member.p_Name + ".y", MemberSpec::T_FLOAT }); struct_types.push_back({ member.p_Name + ".z", MemberSpec::T_FLOAT }); struct_types.push_back({ member.p_Name + ".w", MemberSpec::T_FLOAT });
					} else {
						struct_types.push_back({ member.p_Name, member.p_Type });
					}
				}
				int cycles = (int)struct_types.size();

				ImGui::TableSetupColumn("-", ImGuiTableColumnFlags_WidthFixed, struct_types.empty() ? 50 : 100);
				for (int column = 0; column < max_frames; column++) {
					auto head_name = QString("%1 %2").arg(frames[column].p_Stage).arg(column).toLocal8Bit();
					ImGui::TableSetupColumn(head_name, ImGuiTableColumnFlags_WidthFixed, 100);
				}

				ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
				for (int column = 0; column <= max_frames; column++) {
					ImGui::TableSetColumnIndex(column);
					const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
					ImGui::PushID(column);
					ImGui::TableHeader(column_name);
					if ((column > 0) && ImGui::IsItemHovered()) {
						auto info = frames[column - 1].p_Info.toLocal8Bit();
						if (info.length() > 0) {
							ImGui::BeginTooltip();
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							ImGui::TextUnformatted(info.data());
							ImGui::PopTextWrapPos();
							ImGui::EndTooltip();
						}
					}
					ImGui::PopID();
				}

				ImGuiListClipper clipper;
				clipper.Begin(max_count * cycles);
				while (clipper.Step()) {
					for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

						MemberSpec::Type cast_type = MemberSpec::T_INT;
						int real_count = cycles ? row / cycles : row;
						int struct_ind = cycles ? row % cycles : 0;
						if (!struct_types.empty()) {
							const auto& spec = struct_types[struct_ind];
							auto info = QString("%1%2").arg(struct_ind ? "" : QString("[%1] ").arg(real_count)).arg(spec.first).toLocal8Bit();
							ImGui::Text(info.data());
							cast_type = spec.second;
							ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(real_count % 2 ? 220 : 180, 0, 0, 200));
						} else {
							ImGui::Text("%i", row);
						}

						for (int column = 0; column < max_frames; column++) {
							auto& col = frames[column];
							if ((real_count >= col.p_Count) || (!col.p_Data)) {
								continue;
							}
							ImGui::TableSetColumnIndex(column + 1);
							unsigned char* rawptr = &(col.p_Data[0]);
							if ((struct_ind == 0) && (cycles >= 2) && col.p_UsingFreeList) {
								int id = ((int*)rawptr)[row];
								if (id == -1) {
									ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(255, 0, 128, 200));
									ImGui::Text("DELETED");
									continue;
								}
							}
							if (cast_type == MemberSpec::T_FLOAT) {
								auto txt = QString("%1").arg(((float*)rawptr)[row], 0, 'f', 4).toLocal8Bit();
								ImGui::Text(txt.data());
							} else if (cast_type == MemberSpec::T_UINT) {
								ImGui::Text("%i", ((unsigned int*)rawptr)[row]);
							} else {
								ImGui::Text("%i", ((int*)rawptr)[row]);
							}
						}
					}
				}
				ImGui::EndTable();
			}
		}
	}
	ImGui::PopStyleColor();
	ImGui::EndChild();

	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
void ShaderViewEditor::RenderImGui(InterfaceShaderViewer& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("Shader Viewer", &data.p_Visible, ImGuiWindowFlags_NoCollapse);

	auto shaders = Core::Singleton().GetShaders();
	auto compute_shaders = Core::Singleton().GetComputeShaders();

	for (auto& shader : compute_shaders) {
		RenderShader(data, shader.first, shader.second, true);
	}
	for (auto& shader : shaders) {
		RenderShader(data, shader.first, shader.second, false);
	}

	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
void ShaderViewEditor::RenderShader(InterfaceShaderViewer& data, QString name, GLShader* shader, bool is_compute) {

	if (shader->IsValid()) {
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.15f, 0.3f, 1.0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.2f, 0.4f, 1.0));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.25f, 0.5f, 1.0));
	} else {
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.0f, 0.0f, 1.0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.8f, 0.0f, 0.0f, 1.0));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.0f, 0.0f, 1.0));
	}

	InterfaceCollapsible* found = nullptr;
	for (auto& item : data.p_Items) {
		if (item.p_Name == name) {
			found = &item;
			break;
		}
	}
	if (!found) {
		data.p_Items.push_back({ name, false, false });
		found = &(data.p_Items.back());
	}

	ImGui::SetNextItemOpen(found->p_Open);

	if (found->p_Open = ImGui::CollapsingHeader(name.toLocal8Bit().data())) {
		auto& sources = shader->GetSources();
		for (auto& src : sources) {
			ImGui::TextUnformatted(src.m_Type.toLocal8Bit().data());

			QStringList lines = src.m_Source.split('\n');

			if (!src.m_Error.isNull()) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), src.m_Error.toLocal8Bit().data());
			}

			for(int line = 0; line < lines.size(); line++) {
				ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), QString("%1").arg(line).toLocal8Bit().data());
				ImGui::SameLine();
				ImGui::SetCursorPosX(50);
				ImGui::TextUnformatted(lines[line].toLocal8Bit().data());
			}

			/*
			auto txt = src.m_Source.toLocal8Bit();
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
			//flags |= ImGuiInputTextFlags_ReadOnly;
			ImGui::InputTextMultiline("##source", txt.data(), txt.size(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * lines.size()), flags);
			*/

			//ImGui::TextUnformatted(src.m_Source.toLocal8Bit().data());
		}
	}
	ImGui::PopStyleColor(3);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
PipelineStage::PipelineStage(GPUEntity& entity, QString shader_name, bool replace_main, const std::vector<QString>& shader_defines, GLSSBO* control_buffer, GLSSBO* destruction_buffer) :
	CommonPipeline		( entity, shader_name, replace_main, shader_defines )
	,m_ControlBuffer	( control_buffer )
	,m_DestroyBuffer	( destruction_buffer )
{
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddEntity(GPUEntity& entity) {
	m_Entities.push_back({ entity, false });
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
PipelineStage& PipelineStage::AddCreatableEntity(GPUEntity& entity) {
	m_Entities.push_back({ entity, true });
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
	if (m_ControlBuffer) {
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
	}

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

	if (m_ControlBuffer) {
		if (m_DestroyBuffer && (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT)) {
			int buffer_index = insertion_buffers.size();
			m_DestroyBuffer->EnsureSize(m_Entity.GetMaxIndex() * sizeof(int));
			m_DestroyBuffer->Bind(buffer_index);
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

		if (m_DestroyBuffer && m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) {
			insertion += QString("\tint death_index = atomicAdd(b_Control.i[%1], 1);").arg((int)var_vals.size());
			insertion += QString("\tb_Death.i[death_index] = index;");
			var_vals.push_back({ 0, &entity_deaths });
		} else if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			insertion += "\tatomicAdd(b_Control.i[0], 1);"; // assume b_Control has 0 and 1 reserved for death
			insertion += "\tint free_index = atomicAdd(b_Control.i[1], 1);";
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
		if (m_Entities[e].p_Creatable && m_ControlBuffer) {
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

	if (m_ControlBuffer && (!var_vals.empty())) {

		m_ControlBuffer->EnsureSize((int)var_vals.size() * sizeof(int));
		m_ControlBuffer->Bind(0); // you can assume this is at index zero

		std::vector<int> values;
		for (auto var: var_vals) {
			values.push_back(var.first);
		}
		m_ControlBuffer->SetValues(values);
	}

	//DebugGPU::Checkpoint("PreRun", m_Entity);

	QString insertion_str = QString(insertion.join("\n")).arg(m_Entity.GetName());
	GLShader* prog = Core::Singleton().GetComputeShader(m_ShaderName, insertion_str);
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
	Core::DispatchMultiple(prog, m_Entity.GetMaxIndex());
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	////////////////////////////////////////////////////

	if (m_DestroyBuffer) {
		//DebugGPU::Checkpoint(QString("%1 Death").arg(m_Entity.GetName()), "PostRun", *m_DestroyBuffer, MemberSpec::Type::T_INT);
	}

	if (m_ControlBuffer) {

		std::vector<int> control_values;
		m_ControlBuffer->GetValues<int>(control_values, (int)var_vals.size());
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
		if ((m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT) && m_DestroyBuffer && (entity_deaths > 0)) {
			m_Entity.ProcessMoveDeaths(entity_deaths, *m_DestroyBuffer, *m_ControlBuffer);
			if (true) {
				DebugGPU::Checkpoint(QString("%1 Death Control").arg(m_Entity.GetName()), "PostRun", *m_ControlBuffer, MemberSpec::Type::T_INT);
			}
		} else if (m_Entity.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			m_Entity.ProcessStableDeaths(entity_deaths);
		}
	}

	if (m_ControlBuffer) {
		//DebugGPU::Checkpoint(QString("%1 Control").arg(m_Entity.GetName()), "PostRun", *m_ControlBuffer, MemberSpec::Type::T_INT);
	}
	if (m_Entity.GetStoreMode() == GPUEntity::StoreMode::SSBO) {
		DebugGPU::Checkpoint("PostRun", m_Entity);
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
	int time_slider = Core::GetTimeSlider();

	int num_entities = m_Entity.GetMaxIndex();
	std::shared_ptr<GLSSBO> replace = nullptr;

	if (time_slider > 0) {
		replace = DebugGPU::GetStoredFrameAt(m_Entity.GetName(), Core::GetTicks() - time_slider, num_entities);
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

	QString insertion_str = QString(insertion.join("\n")).arg(m_Entity.GetName());
	GLShader* prog = Core::Singleton().GetShader(m_ShaderName, insertion_str);
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
