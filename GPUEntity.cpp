////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
bool GPUEntity::Init(void) {

	if (m_StoreMode == StoreMode::SSBO) {
		m_SSBO = new GLSSBO(BUFFER_TEX_SIZE * BUFFER_TEX_SIZE * sizeof(float)); // TODO: what is the deal with this size? should be somewhat specified
	} else {
		if (m_Texture) {
			return false;
		}

		glGenTextures(1, &m_Texture);
		glBindTexture(GL_TEXTURE_2D, m_Texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, BUFFER_TEX_SIZE, BUFFER_TEX_SIZE, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		float clear_val = 0;
		glClearTexImage(m_Texture, 0, GL_RED, GL_FLOAT, &clear_val);
	}
	if (m_DeleteMode == DeleteMode::STABLE_WITH_GAPS) {
		m_FreeList = new GLSSBO(BUFFER_TEX_SIZE * sizeof(float)); // TODO: what is the deal with this size? should be somewhat specified
	}

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
	if (m_Texture) {
		glDeleteTextures(1, &m_Texture);
		m_Texture = 0;
	}
	if (m_SSBO) {
		delete m_SSBO;
		m_SSBO = nullptr;
	}
	if (m_FreeList) {
		delete m_FreeList;
		m_FreeList = nullptr;
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

	if (m_SSBO) {
		int base = creation_index * m_NumDataFloats;
		glNamedBufferSubData(m_SSBO->Get(), base * sizeof(float), m_NumDataFloats * sizeof(float), data);
	} else {
		int x = creation_index % BUFFER_TEX_SIZE;
		int y = creation_index / BUFFER_TEX_SIZE;
		glTextureSubImage2D(m_Texture, 0, x, y * m_NumDataFloats, 1, m_NumDataFloats, GL_RED, GL_FLOAT, data);
	}

	m_CurrentCount++;
	m_NextId++;
	return id;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessMoveDeaths(int death_count, GLSSBO& death_indices, GLSSBO& control_buffer) {

	// todo: for each death, take index d from alive and copy it
	//return;
	Core& core = Core::Singleton();
	control_buffer.EnsureSize(sizeof(int));

	QString defines = QString("#define FLOATS_PER %1").arg(m_NumDataFloats);
	if (m_SSBO) {
		defines += "\n#define USE_SSBO\n";
	} else {
		defines += QString("\n#define BUFFER_TEX_SIZE %1\n").arg(BUFFER_TEX_SIZE);
	}

	GLShader* death_prog = core.GetComputeShader("Death", defines);
	death_prog->UseProgram();

	control_buffer.Bind(0);
	death_indices.Bind(1);

	if (m_SSBO) {
		m_SSBO->Bind(2);
	} else {
		glBindImageTexture(0, m_Texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
		glUniform1i(death_prog->GetUniform("uTexture"), 0);
	}
	glUniform1i(death_prog->GetUniform("uLifeCount"), m_MaxIndex);

	Core::DispatchMultiple(death_prog, death_count);
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
QString GPUEntity::GetDebugInfo(void) {
	return QString("# %1 mx %2 id %3 free %4").arg(m_CurrentCount).arg(m_MaxIndex).arg(m_NextId).arg(m_FreeCount);
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<unsigned char[]> GPUEntity::MakeCopy(void) {

	const int entity_size = m_NumDataFloats * sizeof(float);
	const int size = m_MaxIndex * entity_size;
	if (m_SSBO) {
		unsigned char* ptr = new unsigned char[size];
		glGetNamedBufferSubData(m_SSBO->Get(), 0, size, ptr);
		auto result = std::shared_ptr<unsigned char[]>(ptr);
		return result;
	}

	const int row_size = BUFFER_TEX_SIZE * entity_size;
	const int rows = (int)ceil((double)m_MaxIndex / BUFFER_TEX_SIZE);
	unsigned char* ptr = new unsigned char[size];

	unsigned char* row_ptr = new unsigned char[row_size];

	int entire_size = BUFFER_TEX_SIZE * BUFFER_TEX_SIZE * sizeof(float);
	unsigned char* entire_ptr = new unsigned char[entire_size];
	glGetTextureImage(m_Texture, 0, GL_RED, GL_FLOAT, entire_size, entire_ptr);
	delete[] entire_ptr;

	//GLenum err;
	//while ((err = glGetError()) != GL_NO_ERROR)
	//{
	//	bool INVALID_ENUM = (err == GL_INVALID_ENUM);
	//	bool INVALID_VALUE = (err == GL_INVALID_VALUE);
	//	bool INVALID_OPERATION = (err == GL_INVALID_OPERATION);
	//	int brk = 0;
	//}

	int entity_num = 0;
	for (int r = 0; r < rows; r++) {

		glGetTextureSubImage(m_Texture, 0, 0, 0, 0, BUFFER_TEX_SIZE, 1, 1, GL_RED, GL_FLOAT, row_size, row_ptr);
	
		// de-stride
		for (int c = 0; c < BUFFER_TEX_SIZE; c++) {
			if (entity_num >= m_MaxIndex) {
				break;
			}
			for (int i = 0; i < m_NumDataFloats; i++) {
				unsigned char* float_ptr = row_ptr + (i * BUFFER_TEX_SIZE + c) * sizeof(float);
				memcpy(ptr + entity_num * entity_size + i * sizeof(float), float_ptr, sizeof(float));
			}

			entity_num--;
		}
	}

	delete[] row_ptr;
	auto result = std::shared_ptr<unsigned char[]>(ptr);
	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Token RTT::Activate(Mode mode, int width, int height) {

	GLint draw_fbo;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo);

	// set up the stuff if it doesn't exist
	if ((m_Mode != mode) || (m_Width != width) || (m_Height != height)) {

		Destroy();
		m_Mode = mode;
		m_Width = width;
		m_Height = height;
		if (mode == Mode::NONE) {
			return Token();
		}

		glGenFramebuffers(1, &m_FrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

		if (mode != Mode::DEPTH_STENCIL) {
			glGenTextures(1, &m_ColorTex);
			glBindTexture(GL_TEXTURE_2D, m_ColorTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTex, 0);
		} else {

			//glGenRenderbuffers(1, &m_ColorBuffer);
			//glBindRenderbuffer(GL_RENDERBUFFER, m_ColorBuffer);
			//glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, m_Width, m_Height);
			//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_ColorBuffer);
		}

		if ((mode == Mode::RGBA_DEPTH_STENCIL) || (mode == Mode::DEPTH_STENCIL)) {

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

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			//auto state = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			//bool FRAMEBUFFER_UNSUPPORTED = (state == GL_FRAMEBUFFER_UNSUPPORTED);
			glBindFramebuffer(GL_FRAMEBUFFER, draw_fbo);
			return Token();
		}
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
	if (m_ColorTex) {
		glDeleteTextures(1, &m_ColorTex);
		m_ColorTex = 0;
	}
	if (m_DepthTex) {
		glDeleteTextures(1, &m_DepthTex);
		m_DepthTex = 0;
	}
	if (m_ColorBuffer) {
		glDeleteRenderbuffers(1, &m_ColorBuffer);
		m_ColorBuffer = 0;
	}
	if (m_DepthBuffer) {
		glDeleteRenderbuffers(1, &m_DepthBuffer);
		m_DepthBuffer = 0;
	}
}
