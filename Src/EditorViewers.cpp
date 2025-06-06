////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

#if defined(NESHNY_WEBGPU)

////////////////////////////////////////////////////////////////////////////////
BaseSimpleRender::BaseSimpleRender(void) {
}

////////////////////////////////////////////////////////////////////////////////
BaseSimpleRender::~BaseSimpleRender(void) {
	delete m_Uniforms;
	delete m_CircleBuffer;
}

////////////////////////////////////////////////////////////////////////////////
void BaseSimpleRender::IRender(WebGPURTT& rtt, const Matrix4& view_perspective, int width, int height, Vec3 offset, double scale, double point_size) {

	struct RenderPoint {
		fVec4	p_Pos;
		fVec4	p_Col;
	};
	std::vector<RenderPoint> simple_lines;
	std::vector<RenderPoint> simple_circles;
	std::vector<RenderPoint> simple_triangles;

	for (WebGPUBuffer* prev_buffer : m_PreviousFrameBuffers) {
		delete prev_buffer;
	}
	m_PreviousFrameBuffers.clear();

	auto gpu_vp = view_perspective.ToGPU();

	simple_lines.reserve(m_Lines.size() * 2);
	simple_circles.reserve(m_Circles.size());
	Vec2 offset2d(offset.x, offset.y);
	for (const auto& line : m_Lines) {
		simple_lines.push_back({ fVec4(((line.p_A - offset) * scale).ToFloat3(), 1.0), line.p_Col.ToFloat4() });
		simple_lines.push_back({ fVec4(((line.p_B - offset) * scale).ToFloat3(), 1.0), line.p_Col.ToFloat4() });
	}
	for (const auto& circle : m_Circles) {
		simple_circles.push_back({ fVec4((circle.p_Pos.x - offset.x) * scale, (circle.p_Pos.y - offset.y) * scale, circle.p_Radius * scale, circle.p_Radius * scale), circle.p_Col.ToFloat4() });
	}
	for (auto it = m_Points.begin(); it != m_Points.end(); it++) {

		// TODO: use on_top
		//if (it->p_OnTop) {
		//} else {
		//}
		Vec3 dpos = (it->p_Pos - offset) * scale;

		for (int i = 0; i < 2; i++) {
			Vec3 off = (it->p_Pos - offset + Vec3(Random() - 0.5, Random() - 0.5, Random() - 0.5) * point_size) * scale;
			simple_lines.push_back({ fVec4(it->p_Pos.ToFloat3(), 1.0), it->p_Col.ToFloat4() });
			simple_lines.push_back({ fVec4(off.ToFloat3(), 1.0), it->p_Col.ToFloat4() });
		}
		if (it->p_Str.size() <= 0) {
			continue;
		}
		fVec3 result = gpu_vp * fVec3(dpos.x, dpos.y, dpos.z);
		if (result.z > 1) {
			continue;
		}
		int x = (int)floor((result.x + 1.0) * 0.5 * width);
		int y = (int)floor((1.0 - result.y) * 0.5 * height);
		ImGui::SetCursorPos(ImVec2(x, y));
		ImGui::Text(it->p_Str.c_str());
	}
	for (const auto& square: m_Squares) {
		fVec4 col = square.p_Col.ToFloat4();
		fVec4 pos_a = fVec4(((square.p_MinPos - offset) * scale).ToFloat3(), 1.0);
		fVec4 pos_c = fVec4(((square.p_MaxPos - offset) * scale).ToFloat3(), 1.0);
		fVec4 pos_b(pos_a.x, pos_c.y, 0.0, 1.0);
		fVec4 pos_d(pos_c.x, pos_a.y, 0.0, 1.0);
		if (square.p_Filled) {

			simple_triangles.push_back({ pos_a, col });
			simple_triangles.push_back({ pos_c, col });
			simple_triangles.push_back({ pos_b, col });

			simple_triangles.push_back({ pos_a, col });
			simple_triangles.push_back({ pos_d, col });
			simple_triangles.push_back({ pos_c, col });
		} else {
			simple_lines.push_back({ pos_a, col }); simple_lines.push_back({ pos_b, col });
			simple_lines.push_back({ pos_b, col }); simple_lines.push_back({ pos_c, col });
			simple_lines.push_back({ pos_c, col }); simple_lines.push_back({ pos_d, col });
			simple_lines.push_back({ pos_d, col }); simple_lines.push_back({ pos_a, col });
		}
	}
	for (const auto& triangle: m_Triangles) {
		auto col = triangle.p_Col.ToFloat4();
		simple_triangles.push_back({ fVec4(((triangle.p_A - offset) * scale).ToFloat3(), 1.0), col });
		simple_triangles.push_back({ fVec4(((triangle.p_B - offset) * scale).ToFloat3(), 1.0), col });
		simple_triangles.push_back({ fVec4(((triangle.p_C - offset) * scale).ToFloat3(), 1.0), col });
	}

	m_LineBuffer.Init({ WGPUVertexFormat_Float32x4, WGPUVertexFormat_Float32x4 }, WGPUPrimitiveTopology_LineList, (unsigned char*)simple_lines.data(), (int)simple_lines.size() * sizeof(RenderPoint));
	m_TriangleBuffer.Init({ WGPUVertexFormat_Float32x4, WGPUVertexFormat_Float32x4 }, WGPUPrimitiveTopology_TriangleList, (unsigned char*)simple_triangles.data(), (int)simple_triangles.size() * sizeof(RenderPoint));

	if (!m_Uniforms) {
		// TODO: this throws an error for one frame due to buffers being empty at first
		m_Uniforms = new WebGPUBuffer(WGPUBufferUsage_Uniform, nullptr, sizeof(fMatrix4));
		m_CircleBuffer = new WebGPUBuffer(WGPUBufferUsage_Storage);
		m_LinePipline
			.AddBuffer(*m_Uniforms, WGPUShaderStage_Vertex, true)
			.FinalizeRender("SimplePoint", m_LineBuffer);
		m_CirclePipline
			.AddBuffer(*m_Uniforms, WGPUShaderStage_Vertex, true)
			.AddBuffer(*m_CircleBuffer, WGPUShaderStage_Vertex, true)
			.FinalizeRender("DebugCircle", *Core::Singleton().GetBuffer("Circle"));
		m_TrianglePipline
			.AddBuffer(*m_Uniforms, WGPUShaderStage_Vertex, true)
			.FinalizeRender("SimplePoint", m_TriangleBuffer);
		m_TexturePipline
			.AddBuffer(*m_Uniforms, WGPUShaderStage_Vertex, true)
			.AddTexture(WGPUTextureViewDimension_2D, nullptr)
			.AddSampler(*Core::GetSampler(WGPUAddressMode_Repeat))
			.FinalizeRender("SimpleTexture", *Core::GetBuffer("Square"), {});
	}
	m_CircleBuffer->EnsureSizeBytes((int)simple_circles.size() * sizeof(RenderPoint));
	m_CircleBuffer->SetValues(simple_circles);

	wgpuQueueWriteBuffer(Core::Singleton().GetWebGPUQueue(), m_Uniforms->Get(), 0, &gpu_vp, sizeof(fMatrix4));

	rtt.Render(&m_LinePipline);
	rtt.Render(&m_TrianglePipline);
	rtt.Render(&m_CirclePipline, (int)m_Circles.size());

	// TODO: sort by texture name and render in batches
	for (const auto& tex : m_Textures) {
		const WebGPUTexture* texture = nullptr;
		if (std::holds_alternative<std::string>(tex.p_Texture)) {
			auto texture_res = Core::GetResource<Texture2D>(std::get<std::string>(tex.p_Texture));
			if (!texture_res.IsValid()) {
				return;
			}
			texture = &(texture_res->Get());
		} else {
			texture = std::get<WebGPUTexture*>(tex.p_Texture);
		}
		Matrix4 modded = Matrix4::Identity();

		Vec3 center = (tex.p_MaxPos + tex.p_MinPos) * 0.5;
		Vec3 size = (tex.p_MaxPos - tex.p_MinPos) * 0.5;

		modded.Translate(center);
		modded.Scale(Vec3(size.x, size.y, 1.0));

		modded = view_perspective * modded;
		auto new_gpu = modded.ToGPU();

		WebGPUBuffer* uniform_mat = new WebGPUBuffer(WGPUBufferUsage_Uniform, nullptr, sizeof(fMatrix4));
		wgpuQueueWriteBuffer(Core::Singleton().GetWebGPUQueue(), uniform_mat->Get(), 0, &new_gpu, sizeof(fMatrix4));

		m_PreviousFrameBuffers.push_back(uniform_mat);

		m_TexturePipline.ReplaceTexture(0, texture->GetTextureView());
		m_TexturePipline.ReplaceBuffer(0, *uniform_mat);
		rtt.Render(&m_TexturePipline);
	}
}

#elif defined(NESHNY_GL)
////////////////////////////////////////////////////////////////////////////////
BaseSimpleRender::BaseSimpleRender(void) {
}

////////////////////////////////////////////////////////////////////////////////
BaseSimpleRender::~BaseSimpleRender(void) {
}

////////////////////////////////////////////////////////////////////////////////
void BaseSimpleRender::IRender(const Matrix4 & view_perspective, int width, int height, Vec3 offset, double scale, double point_size) {

	auto gpu_vp = view_perspective.ToGPU();

	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	GLShader* debug_prog = Core::GetShader("Debug");
	debug_prog->UseProgram();
	GLBuffer* line_buffer = Core::GetBuffer("DebugLine");
	line_buffer->UseBuffer(debug_prog);

	glUniformMatrix4fv(debug_prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, gpu_vp.Data());

	for (auto it = m_Lines.begin(); it != m_Lines.end(); it++) {
		if (it->p_OnTop) {
			glDisable(GL_DEPTH_TEST);
		} else {
			glEnable(GL_DEPTH_TEST);
		}
		Vec3 da = (it->p_A - offset) * scale;
		Vec3 db = (it->p_B - offset) * scale;
		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x, it->p_Col.y, it->p_Col.z, it->p_Col.w);
		glUniform3f(debug_prog->GetUniform("uPosA"), da.x, da.y, da.z);
		glUniform3f(debug_prog->GetUniform("uPosB"), db.x, db.y, db.z);
		line_buffer->Draw();
	}

	ImVec2 stashed_cursor_pos = ImGui::GetCursorPos();
	for (auto it = m_Points.begin(); it != m_Points.end(); it++) {

		if (it->p_OnTop) {
			glDisable(GL_DEPTH_TEST);
		} else {
			glEnable(GL_DEPTH_TEST);
		}
		Vec3 dpos = (it->p_Pos - offset) * scale;

		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x, it->p_Col.y, it->p_Col.z, it->p_Col.w);
		glUniform3f(debug_prog->GetUniform("uPosA"), dpos.x, dpos.y, dpos.z);
		for (int i = 0; i < 2; i++) {
			Vec3 off = (it->p_Pos - offset + Vec3(Random() - 0.5, Random() - 0.5, Random() - 0.5) * point_size) * scale;
			glUniform3f(debug_prog->GetUniform("uPosB"), off.x, off.y, off.z);
			line_buffer->Draw();
		}
		if (it->p_Str.empty()) {
			continue;
		}
		fVec3 result = gpu_vp * fVec3(dpos.x, dpos.y, dpos.z);
		if (result.z > 1) {
			continue;
		}
		int x = (int)floor((result.x + 1.0) * 0.5 * width);
		int y = (int)floor((1.0 - result.y) * 0.5 * height);
		ImGui::SetCursorPos(ImVec2(x, y));
		ImGuiTextColoredUnformatted(it->p_Str, ImVec4(it->p_Col.x, it->p_Col.y, it->p_Col.z, it->p_Col.w));
	}
	ImGui::SetCursorPos(stashed_cursor_pos);

	debug_prog = Core::GetShader("DebugTriangle");
	debug_prog->UseProgram();
	GLBuffer* buffer = Core::GetBuffer("DebugTriangle");
	buffer->UseBuffer(debug_prog);

	glUniformMatrix4fv(debug_prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, gpu_vp.Data());

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	for (auto it = m_Triangles.begin(); it != m_Triangles.end(); it++) {

		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x, it->p_Col.y, it->p_Col.z, it->p_Col.w);

		Vec3 da = (it->p_A - offset) * scale;
		Vec3 db = (it->p_B - offset) * scale;
		Vec3 dc = (it->p_C - offset) * scale;

		glUniform3f(debug_prog->GetUniform("uPosA"), da.x, da.y, da.z);
		glUniform3f(debug_prog->GetUniform("uPosB"), db.x, db.y, db.z);
		glUniform3f(debug_prog->GetUniform("uPosC"), dc.x, dc.y, dc.z);
		buffer->Draw();
	}

	debug_prog = Core::GetShader("DebugPoint");
	debug_prog->UseProgram();
	buffer = Core::GetBuffer("Circle");
	buffer->UseBuffer(debug_prog);

	glUniformMatrix4fv(debug_prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, gpu_vp.Data());

	for (auto it = m_Circles.begin(); it != m_Circles.end(); it++) {

		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x, it->p_Col.y, it->p_Col.z, it->p_Col.w);
		Vec3 dpos = (it->p_Pos - offset) * scale;
		glUniform3f(debug_prog->GetUniform("uPos"), dpos.x, dpos.y, dpos.z);
		glUniform1f(debug_prog->GetUniform("uSize"), it->p_Radius * scale);
		buffer->Draw();
	}
	
	debug_prog = Core::GetShader("Debug");
	debug_prog->UseProgram();
	buffer = Core::GetBuffer("DebugSquare");
	buffer->UseBuffer(debug_prog);

	glUniformMatrix4fv(debug_prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, gpu_vp.Data());

	for (auto it = m_Squares.begin(); it != m_Squares.end(); it++) {

		glUniform4f(debug_prog->GetUniform("uColor"), it->p_Col.x, it->p_Col.y, it->p_Col.z, it->p_Col.w);
		Vec3 dmin_pos = (it->p_MinPos - offset) * scale;
		Vec3 dmax_pos = (it->p_MaxPos - offset) * scale;
		glUniform3f(debug_prog->GetUniform("uPosA"), dmin_pos.x, dmin_pos.y, dmin_pos.z);
		glUniform3f(debug_prog->GetUniform("uPosB"), dmax_pos.x, dmax_pos.y, dmax_pos.z);
		buffer->Draw();
	}

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}
#endif

////////////////////////////////////////////////////////////////////////////////
void InfoViewer::ILoopTime(TimerNanos nanos) {
	double loop_seconds = nanos.count() * NANO_CONVERT;
	for (auto& segment : m_LoopHistogram) {
		if (loop_seconds < segment.first) {
			segment.second++;
			return;
		}
	}
	m_LoopHistogramOverflow++;
}

////////////////////////////////////////////////////////////////////////////////
void InfoViewer::IClearLoopTime(void) {
	m_LoopHistogram.clear();
	m_LoopHistogramOverflow = 0;
	for (double s = 0.01; s <= 0.05; s += 0.01) {
		m_LoopHistogram.push_back({ s, 0 });
	}
	for (double s = 0.1; s <= 0.5; s += 0.1) {
		m_LoopHistogram.push_back({ s, 0 });
	}
	m_LoopHistogram.push_back({ 1.0, 0 });
}

////////////////////////////////////////////////////////////////////////////////
void InfoViewer::IRenderImGui(InterfaceInfoViewer& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("Info Viewer", &data.p_Visible, ImGuiWindowFlags_NoCollapse);
	//ImVec2 space_available = ImGui::GetWindowContentRegionMax();

	ImGui::Text("Avg %.3f ms/frame (%.1f FPS), Tick %i, Second %.1f", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate, Core::GetTicks(), Core::GetRuntimeSeconds());
	ImGui::SameLine();

#ifdef NESHNY_TESTING
	if (ImGui::Button("Run Unit Tests")) {
		UnitTester::Execute();
	}
#endif
	ImGui::SameLine();
	auto& timings = DebugTiming::GetTimings();
	if (ImGui::Button("Reset Timings")) {
		timings.clear();
	}

	ImGui::Text("End-to-end frame loop histogram");

	if (ImGui::BeginTable("##SegmentTable", (int)m_LoopHistogram.size() + 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV)) {
		ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

		for (const auto& segment : m_LoopHistogram) {
			std::string label = std::format("<{:.2f}s", segment.first);
			ImGui::TableSetupColumn(label.data(), ImGuiTableColumnFlags_WidthFixed, 50);
		}
		ImGui::TableSetupColumn("Overflow", ImGuiTableColumnFlags_WidthFixed, 60);
		ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		ImGui::TableNextRow();
		int col = 0;
		for (const auto& segment : m_LoopHistogram) {
			ImGui::TableSetColumnIndex(col++);
			if (segment.second) {
				ImGuiTextColoredUnformatted(std::format("{}", segment.second), segment.first >= 0.04 ? (segment.first >= 0.2 ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(1.0f, 0.6f, 0.3f, 1.0f)) : ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
			}
		}
		ImGui::TableSetColumnIndex(col++);
		if (m_LoopHistogramOverflow) {
			ImGuiTextColoredUnformatted(std::format("{}", m_LoopHistogramOverflow), ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		}
		ImGui::TableSetColumnIndex(col++);
		if (ImGui::Button("Clear")) {
			IClearLoopTime();
		}
		ImGui::EndTable();
	}

	int64_t total_nanos = 0;
	for (auto iter = timings.begin(); iter != timings.end(); iter++) {
		total_nanos += iter->p_Nanos;
	}
	
	if (ImGui::BeginTable("##PerfTable", 7, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV)) {
		ImGui::TableSetupScrollFreeze(1, 1); // Make top row + left col always visible

		ImGui::TableSetupColumn("Timing label", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Roll Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 90);
		ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("%", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableHeadersRow();

		for (auto iter = timings.begin(); iter != timings.end(); iter++) {

			double total_av_secs = ((double)iter->p_Nanos * NANO_CONVERT) / (double)iter->p_NumCalls;
			double percent = 100.0 * (double)iter->p_Nanos / (double)total_nanos;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", iter->p_Label);
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.6f", iter->p_RollingAvSeconds * 1000.0);
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.6f", total_av_secs * 1000.0);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%lli", iter->p_NumCalls);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.6f", iter->p_MinNanos * MICRO_CONVERT);
			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%.6f", iter->p_MaxNanos * MICRO_CONVERT);
			ImGui::TableSetColumnIndex(6);
			ImGui::Text("%.3f", percent);
		}
		ImGui::EndTable();
	}
	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
void LogViewer::IRenderImGui(InterfaceLogViewer& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("Log Viewer", &data.p_Visible, ImGuiWindowFlags_NoCollapse);

	ImGui::Checkbox("Auto scroll", &data.p_AutoScroll);
	ImGui::SameLine(ImGui::GetWindowWidth() - 60);
	ImGui::SetNextItemWidth(60);
	if (ImGui::Button("Clear")) {
		Clear();
	}

	if (ImGui::BeginTable("##logTable", 2, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV)) {
		ImGui::TableSetupScrollFreeze(1, 1);

		ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed, 90);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		for (const auto& log: m_Logs) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			//auto time_str = log.p_Time.toString("hh:mm:ss.zzz").toLocal8Bit();
			//ImGui::Text(time_str.data());
			ImGui::Text(log.p_TimeStr.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGuiTextColoredUnformatted(log.p_Message, log.p_Color);
		}

		if (data.p_AutoScroll) {
			ImGui::ScrollToItem();
		}
		ImGui::EndTable();
	}
	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
void BufferViewer::ICheckpoint(std::string_view name, std::string_view stage, SSBO& buffer, int count, const StructInfo* info, MemberSpec::Type type) {
	int item_size = MemberSpec::GetGPUTypeSizeBytes(type);
	count = count >= 0 ? count : buffer.GetSizeBytes() / item_size;

	std::shared_ptr<unsigned char[]> mem = nullptr;
	if (Core::IsBufferEnabled(name)) {
		mem = buffer.MakeCopy(count * item_size);
	}
	IStoreCheckpoint(std::string(name), { std::string(stage), "", count, Core::GetTicks(), false, mem }, info, type);
}

////////////////////////////////////////////////////////////////////////////////
void BufferViewer::ICheckpoint(std::string_view stage, GPUEntity& buffer) {
	if (!Core::IsBufferEnabled(buffer.GetName())) {
		return;
	}
#if defined(NESHNY_GL)
	std::shared_ptr<unsigned char[]> mem = buffer.MakeCopySync();
	IStoreCheckpoint(std::string(buffer.GetName()), { std::string(stage), buffer.GetDebugInfo(), buffer.GetMaxIndex(), Core::GetTicks(), buffer.GetDeleteMode() == GPUEntity::DeleteMode::STABLE_WITH_GAPS, mem }, &buffer.GetSpecs(), MemberSpec::Type::T_UNKNOWN);
#elif defined(NESHNY_WEBGPU)

	int ticks = Core::GetTicks();
	buffer.AccessData([buffer_name = buffer.GetName(), ticks](unsigned char* data, int size_bytes, EntityInfo info) {
		unsigned char* copy = new unsigned char[size_bytes];
		memcpy(copy, data, size_bytes);
		std::shared_ptr<unsigned char[]> mem(copy);

		std::string new_info = std::format("# {} mx {} id {} free {}", info.p_Count, info.p_MaxIndex, info.p_NextId, info.p_FreeCount);
		BufferViewer::Singleton().UpdateCheckpoint(buffer_name, ticks, info.p_MaxIndex, new_info, mem);
	});
	IStoreCheckpoint(std::string(buffer.GetName()), { std::string(stage), {}, 0, ticks, true, nullptr }, &buffer.GetSpecs(), MemberSpec::Type::T_UNKNOWN);

#endif
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<SSBO> BufferViewer::IGetStoredFrameAt(std::string_view name, int tick, int& count) {

	auto existing = m_Frames.find(std::string(name));
	if (existing == m_Frames.end()) {
		return nullptr;
	}
	for (auto& frame : existing->second.p_Frames) {
		if (frame.p_Tick == tick) {
			count = frame.p_Count;
#if defined(NESHNY_GL)
			return std::make_shared<SSBO>(existing->second.p_StructSize * frame.p_Count + (int)sizeof(int) * ENTITY_OFFSET_INTS, frame.p_Data.get()); // TODO: cache this if it's not performant
#elif defined(NESHNY_WEBGPU)
			return std::make_shared<SSBO>(WGPUBufferUsage_Storage, frame.p_Data.get(), existing->second.p_StructSize * frame.p_Count + (int)sizeof(int) * ENTITY_OFFSET_INTS); // TODO: cache this if it's not performant
#endif
		}
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
void BufferViewer::IStoreCheckpoint(std::string name, CheckpointData data, const StructInfo* info, MemberSpec::Type type) {
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
void BufferViewer::UpdateCheckpoint(std::string_view name, int tick, int new_count, std::string_view new_info, std::shared_ptr<unsigned char[]> new_data) {
	auto existing = m_Frames.find(std::string(name));
	if (existing == m_Frames.end()) {
		return;
	}
	for (auto iter = existing->second.p_Frames.begin(); iter != existing->second.p_Frames.end(); iter++) {
		if (iter->p_Tick == tick) {
			iter->p_Count = new_count;
			iter->p_Data = new_data;
			iter->p_Info = std::string(new_info);
			return;
		}
	}
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

	//ImGui::SetNextItemWidth(300);
	ImGui::SetNextItemWidth(space_available.x - 400);
	ImGui::SliderInt("Rewind", &data.p_TimeSlider, 0, max_rewind);
	ImGui::SameLine();
	if (ImGui::Button("<")) {
		data.p_TimeSlider = std::max(0, data.p_TimeSlider - 1);
	}
	ImGui::SameLine();
	if (ImGui::Button(">")) {
		data.p_TimeSlider = std::min(max_rewind, data.p_TimeSlider + 1);
	}
	ImGui::SameLine();
	ImGui::Text("%i", max_rewind);

	const int size_banner = 50;
	ImGui::SetCursorPos(ImVec2(8, size_banner));
	ImGui::BeginChild("BufferList", ImVec2(space_available.x - 8, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(255, 0, 0, 200));

	m_Hovered = std::nullopt;
	const int MAX_COLS = 50;
	for (auto& buffer : m_Frames) {
		const auto& frames = buffer.second.p_Frames;
		auto header = (buffer.first + (frames.empty() ? "" : frames.begin()->p_Info) + "###" + buffer.first);
		bool highlight_buffer = buffer.first == m_HighlightName;

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

		auto check_header = ("##" + buffer.first);

		if (data.p_AllEnabled) {
			ImGui::BeginDisabled();
		}
		ImGui::Checkbox(check_header.c_str(), &found->p_Enabled);
		if (data.p_AllEnabled) {
			ImGui::EndDisabled();
		}
		ImGui::SameLine();

		ImGui::SetNextItemOpen(found->p_Open);
		if ((found->p_Open = ImGui::CollapsingHeader(header.data()))) {

			int max_frames = std::min((int)frames.size(), MAX_COLS);
			int max_count = 0;
			for (auto frame : frames) {
				max_count = std::max(max_count, frame.p_Count);
			}

			ImGuiTableFlags table_flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
			if (ImGui::BeginTable("##Table", max_frames + 1, table_flags)) {
				ImGui::TableSetupScrollFreeze(1, 1); // Make top row + left col always visible

				struct StructType {
					std::string			p_Name;
					MemberSpec::Type	p_Type;
					MemberSpec::Type	p_FullType;
					int					p_FullOffset = 0;
				};

				std::vector<StructType> struct_types;
				for (const auto& member : buffer.second.p_Members) {
					int array_count = member.p_ArrayCount.value_or(1);
					for (int a = 0; a < array_count; a++) {
						auto memname = array_count == 1 ? member.p_Name : std::format("{}[{}]", member.p_Name, a);
						if (member.p_Type == MemberSpec::T_VEC2) {
							struct_types.push_back({ memname + ".x", MemberSpec::T_FLOAT, member.p_Type, 0 });
							struct_types.push_back({ memname + ".y", MemberSpec::T_FLOAT, member.p_Type, -1 });
						} else if (member.p_Type == MemberSpec::T_VEC3) {
							struct_types.push_back({ memname + ".x", MemberSpec::T_FLOAT, member.p_Type, 0 });
							struct_types.push_back({ memname + ".y", MemberSpec::T_FLOAT, member.p_Type, -1 });
							struct_types.push_back({ memname + ".z", MemberSpec::T_FLOAT, member.p_Type, -2 });
						} else if (member.p_Type == MemberSpec::T_VEC4) {
							struct_types.push_back({ memname + ".x", MemberSpec::T_FLOAT, member.p_Type, 0 });
							struct_types.push_back({ memname + ".y", MemberSpec::T_FLOAT, member.p_Type, -1 });
							struct_types.push_back({ memname + ".z", MemberSpec::T_FLOAT, member.p_Type, -2 });
							struct_types.push_back({ memname + ".w", MemberSpec::T_FLOAT, member.p_Type, -3 });
						} else if (member.p_Type == MemberSpec::T_IVEC2) {
							struct_types.push_back({ memname + ".x", MemberSpec::T_INT, member.p_Type, 0 });
							struct_types.push_back({ memname + ".y", MemberSpec::T_INT, member.p_Type, -1 });
						} else if (member.p_Type == MemberSpec::T_IVEC3) {
							struct_types.push_back({ memname + ".x", MemberSpec::T_INT, member.p_Type, 0 });
							struct_types.push_back({ memname + ".y", MemberSpec::T_INT, member.p_Type, -1 });
							struct_types.push_back({ memname + ".z", MemberSpec::T_INT, member.p_Type, -2 });
						} else if (member.p_Type == MemberSpec::T_IVEC4) {
							struct_types.push_back({ memname + ".x", MemberSpec::T_INT, member.p_Type, 0 });
							struct_types.push_back({ memname + ".y", MemberSpec::T_INT, member.p_Type, -1 });
							struct_types.push_back({ memname + ".z", MemberSpec::T_INT, member.p_Type, -2 });
							struct_types.push_back({ memname + ".w", MemberSpec::T_INT, member.p_Type, -3 });
						} else {
							struct_types.push_back({ memname, member.p_Type, member.p_Type, 0 });
						}
					}
				}
				int cycles = (int)struct_types.size();

				ImGui::TableSetupColumn("-", ImGuiTableColumnFlags_WidthFixed, struct_types.empty() ? 50 : 100);
				for (int column = 0; column < max_frames; column++) {
					auto head_name = std::format("{} {} [{}]", frames[column].p_Stage, column, frames[column].p_Tick);
					ImGui::TableSetupColumn(head_name.c_str(), ImGuiTableColumnFlags_WidthFixed, 100);
				}

				ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
				for (int column = 0; column <= max_frames; column++) {
					ImGui::TableSetColumnIndex(column);
					const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
					ImGui::PushID(column);
					ImGui::TableHeader(column_name);
					if ((column > 0) && ImGui::IsItemHovered()) {
						auto info = frames[column - 1].p_Info;
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
						std::optional<MemberSpec::Type> full_cast_type = std::nullopt;
						int full_type_offet = 0;
						int real_count = cycles ? row / cycles : row;
						int struct_ind = cycles ? row % cycles : 0;
						if (!struct_types.empty()) {
							const auto& spec = struct_types[struct_ind];
							auto info = std::format("{}{}", struct_ind ? "" : std::format("[{}] ", real_count), spec.p_Name);
							ImGui::Text(info.c_str());
							cast_type = spec.p_Type;
							full_cast_type = spec.p_FullType;
							full_type_offet = spec.p_FullOffset;
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
							unsigned char* rawptr = &(col.p_Data[0]) + ENTITY_OFFSET_INTS * sizeof(int);
							if ((struct_ind == 0) && (cycles >= 2) && col.p_UsingFreeList) {
								int id = ((int*)rawptr)[row];
								if (id == -1) {
									ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(255, 0, 128, 200));
									ImGui::Text("DELETED");
									continue;
								} else if (id == m_HighlightID && highlight_buffer) {
									ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(128, 220, 128, 200));
									//ImGui::SetNextWindowScroll(ImGui::GetWindowPos());
									//auto pos = ImGui::GetCursorPos();
									//ImGui::ScrollToRect(ImGui::GetCurrentWindow(), ImRect(pos, pos));
								}
							}
							if (cast_type == MemberSpec::T_FLOAT) {
								float val = ((float*)rawptr)[row];
								std::string val_str;
								if (val < 0.001) {
									val_str = std::format("{:.10f}", val);
								} else if (val < 1.0) {
									val_str = std::format("{:.6f}", val);
								} else {
									val_str = std::format("{:.4f}", val);
								}
								ImGui::Text(val_str.c_str());
							} else if (cast_type == MemberSpec::T_UINT) {
								ImGui::Text("%i", ((unsigned int*)rawptr)[row]);
							} else {
								ImGui::Text("%i", ((int*)rawptr)[row]);
							}
							if (ImGui::IsItemHovered(0) && full_cast_type.has_value()) {
								if (*full_cast_type == MemberSpec::T_INT) {
									m_Hovered = ((int*)rawptr)[row + full_type_offet];
								} else if (*full_cast_type == MemberSpec::T_UINT) {
									m_Hovered = ((unsigned int*)rawptr)[row + full_type_offet];
								} else if (*full_cast_type == MemberSpec::T_FLOAT) {
									m_Hovered = ((float*)rawptr)[row + full_type_offet];
								} else if (*full_cast_type == MemberSpec::T_VEC2) {
									m_Hovered = fVec2(((float*)rawptr)[row + full_type_offet], ((float*)rawptr)[row + full_type_offet + 1]);
								} else if (*full_cast_type == MemberSpec::T_VEC3) {
									m_Hovered = fVec3(((float*)rawptr)[row + full_type_offet], ((float*)rawptr)[row + full_type_offet + 1], ((float*)rawptr)[row + full_type_offet + 2]);
								} else if (*full_cast_type == MemberSpec::T_VEC4) {
									m_Hovered = fVec4(((float*)rawptr)[row + full_type_offet], ((float*)rawptr)[row + full_type_offet + 1], ((float*)rawptr)[row + full_type_offet + 2], ((float*)rawptr)[row + full_type_offet + 3]);
								} else if (*full_cast_type == MemberSpec::T_IVEC2) {
									m_Hovered = iVec2(((int*)rawptr)[row + full_type_offet], ((int*)rawptr)[row + full_type_offet + 1]);
								} else if (*full_cast_type == MemberSpec::T_IVEC3) {
									m_Hovered = iVec3(((int*)rawptr)[row + full_type_offet], ((int*)rawptr)[row + full_type_offet + 1], ((int*)rawptr)[row + full_type_offet + 2]);
								} else if (*full_cast_type == MemberSpec::T_IVEC4) {
									m_Hovered = iVec4(((int*)rawptr)[row + full_type_offet], ((int*)rawptr)[row + full_type_offet + 1], ((int*)rawptr)[row + full_type_offet + 2], ((int*)rawptr)[row + full_type_offet + 3]);
								}
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
	std::string search;
	if (!data.p_Search.empty()) {
		search = data.p_Search;
	}

	ImGui::SameLine();
	ImGui::Checkbox("Show Preprocessed", &data.p_ShowPreprocessed);

	const int size_banner = 50;
	ImGui::SetCursorPos(ImVec2(8, size_banner));
	ImGui::BeginChild("ShaderList", ImVec2(space_available.x - 8, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);

	auto shaders = Core::Singleton().GetShaders();
	for (const auto& group : shaders) {
		int num_inst = (int)group.p_Instances.size();
		for (int i = 0; i < num_inst; i++) {
			std::string name = num_inst > 1 ? std::format("{} [{}]", group.p_Name, i) : group.p_Name;
			auto info = RenderShader(data, name, group.p_Instances[i].p_Shader, false, search);
			info->p_Open = (info->p_Open || all_open) && (!all_close);
		}
	}
#if defined(NESHNY_GL)
	auto compute_shaders = Core::Singleton().GetComputeShaders();

	for (const auto& group : compute_shaders) {
		int num_inst = (int)group.p_Instances.size();
		for (int i = 0; i < num_inst; i++) {
			std::string name = num_inst > 1 ? std::format("{} [{}]", group.p_Name, i) : group.p_Name;
			auto info = RenderShader(data, name, group.p_Instances[i].p_Shader, true, search);
			info->p_Open = (info->p_Open || all_open) && (!all_close);
		}
	}
#elif defined NESHNY_WEBGPU
#endif
	ImGui::EndChild();
	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
#if defined(NESHNY_GL)
InterfaceCollapsible* ShaderViewer::RenderShader(InterfaceShaderViewer& data, std::string_view name, GLShader* shader, bool is_compute, std::string_view search) {
#elif defined(NESHNY_WEBGPU)
InterfaceCollapsible* ShaderViewer::RenderShader(InterfaceShaderViewer& data, std::string_view name, WebGPUShader* shader, bool is_compute, std::string_view search) {
#endif

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
		data.p_Items.push_back({ std::string(name), false, false });
		found = &(data.p_Items.back());
	}

	ImGui::SetNextItemOpen(found->p_Open);

	if ((found->p_Open = ImGui::CollapsingHeader(name.data()))) {
#if defined(NESHNY_GL)
		auto& sources = shader->GetSources();
		for (auto& src : sources) {
			ImGui::TextUnformatted(src.m_Type.c_str());

			std::vector<std::string> lines;
			for (const auto line : std::views::split(src.m_Source, std::string{ "\n" })) {
				lines.push_back(std::string(line.data(), line.size()));
			}

			if (!src.m_Error.empty()) {
				ImGuiTextColoredUnformatted(src.m_Error, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			}
			if (search.empty()) {
				for (int line = 0; line < lines.size(); line++) {
					ImGuiTextColoredUnformatted(std::format("{}", line), ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
					ImGui::SameLine();
					ImGui::SetCursorPosX(number_width);
					ImGui::TextUnformatted(lines[line].c_str());
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
						bool found = (ahead < num_lines) && StringContains(lines[ahead], search, true);
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
					auto line_num_str = std::format("{}", line);
					ImGuiTextColoredUnformatted(line_num_str, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
					ImGui::SameLine();
					ImGui::SetCursorPosX(number_width);
					ImGuiTextColoredUnformatted(lines[line], found ? ImVec4(1.0f, 0.5f, 0.5f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

					last_line = line;
					match_count -= found ? 1 : 0;
				}
			}
		}
#elif defined(NESHNY_WEBGPU)

		std::string_view shader_source = data.p_ShowPreprocessed ? shader->GetSource() : shader->GetRawSource();

		std::vector<std::string> lines;
		for (const auto line : std::views::split(shader_source, std::string{"\n"})) {
			lines.push_back(std::string(line.data(), line.size()));
		}

		//if (ImGui::Button("Copy To Clipboard")) {
		//	ImGui::SetClipboardText(shader_source.data());
		//}

		const auto& errors = shader->GetErrors();
		const ImVec4 line_num_col(0.4f, 0.4f, 0.4f, 1.0f);
		const ImVec4 error_col(1.0f, 0.0f, 0.0f, 1.0f);
		for (const auto& err : errors) {
			ImGuiTextColoredUnformatted(err.m_Message, error_col);
		}
		if (search.empty()) {
			for (int line = 0; line < lines.size(); line++) {
				bool err_found = false;
				if (data.p_ShowPreprocessed) {
					for (const auto& err : errors) {
						if (err.m_LineNum == line) {
							err_found = true;
							std::string line_str = std::format("{}", line);
							ImGuiTextColoredUnformatted(line_str, error_col);
							ImGui::SameLine();
							ImGui::SetCursorPosX(number_width);

							auto before_error = lines[line].substr(0, err.m_LinePos + 1);
							auto after_error = lines[line].substr(err.m_LinePos + 1, lines[line].size() - err.m_LinePos - 1);

							ImGui::TextUnformatted(before_error.data());
							ImGui::SameLine(0.0, 0.0);
							ImGuiTextColoredUnformatted(after_error, error_col);
							break;
						}
					}
				}
				if (!err_found) {
					std::string line_str = std::format("{}", line);
					ImGuiTextColoredUnformatted(line_str, line_num_col);
					ImGui::SameLine();
					ImGui::SetCursorPosX(number_width);
					ImGui::TextUnformatted(lines[line].data());
				}
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
					bool found = (ahead < num_lines) && StringContains(lines[ahead], search, true);
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
				std::string line_num_str = std::format("{}", line);
				ImGuiTextColoredUnformatted(line_num_str, line_num_col);
				ImGui::SameLine();
				ImGui::SetCursorPosX(number_width);

				ImGui::PushStyleColor(ImGuiCol_Text, found ? ImVec4(1.0f, 0.5f, 0.5f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				ImGui::TextUnformatted(lines[line].data());
				ImGui::PopStyleColor();

				last_line = line;
				match_count -= found ? 1 : 0;
			}
		}
#endif
	}
	ImGui::PopStyleColor(3);
	return found;
}

////////////////////////////////////////////////////////////////////////////////
void PipelineViewer::RenderImGui(InterfacePipelineViewer& data) {
	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("Pipeline Viewer", &data.p_Visible, ImGuiWindowFlags_NoCollapse);

#ifdef NESHNY_WEBGPU

	auto existing_pipelines = Core::Singleton().GetPreparedPipelines();

	ImGuiTableFlags table_flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
	if (ImGui::BeginTable("##Table", 1, table_flags)) {

		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		for (const auto& pipeline_info : existing_pipelines) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", pipeline_info->GetIdentifier().data());
		}

		ImGui::EndTable();
	}

#endif

	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
void ResourceViewer::RenderImGui(InterfaceResourceViewer& data) {

	if (!data.p_Visible) {
		return;
	}
	auto& core = Core::Singleton();
	int ticks = core.GetTicks();

	ImGui::Begin("Resource Viewer", &data.p_Visible, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("CPU Memory Allocated: %lli", core.GetMemoryAllocated());
	ImGui::Text("Graphics Memory Allocated: %lli", core.GetGPUMemoryAllocated());
	ImGui::Text("Tick: %i", core.GetTicks());

	ImVec2 space_available = ImGui::GetWindowContentRegionMax();
	const int size_banner = 80;
	ImGui::SetCursorPos(ImVec2(8, size_banner));
	ImGui::BeginChild("List", ImVec2(space_available.x - 8, space_available.y - size_banner), false, ImGuiWindowFlags_HorizontalScrollbar);

	const auto& resources = Core::GetResources();

	ImGuiTableFlags table_flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
	if (ImGui::BeginTable("##Table", 7, table_flags)) {
		ImGui::TableSetupScrollFreeze(1, 1); // Make top row + left col always visible

		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 90);
		ImGui::TableSetupColumn("Mem", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("GPU Mem", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Ticks Since Use", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("Tick Created", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("Error", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		for (const auto& resource : resources) {

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", resource.first.c_str());

			ImGui::TableSetColumnIndex(1);
			std::string state = "Pending";
			if (resource.second.m_State == Core::ResourceState::DONE) {
				state = "Done";
			}
			else if (resource.second.m_State == Core::ResourceState::IN_ERROR) {
				state = "Error - " + resource.second.m_Error;
			}
			ImGui::Text("%s", state.c_str());

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%lli", resource.second.m_Memory);

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%lli", resource.second.m_GPUMemory);

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%i", ticks - resource.second.m_LastTickAccessed);

			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%i", resource.second.m_TickCreated);

			ImGui::TableSetColumnIndex(6);
			ImGui::Text("%s", resource.second.m_Error.c_str());
		}
		ImGui::EndTable();
	}

	ImGui::EndChild();
	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
Token Scrapbook2D::ActivateRTT(void) {
	auto& self = Singleton();
	bool reset = self.m_NeedsReset;
	self.m_NeedsReset = false;
#ifdef NESHNY_WEBGPU
	return self.m_RTT.Activate({ WebGPUPipeline::AttachmentMode::RGBA }, true, self.m_Width, self.m_Height, reset);
#else
	return self.m_RTT.Activate({ RTT::Mode::RGBA }, true, self.m_Width, self.m_Height, reset);
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Scrapbook2D::IRenderImGui(InterfaceScrapbook2D& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("2D Scrapbook", &data.p_Visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);


	if (ImGui::Button("Reset Camera")) {
		data.p_Cam = Camera2D{};
	}
	ImGui::Text("Pos: [%.3f, %.3f]", data.p_Cam.p_Pos.x, data.p_Cam.p_Pos.y);
	ImGui::Text("Size: [%i, %i]", m_Width, m_Height);
	if (m_LastMousePos.has_value()) {
		ImGui::Text("Mouse: [%.3f, %.3f]", m_LastMousePos->x, m_LastMousePos->y);

		//Vec2 screen = data.p_Cam.WorldToScreen(m_LastMousePos.value(), m_Width, m_Height);
		//ImGui::Text("Mouse Screen: [%.3f, %.3f]", screen.x, screen.y);
	}

	ImVec2 space_available = ImGui::GetWindowContentRegionMax();
	ImVec2 min_available = ImGui::GetWindowContentRegionMin();
	const int size_banner = 120;

	int user_width = (space_available.x - min_available.x) * 0.75;
	int user_height = size_banner - min_available.y - 5;
	ImGui::SetCursorPos(ImVec2(min_available.x + (space_available.x - user_width), min_available.y));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(32, 32, 32, 255));
	ImGui::BeginChild("UserControls", ImVec2(user_width, user_height), false, ImGuiWindowFlags_HorizontalScrollbar);
	 
	for (int i = 0; i < m_Controls.size(); i++) {
		m_Controls[i](user_width, user_height);
	}
	m_Controls.clear();
	ImGui::EndChild();
	ImGui::PopStyleColor();

	m_Width = space_available.x - 8;
	m_Height = space_available.y - size_banner;
	m_CachedViewPerspective = data.p_Cam.Get4x4Matrix(m_Width, m_Height);

	ImVec2 im_pos(8, size_banner);
	ImVec2 im_size(m_Width, m_Height);
	ImGui::SetCursorPos(im_pos);
	auto im_screen_pos = ImGui::GetCursorScreenPos();
	{
		auto token = ActivateRTT();
#if defined(NESHNY_GL)
		glEnable(GL_DEPTH_TEST);
#endif

		// todo: make this a checkbox
		const double size_grid = 1.0;
		Line(Vec2(0, 0), Vec2(size_grid, 0), Vec4(1, 0, 0, 1));
		Line(Vec2(0, 0), Vec2(0, size_grid), Vec4(0, 1, 0, 1));

#if defined(NESHNY_WEBGPU)
		IRender((WebGPURTT&)m_RTT, m_CachedViewPerspective, m_Width, m_Height, Vec3(0, 0, 0), 1.0, data.p_Cam.p_Zoom * 0.02);
#else
		IRender(m_CachedViewPerspective, m_Width, m_Height, Vec3(0, 0, 0), 1.0, data.p_Cam.p_Zoom * 0.02);
#endif
		IClear();
		m_NeedsReset = true;
	}

#if defined(NESHNY_GL)
	ImTextureID tex_id = (ImTextureID)(unsigned long long)m_RTT.GetColorTex(0);
	ImGui::Image(tex_id, im_size, ImVec2(0, 1), ImVec2(1, 0));
#else
	ImTextureID tex_id = (ImTextureID)(unsigned long long)m_RTT.GetColorTexView(0);
	ImGui::Image(tex_id, im_size, ImVec2(0, 0), ImVec2(1, 1));
#endif

	ImGui::SetCursorPos(im_pos);
	ImGui::InvisibleButton("##FullScreen", im_size);
	if (ImGui::IsItemHovered()) {
		const double ZOOM_SPEED = 0.1;
		const double ROTATE_SPEED = 0.2;

		ImGuiIO& io = ImGui::GetIO();
		ImVec2 mpos(ImGui::GetMousePos().x - im_screen_pos.x, ImGui::GetMousePos().y - im_screen_pos.y);
		m_LastMousePos = data.p_Cam.ScreenToWorld(Vec2(mpos.x, mpos.y), m_Width, m_Height);

		if (io.MouseWheel != 0.0f) {
			bool up = io.MouseWheel > 0.0f;
			data.p_Cam.Zoom(data.p_Cam.p_Zoom * (1.0 + (up ? -1 : 1) * ZOOM_SPEED), m_LastMousePos.value(), m_Width, m_Height);
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
			data.p_Cam.Pan(m_Width, io.MouseDelta.x, io.MouseDelta.y);
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			data.p_Cam.p_RotationAngle -= io.MouseDelta.x * ROTATE_SPEED;
		}

	} else {
		m_LastMousePos = std::nullopt;
	}

	for (auto& text : m_Texts) {
		auto screen_pos = data.p_Cam.WorldToScreen(text.p_Pos, m_Width, m_Height);
		ImGui::SetCursorPos(ImVec2(screen_pos.x + im_pos.x, screen_pos.y + im_pos.y));
		ImGuiTextColoredUnformatted(text.p_Text, ImVec4(text.p_Col.x, text.p_Col.y, text.p_Col.z, text.p_Col.w));
	}
	m_Texts.clear();

	ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
Token Scrapbook3D::ActivateRTT(void) {
	auto& self = Singleton();
	bool reset = self.m_NeedsReset;
	self.m_NeedsReset = false;

#ifdef NESHNY_WEBGPU
	return self.m_RTT.Activate({ WebGPUPipeline::AttachmentMode::RGBA }, true, self.m_Width, self.m_Height, reset);
#else
	return self.m_RTT.Activate({ RTT::Mode::RGBA }, true, self.m_Width, self.m_Height, reset);
#endif
}

////////////////////////////////////////////////////////////////////////////////
void Scrapbook3D::IRenderImGui(InterfaceScrapbook3D& data) {

	if (!data.p_Visible) {
		return;
	}

	ImGui::Begin("3D Scrapbook", &data.p_Visible, ImGuiWindowFlags_NoCollapse);

	if (ImGui::Button("Reset Camera")) {
		data.p_Cam = Camera3DOrbit{ Vec3(), 100, 30, 30 };
	}

	ImVec2 space_available = ImGui::GetWindowContentRegionMax();
	ImVec2 min_available = ImGui::GetWindowContentRegionMin();
	const int size_banner = 120;

	int user_width = (space_available.x - min_available.x) * 0.75;
	int user_height = size_banner - min_available.y - 5;
	ImGui::SetCursorPos(ImVec2(min_available.x + (space_available.x - user_width), min_available.y));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(32, 32, 32, 255));
	ImGui::BeginChild("UserControls", ImVec2(user_width, user_height), false, ImGuiWindowFlags_HorizontalScrollbar);

	for (int i = 0; i < m_Controls.size(); i++) {
		m_Controls[i](user_width, user_height);
	}
	m_Controls.clear();
	ImGui::EndChild();
	ImGui::PopStyleColor();

	m_Width = space_available.x - 8;
	m_Height = space_available.y - size_banner;
	m_CachedViewPerspective = data.p_Cam.GetViewPerspectiveMatrix(m_Width, m_Height);

	ImVec2 im_pos(8, size_banner);
	ImVec2 im_size(m_Width, m_Height);
	ImGui::SetCursorPos(im_pos);
	{
		auto token = ActivateRTT();
#if defined(NESHNY_GL)
		glEnable(GL_DEPTH_TEST);
#endif
		// todo: make this a checkbox
		const double size_grid = 10.0;
		AddLine(Vec3(0, 0, 0), Vec3(size_grid, 0, 0), Vec4(1, 0, 0, 1));
		AddLine(Vec3(0, 0, 0), Vec3(0, size_grid, 0), Vec4(0, 1, 0, 1));
		AddLine(Vec3(0, 0, 0), Vec3(0, 0, size_grid), Vec4(0, 0, 1, 1));

#if defined(NESHNY_WEBGPU)
		IRender((WebGPURTT&)m_RTT, m_CachedViewPerspective, m_Width, m_Height, Vec3(0, 0, 0), 1.0);
#else
		IRender(m_CachedViewPerspective, m_Width, m_Height, Vec3(0, 0, 0), 1.0);
#endif

		IClear();
		m_NeedsReset = true;
	}

#if defined(NESHNY_WEBGPU)
	ImTextureID tex_id = (ImTextureID)(unsigned long long)m_RTT.GetColorTexView(0);
#else
	ImTextureID tex_id = (ImTextureID)(unsigned long long)m_RTT.GetColorTex(0);
#endif
	ImGui::Image(tex_id, im_size, ImVec2(0, 1), ImVec2(1, 0));

	ImGui::SetCursorPos(im_pos);
	ImGui::InvisibleButton("##FullScreen", im_size);
	if (ImGui::IsItemHovered()) {
		const double ZOOM_SPEED = 0.1;
		const double ANGLE_SPEED = 0.2;

		ImGuiIO& io = ImGui::GetIO();
		if (io.MouseWheel != 0.0f) {
			bool up = io.MouseWheel > 0.0f;
			data.p_Cam.p_Zoom *= 1.0 + (up ? -1 : 1) * ZOOM_SPEED;
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			data.p_Cam.p_HorizontalDegrees += io.MouseDelta.x * ANGLE_SPEED;
			data.p_Cam.p_VerticalDegrees += io.MouseDelta.y * ANGLE_SPEED;
		}
	}

	ImGui::End();
}

} // namespace Neshny