////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
bool GPUEntity::Init(int expected_max_count) {

	Destroy();
	m_MaxItems = expected_max_count;

	int max_size = (expected_max_count * m_NumDataFloats + ENTITY_OFFSET_INTS) * sizeof(int);
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

	m_ControlSSBO = new SSBO(WGPUBufferUsage_Storage, sizeof(int)); // prevent zero sized buffer error
	m_FreeList = new SSBO(WGPUBufferUsage_Storage, expected_max_count * sizeof(int));
#endif

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::Clear(void) {
	m_Info.p_Count = 0;
	m_Info.p_MaxIndex = 0;
	m_Info.p_NextId = 0;
	m_Info.p_FreeCount = 0;
#if defined(NESHNY_WEBGPU)
	if (m_SSBO) {
		m_SSBO->Write((unsigned char*)&m_Info, 0, sizeof(EntityInfo));
	}
#endif
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
void GPUEntity::AddInstancesInternal(unsigned char* data, int item_count, int item_size) {

#if defined(NESHNY_GL)
	// TODO: this could be much more efficient, or converted to shader just like webgpu
	for (int i = 0; i < item_count; i++) {
		AddInstance(data);
		data += item_size;
	}
#elif defined(NESHNY_WEBGPU)

	int data_size = item_count * item_size;

	struct CreatePipeObjects {
		CreatePipeObjects(int size) : p_Data(WGPUBufferUsage_Storage, size) {}
		WebGPUPipeline	p_Pipe;
		WebGPUBuffer	p_Data;
	};

	// global per-thread object since this will get reused many times
	static thread_local CreatePipeObjects* create_obj = nullptr; // leaks at end of run, not important
	if (!create_obj) {
		create_obj = new CreatePipeObjects(data_size + sizeof(int));
		create_obj->p_Pipe
			.AddBuffer(*m_SSBO, WGPUShaderStage_Compute, false)
			.AddBuffer(*m_FreeList, WGPUShaderStage_Compute, true)
			.AddBuffer(create_obj->p_Data, WGPUShaderStage_Compute, true)
			.FinalizeCompute("EntityCreation", QString("#define ENTITY_OFFSET_INTS %1\n").arg(ENTITY_OFFSET_INTS).toLocal8Bit());
	}

	struct Info {
		int p_Count;
		int p_ItemInts;
		int m_IdOffsetInts;
	};
	Info info = {
		item_count,
		item_size / (int)sizeof(int),
		m_IdOffset / (int)sizeof(int)
	};

	create_obj->p_Data.EnsureSizeBytes(sizeof(Info) + data_size);

	// TODO: collapse to one write command
	create_obj->p_Data.Write((unsigned char*)&info, 0, sizeof(Info));
	create_obj->p_Data.Write(data, sizeof(Info), data_size);

	create_obj->p_Pipe.ReplaceBuffer(0, *m_SSBO);
	create_obj->p_Pipe.ReplaceBuffer(1, *m_FreeList);
	create_obj->p_Pipe.ReplaceBuffer(2, create_obj->p_Data);
	create_obj->p_Pipe.Compute(item_count, iVec3(256, 1, 1));
#endif
}

////////////////////////////////////////////////////////////////////////////////
int GPUEntity::AddInstance(void* data, int* index) {

	// TODO: disable this for webgpu

	int id = m_Info.p_NextId;
	*((int*)data) = id; // todo: can this be a specified position
	int creation_index = m_Info.p_MaxIndex;

	if ((m_DeleteMode == DeleteMode::STABLE_WITH_GAPS) && (m_Info.p_FreeCount > 0)) {
		creation_index = m_FreeList->GetSingleValue<int>(m_Info.p_FreeCount - 1);
		m_Info.p_FreeCount--;
	} else {
		m_Info.p_MaxIndex++;
	}

	int base = creation_index * m_NumDataFloats;
	m_SSBO->Write((unsigned char*)data, (base + ENTITY_OFFSET_INTS) * sizeof(float), m_NumDataFloats * sizeof(float));
	if (index) {
		*index = creation_index;
	}

	m_Info.p_Count++;
	m_Info.p_NextId++;
	return id;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::DeleteInstance(int index) {

	// TODO: needs webgpu conversion

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

		m_SSBO->Write((unsigned char*)&id_value, index * size_item + pos_index + ENTITY_OFFSET_INTS * sizeof(int), sizeof(float));
		m_FreeList->EnsureSizeBytes((m_Info.p_FreeCount + 1) * sizeof(int), false);
		m_FreeList->Write((unsigned char*)&index, m_Info.p_FreeCount * sizeof(int), sizeof(int));
		m_Info.p_FreeCount++;
	} else {
		if (index != (m_Info.p_MaxIndex - 1)) {
			// copy the end item into this pos
#if defined(NESHNY_GL)
			glCopyNamedBufferSubData(m_SSBO->Get(), m_SSBO->Get(), (m_Info.p_MaxIndex - 1) * size_item, index * size_item, size_item);
#elif defined(NESHNY_WEBGPU)
			Core::CopyBufferToBuffer(m_SSBO->Get(), m_SSBO->Get(), (m_Info.p_MaxIndex - 1) * size_item + ENTITY_OFFSET_INTS * sizeof(int), index * size_item + ENTITY_OFFSET_INTS * sizeof(int), size_item);
#endif
		}
		m_Info.p_MaxIndex--;
	}

	m_Info.p_Count--;
}

#if defined(NESHNY_GL)

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessMoveDeaths(int death_count) {

	// todo: for each death, take index d from alive and copy it
	m_ControlSSBO->EnsureSizeBytes(sizeof(int));

	QString defines = QString("#define FLOATS_PER %1").arg(m_NumDataFloats);

	GLShader* death_prog = Core::GetComputeShader("Death", defines);
	death_prog->UseProgram();

	m_ControlSSBO->Bind(0);
	m_FreeList->Bind(1);

	m_SSBO->Bind(2);
	glUniform1i(death_prog->GetUniform("uLifeCount"), m_Info.p_MaxIndex);

	Core::DispatchMultiple(death_prog, death_count, 512);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	m_Info.p_MaxIndex -= death_count;
	m_Info.p_Count -= death_count;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessMoveCreates(int new_count, int new_next_id) {
	// todo: for each creation, copy data out if it's asked for
	m_Info.p_Count = new_count;
	m_Info.p_MaxIndex = new_count;
	m_Info.p_NextId = new_next_id;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessStableDeaths(int death_count) {
	m_Info.p_Count -= death_count;
	m_Info.p_FreeCount += death_count;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::ProcessStableCreates(int new_max_index, int new_next_id, int new_free_count) {
	// todo: for each creation, copy data out if it's asked for

	int added_num = m_Info.p_FreeCount - new_free_count;
	m_Info.p_FreeCount = std::max(new_free_count, 0);
	m_Info.p_Count += added_num;
	m_Info.p_MaxIndex = new_max_index;
	m_Info.p_NextId = new_next_id;
}
#endif

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
	return QString("# %1 mx %2 id %3 free %4").arg(m_Info.p_Count).arg(m_Info.p_MaxIndex).arg(m_Info.p_NextId).arg(m_Info.p_FreeCount);
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<unsigned char[]> GPUEntity::MakeCopySync(void) {
	const int entity_size = m_NumDataFloats * sizeof(float);
	const int offset_size = ENTITY_OFFSET_INTS * sizeof(int);
	const int size = m_MaxItems * entity_size + offset_size;
	unsigned char* ptr = new unsigned char[size];
	MakeCopyIn(ptr, 0, size);
	auto result = std::shared_ptr<unsigned char[]>(ptr);
	return result;
}

#if defined(NESHNY_WEBGPU)
////////////////////////////////////////////////////////////////////////////////
void GPUEntity::AccessData(std::function<void(unsigned char* data, int size_bytes, int item_count)>&& callback) {

	m_SSBO->Read(0, m_SSBO->GetSizeBytes(), [this, callback](unsigned char* data, int size) -> std::shared_ptr<void> {
		int max_index = ((int*)data)[3];
		const int entity_size = m_NumDataFloats * sizeof(int);
		const int offset_size = ENTITY_OFFSET_INTS * sizeof(int);
		const int useful_size = max_index * entity_size + offset_size;

		callback(data, useful_size, max_index);
		return nullptr;
	});
}
#endif

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
	m_SSBO->ReadSync(ptr, offset, size);
#endif

}

} // namespace Neshny