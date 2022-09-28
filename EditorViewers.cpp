////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void BaseDebugRender::IRender3DDebug(const QMatrix4x4& view_perspective, int width, int height, Triple offset, double scale) {

	Core& core = Core::Singleton();

	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	GLShader* debug_prog = core.GetShader("Debug");
	debug_prog->UseProgram();
	GLBuffer* line_buffer = core.GetBuffer("DebugLine");
	line_buffer->UseBuffer(debug_prog);

	glUniformMatrix4fv(debug_prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, view_perspective.data());

	for (auto it = m_Lines.begin(); it != m_Lines.end(); it++) {
		if (it->p_OnTop) {
			glDisable(GL_DEPTH_TEST);
		} else {
			glEnable(GL_DEPTH_TEST);
		}
		Triple da = (it->p_A - offset) * scale;
		Triple db = (it->p_B - offset) * scale;
		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x(), it->p_Col.y(), it->p_Col.z(), it->p_Col.w());
		glUniform3f(debug_prog->GetUniform("uPosA"), da.x, da.y, da.z);
		glUniform3f(debug_prog->GetUniform("uPosB"), db.x, db.y, db.z);
		line_buffer->Draw();
	}

	for (auto it = m_Points.begin(); it != m_Points.end(); it++) {

		if (it->p_OnTop) {
			glDisable(GL_DEPTH_TEST);
		} else {
			glEnable(GL_DEPTH_TEST);
		}
		Triple dpos = (it->p_Pos - offset) * scale;

		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x(), it->p_Col.y(), it->p_Col.z(), it->p_Col.w());
		glUniform3f(debug_prog->GetUniform("uPosA"), dpos.x, dpos.y, dpos.z);
		for (int i = 0; i < 2; i++) {
			Triple off = (it->p_Pos - offset + Triple(Random() - 0.5, Random() - 0.5, Random() - 0.5) * DEBUG_POINT_SIZE) * scale;
			glUniform3f(debug_prog->GetUniform("uPosB"), off.x, off.y, off.z);
			line_buffer->Draw();
		}
		if (it->p_Str.size() <= 0) {
			continue;
		}
		QVector3D result = view_perspective * QVector3D(dpos.x, dpos.y, dpos.z);
		if (result.z() > 1) {
			continue;
		}
		int x = (int)floor((result.x() + 1.0) * 0.5 * width);
		int y = (int)floor((1.0 - result.y()) * 0.5 * height);
		ImGui::SetCursorPos(ImVec2(x, y));
		ImGui::Text(it->p_Str.c_str());
	}

	debug_prog = core.GetShader("DebugTriangle");
	debug_prog->UseProgram();
	GLBuffer* buffer = core.GetBuffer("DebugTriangle");
	buffer->UseBuffer(debug_prog);

	glUniformMatrix4fv(debug_prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, view_perspective.data());

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	for (auto it = m_Triangles.begin(); it != m_Triangles.end(); it++) {

		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x(), it->p_Col.y(), it->p_Col.z(), it->p_Col.w());

		Triple da = (it->p_A - offset) * scale;
		Triple db = (it->p_B - offset) * scale;
		Triple dc = (it->p_C - offset) * scale;

		glUniform3f(debug_prog->GetUniform("uPosA"), da.x, da.y, da.z);
		glUniform3f(debug_prog->GetUniform("uPosB"), db.x, db.y, db.z);
		glUniform3f(debug_prog->GetUniform("uPosC"), dc.x, dc.y, dc.z);
		buffer->Draw();
	}

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

////////////////////////////////////////////////////////////////////////////////
void BufferViewer::ICheckpoint(QString name, QString stage, class GLSSBO& buffer, int count, const StructInfo* info, MemberSpec::Type type) {
	int item_size = MemberSpec::GetGPUTypeSizeBytes(type);
	count = count >= 0 ? count : buffer.GetSizeBytes() / item_size;

	std::shared_ptr<unsigned char[]> mem = nullptr;
	if (Core::IsBufferEnabled(name)) {
		mem = buffer.MakeCopy(count * item_size);
	}
	IStoreCheckpoint(name, { stage, "", count, Core::GetTicks(), false, mem }, info, type);
}

////////////////////////////////////////////////////////////////////////////////
void BufferViewer::ICheckpoint(QString stage, GPUEntity& buffer) {
	std::shared_ptr<unsigned char[]> mem = nullptr;
	if (Core::IsBufferEnabled(buffer.GetName())) {
		mem = buffer.MakeCopy();
	}
	IStoreCheckpoint(buffer.GetName(), { stage, buffer.GetDebugInfo(), buffer.GetMaxIndex(), Core::GetTicks(), buffer.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS, mem }, &buffer.GetSpecs(), MemberSpec::Type::T_UNKNOWN);
}

////////////////////////////////////////////////////////////////////////////////
void BufferViewer::IStoreCheckpoint(QString name, CheckpointData data, const StructInfo* info, MemberSpec::Type type) {
	auto existing = m_Frames.find(name);
	if (existing == m_Frames.end()) {
		existing = m_Frames.insert_or_assign(name, info ? CheckpointList{ info->p_Members } : CheckpointList{ { MemberSpec{ "", type, MemberSpec::GetGPUTypeSizeBytes(type) } } }).first;
		existing->second.p_StructSize = 0;
		for (const auto& member : existing->second.p_Members) {
			existing->second.p_StructSize += member.p_Size;
		}
	}
	int max_frames = Core::GetInterfaceData().p_BufferView.p_MaxFrames;
	existing->second.p_Frames.push_front(data);
	while (existing->second.p_Frames.size() > max_frames) {
		existing->second.p_Frames.pop_back();
	}
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<GLSSBO> BufferViewer::IGetStoredFrameAt(QString name, int tick, int& count) {

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
void BufferViewer::RenderImGui(InterfaceBufferViewer& data) {

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
	ImVec2 space_available = ImGui::GetWindowContentRegionMax();

	ImGui::Checkbox("All", &data.p_AllEnabled);
	ImGui::SameLine();

	int max_rewind = curr_tick - min_stored_tick;
	ImGui::SetNextItemWidth(100);
	ImGui::InputInt("Max Frames", &data.p_MaxFrames);
	ImGui::SameLine();

	ImGui::SetNextItemWidth(300);
	ImGui::SliderInt("Rewind", &data.p_TimeSlider, 0, max_rewind);
	ImGui::SameLine();
	ImGui::Text("%i", max_rewind);


	const int size_banner = 50;
	ImGui::SetCursorPos(ImVec2(8, size_banner));
	ImGui::BeginChild("BufferList", ImVec2(space_available.x - 8, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);
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
					auto head_name = QString("%1 %2 [%3]").arg(frames[column].p_Stage).arg(column).arg(frames[column].p_Tick).toLocal8Bit();
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
void ShaderViewer::RenderImGui(InterfaceShaderViewer& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("Shader Viewer", &data.p_Visible, ImGuiWindowFlags_NoCollapse);
	ImVec2 space_available = ImGui::GetWindowContentRegionMax();

	bool all_close = ImGui::Button("<<");
	ImGui::SameLine();
	bool all_open = ImGui::Button(">>");
	ImGui::SameLine();

	ImGui::SetNextItemWidth(300);
	ImGui::InputText("Search", &data.p_Search);
	QString search = QString();
	if (!data.p_Search.empty()) {
		search = data.p_Search.c_str();
	}

	const int size_banner = 50;
	ImGui::SetCursorPos(ImVec2(8, size_banner));
	ImGui::BeginChild("ShaderList", ImVec2(space_available.x - 8, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);

	auto shaders = Core::Singleton().GetShaders();
	auto compute_shaders = Core::Singleton().GetComputeShaders();

	for (auto& shader : compute_shaders) {
		auto info = RenderShader(data, shader.first, shader.second, true, search);
		info->p_Open = (info->p_Open || all_open) && (!all_close);
	}
	for (auto& shader : shaders) {
		auto info = RenderShader(data, shader.first, shader.second, false, search);
		info->p_Open = (info->p_Open || all_open) && (!all_close);
	}
	ImGui::EndChild();
	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
InterfaceCollapsible* ShaderViewer::RenderShader(InterfaceShaderViewer& data, QString name, GLShader* shader, bool is_compute, QString search) {

	const int context_lines = 4;
	const int number_width = 50;
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
			if (search.isNull()) {
				for (int line = 0; line < lines.size(); line++) {
					ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), QString("%1").arg(line).toLocal8Bit().data());
					ImGui::SameLine();
					ImGui::SetCursorPosX(number_width);
					ImGui::TextUnformatted(lines[line].toLocal8Bit().data());
				}
			} else {

				int context_countdown = 0;
				int last_line = 0;
				int num_lines = lines.size();
				int match_count = 0;
				std::deque<bool> look_ahead_match;

				for(int line = 0; line < num_lines; line++) {
					while (look_ahead_match.size() <= context_lines) {
						int ahead = line + (int)look_ahead_match.size();
						bool found = (ahead < num_lines) && lines[ahead].contains(search, Qt::CaseInsensitive);
						match_count += found ? 1 : 0;
						look_ahead_match.push_back(found);
					}
					bool found = false;
					if (!look_ahead_match.empty()) {
						found = look_ahead_match[0];
						look_ahead_match.pop_front();
					}
					context_countdown = found ? context_lines + 1 : context_countdown - 1;

					if ((match_count <= 0) && (context_countdown <= 0)) {
						continue;
					}
					if (last_line != (line - 1)) {
						ImGui::Separator();
					}
					auto line_num_str = QString("%1").arg(line).toLocal8Bit();
					auto line_str = lines[line].toLocal8Bit();
					ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), line_num_str.data());
					ImGui::SameLine();
					ImGui::SetCursorPosX(number_width);
					ImGui::TextColored(found ? ImVec4(1.0f, 0.5f, 0.5f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), line_str.data());
					last_line = line;

					match_count -= found ? 1 : 0;
				}
			}
		}
	}
	ImGui::PopStyleColor(3);
	return found;
}

////////////////////////////////////////////////////////////////////////////////
void ResourceViewer::RenderImGui(InterfaceResourceViewer& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("Resource Viewer", &data.p_Visible, ImGuiWindowFlags_NoCollapse);
	ImVec2 space_available = ImGui::GetWindowContentRegionMax();

	//const int size_banner = 50;
	//ImGui::SetCursorPos(ImVec2(8, size_banner));
	//ImGui::BeginChild("List", ImVec2(space_available.x - 8, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);

	const auto& resources = Core::GetResources();
	for (const auto& resource : resources) {
		QString state = "Pending";
		if (resource.second.m_State == Core::ResourceState::DONE) {
			state = "Done";
		} else if (resource.second.m_State == Core::ResourceState::ERROR) {
			state = "Error - " + resource.second.m_Error;
		}
		QByteArray info = QString("%1: %2").arg(resource.first).arg(state).toLocal8Bit();
		ImGui::Text(info.data());
		
	}

	//ImGui::EndChild();
	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
void Scrapbook2D::RenderImGui(InterfaceScrapbook2D& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("2D Scrapbook", &data.p_Visible, ImGuiWindowFlags_NoCollapse);
	ImVec2 space_available = ImGui::GetWindowContentRegionMax();

	ImGui::Text("content");

	//const int size_banner = 50;
	//ImGui::SetCursorPos(ImVec2(8, size_banner));
	//ImGui::BeginChild("List", ImVec2(space_available.x - 8, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);

	//ImGui::Text("content");

	//ImGui::EndChild();
	ImGui::End();

}

////////////////////////////////////////////////////////////////////////////////
auto Scrapbook3D::ActivateRTT(void) {
	auto& self = Singleton();
	bool reset = self.m_NeedsReset;
	self.m_NeedsReset = false;
	return self.m_RTT.Activate(RTT::Mode::RGBA_DEPTH_STENCIL, self.m_Width, self.m_Height, reset);
}

////////////////////////////////////////////////////////////////////////////////
void Scrapbook3D::IRenderImGui(InterfaceScrapbook3D& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("3D Scrapbook", &data.p_Visible, ImGuiWindowFlags_NoCollapse);
	ImVec2 space_available = ImGui::GetWindowContentRegionMax();
	const int size_banner = 50;
	m_Width = space_available.x - 8;
	m_Height = space_available.y - size_banner;

	ImGui::Text("content");

	{
		auto token = ActivateRTT();

		glEnable(GL_DEPTH_TEST);

		const double size_grid = 10.0;
		AddLine(Triple(0, 0, 0), Triple(size_grid, 0, 0), QVector4D(1, 0, 0, 1));
		AddLine(Triple(0, 0, 0), Triple(0, size_grid, 0), QVector4D(0, 1, 0, 1));
		AddLine(Triple(0, 0, 0), Triple(0, 0, size_grid), QVector4D(0, 0, 1, 1));

		auto vp = GetViewPerspectiveMatrix();
		IRender3DDebug(vp, m_Width, m_Height, Triple(0, 0, 0), 1.0);
		IClear();
		m_NeedsReset = true;
	}

	ImVec2 im_pos(8, size_banner);
	ImVec2 im_size(m_Width, m_Height);
	ImGui::SetCursorPos(im_pos);
	ImTextureID tex_id = (ImTextureID)(unsigned long long)m_RTT.GetColorTex();
	ImGui::Image(tex_id, im_size, ImVec2(0, 1), ImVec2(1, 0));

	ImGui::SetCursorPos(im_pos);
	ImGui::InvisibleButton("##FullScreen", im_size);
	if (ImGui::IsItemHovered()) {
		const double ZOOM_SPEED = 0.1;
		const double ANGLE_SPEED = 0.2;

		ImGuiIO& io = ImGui::GetIO();
		if (io.MouseWheel != 0.0f) {
			bool up = io.MouseWheel > 0.0f;
			m_Cam.p_Zoom *= 1.0 + (up ? -1 : 1) * ZOOM_SPEED;
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			m_Cam.p_HorizontalDegrees += io.MouseDelta.x * ANGLE_SPEED;
			m_Cam.p_VerticalDegrees += io.MouseDelta.y * ANGLE_SPEED;
		}
	}

	ImGui::End();

}
