////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_WEBGPU

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WGPUShader {

public:

	struct Error {
		WGPUCompilationMessageType	m_Type;
		QString						m_Message;
		uint64_t					m_LineNum;
		uint64_t					m_LinePos;
	};

													WGPUShader				( void );
													~WGPUShader				( void);

	bool											Init					( const std::function<QByteArray(QString, QString&)>& loader, QString filename, QString insertion );

	inline WGPUShaderModule							Get						( void ) const { return m_Shader; }
	inline bool										IsValid					( void ) const { return m_Shader != nullptr; }

	inline const std::vector<Error>&				GetErrors				( void ) const { return m_Errors; }
	inline QByteArray								GetSource				( void ) const { return m_Source; }

private:

	static void										CompilationInfoCallback ( WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const* compilationInfo, void* userdata );

	WGPUShaderModule					m_Shader;
	QByteArray							m_Source;
	std::vector<Error>					m_Errors;

};

} // namespace Neshny