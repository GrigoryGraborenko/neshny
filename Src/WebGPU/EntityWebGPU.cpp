////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
bool GPUEntity::Init(int expected_max_count) {

	Destroy();
	m_MaxItems = expected_max_count;

	int max_size = (expected_max_count * m_NumDataFloats + ENTITY_OFFSET_INTS) * sizeof(int);
	m_SSBO = new SSBO(WGPUBufferUsage_Storage, max_size);
	if (m_DoubleBuffering) {
		m_OutputSSBO = new SSBO(WGPUBufferUsage_Storage, max_size);
	}

	m_ControlSSBO = new SSBO(WGPUBufferUsage_Storage, sizeof(int)); // prevent zero sized buffer error
	m_FreeList = new SSBO(WGPUBufferUsage_Storage, expected_max_count * sizeof(int));

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::Clear(void) {
	m_LastKnownInfo.p_Count = 0;
	m_LastKnownInfo.p_MaxIndex = 0;
	m_LastKnownInfo.p_NextId = 0;
	m_LastKnownInfo.p_FreeCount = 0;
	if (m_SSBO) {
		m_SSBO->Write((unsigned char*)&m_LastKnownInfo, 0, sizeof(EntityInfo));
	}
	m_Pending.clear();
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
void GPUEntity::AddInstancesInternal(unsigned char* data, int item_count, int item_size, std::vector<PlacementInfo>* sync_placements) {

	int data_size = item_count * item_size;
	int index_id_size = sizeof(PlacementInfo) * item_count;

	struct CreatePipeObjects {
		CreatePipeObjects(int size) : p_Data(WGPUBufferUsage_Storage, size), p_IndexIdData(WGPUBufferUsage_Storage) {}
		WebGPUPipeline	p_Pipe;
		WebGPUBuffer	p_Data;
		WebGPUBuffer	p_IndexIdData;
	};

	// global per-thread object since this will get reused many times
	static thread_local CreatePipeObjects* create_obj = nullptr; // leaks at end of run, not important
	if (!create_obj) {
		create_obj = new CreatePipeObjects(data_size + sizeof(int));
		create_obj->p_Pipe
			.AddBuffer(*m_SSBO, WGPUShaderStage_Compute, false)
			.AddBuffer(*m_FreeList, WGPUShaderStage_Compute, true)
			.AddBuffer(create_obj->p_Data, WGPUShaderStage_Compute, true)
			.AddBuffer(create_obj->p_IndexIdData, WGPUShaderStage_Compute, false)
			.FinalizeCompute("EntityCreation", std::format("#define ENTITY_OFFSET_INTS {}\n", ENTITY_OFFSET_INTS));
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

	create_obj->p_Data.EnsureSizeBytes(sizeof(Info) + data_size, false);
	create_obj->p_IndexIdData.EnsureSizeBytes(index_id_size, false);

	// TODO: collapse to one write command
	create_obj->p_Data.Write((unsigned char*)&info, 0, sizeof(Info));
	create_obj->p_Data.Write(data, sizeof(Info), data_size);

	create_obj->p_Pipe.ReplaceBuffer(0, *m_SSBO);
	create_obj->p_Pipe.ReplaceBuffer(1, *m_FreeList);
	create_obj->p_Pipe.ReplaceBuffer(2, create_obj->p_Data);
	create_obj->p_Pipe.ReplaceBuffer(3, create_obj->p_IndexIdData);
#pragma msg("does this work for large numbers?")

	create_obj->p_Pipe.Compute(item_count, iVec3(256, 1, 1));

	m_LastKnownInfo.p_Count += item_count;

	if (sync_placements && (item_count > 0)) {
		sync_placements->resize(item_count);
		create_obj->p_IndexIdData.ReadSync((unsigned char*)sync_placements->data(), 0, index_id_size);
	}
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::SetInstancesInternal(unsigned char* data, int item_count, int item_size) {

	if (!m_SSBO) {
		return;
	}
	m_LastKnownInfo.p_Count = item_count;
	m_LastKnownInfo.p_MaxIndex = item_count;
	m_LastKnownInfo.p_NextId = item_count;
	m_LastKnownInfo.p_FreeCount = 0;

#pragma msg("might be good to auto-assign ids here, otherwise user is responsible for setting it from 0...N - would need offset though")

	int data_size = item_count * item_size;
	int total_size = sizeof(EntityInfo) + data_size;
	std::vector<std::byte> buffer(total_size, std::byte(0));
	memcpy(buffer.data(), &m_LastKnownInfo, sizeof(EntityInfo));
	memcpy(buffer.data() + sizeof(EntityInfo), data, data_size);

	m_SSBO->Write((unsigned char*)buffer.data(), 0, buffer.size());
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::DeleteInstance(int index) {

	// TODO: needs webgpu conversion
#pragma msg("make a shader-based version of this")

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
std::shared_ptr<unsigned char[]> GPUEntity::MakeCopySync(void) {
	const int entity_size = m_NumDataFloats * sizeof(float);
	const int offset_size = ENTITY_OFFSET_INTS * sizeof(int);
	const int size = m_MaxItems * entity_size + offset_size;
	unsigned char* ptr = new unsigned char[size];
	MakeCopyIn(ptr, 0, size);
	auto result = std::shared_ptr<unsigned char[]>(ptr);
	return result;
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::AccessData(std::function<void(unsigned char* data, int size_bytes, EntityInfo item_info)>&& callback) {

	m_SSBO->Read<void>(0, m_SSBO->GetSizeBytes(), [this, callback](unsigned char* data, int size, WebGPUBuffer::AsyncToken<void> token) -> std::shared_ptr<void> {
		EntityInfo info;
		memcpy((unsigned char*)&info, data, sizeof(EntityInfo));
		const int entity_size = m_NumDataFloats * sizeof(int);
		const int offset_size = ENTITY_OFFSET_INTS * sizeof(int);
		const int useful_size = info.p_MaxIndex * entity_size + offset_size;

		callback(data, useful_size, info);
		return nullptr;
	});
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::QueueInfoRead() {

	m_Pending.push_back(m_SSBO->Read<EntityInfo>(0, sizeof(EntityInfo), [this](unsigned char* data, int size, WebGPUBuffer::AsyncToken<EntityInfo> token) -> std::shared_ptr<EntityInfo> {
		EntityInfo* result = new EntityInfo();
		memcpy((unsigned char*)result, data, sizeof(EntityInfo));

		while (!m_Pending.empty() && (m_Pending.front().IsFinished() || (m_Pending.front() == token))) {
			auto payload = m_Pending.front().GetPayload();
			if (payload.get()) {
				memcpy((unsigned char*)(&m_LastKnownInfo), (unsigned char*)payload.get(), sizeof(EntityInfo));
			} else if (m_Pending.front() == token) {
				memcpy((unsigned char*)(&m_LastKnownInfo), data, sizeof(EntityInfo));
			}
			m_Pending.pop_front();
		}
		return std::shared_ptr<EntityInfo>(result);
	}));
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::SyncInfo() {
	while (!m_Pending.empty()) {
		m_Pending.front().Wait();
		m_Pending.pop_front();
	}
}

////////////////////////////////////////////////////////////////////////////////
void GPUEntity::MakeCopyIn(unsigned char* ptr, int offset, int size) {
	m_SSBO->ReadSync(ptr, offset, size);
}

} // namespace Neshny