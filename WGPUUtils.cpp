//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

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
void WebGPUShader::CompilationInfoCallback(WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const* compilationInfo, void* userdata) {

	WebGPUShader* shader = (WebGPUShader*)userdata;
	for (uint32_t i = 0; i < compilationInfo->messageCount; ++i) {
		const auto& msg = compilationInfo->messages[i];
		shader->m_Errors.push_back({
			msg.type,
			QByteArray(msg.message),
			msg.lineNum,
			msg.linePos
		});
	}
}

////////////////////////////////////////////////////////////////////////////////
bool WebGPUShader::Init(const std::function<QByteArray(QString, QString&)>& loader, QString filename, QString insertion) {

#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPUShader::Init");
#endif

	QString err_msg;
	QByteArray arr = loader(filename, err_msg);
	if (arr.isNull()) {
		m_Errors.push_back({
			WGPUCompilationMessageType_Error,
			("File error - " + err_msg).toLocal8Bit(),
			-1u,
			-1u
		});
		return false;
	}
	m_Source = arr;

	WGPUShaderModuleWGSLDescriptor wgsl = {};
	wgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	wgsl.source = arr.data();
	WGPUShaderModuleDescriptor desc = {};
	desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl);
	auto fname_bytes = filename.toLocal8Bit();
	desc.label = fname_bytes.data();

	m_Shader = wgpuDeviceCreateShaderModule(Core::Singleton().GetWebGPUDevice(), &desc);
#ifndef __EMSCRIPTEN__
	wgpuShaderModuleGetCompilationInfo(m_Shader, CompilationInfoCallback, this);
#endif

	return m_Errors.size() == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
WebGPUBuffer::WebGPUBuffer(WGPUBufferUsageFlags flags, int size) :
	m_Flags	( flags )
{
	if (size > 0) {
		EnsureSizeBytes(size);
	}
}

////////////////////////////////////////////////////////////////////////////////
WebGPUBuffer::WebGPUBuffer(WGPUBufferUsageFlags flags, unsigned char* data, int size) :
	m_Flags		( flags )
	,m_Size		( size )
{
	Create(size, data);
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::Create(int size, unsigned char* data) {

	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Index | WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage;
	desc.size = size;
	desc.nextInChain = nullptr;
	desc.label = nullptr;

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

	// ensure size doubles each time
	size_bytes = RoundUpPowerTwo(size_bytes);

	if (m_Size >= size_bytes) {
		if (clear_after) {
			ClearBuffer();
		}
		return;
	}

	WGPUBuffer old_buffer = m_Buffer;
	Create(size_bytes, nullptr);
	
	ClearBuffer();
	if (!clear_after) {
		// TODO: replace
		//glCopyNamedBufferSubData(old_buffer, m_Buffer, 0, 0, m_Size);
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	if (old_buffer) {
		wgpuBufferDestroy(old_buffer);
	}

	m_Size = size_bytes;
#ifdef SSBO_DEBUG
	m_NumberResizes++;
#endif
}

////////////////////////////////////////////////////////////////////////////////
void WebGPUBuffer::ClearBuffer() {

	unsigned char* tmp = new unsigned char[m_Size];
	memset(tmp, 0, m_Size);
	wgpuQueueWriteBuffer(Core::Singleton().GetWebGPUQueue(), m_Buffer, 0, tmp, m_Size);
	delete[] tmp;

	// TODO: replace
	//GLubyte val = 0;
	//glClearNamedBufferData(m_Buffer, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &val);
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<unsigned char[]> WebGPUBuffer::MakeCopy(int max_size) {
	max_size = max_size >= 0 ? max_size : m_Size;
	if (max_size <= 0) {
		return std::shared_ptr<unsigned char[]>(nullptr);
	}
	unsigned char* ptr = new unsigned char[max_size];
	// TODO: replace
	//glGetNamedBufferSubData(m_Buffer, 0, max_size, ptr);
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	return std::shared_ptr<unsigned char[]>(ptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void WebGPURenderBuffer::Init(std::vector<WGPUVertexFormat> attributes, WGPUPrimitiveTopology topology, unsigned char* vertex_data, int vertex_data_size, std::vector<uint16_t> index_data) {

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
			case WGPUVertexFormat_Force32:
			case WGPUVertexFormat_Undefined: item.p_Size = 0; break;
		}
		vertex_bytes += item.p_Size;
		m_Attributes.push_back(item);
	}

	m_Topology = topology;
	m_NumVertices = vertex_data_size / vertex_bytes;
	m_NumIndices = (int)index_data.size();
	m_VertexBuffer = new WebGPUBuffer(WGPUBufferUsage_Vertex, vertex_data, vertex_data_size);
	if (!index_data.empty()) {
		m_IndexBuffer = new WebGPUBuffer(WGPUBufferUsage_Index, (unsigned char*)&index_data[0], sizeof(uint16_t) * m_NumIndices);
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
void WebGPUTexture::Init(int width, int height, int depth, WGPUTextureFormat format, WGPUTextureDimension dimension, WGPUTextureViewDimension view_dimension, WGPUTextureUsageFlags usage, WGPUTextureAspect aspect, int mip_maps) {

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
	descriptor.label = nullptr;
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
	view_desc.label = nullptr;
	view_desc.nextInChain = nullptr;
	view_desc.arrayLayerCount = depth;
	view_desc.aspect = aspect;
	view_desc.baseArrayLayer = 0;
	view_desc.baseMipLevel = 0;
	view_desc.dimension = view_dimension;
	view_desc.format = format;
	view_desc.mipLevelCount = mip_maps;

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
	tex_cpy.nextInChain = nullptr;
	tex_cpy.mipLevel = mip_map;
	tex_cpy.origin.x = 0;
	tex_cpy.origin.y = 0;
	tex_cpy.origin.z = layer;
	tex_cpy.texture = m_Texture;
	tex_cpy.aspect = WGPUTextureAspect_All;
	WGPUTextureDataLayout tex_layout;
	tex_layout.nextInChain = nullptr;
	tex_layout.bytesPerRow = bytes_per_row;
	tex_layout.rowsPerImage = hei;
	tex_layout.offset = 0;
	WGPUExtent3D tex_extent;
	tex_extent.width = wid;
	tex_extent.height = hei;
	tex_extent.depthOrArrayLayers = 1;

	wgpuQueueWriteTexture(Core::Singleton().GetWebGPUQueue(), &tex_cpy, data, bytes_per_row * hei, &tex_layout, &tex_extent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
WebGPUTextureView::WebGPUTextureView(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect, int layers, int mipmaps) {
	WGPUTextureViewDescriptor view_desc;
	view_desc.nextInChain = nullptr;
	view_desc.label = nullptr;
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
	desc.label = nullptr;
	desc.compare = WGPUCompareFunction_Undefined;
	desc.addressModeU = mode;
	desc.addressModeV = mode;
	desc.addressModeW = mode;
	desc.lodMinClamp = 0;
	desc.lodMaxClamp = 32;
	desc.magFilter = filter;
	desc.minFilter = filter;
	desc.maxAnisotropy = 1;

	// odd inconsistency between web and native - web is probably more logical here
#ifdef __EMSCRIPTEN__
	desc.mipmapFilter = linear_mipmaps ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
#else
	desc.mipmapFilter = linear_mipmaps ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;
#endif

	m_Sampler = wgpuDeviceCreateSampler(Core::Singleton().GetWebGPUDevice(), &desc);
}

////////////////////////////////////////////////////////////////////////////////
WebGPUSampler::~WebGPUSampler(void) {
	wgpuSamplerRelease(m_Sampler);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WebGPURenderPipeline::~WebGPURenderPipeline(void) {
	if (m_Pipeline) {
		wgpuRenderPipelineRelease(m_Pipeline);
	}
	if (m_BindGroup) {
		wgpuBindGroupRelease(m_BindGroup);
	}
	if (m_BindGroupLayout) {
		wgpuBindGroupLayoutRelease(m_BindGroupLayout);
	}
}

////////////////////////////////////////////////////////////////////////////////
void WebGPURenderPipeline::Finalize(QString shader_name, WebGPURenderBuffer& render_buffer) {
#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt0("WebGPURenderPipeline::Finalize");
#endif
	m_RenderBuffer = &render_buffer;

	WGPUShaderModule shader = Core::GetShader(shader_name)->Get();

	int binding_num = 0;
	int num_bindings = int(m_Buffers.size() + m_Textures.size() + m_Samplers.size());
	std::vector<WGPUBindGroupLayoutEntry> layout_entries;
	layout_entries.resize(num_bindings);

	for (auto buffer : m_Buffers) {
		WGPUBindGroupLayoutEntry& layout_entry = layout_entries[binding_num];
		layout_entry.nextInChain = nullptr;
		layout_entry.binding = binding_num;
		layout_entry.visibility = buffer.p_VisibilityFlags;

		WGPUBufferBindingLayout buffer_layout;
		buffer_layout.nextInChain = nullptr;
		buffer_layout.hasDynamicOffset = false;
		buffer_layout.minBindingSize = 0;
		if (buffer.p_Buffer.GetFlags() & WGPUBufferUsage_Uniform) {
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
		layout_entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

		WGPUTextureBindingLayout buff_texture;
		buff_texture.nextInChain = nullptr;
		buff_texture.multisampled = false;
		buff_texture.viewDimension = tex->GetViewDimension();
		buff_texture.sampleType = WGPUTextureSampleType_Float;

		layout_entry.texture = buff_texture;
		binding_num++;
	}
	for (auto sampler : m_Samplers) {
		WGPUBindGroupLayoutEntry& layout_entry = layout_entries[binding_num];
		layout_entry.nextInChain = nullptr;
		layout_entry.binding = binding_num;
		layout_entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

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

	// pipeline layout (used by the render pipeline, released after its creation)
	WGPUPipelineLayoutDescriptor layout_desc = {};
	layout_desc.bindGroupLayoutCount = 1;
	layout_desc.bindGroupLayouts = &m_BindGroupLayout;
	WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(Core::Singleton().GetWebGPUDevice(), &layout_desc);

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
	blend.color.operation = WGPUBlendOperation_Add;
	blend.color.srcFactor = WGPUBlendFactor_One;
	blend.color.dstFactor = WGPUBlendFactor_One;
	blend.alpha.operation = WGPUBlendOperation_Add;
	blend.alpha.srcFactor = WGPUBlendFactor_One;
	blend.alpha.dstFactor = WGPUBlendFactor_One;

	WGPUColorTargetState colorTarget = {};
	colorTarget.format = WGPUTextureFormat_BGRA8Unorm;

	colorTarget.blend = &blend;
	colorTarget.writeMask = WGPUColorWriteMask_All;

	WGPUDepthStencilState depthState = {};
	depthState.depthCompare = WGPUCompareFunction_Less;
	depthState.depthWriteEnabled = true;
	depthState.format = WGPUTextureFormat_Depth24Plus;
	depthState.stencilReadMask = 0;
	depthState.stencilWriteMask = 0;
	depthState.stencilFront.compare = WGPUCompareFunction_Always;
	depthState.stencilBack.compare = WGPUCompareFunction_Always;

	{
		WGPUFragmentState fragment = {};
		fragment.module = shader;
		fragment.entryPoint = "frag_main";
		fragment.targetCount = 1;
		fragment.targets = &colorTarget;

		WGPURenderPipelineDescriptor desc = {};
		desc.fragment = &fragment;
		desc.depthStencil = &depthState;

		// Other state
		desc.layout = pipelineLayout;

		desc.vertex.module = shader;
		desc.vertex.entryPoint = "vertex_main";
		desc.vertex.bufferCount = 1;
		desc.vertex.buffers = &vertex_buffer_layout;

		desc.multisample.count = 1;
		desc.multisample.mask = 0xFFFFFFFF;
		desc.multisample.alphaToCoverageEnabled = false;

#pragma msg("culling mode needs to be parameterized")
		desc.primitive.frontFace = WGPUFrontFace_CCW;
		desc.primitive.cullMode = WGPUCullMode_None;
		desc.primitive.topology = render_buffer.GetTopology();
		desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

#ifdef NESHNY_WEBGPU_PROFILE
	DebugTiming dt1("WebGPURenderPipeline::Finalize wgpuDeviceCreateRenderPipeline");
#endif
		m_Pipeline = wgpuDeviceCreateRenderPipeline(Core::Singleton().GetWebGPUDevice(), &desc);
	}
	wgpuPipelineLayoutRelease(pipelineLayout);

	RefreshBindings();
}

////////////////////////////////////////////////////////////////////////////////
void WebGPURenderPipeline::RefreshBindings(void) {

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
	for (auto buffer : m_Buffers) {
		WGPUBindGroupEntry& entry = entries[binding_num];
		entry.nextInChain = nullptr;
		entry.binding = binding_num;
		entry.offset = 0;
		entry.size = buffer.p_Buffer.GetSizeBytes();
		entry.buffer = buffer.p_Buffer.Get();
		binding_num++;
	}
	for (auto tex : m_Textures) {
		WGPUBindGroupEntry& entry = entries[binding_num];
		entry.nextInChain = nullptr;
		entry.binding = binding_num;
		entry.offset = 0;
		entry.textureView = tex->GetTextureView();
		binding_num++;
	}
	for (auto sampler : m_Samplers) {
		WGPUBindGroupEntry& entry = entries[binding_num];
		entry.nextInChain = nullptr;
		entry.binding = binding_num;
		entry.offset = 0;
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
void WebGPURenderPipeline::Render(WGPURenderPassEncoder pass, int instances) {
	wgpuRenderPassEncoderSetPipeline(pass, m_Pipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, m_BindGroup, 0, 0);
	wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_RenderBuffer->GetVertex(), 0, WGPU_WHOLE_SIZE);

	if (m_RenderBuffer->GetIndex()) {
		wgpuRenderPassEncoderSetIndexBuffer(pass, m_RenderBuffer->GetIndex(), WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
		wgpuRenderPassEncoderDrawIndexed(pass, m_RenderBuffer->GetNumIndices(), instances, 0, 0, 0);
	} else {
		wgpuRenderPassEncoderDraw(pass, m_RenderBuffer->GetNumVertices(), instances, 0, 0);
	}
}

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
Token WebGPURTT::Activate(std::vector<Mode> color_attachments, bool capture_depth_stencil, int width, int height, bool clear) {

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
			tex->Init2D(m_Width, m_Height, WGPUTextureFormat_BGRA8Unorm, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding, 1);
			m_ColorTextures.push_back(tex);
		}
		if (m_CaptureDepthStencil) {
			m_DepthTex = new WebGPUTexture();
			m_DepthTex->InitDepth(m_Width, m_Height);
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
		}

		m_PassDescriptor.colorAttachmentCount = num_color_tex;
		m_PassDescriptor.colorAttachments = &m_ColorDescriptors[0];

		if (m_DepthTex) {
			m_DepthDesc.view = m_DepthTex->GetTextureView();
			m_PassDescriptor.depthStencilAttachment = &m_DepthDesc;
		} else {
			m_PassDescriptor.depthStencilAttachment = nullptr;
		}
	}

	for (auto& color_desc : m_ColorDescriptors) {
		color_desc.loadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
	}
	m_DepthDesc.depthLoadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;

	m_ActiveEncoder = wgpuDeviceCreateCommandEncoder(Core::Singleton().GetWebGPUDevice(), nullptr);

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
		color_desc.loadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
		color_desc.storeOp = WGPUStoreOp_Store;
		color_desc.clearValue.r = 0.0f;
		color_desc.clearValue.g = 0.0f;
		color_desc.clearValue.b = 0.0f;
		color_desc.clearValue.a = 1.0f;
	}

	m_PassDescriptor.colorAttachmentCount = num_color_tex;
	m_PassDescriptor.colorAttachments = &m_ColorDescriptors[0];

	if (depth_tex) {
		m_DepthDesc.view = depth_tex;
		m_DepthDesc.depthLoadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
		m_PassDescriptor.depthStencilAttachment = &m_DepthDesc;
	} else {
		m_PassDescriptor.depthStencilAttachment = nullptr;
	}

	m_ActiveEncoder = wgpuDeviceCreateCommandEncoder(Core::Singleton().GetWebGPUDevice(), nullptr);

	return Token([this]() {
		WGPUCommandBuffer commands = wgpuCommandEncoderFinish(m_ActiveEncoder, nullptr);
		wgpuCommandEncoderRelease(m_ActiveEncoder);
		m_ActiveEncoder = nullptr;
		wgpuQueueSubmit(Core::Singleton().GetWebGPUQueue(), 1, &commands);
		wgpuCommandBufferRelease(commands);
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WebGPURTT::Render(WebGPURenderPipeline* pipeline, int instances) {
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(m_ActiveEncoder, &m_PassDescriptor);
	if (pipeline) {
		pipeline->Render(pass, instances);
	}
	wgpuRenderPassEncoderEnd(pass);
	wgpuRenderPassEncoderRelease(pass);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Token WebGPURTT::RenderPassToken(WGPURenderPassEncoder& pass) {
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