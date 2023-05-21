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
			QString(msg.message),
			msg.lineNum,
			msg.linePos
		});
	}
}

////////////////////////////////////////////////////////////////////////////////
bool WebGPUShader::Init(const std::function<QByteArray(QString, QString&)>& loader, QString filename, QString insertion) {

	QString err_msg;
	QByteArray arr = loader(filename, err_msg);
	if (arr.isNull()) {
		m_Errors.push_back({
			WGPUCompilationMessageType_Error,
			"File error - " + err_msg,
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

	m_Shader = wgpuDeviceCreateShaderModule(Core::Singleton().GetDevice(), &desc);
#ifndef __EMSCRIPTEN__
	wgpuShaderModuleGetCompilationInfo(m_Shader, CompilationInfoCallback, this);
#endif

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
WebGPUTexture::~WebGPUTexture(void) {
	wgpuTextureViewRelease(m_View);
	wgpuTextureDestroy(m_Texture);
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
	m_Format = format;
	//m_DepthBytes = 0;

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

	m_Texture = wgpuDeviceCreateTexture(Core::Singleton().GetDevice(), &descriptor);

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

	wgpuQueueWriteTexture(Core::Singleton().GetQueue(), &tex_cpy, data, bytes_per_row * hei, &tex_layout, &tex_extent);
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

} // namespace Neshny