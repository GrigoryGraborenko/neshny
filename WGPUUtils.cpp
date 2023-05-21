//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
WGPUShader::WGPUShader(void) :
	m_Shader	( nullptr )
{
}

////////////////////////////////////////////////////////////////////////////////
WGPUShader::~WGPUShader(void) {
	wgpuShaderModuleRelease(m_Shader);
}

////////////////////////////////////////////////////////////////////////////////
void WGPUShader::CompilationInfoCallback(WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const* compilationInfo, void* userdata) {

	WGPUShader* shader = (WGPUShader*)userdata;
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
bool WGPUShader::Init(const std::function<QByteArray(QString, QString&)>& loader, QString filename, QString insertion) {

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

} // namespace Neshny