//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

WGPUDevice GlobalWebGPUDevice() {
	return Core::Singleton().GetWebGPUDevice();
}
WGPUInstance GlobalWebGPUInstance() {
	return Core::Singleton().GetWebGPUInstance();
}

////////////////////////////////////////////////////////////////////////////////
WebGPUShader::WebGPUShader(void) :
	m_Shader	( nullptr )
{
}

////////////////////////////////////////////////////////////////////////////////
WebGPUShader::~WebGPUShader(void) {
	wgpuShaderModuleRelease(m_Shader);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUShader::CompilationInfoCallback(WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const* compilationInfo, void* userdata1, void* userdata2) {
	if (!compilationInfo) {
		return;
	}
	WebGPUShader* shader = (WebGPUShader*)userdata1;
	for (uint32_t i = 0; i < compilationInfo->messageCount; ++i) {
		const auto& msg = compilationInfo->messages[i];
		shader->m_Errors.push_back({
			msg.type,
#ifdef __EMSCRIPTEN__
			std::string(msg.message),
#else
			std::string(msg.message.data, msg.message.length),
#endif
			msg.lineNum - 1, // device outputs line numbers starting with 1
			msg.linePos - 2 // device outputs line pos starting with 2 for some reason
		});
	}
}

////////////////////////////////////////////////////////////////////////////////
bool WebGPUShader::Init(const std::function<std::string(std::string_view, std::string&)>& loader, std::string_view filename, std::string_view start_insert, std::string_view end_insert) {

#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPUShader::Init");
#endif

	std::string err_msg;
	std::string arr = loader(filename, err_msg);
	if (arr.empty()) {
		m_Errors.push_back({
			WGPUCompilationMessageType_Error,
			("File error - " + err_msg),
			-1u,
			-1u
		});
		return false;
	}
	m_SourcePrePreProcessor = std::format("{}{}{}", start_insert, arr, end_insert);
	m_Source = Preprocess(m_SourcePrePreProcessor, loader, err_msg);
	if (!err_msg.empty()) {
		m_Errors.push_back({
			WGPUCompilationMessageType_Error,
			("Preprocessor error - " + err_msg),
			-1u,
			-1u
		});
		return false;
	}

	WGPUShaderModuleDescriptor desc = {};
#ifdef __EMSCRIPTEN__
	WGPUShaderModuleWGSLDescriptor wgsl = {};
	wgsl.chain.next = nullptr;
	wgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	wgsl.code = m_Source.data();
	desc.label = filename.data();
#else
	WGPUShaderSourceWGSL wgsl = {};
	wgsl.chain.next = nullptr;
	wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
	wgsl.code = { m_Source.data(), m_Source.length() };
	desc.label = { filename.data(), filename.length() };
#endif
	desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl);

	m_Shader = wgpuDeviceCreateShaderModule(Core::Singleton().GetWebGPUDevice(), &desc);
#ifndef __EMSCRIPTEN__
	WGPUCompilationInfoCallbackInfo callback_info = {
		nullptr,
		DEFAULT_CALLBACK_MODE,
		CompilationInfoCallback,
		this, nullptr
	};
	wgpuShaderModuleGetCompilationInfo(m_Shader, std::move(callback_info));
#endif

	return m_Errors.size() == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
WebGPUBuffer::WebGPUBuffer(WGPUBufferUsage flags, int size) :
	m_Flags	( flags )
{
	Init();
	if (size > 0) {
		EnsureSizeBytes(size);
	} else {
		m_Size = std::max(size, (int)sizeof(int));
		Create(m_Size, nullptr); // zero sized buffers cause too many issues
	}
}

////////////////////////////////////////////////////////////////////////////////
WebGPUBuffer::WebGPUBuffer(WGPUBufferUsage flags, unsigned char* data, int size) :
	m_Flags		( flags )
	,m_Size		( std::max(size, (int)sizeof(int)) ) // zero sized buffers cause too many issues
{
	Init();
	Create(m_Size, data);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::Init(void) {
	// this is the only option if you want an expandable buffer - CopySrc has to be included, and thus MapRead can't be
	m_Flags = WGPUBufferUsage(m_Flags | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Storage);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::Create(int size, unsigned char* data) {

	WGPUBufferDescriptor desc = {};
	desc.usage = m_Flags;
	desc.size = size;
	desc.nextInChain = nullptr;
#ifdef __EMSCRIPTEN__
	desc.label = nullptr;
#else
	desc.label = { nullptr, 0 };
#endif
	desc.mappedAtCreation = false;

	m_Buffer = wgpuDeviceCreateBuffer(Core::Singleton().GetWebGPUDevice(), &desc);
	if (data) {
		wgpuQueueWriteBuffer(Core::Singleton().GetWebGPUQueue(), m_Buffer, 0, data, size);
	}
}

////////////////////////////////////////////////////////////////////////////////
WebGPUBuffer::~WebGPUBuffer(void) {
	wgpuBufferDestroy(m_Buffer);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::EnsureSizeBytes(int size_bytes, bool clear_after) {

#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt1("WebGPUBuffer::EnsureSizeBytes");
#endif

	// ensure size doubles each time
	size_bytes = RoundUpPowerTwo(size_bytes);

	if (m_Size >= size_bytes) {
		if (clear_after) {
			ClearBuffer();
		}
		return;
	}
	int old_size = m_Size;
	WGPUBuffer old_buffer = m_Buffer;

	m_Size = size_bytes;
	Create(size_bytes, nullptr);
	
	ClearBuffer();
	if (!clear_after) {
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(Core::Singleton().GetWebGPUDevice(), nullptr);
		wgpuCommandEncoderCopyBufferToBuffer(encoder, old_buffer, 0, m_Buffer, 0, old_size);
		WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
		wgpuCommandEncoderRelease(encoder);
		wgpuQueueSubmit(Core::Singleton().GetWebGPUQueue(), 1, &commands);
		wgpuCommandBufferRelease(commands);
	}

	if (old_buffer) {
		wgpuBufferDestroy(old_buffer);
	}

#ifdef SSBO_DEBUG
	m_NumberResizes++;
#endif
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::ClearBuffer() {

	if ((m_Size <= 0) || (m_Buffer == nullptr)) {
		return;
	}

	unsigned char* tmp = new unsigned char[m_Size];
	memset(tmp, 0, m_Size);
	wgpuQueueWriteBuffer(Core::Singleton().GetWebGPUQueue(), m_Buffer, 0, tmp, m_Size);
	delete[] tmp;
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::ReadSync(unsigned char* buffer, int offset, int size) {
	size = size >= 0 ? size : m_Size - offset;
	if (size <= 0) {
		return;
	}
	DebugTiming debug_func("WebGPUBuffer::ReadSync");
	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
	desc.size = size;
	desc.nextInChain = nullptr;
#ifdef __EMSCRIPTEN__
	desc.label = nullptr;
#else
	desc.label = { nullptr, 0 };
#endif
	desc.mappedAtCreation = false;
	WGPUBuffer copy_buffer = wgpuDeviceCreateBuffer(Core::Singleton().GetWebGPUDevice(), &desc);
	CopyBufferToBuffer(m_Buffer, copy_buffer, offset, 0, size);

#ifdef __EMSCRIPTEN__
	std::optional<WGPUBufferMapAsyncStatus> status;
	wgpuBufferMapAsync(copy_buffer, WGPUMapMode_Read, 0, size, [](WGPUBufferMapAsyncStatus status, void* user_data) {
		auto info = (std::optional<WGPUBufferMapAsyncStatus>*)user_data;
		*info = status;
	}, &status);
#else
	std::optional<WGPUMapAsyncStatus> status;
	WGPUBufferMapCallbackInfo callback_info = {
		nullptr, DEFAULT_CALLBACK_MODE,
		[] (WGPUMapAsyncStatus status, WGPUStringView message, void* userdata1, void* userdata2) {
			auto info = (std::optional<WGPUMapAsyncStatus>*)userdata1;
			*info = status;
		}, &status, nullptr
	};

	// since you are copying from a temp buffer that already took the offset into account, offset must be set to zero
	wgpuBufferMapAsync(copy_buffer, WGPUMapMode_Read, 0, size, std::move(callback_info));
#endif

	Core::WaitForCommandsToFinish();

	auto buffer_data = wgpuBufferGetConstMappedRange(copy_buffer, 0, size);
	memcpy(buffer, buffer_data, size);
	wgpuBufferUnmap(copy_buffer);

	wgpuBufferDestroy(copy_buffer);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::CopyBufferToBuffer(WGPUBuffer source, WGPUBuffer destination, int source_offset_bytes, int dest_offset_bytes, int size, WGPUCommandEncoder existing_encoder) {
	WGPUCommandEncoder encoder = existing_encoder ? existing_encoder : wgpuDeviceCreateCommandEncoder(Core::Singleton().GetWebGPUDevice(), nullptr);
	wgpuCommandEncoderCopyBufferToBuffer(encoder, source, source_offset_bytes, destination, dest_offset_bytes, size);
	if (!existing_encoder) {
		WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
		wgpuCommandEncoderRelease(encoder);
		wgpuQueueSubmit(Core::Singleton().GetWebGPUQueue(), 1, &commands);
		wgpuCommandBufferRelease(commands);
	}
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::Write(unsigned char* buffer, int offset, int size) {
	if ((offset + size) > m_Size) {
		throw std::invalid_argument("Attempting to write beyond the size of the buffer, use EnsureSize");
	}
	wgpuQueueWriteBuffer(Core::Singleton().GetWebGPUQueue(), m_Buffer, offset, buffer, size);
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<unsigned char[]> WebGPUBuffer::MakeCopy(int max_size) {
	max_size = max_size >= 0 ? std::min(max_size, m_Size) : m_Size;
	if (max_size <= 0) {
		return std::shared_ptr<unsigned char[]>(nullptr);
	}
	unsigned char* ptr = new unsigned char[max_size];
	ReadSync(ptr, 0, max_size);
	return std::shared_ptr<unsigned char[]>(ptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void WebGPURenderBuffer::Init(std::vector<WGPUVertexFormat> attributes, WGPUPrimitiveTopology topology, unsigned char* vertex_data, int vertex_data_size, std::vector<uint32_t> index_data) {

	delete m_VertexBuffer;
	delete m_IndexBuffer;
	m_VertexBuffer = nullptr;
	m_IndexBuffer = nullptr;
	m_Attributes.clear();

	int vertex_bytes = 0;
	for (auto attr: attributes) {

		VertexFormatItem item;
		item.p_Type = attr;

		switch (attr) {
			case WGPUVertexFormat_Uint8x2: item.p_Size = sizeof(char) * 2; break;
			case WGPUVertexFormat_Uint8x4: item.p_Size = sizeof(char) * 4; break;
			case WGPUVertexFormat_Sint8x2: item.p_Size = sizeof(char) * 2; break;
			case WGPUVertexFormat_Sint8x4: item.p_Size = sizeof(char) * 4; break;
			case WGPUVertexFormat_Unorm8x2: item.p_Size = sizeof(char) * 2; break;
			case WGPUVertexFormat_Unorm8x4: item.p_Size = sizeof(char) * 4; break;
			case WGPUVertexFormat_Snorm8x2: item.p_Size = sizeof(char) * 2; break;
			case WGPUVertexFormat_Snorm8x4: item.p_Size = sizeof(char) * 4; break;
			case WGPUVertexFormat_Uint16x2: item.p_Size = sizeof(short) * 2; break;
			case WGPUVertexFormat_Uint16x4: item.p_Size = sizeof(short) * 4; break;
			case WGPUVertexFormat_Sint16x2: item.p_Size = sizeof(short) * 2; break;
			case WGPUVertexFormat_Sint16x4: item.p_Size = sizeof(short) * 4; break;
			case WGPUVertexFormat_Unorm16x2: item.p_Size = sizeof(short) * 2; break;
			case WGPUVertexFormat_Unorm16x4: item.p_Size = sizeof(short) * 4; break;
			case WGPUVertexFormat_Snorm16x2: item.p_Size = sizeof(short) * 2; break;
			case WGPUVertexFormat_Snorm16x4: item.p_Size = sizeof(short) * 4; break;
			case WGPUVertexFormat_Float16x2: item.p_Size = sizeof(short) * 2; break;
			case WGPUVertexFormat_Float16x4: item.p_Size = sizeof(short) * 4; break;
			case WGPUVertexFormat_Float32: item.p_Size = sizeof(float) * 1; break;
			case WGPUVertexFormat_Float32x2: item.p_Size = sizeof(float) * 2; break;
			case WGPUVertexFormat_Float32x3: item.p_Size = sizeof(float) * 3; break;
			case WGPUVertexFormat_Float32x4: item.p_Size = sizeof(float) * 4; break;
			case WGPUVertexFormat_Uint32: item.p_Size = sizeof(int) * 1; break;
			case WGPUVertexFormat_Uint32x2: item.p_Size = sizeof(int) * 2; break;
			case WGPUVertexFormat_Uint32x3: item.p_Size = sizeof(int) * 3; break;
			case WGPUVertexFormat_Uint32x4: item.p_Size = sizeof(int) * 4; break;
			case WGPUVertexFormat_Sint32: item.p_Size = sizeof(int) * 1; break;
			case WGPUVertexFormat_Sint32x2: item.p_Size = sizeof(int) * 2; break;
			case WGPUVertexFormat_Sint32x3: item.p_Size = sizeof(int) * 3; break;
			case WGPUVertexFormat_Sint32x4: item.p_Size = sizeof(int) * 4; break;
#ifdef __EMSCRIPTEN__
			case WGPUVertexFormat_Undefined:
#endif
			case WGPUVertexFormat_Unorm10_10_10_2:
			case WGPUVertexFormat_Force32: item.p_Size = 0; break;
		}
		vertex_bytes += item.p_Size;
		m_Attributes.push_back(item);
	}

	m_Topology = topology;
	m_NumVertices = vertex_data_size / vertex_bytes;
	m_NumIndices = (int)index_data.size();
	m_VertexBuffer = new WebGPUBuffer(WGPUBufferUsage_Vertex, vertex_data, vertex_data_size);
	if (!index_data.empty()) {
		m_IndexBuffer = new WebGPUBuffer(WGPUBufferUsage_Index, (unsigned char*)index_data.data(), sizeof(uint32_t) * m_NumIndices);
	}
}

////////////////////////////////////////////////////////////////////////////////
WebGPURenderBuffer::~WebGPURenderBuffer(void) {
	delete m_VertexBuffer;
	delete m_IndexBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
WebGPUTexture::~WebGPUTexture(void) {
	if (m_View) {
		wgpuTextureViewRelease(m_View);
	}
	if (m_Texture) {
		wgpuTextureDestroy(m_Texture);
	}
}

////////////////////////////////////////////////////////////////////////////////
int WebGPUTexture::GetMipMaps(int width, int height, int mip_maps) {
	if (mip_maps == AUTO_MIPMAPS) {
		double two_power = log(double(std::min(width, height))) / log(2.0);
		mip_maps = int(floor(two_power));
	}
	return mip_maps;
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUTexture::Init(int width, int height, int depth, WGPUTextureFormat format, WGPUTextureDimension dimension, WGPUTextureViewDimension view_dimension, WGPUTextureUsage usage, WGPUTextureAspect aspect, int mip_maps) {

	if (m_Texture) {
		throw "Cannot init texture more than once";
	}
	m_Width = width;
	m_Height = height;
	m_Layers = depth;
	m_MipMaps = mip_maps;
	m_Format = format;
	m_ViewDimension = view_dimension;

	m_DepthBytes = 4; // TODO: support other formats

	WGPUTextureDescriptor descriptor;
#ifdef __EMSCRIPTEN__
	descriptor.label = nullptr;
#else
	descriptor.label = { nullptr, 0 };
#endif
	descriptor.nextInChain = nullptr;
	descriptor.viewFormats = nullptr;
	descriptor.viewFormatCount = 0;
	descriptor.dimension = dimension;
	descriptor.size.width = width;
	descriptor.size.height = height;
	descriptor.size.depthOrArrayLayers = depth;
	descriptor.sampleCount = 1;
	descriptor.format = format;
	descriptor.mipLevelCount = mip_maps;
	descriptor.usage = usage;

	m_Texture = wgpuDeviceCreateTexture(Core::Singleton().GetWebGPUDevice(), &descriptor);

	WGPUTextureViewDescriptor view_desc;
#ifdef __EMSCRIPTEN__
	view_desc.label = nullptr;
#else
	view_desc.label = { nullptr, 0 };
#endif
	view_desc.nextInChain = nullptr;
	view_desc.arrayLayerCount = depth;
	view_desc.aspect = aspect;
	view_desc.baseArrayLayer = 0;
	view_desc.baseMipLevel = 0;
	view_desc.dimension = view_dimension;
	view_desc.format = format;
	view_desc.mipLevelCount = mip_maps;
#ifndef __EMSCRIPTEN__
	view_desc.usage = usage;
#endif

	m_View = wgpuTextureCreateView(m_Texture, &view_desc);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUTexture::CopyDataLayer(int layer, unsigned char* data, int bytes_per_pixel, int bytes_per_row, bool auto_mipmap) {

	CopyDataLayerMipMap(layer, 0, data, bytes_per_pixel, bytes_per_row);
	if (auto_mipmap) {
		for (int m = 1; m < m_MipMaps; m++) {
			const int mip_mult = 1 << m;
			const int wid = m_Width / mip_mult;
			const int hei = m_Height / mip_mult;
			const int size_row = wid * bytes_per_pixel;
			const int size_full = size_row * hei;

			unsigned char* temp_data = new unsigned char[size_full];
			
			const int input_row_mult = bytes_per_row * mip_mult;
			const int input_pix_mult = bytes_per_pixel * mip_mult;
			unsigned char* row_input = data;
			float inv_sum = 1.0f / (mip_mult * mip_mult);
			for (int y = 0; y < hei; y++) {
				unsigned char* col_input = row_input;
				for (int x = 0; x < wid; x++) {
					for (int d = 0; d < bytes_per_pixel; d++) {
						int sum = 0;

						unsigned char* pix_input = col_input + d;
						for (int yy = 0; yy < mip_mult; yy++) {
							unsigned char* inner_pix_input = pix_input;
							for (int xx = 0; xx < mip_mult; xx++) {
								sum += *inner_pix_input;
								inner_pix_input += bytes_per_pixel;
							}
							pix_input += bytes_per_row;
						}
						int result = floor(inv_sum * float(sum));
						temp_data[(y * wid + x) * bytes_per_pixel + d] = (unsigned char)result;
					}
					col_input += input_pix_mult;
				}
				row_input += input_row_mult;
			}
			CopyDataLayerMipMap(layer, m, temp_data, bytes_per_pixel, size_row);
			delete[] temp_data;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUTexture::CopyDataLayerMipMap(int layer, int mip_map, unsigned char* data, int bytes_per_pixel, int bytes_per_row) {

	int mip_div = 1 << mip_map;
	int wid = m_Width / mip_div;
	int hei = m_Height / mip_div;

	WGPUImageCopyTexture tex_cpy;
	tex_cpy.mipLevel = mip_map;
	tex_cpy.origin.x = 0;
	tex_cpy.origin.y = 0;
	tex_cpy.origin.z = layer;
	tex_cpy.texture = m_Texture;
	tex_cpy.aspect = WGPUTextureAspect_All;
#ifdef __EMSCRIPTEN__
	WGPUTextureDataLayout tex_layout;
#else
	WGPUTexelCopyBufferLayout tex_layout;
#endif

	tex_layout.bytesPerRow = bytes_per_row;
	tex_layout.rowsPerImage = hei;
	tex_layout.offset = 0;
	WGPUExtent3D tex_extent;
	tex_extent.width = wid;
	tex_extent.height = hei;
	tex_extent.depthOrArrayLayers = 1;

	std::ignore = std::format("Adding this line changes the way emscripten compiles in such a way as to prevent wgpuQueueWriteTexture from crashing :)");

	wgpuQueueWriteTexture(Core::Singleton().GetWebGPUQueue(), &tex_cpy, data, bytes_per_row * hei, &tex_layout, &tex_extent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
WebGPUTextureView::WebGPUTextureView(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect, int layers, int mipmaps) {
	WGPUTextureViewDescriptor view_desc;
	view_desc.nextInChain = nullptr;
#ifdef __EMSCRIPTEN__
	view_desc.label = nullptr;
#else
	view_desc.label = { nullptr, 0 };
	view_desc.usage = WGPUTextureUsage_TextureBinding;
#endif
	view_desc.arrayLayerCount = layers;
	view_desc.aspect = aspect;
	view_desc.baseArrayLayer = 0;
	view_desc.baseMipLevel = 0;
	view_desc.dimension = dimension;
	view_desc.format = format;
	view_desc.mipLevelCount = mipmaps;
	m_View = wgpuTextureCreateView(texture, &view_desc);
}

////////////////////////////////////////////////////////////////////////////////
WebGPUTextureView::~WebGPUTextureView(void) {
	wgpuTextureViewRelease(m_View);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
WebGPUSampler::WebGPUSampler(WGPUAddressMode mode, WGPUFilterMode filter, bool linear_mipmaps, unsigned int max_anisotropy) :
	m_Mode				( mode )
	,m_Filter			( filter )
	,m_LinearMipMaps	( linear_mipmaps )
	,m_MaxAnisotropy	( max_anisotropy )
{
	WGPUSamplerDescriptor desc;
	desc.nextInChain = nullptr;
#ifdef __EMSCRIPTEN__
	desc.label = nullptr;
#else
	desc.label = { nullptr, 0 };
#endif
	desc.compare = WGPUCompareFunction_Undefined;
	desc.addressModeU = mode;
	desc.addressModeV = mode;
	desc.addressModeW = mode;
	desc.lodMinClamp = 0;
	desc.lodMaxClamp = 32;
	desc.magFilter = filter;
	desc.minFilter = filter;
	desc.maxAnisotropy = 1;
	desc.mipmapFilter = linear_mipmaps ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;

	m_Sampler = wgpuDeviceCreateSampler(Core::Singleton().GetWebGPUDevice(), &desc);
}

////////////////////////////////////////////////////////////////////////////////
WebGPUSampler::~WebGPUSampler(void) {
	wgpuSamplerRelease(m_Sampler);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WebGPUPipeline::~WebGPUPipeline(void) {
	Reset();
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::Reset(void) {
	if (m_RenderPipeline) {
		wgpuRenderPipelineRelease(m_RenderPipeline);
		m_RenderPipeline = nullptr;
	}
	if (m_ComputePipeline) {
		wgpuComputePipelineRelease(m_ComputePipeline);
		m_ComputePipeline = nullptr;
	}

	if (m_BindGroup) {
		wgpuBindGroupRelease(m_BindGroup);
		m_BindGroup = nullptr;
	}
	if (m_BindGroupLayout) {
		wgpuBindGroupLayoutRelease(m_BindGroupLayout);
		m_BindGroupLayout = nullptr;
	}
	m_Type = Type::UNKNOWN;

	m_RenderBuffer = nullptr;
	m_Buffers.clear();
	m_Textures.clear();
	m_Samplers.clear();
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::CreateBindGroupLayout(void) {

	int binding_num = 0;
	int num_bindings = int(m_Buffers.size() + m_Textures.size() + m_Samplers.size());
	std::vector<WGPUBindGroupLayoutEntry> layout_entries;
	layout_entries.resize(num_bindings);

	for (auto buffer : m_Buffers) {
		WGPUBindGroupLayoutEntry& layout_entry = layout_entries[binding_num];
		layout_entry.nextInChain = nullptr;
		layout_entry.binding = binding_num;
		layout_entry.visibility = m_Type == Type::COMPUTE ? WGPUShaderStage_Compute : buffer.p_VisibilityFlags;

		WGPUBufferBindingLayout buffer_layout;
		buffer_layout.nextInChain = nullptr;
		buffer_layout.hasDynamicOffset = false;
		buffer_layout.minBindingSize = 0;
		if (buffer.p_Buffer->GetUsageFlags() & WGPUBufferUsage_Uniform) {
			buffer_layout.type = WGPUBufferBindingType_Uniform;
		} else if(buffer.p_ReadOnly) {
			buffer_layout.type = WGPUBufferBindingType_ReadOnlyStorage;
		} else {
			buffer_layout.type = WGPUBufferBindingType_Storage;
		}
		layout_entry.buffer = buffer_layout;
		binding_num++;
	}
	for (auto tex : m_Textures) {
		WGPUBindGroupLayoutEntry& layout_entry = layout_entries[binding_num];
		layout_entry.nextInChain = nullptr;
		layout_entry.binding = binding_num;
		layout_entry.visibility = m_Type == Type::COMPUTE ? WGPUShaderStage_Compute : WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

		WGPUTextureBindingLayout buff_texture;
		buff_texture.nextInChain = nullptr;
		buff_texture.multisampled = false;
		buff_texture.viewDimension = tex.p_Dimensions;
		buff_texture.sampleType = WGPUTextureSampleType_Float;

		layout_entry.texture = buff_texture;
		binding_num++;
	}
	for (auto sampler : m_Samplers) {
		WGPUBindGroupLayoutEntry& layout_entry = layout_entries[binding_num];
		layout_entry.nextInChain = nullptr;
		layout_entry.binding = binding_num;
		layout_entry.visibility = m_Type == Type::COMPUTE ? WGPUShaderStage_Compute : WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

		WGPUSamplerBindingLayout buff_sampler;
		buff_sampler.nextInChain = nullptr;
		buff_sampler.type = WGPUSamplerBindingType_Filtering;

		layout_entry.sampler = buff_sampler;
		binding_num++;
	}

	WGPUBindGroupLayoutDescriptor bgl_desc = {};
	bgl_desc.entryCount = num_bindings;
	bgl_desc.entries = layout_entries.empty() ? nullptr : &layout_entries[0];

	m_BindGroupLayout = wgpuDeviceCreateBindGroupLayout(Core::Singleton().GetWebGPUDevice(), &bgl_desc);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::FinalizeRender(std::string_view shader_name, WebGPURenderBuffer& render_buffer, RenderParams params, std::string_view insertion, std::string_view end_insertion) {
#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPURenderPipeline::FinalizeRender");
#endif
	if (m_Type != Type::UNKNOWN) {
		throw std::logic_error("Cannot finalize more than once");
	}
	m_Type = Type::RENDER;
	m_RenderBuffer = &render_buffer;

	WGPUShaderModule shader = Core::GetShader(shader_name, insertion, end_insertion)->Get();
	if (shader == nullptr) {
		throw std::logic_error("Cannot find shader");
	}
	CreateBindGroupLayout();

	// pipeline layout (used by the render pipeline, released after its creation)
	WGPUPipelineLayoutDescriptor layout_desc = {};
	layout_desc.bindGroupLayoutCount = 1;
	layout_desc.bindGroupLayouts = &m_BindGroupLayout;
	WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(Core::Singleton().GetWebGPUDevice(), &layout_desc);

	// describe buffer layouts
	WGPUVertexBufferLayout vertex_buffer_layout = {};
	std::vector<WGPUVertexAttribute> formats;
	{
		auto vertex_format = m_RenderBuffer->GetFormat();
		int num_formats = vertex_format.size();

		int stride = 0;
		formats.resize(num_formats);
		for(int i = 0; i < num_formats; i++) {
			auto& item = vertex_format[i];
			auto& output = formats[i];
			output.shaderLocation = i;
			output.offset = stride;
			output.format = item.p_Type;
			stride += item.p_Size;
		}
		vertex_buffer_layout.arrayStride = stride;
		vertex_buffer_layout.stepMode = WGPUVertexStepMode_Vertex;
	}
	vertex_buffer_layout.attributeCount = (int)formats.size();
	vertex_buffer_layout.attributes = &formats[0];

	// Fragment state
	WGPUBlendState blend = {};
	blend.color.operation = params.p_ColorBlend.operation;
	blend.color.srcFactor = params.p_ColorBlend.srcFactor;
	blend.color.dstFactor = params.p_ColorBlend.dstFactor;
	blend.alpha.operation = params.p_AlphaBlend.operation;
	blend.alpha.srcFactor = params.p_AlphaBlend.srcFactor;
	blend.alpha.dstFactor = params.p_AlphaBlend.dstFactor;

	std::vector<WGPUColorTargetState> color_targets;
	for (auto attach: params.p_Attachments) {
		WGPUColorTargetState color_target;
		color_target.nextInChain = nullptr;
		if (attach == AttachmentMode::RGBA) {
			color_target.format = WGPUTextureFormat_BGRA8Unorm;
		} else {
			throw "Non-RGBA attachment modes not implemented yet";
		}
		color_target.blend = &blend;
		color_target.writeMask = WGPUColorWriteMask_All;
		color_targets.push_back(color_target);
	}

	WGPUDepthStencilState depth_state = {};
	depth_state.nextInChain = nullptr;
	depth_state.depthCompare = params.p_DepthCompare;
#ifdef __EMSCRIPTEN__
	depth_state.depthWriteEnabled = params.p_DepthWriteEnabled;
#else
	depth_state.depthWriteEnabled = params.p_DepthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
#endif
	depth_state.format = WGPUTextureFormat_Depth24Plus;
	depth_state.stencilReadMask = 0;
	depth_state.stencilWriteMask = 0;
	depth_state.stencilFront = params.p_StencilFront;
	depth_state.stencilBack = params.p_StencilBack;

	{
		WGPUFragmentState fragment = {};
		fragment.module = shader;
#ifdef __EMSCRIPTEN__
		fragment.entryPoint = "frag_main"; // todo: param this instead of hardcoding
#else
		fragment.entryPoint = { "frag_main", WGPU_STRLEN }; // todo: param this instead of hardcoding
#endif
		fragment.targetCount = color_targets.size();
		fragment.targets = color_targets.data();

		WGPURenderPipelineDescriptor desc = {};
		desc.fragment = &fragment;
		desc.depthStencil = &depth_state;

		// Other state
		desc.layout = pipeline_layout;

		desc.vertex.module = shader;
#ifdef __EMSCRIPTEN__
		desc.vertex.entryPoint = "vertex_main"; // todo: param this instead of hardcoding
#else
		desc.vertex.entryPoint = { "vertex_main", WGPU_STRLEN }; // todo: param this instead of hardcoding
#endif
		desc.vertex.bufferCount = 1;
		desc.vertex.buffers = &vertex_buffer_layout;

		desc.multisample.count = 1;
		desc.multisample.mask = 0xFFFFFFFF;
		desc.multisample.alphaToCoverageEnabled = false;

		desc.primitive.frontFace = params.p_FrontFace;
		desc.primitive.cullMode = params.p_CullMode;
		desc.primitive.topology = render_buffer.GetTopology();
		desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

		m_RenderPipeline = wgpuDeviceCreateRenderPipeline(Core::Singleton().GetWebGPUDevice(), &desc);
	}
	wgpuPipelineLayoutRelease(pipeline_layout);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::FinalizeCompute(std::string_view shader_name, std::string_view insertion, std::string_view end_insertion) {
#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPURenderPipeline::FinalizeCompute");
#endif
	if (m_Type != Type::UNKNOWN) {
		throw std::logic_error("Cannot finalize more than once");
	}
	m_Type = Type::COMPUTE;
	m_RenderBuffer = nullptr;

	CreateBindGroupLayout();
	WGPUShaderModule shader = Core::GetShader(shader_name, insertion, end_insertion)->Get();
	if (!shader) {
		throw std::logic_error("Could not load shader");
	}

	WGPUPipelineLayoutDescriptor layout_desc = {};
	layout_desc.bindGroupLayoutCount = 1;
	layout_desc.bindGroupLayouts = &m_BindGroupLayout;
	WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(Core::Singleton().GetWebGPUDevice(), &layout_desc);

	WGPUComputePipelineDescriptor desc;
	desc.nextInChain = nullptr;
#ifdef __EMSCRIPTEN__
	desc.label = nullptr;
	desc.compute.entryPoint = "main";
#else
	desc.label = { nullptr, 0 };
	desc.compute.entryPoint = { "main", WGPU_STRLEN };
#endif
	desc.layout = pipeline_layout;
	desc.compute.nextInChain = nullptr;
	desc.compute.module = shader;
	desc.compute.constants = nullptr;
	desc.compute.constantCount = 0;

	m_ComputePipeline = wgpuDeviceCreateComputePipeline(Core::Singleton().GetWebGPUDevice(), &desc);

	wgpuPipelineLayoutRelease(pipeline_layout);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::RefreshBindings(void) {

#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPURenderPipeline::RefreshBindings");
#endif

	if (m_BindGroup) {
		wgpuBindGroupRelease(m_BindGroup);
	}

	int num_bindings = int(m_Buffers.size() + m_Textures.size() + m_Samplers.size());

	std::vector<WGPUBindGroupEntry> entries;
	entries.resize(num_bindings);

	int binding_num = 0;
	for (auto& buffer : m_Buffers) {
		buffer.p_LastSeenBuffer = buffer.p_Buffer->Get();
		WGPUBindGroupEntry& entry = entries[binding_num];
		entry.nextInChain = nullptr;
		entry.binding = binding_num;
		entry.offset = 0;
		entry.size = buffer.p_Buffer->GetSizeBytes();
		entry.buffer = buffer.p_Buffer->Get();
		entry.textureView = nullptr;
		entry.sampler = nullptr;
		binding_num++;
	}
#pragma msg("don't use same binding numbers for textures - waste of buffer nums, they are orthogonal to each other")
	for (auto& tex : m_Textures) {
		tex.p_LastSeenTextureView = tex.p_TextureView;
		WGPUBindGroupEntry& entry = entries[binding_num];
		entry.nextInChain = nullptr;
		entry.binding = binding_num;
		entry.offset = 0;
		entry.buffer = nullptr;
		entry.textureView = tex.p_TextureView;
		entry.sampler = nullptr;
		binding_num++;
	}
	for (auto& sampler : m_Samplers) {
		WGPUBindGroupEntry& entry = entries[binding_num];
		entry.nextInChain = nullptr;
		entry.binding = binding_num;
		entry.offset = 0;
		entry.buffer = nullptr;
		entry.textureView = nullptr;
		entry.sampler = sampler->Get();
		binding_num++;
	}

	WGPUBindGroupDescriptor bind_group_desc = {};
	bind_group_desc.layout = m_BindGroupLayout;
	bind_group_desc.entryCount = num_bindings;
	bind_group_desc.entries = entries.empty() ? nullptr : &entries[0];

	m_BindGroup = wgpuDeviceCreateBindGroup(Core::Singleton().GetWebGPUDevice(), &bind_group_desc);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::CheckBuffersUpToDate(void) {
	bool needs_refresh = m_BindGroup == nullptr;
	for (auto& buffer : m_Buffers) {
		needs_refresh = needs_refresh || (buffer.p_LastSeenBuffer != buffer.p_Buffer->Get());
	}
	for (auto& texture : m_Textures) {
		needs_refresh = needs_refresh || (texture.p_LastSeenTextureView != texture.p_TextureView);
	}
	if (needs_refresh) {
		RefreshBindings();
	}
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::ReplaceBuffer(WebGPUBuffer& original, WebGPUBuffer& replacement) {
	for (auto& buff : m_Buffers) {
		if ((&original) == buff.p_Buffer) {
			buff.p_Buffer = &replacement;
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::ReplaceBuffer(int index, WebGPUBuffer& replacement) {
	m_Buffers[index].p_Buffer = &replacement;
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::ReplaceTexture(WGPUTextureView original, WGPUTextureView replacement) {
	for (auto& tex : m_Textures) {
		if (original == tex.p_TextureView) {
			tex.p_TextureView = replacement;
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::ReplaceTexture(int index, WGPUTextureView replacement) {
	m_Textures[index].p_TextureView = replacement;
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::Render(WGPURenderPassEncoder pass, int instances) {
	if (instances <= 0) {
		return;
	}
#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPUPipeline::Render");
#endif

	CheckBuffersUpToDate();
	wgpuRenderPassEncoderSetPipeline(pass, m_RenderPipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, m_BindGroup, 0, nullptr);
	wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_RenderBuffer->GetVertex(), 0, WGPU_WHOLE_SIZE);

	if (m_RenderBuffer->GetIndex()) {
		wgpuRenderPassEncoderSetIndexBuffer(pass, m_RenderBuffer->GetIndex(), WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
		wgpuRenderPassEncoderDrawIndexed(pass, m_RenderBuffer->GetNumIndices(), instances, 0, 0, 0);
	} else {
		wgpuRenderPassEncoderDraw(pass, m_RenderBuffer->GetNumVertices(), instances, 0, 0);
	}
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUPipeline::Compute(int calls, iVec3 workgroup_size, std::optional<std::function<void(WGPUCommandEncoder encoder)>> pre_execute) {

#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPUPipeline::Compute");
#endif

	CheckBuffersUpToDate();

	const auto& limits = Core::Singleton().GetLimits();
	if ((workgroup_size.x < 1) || (workgroup_size.y < 1) || (workgroup_size.z < 1)) {
		workgroup_size = iVec3(limits.maxComputeInvocationsPerWorkgroup, 1, 1);
	}
	int num_threads_per_workgroup = workgroup_size.x * workgroup_size.y * workgroup_size.z;
	div_t workgroup_div = div(calls, num_threads_per_workgroup);
	int workgroups = workgroup_div.quot + ((workgroup_div.rem > 0) ? 1 : 0);
	if (workgroups > limits.maxComputeWorkgroupsPerDimension) {
		throw std::invalid_argument("Exceeded maxComputeWorkgroupsPerDimension");
	}
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(Core::Singleton().GetWebGPUDevice(), nullptr);
	WGPUComputePassDescriptor pass_desc;
	pass_desc.nextInChain = nullptr;

#ifdef __EMSCRIPTEN__
	pass_desc.label = nullptr;
#else
	pass_desc.label = { nullptr, 0 };
#endif
	pass_desc.timestampWrites = nullptr;
	WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(encoder, &pass_desc);

	wgpuComputePassEncoderSetPipeline(pass, m_ComputePipeline);
	wgpuComputePassEncoderSetBindGroup(pass, 0, m_BindGroup, 0, nullptr);
	wgpuComputePassEncoderDispatchWorkgroups(pass, workgroups, 1, 1);

	wgpuComputePassEncoderEnd(pass);
	wgpuComputePassEncoderRelease(pass);

	if (pre_execute.has_value()) {
		(*pre_execute)(encoder);
	}

	WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
	wgpuCommandEncoderRelease(encoder);
	wgpuQueueSubmit(Core::Singleton().GetWebGPUQueue(), 1, &commands);
	wgpuCommandBufferRelease(commands);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WebGPURTT::WebGPURTT(void) :
	m_PassDescriptor{}
	,m_DepthDesc	{}
{
	m_DepthDesc.depthClearValue = 1.0f;
	m_DepthDesc.depthLoadOp = WGPULoadOp_Clear;
	m_DepthDesc.depthStoreOp = WGPUStoreOp_Store;
	m_DepthDesc.depthReadOnly = false;
	m_DepthDesc.stencilClearValue = 0;
	m_DepthDesc.stencilLoadOp = WGPULoadOp_Undefined;
	m_DepthDesc.stencilStoreOp = WGPUStoreOp_Undefined;
	m_DepthDesc.stencilReadOnly = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Token WebGPURTT::Activate(std::vector<WebGPUPipeline::AttachmentMode> color_attachments, bool capture_depth_stencil, int width, int height, bool clear, WGPUTextureView existing_depth_tex) {

	int num_color_tex = (int)color_attachments.size();
	bool modes_same = color_attachments.size() == m_Modes.size();
	for (int i = 0; modes_same && (i < num_color_tex); i++) {
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
			return Token([]() {});
		}
		clear = true;

		for (int i = 0; i < num_color_tex; i++) {
			WebGPUTexture* tex = new WebGPUTexture();
			tex->Init2D(m_Width, m_Height, WGPUTextureFormat_BGRA8Unorm, WGPUTextureUsage(WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding), 1);
			m_ColorTextures.push_back(tex);
		}
		if (m_CaptureDepthStencil && (existing_depth_tex == nullptr)) {
			m_DepthTex = new WebGPUTexture();
			m_DepthTex->InitDepth(m_Width, m_Height);
			existing_depth_tex = m_DepthTex->GetTextureView();
		}

		m_ColorDescriptors.resize(num_color_tex);
		for (int i = 0; i < num_color_tex; i++) {
			WGPURenderPassColorAttachment& color_desc = m_ColorDescriptors[i];
			color_desc.view = m_ColorTextures[i]->GetTextureView();
			color_desc.loadOp = WGPULoadOp_Clear;
			color_desc.storeOp = WGPUStoreOp_Store;
			color_desc.clearValue.r = 0.0f;
			color_desc.clearValue.g = 0.0f;
			color_desc.clearValue.b = 0.0f;
			color_desc.clearValue.a = 1.0f;
			color_desc.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
		}

		m_PassDescriptor.colorAttachmentCount = num_color_tex;
		m_PassDescriptor.colorAttachments = &m_ColorDescriptors[0];

		if (existing_depth_tex) {
			m_DepthDesc.view = existing_depth_tex;
			m_PassDescriptor.depthStencilAttachment = &m_DepthDesc;
		} else {
			m_PassDescriptor.depthStencilAttachment = nullptr;
		}
	}

	m_ActiveEncoder = wgpuDeviceCreateCommandEncoder(Core::Singleton().GetWebGPUDevice(), nullptr);
	m_ClearNext = clear;

	return Token([this]() {
		WGPUCommandBuffer commands = wgpuCommandEncoderFinish(m_ActiveEncoder, nullptr);
		wgpuCommandEncoderRelease(m_ActiveEncoder);
		m_ActiveEncoder = nullptr;
		wgpuQueueSubmit(Core::Singleton().GetWebGPUQueue(), 1, &commands);
		wgpuCommandBufferRelease(commands);
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Token WebGPURTT::Activate(std::vector<WGPUTextureView> color_attachments, WGPUTextureView depth_tex, bool clear) {

	int num_color_tex = (int)color_attachments.size();
	m_ColorDescriptors.resize(num_color_tex);

	for (int i = 0; i < num_color_tex; i++) {
		WGPURenderPassColorAttachment& color_desc = m_ColorDescriptors[i];
		color_desc.view = color_attachments[i];
		color_desc.loadOp = WGPULoadOp_Clear;
		color_desc.storeOp = WGPUStoreOp_Store;
		color_desc.clearValue.r = 0.0f;
		color_desc.clearValue.g = 0.0f;
		color_desc.clearValue.b = 0.0f;
		color_desc.clearValue.a = 1.0f;
		color_desc.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
	}

	m_PassDescriptor.colorAttachmentCount = num_color_tex;
	m_PassDescriptor.colorAttachments = &m_ColorDescriptors[0];

	if (depth_tex) {
		m_DepthDesc.view = depth_tex;
		m_DepthDesc.depthLoadOp = WGPULoadOp_Clear;
		m_PassDescriptor.depthStencilAttachment = &m_DepthDesc;
	} else {
		m_PassDescriptor.depthStencilAttachment = nullptr;
	}

	m_ActiveEncoder = wgpuDeviceCreateCommandEncoder(Core::Singleton().GetWebGPUDevice(), nullptr);
	m_ClearNext = clear;

	return Token([this]() {
		WGPUCommandBuffer commands = wgpuCommandEncoderFinish(m_ActiveEncoder, nullptr);
		wgpuCommandEncoderRelease(m_ActiveEncoder);
		m_ActiveEncoder = nullptr;
		wgpuQueueSubmit(Core::Singleton().GetWebGPUQueue(), 1, &commands);
		wgpuCommandBufferRelease(commands);
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WebGPURTT::Render(WebGPUPipeline* pipeline, int instances) {

	for (auto& color_desc : m_ColorDescriptors) {
		color_desc.loadOp = m_ClearNext ? WGPULoadOp_Clear : WGPULoadOp_Load;
	}
	m_DepthDesc.depthLoadOp = m_ClearNext ? WGPULoadOp_Clear : WGPULoadOp_Load;
	m_ClearNext = false;

	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(m_ActiveEncoder, &m_PassDescriptor);
	if (pipeline) {
		pipeline->Render(pass, instances);
	}
	wgpuRenderPassEncoderEnd(pass);
	wgpuRenderPassEncoderRelease(pass);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Token WebGPURTT::RenderPassToken(WGPURenderPassEncoder& pass) {

	for (auto& color_desc : m_ColorDescriptors) {
		color_desc.loadOp = m_ClearNext ? WGPULoadOp_Clear : WGPULoadOp_Load;
	}
	m_DepthDesc.depthLoadOp = m_ClearNext ? WGPULoadOp_Clear : WGPULoadOp_Load;
	m_ClearNext = false;

	pass = wgpuCommandEncoderBeginRenderPass(m_ActiveEncoder, &m_PassDescriptor);
	return Token([&pass]() {
		wgpuRenderPassEncoderEnd(pass);
		wgpuRenderPassEncoderRelease(pass);
		pass = nullptr;
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WebGPURTT::Destroy(void) {
	for (auto tex : m_ColorTextures) {
		delete tex;
	}
	m_ColorTextures.clear();
	delete m_DepthTex;
	m_DepthTex = nullptr;
}

} // namespace Neshny