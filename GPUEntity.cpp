////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
bool GPUEntity::Init(int expected_max_count) {

	int max_size = expected_max_count * m_NumDataFloats * sizeof(float);
#if defined(NESHNY_GL)
	m_SSBO = new SSBO(max_size);
	if (m_DoubleBuffering) {
		m_OutputSSBO = new SSBO(max_size);
	}

	m_ControlSSBO = new SSBO();
	m_FreeList = new SSBO();
#elif defined(NESHNY_WEBGPU)
	m_SSBO = new SSBO(WGPUBufferUsage_Storage, max_size);
	if (m_DoubleBuffering) {
		m_OutputSSBO = new SSBO(WGPUBufferUsage_Storage, max_size);
	}

	m_ControlSSBO = new SSBO(WGPUBufferUsage_Storage);
	m_FreeList = new SSBO(WGPUBufferUsage_Storage);
#endif
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
int GPUEntity::AddInstance(void* data, int* index) {

	int id = m_NextId;
	*((int*)data) = id; // todo: can this be a specified position
	int creation_index = m_MaxIndex;

	if ((m_DeleteMode == DeleteMode::STABLE_WITH_GAPS) && (m_FreeCount > 0)) {
		creation_index = m_FreeList->GetSingleValue<int>(m_FreeCount - 1);
		m_FreeCount--;
	} else {
		m_MaxIndex++;
	}

	int base = creation_index * m_NumDataFloats;
	m_SSBO->Write((unsigned char*)data, base * sizeof(float), m_NumDataFloats * sizeof(float));
	if (index) {
		*index = creation_index;
	}

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

		m_SSBO->Write((unsigned char*)&id_value, index * size_item + pos_index, sizeof(float));
		m_FreeList->EnsureSizeBytes((m_FreeCount + 1) * sizeof(int), false);
		m_FreeList->Write((unsigned char*)&index, m_FreeCount * sizeof(int), sizeof(int));
		m_FreeCount++;
	} else {
		if (index != (m_MaxIndex - 1)) {
			// copy the end item into this pos
#if defined(NESHNY_GL)
			glCopyNamedBufferSubData(m_SSBO->Get(), m_SSBO->Get(), (m_MaxIndex - 1) * size_item, index * size_item, size_item);
#elif defined(NESHNY_WEBGPU)
			Core::CopyBufferToBuffer(m_SSBO->Get(), m_SSBO->Get(), (m_MaxIndex - 1) * size_item, index * size_item, size_item);
#endif
		}
		m_MaxIndex--;
	}

	m_CurrentCount--;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessMoveDeaths(int death_count) {

	// todo: for each death, take index d from alive and copy it
	m_ControlSSBO->EnsureSizeBytes(sizeof(int));

#if defined(NESHNY_GL)

	QString defines = QString("#define FLOATS_PER %1").arg(m_NumDataFloats);

	GLShader* death_prog = Core::GetComputeShader("Death", defines);
	death_prog->UseProgram();

	m_ControlSSBO->Bind(0);
	m_FreeList->Bind(1);

	m_SSBO->Bind(2);
	glUniform1i(death_prog->GetUniform("uLifeCount"), m_MaxIndex);

	Core::DispatchMultiple(death_prog, death_count, 512);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#elif defined(NESHNY_WEBGPU)
	#error finish this - who owns the pipeline? makes sense for it to be global?
#endif

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
	SSBO* tmp = m_SSBO;
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


#if defined(NESHNY_GL)
	if (!m_CopyBuffer) {
		m_CopyBuffer = new SSBO();
	}
	m_CopyBuffer->EnsureSizeBytes(size, false);
	glCopyNamedBufferSubData(m_SSBO->Get(), m_CopyBuffer->Get(), offset, 0, size);
	glGetNamedBufferSubData(m_CopyBuffer->Get(), 0, size, ptr);
#elif defined(NESHNY_WEBGPU)

	#error finish this
#endif

}

} // namespace Neshny