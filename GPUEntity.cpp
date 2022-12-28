////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
bool GPUEntity::Init(int expected_max_count) {

	int max_size = expected_max_count * m_NumDataFloats * sizeof(float);
	m_SSBO = new GLSSBO(max_size);
	if (m_DoubleBuffering) {
		m_OutputSSBO = new GLSSBO(max_size);
	}

	m_ControlSSBO = new GLSSBO();
	m_FreeList = new GLSSBO();

	m_CurrentCount = 0;
	m_MaxIndex = 0;
	m_NextId = 0;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::Clear(void) {
	m_CurrentCount = 0;
	m_MaxIndex = 0;
	m_NextId = 0;
	m_FreeCount = 0;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::Destroy(void) {
	Clear();
	if (m_SSBO) {
		delete m_SSBO;
		m_SSBO = nullptr;
	}
	if (m_OutputSSBO) {
		delete m_OutputSSBO;
		m_OutputSSBO = nullptr;
	}
	if (m_ControlSSBO) {
		delete m_ControlSSBO;
		m_ControlSSBO = nullptr;
	}
	if (m_FreeList) {
		delete m_FreeList;
		m_FreeList = nullptr;
	}
	if (m_CopyBuffer) {
		delete m_CopyBuffer;
		m_CopyBuffer = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////
int GPUEntity::AddInstance(void* data) {

	int id = m_NextId;
	*((int*)data) = id; // todo: can this be a specified position
	int creation_index = m_MaxIndex;

	if ((m_DeleteMode == DeleteMode::STABLE_WITH_GAPS) && (m_FreeCount > 0)) {
		glGetNamedBufferSubData(m_FreeList->Get(), (m_FreeCount - 1) * sizeof(float), sizeof(float), (void*)&creation_index);
		m_FreeCount--;
	} else {
		m_MaxIndex++;
	}

	int base = creation_index * m_NumDataFloats;
	glNamedBufferSubData(m_SSBO->Get(), base * sizeof(float), m_NumDataFloats * sizeof(float), data);

	m_CurrentCount++;
	m_NextId++;
	return id;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::DeleteInstance(int index) {

	int size_item = m_NumDataFloats * sizeof(float);
	if (m_DeleteMode == DeleteMode::STABLE_WITH_GAPS) {

		int pos_index = 0;
		for (const auto& member : m_Specs.p_Members) {
			if (member.p_Name == m_IDName) {
				break;
			}
			pos_index += member.p_Size;
		}

		int id_value = -1;
		glNamedBufferSubData(m_SSBO->Get(), index * size_item + pos_index, sizeof(float), (unsigned char*)&id_value);
		m_FreeList->EnsureSize((m_FreeCount + 1) * sizeof(int), false);
		glNamedBufferSubData(m_FreeList->Get(), m_FreeCount * sizeof(int), sizeof(int), (unsigned char*)&index);
		m_FreeCount++;
	} else {
		if (index != (m_MaxIndex - 1)) {
			// copy the end item into this pos
			glCopyNamedBufferSubData(m_SSBO->Get(), m_SSBO->Get(), (m_MaxIndex - 1) * size_item, index * size_item, size_item);
		}
		m_MaxIndex--;
	}

	m_CurrentCount--;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessMoveDeaths(int death_count) {

	// todo: for each death, take index d from alive and copy it
	m_ControlSSBO->EnsureSize(sizeof(int));

	QString defines = QString("#define FLOATS_PER %1").arg(m_NumDataFloats);
	defines += "\n#define USE_SSBO\n";

	GLShader* death_prog = Neshny::GetComputeShader("Death", defines);
	death_prog->UseProgram();

	m_ControlSSBO->Bind(0);
	m_FreeList->Bind(1);

	m_SSBO->Bind(2);
	glUniform1i(death_prog->GetUniform("uLifeCount"), m_MaxIndex);

	Neshny::DispatchMultiple(death_prog, death_count);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	m_MaxIndex -= death_count;
	m_CurrentCount -= death_count;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessStableDeaths(int death_count) {
	m_CurrentCount -= death_count;
	m_FreeCount += death_count;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessMoveCreates(int new_count, int new_next_id) {
	// todo: for each creation, copy data out if it's asked for
	m_CurrentCount = new_count;
	m_MaxIndex = new_count;
	m_NextId = new_next_id;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessStableCreates(int new_max_index, int new_next_id, int new_free_count) {
	// todo: for each creation, copy data out if it's asked for

	int added_num = m_FreeCount - new_free_count;
	m_FreeCount = std::max(new_free_count, 0);
	m_CurrentCount += added_num;
	m_MaxIndex = new_max_index;
	m_NextId = new_next_id;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::SwapInputOutputSSBOs(void) {
	if (!m_DoubleBuffering) {
		return;
	}
	GLSSBO* tmp = m_SSBO;
	m_SSBO = m_OutputSSBO;
	m_OutputSSBO = tmp;
}

////////////////////////////////////////////////////////////////////////////////
QString GPUEntity::GetDebugInfo(void) {
	return QString("# %1 mx %2 id %3 free %4").arg(m_CurrentCount).arg(m_MaxIndex).arg(m_NextId).arg(m_FreeCount);
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<unsigned char[]> GPUEntity::MakeCopy(void) {

	const int entity_size = m_NumDataFloats * sizeof(float);
	const int size = m_MaxIndex * entity_size;
	unsigned char* ptr = new unsigned char[size];
	MakeCopyIn(ptr, 0, size);
	auto result = std::shared_ptr<unsigned char[]>(ptr);
	return result;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::MakeCopyIn(unsigned char* ptr, int offset, int size) {
	if (!m_CopyBuffer) {
		m_CopyBuffer = new GLSSBO();
	}
	m_CopyBuffer->EnsureSize(size, false);
	glCopyNamedBufferSubData(m_SSBO->Get(), m_CopyBuffer->Get(), offset, 0, size);
	glGetNamedBufferSubData(m_CopyBuffer->Get(), 0, size, ptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Token RTT::Activate(std::vector<Mode> color_attachments, bool capture_depth_stencil, int width, int height, bool clear) {

	GLint draw_fbo;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo);

	bool modes_same = color_attachments.size() == m_Modes.size();
	for (int i = 0; modes_same && (i < color_attachments.size()); i++) {
		modes_same = modes_same && (m_Modes[i] == color_attachments[i]);
	}

	// set up the stuff if it doesn't exist
	if ((!modes_same) || (capture_depth_stencil != m_CaptureDepthStencil) || (m_Width != width) || (m_Height != height)) {

		Destroy();
		m_Modes = color_attachments;
		m_Width = width;
		m_Height = height;
		m_CaptureDepthStencil = capture_depth_stencil;
		if (m_Modes.empty()) {
			return Token();
		}
		clear = true;

		glGenFramebuffers(1, &m_FrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

		std::vector<unsigned int> attachments;
		for (int i = 0; i < m_Modes.size(); i++) {
			auto mode = m_Modes[i];
			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			if (mode == Mode::RGBA_FLOAT32) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
			} else if(mode == Mode::RGBA_FLOAT16) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			unsigned int attach = GL_COLOR_ATTACHMENT0 + i;
			attachments.push_back(attach);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attach, GL_TEXTURE_2D, tex, 0);
			m_ColorTextures.push_back(tex);
		}
		glDrawBuffers((int)attachments.size(), &attachments[0]);

		if (m_CaptureDepthStencil) {
			glGenTextures(1, &m_DepthTex);
			glBindTexture(GL_TEXTURE_2D, m_DepthTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Width, m_Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);
		} else {
			glGenRenderbuffers(1, &m_DepthBuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);
		}

		auto state = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (state != GL_FRAMEBUFFER_COMPLETE) {
			auto state = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			bool FRAMEBUFFER_UNSUPPORTED = (state == GL_FRAMEBUFFER_UNSUPPORTED);
			bool FRAMEBUFFER_UNDEFINED = (state == GL_FRAMEBUFFER_UNDEFINED);
			bool FRAMEBUFFER_INCOMPLETE_ATTACHMENT = (state == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
			bool FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = (state == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
			bool FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER = (state == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
			bool FRAMEBUFFER_INCOMPLETE_READ_BUFFER = (state == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
			bool FRAMEBUFFER_INCOMPLETE_MULTISAMPLE = (state == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
			bool FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS = (state == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);

			glBindFramebuffer(GL_FRAMEBUFFER, draw_fbo);
			return Token();
		}
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
	}
	if (clear) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	glViewport(0, 0, width, height);
	return Token([draw_fbo]() {
		glBindFramebuffer(GL_FRAMEBUFFER, draw_fbo);
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RTT::Destroy(void) {

	if (m_FrameBuffer) {
		glDeleteFramebuffers(1, &m_FrameBuffer);
		m_FrameBuffer = 0;
	}
	for (auto col : m_ColorTextures) {
		glDeleteTextures(1, &col);
	}
	m_ColorTextures.clear();
	if (m_DepthTex) {
		glDeleteTextures(1, &m_DepthTex);
		m_DepthTex = 0;
	}
	if (m_DepthBuffer) {
		glDeleteRenderbuffers(1, &m_DepthBuffer);
		m_DepthBuffer = 0;
	}
}
