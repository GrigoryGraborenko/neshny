////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_WEBGPU

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUShader {

public:

	struct Error {
		WGPUCompilationMessageType	m_Type;
		QString						m_Message;
		uint64_t					m_LineNum;
		uint64_t					m_LinePos;
	};

													WebGPUShader			( void );
													~WebGPUShader			( void );

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

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUBuffer {

public:

													WebGPUBuffer		( void );
			 										~WebGPUBuffer		( void );

protected:

};

constexpr int AUTO_MIPMAPS = 65536;
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUTexture {

public:

													WebGPUTexture		( void ) {}
			 										~WebGPUTexture		( void );

	void											Init2D				(	int width,
																			int height,
																			WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm,
																			WGPUTextureUsageFlags usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = AUTO_MIPMAPS
																		) { Init(width, height, 1, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }
	void											InitCubeMap			(	int width,
																			int height,
																			WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm,
																			WGPUTextureUsageFlags usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = AUTO_MIPMAPS
																		) { Init(width, height, 6, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_Cube, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }
	inline void										InitDepth			( int width, int height ) { Init(width, height, 1, WGPUTextureFormat_Depth24Plus, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_DepthOnly, 1); }
	inline void										InitDepthStencil	( int width, int height ) { Init(width, height, 1, WGPUTextureFormat_Depth24PlusStencil8, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_All, 1); }

	void											Init				(	int width, int height, int depth,
																			WGPUTextureFormat format,
																			WGPUTextureDimension dimension,
																			WGPUTextureViewDimension view_dimension,
																			WGPUTextureUsageFlags usage,
																			WGPUTextureAspect aspect,
																			int mip_maps );

	inline void										CopyData			( unsigned char* data, int bytes_per_pixel, int bytes_per_row, bool auto_mipmap = true ) { CopyDataLayer(0, data, bytes_per_pixel, bytes_per_row, auto_mipmap); }
	void											CopyDataLayer		( int layer, unsigned char* data, int bytes_per_pixel, int bytes_per_row, bool auto_mipmap = true );
	void											CopyDataLayerMipMap	( int layer, int mip_map, unsigned char* data, int bytes_per_pixel, int bytes_per_row );

	inline WGPUTexture								GetTexture			( void ) const { return m_Texture; }
	inline WGPUTextureView							GetTextureView		( void ) const { return m_View; }
	inline int										GetWidth			( void ) const { return m_Width; }
	inline int										GetHeight			( void ) const { return m_Height; }
	inline int										GetLayers			( void ) const { return m_Layers; }
	inline WGPUTextureFormat						GetFormat			( void ) const { return m_Format; }
	//inline int										GetDepthBytes		( void ) const { return m_DepthBytes; }

protected:

	int												GetMipMaps			( int width, int height, int mip_maps );

	WGPUTexture										m_Texture = nullptr;
	WGPUTextureView									m_View = nullptr;
	WGPUTextureFormat								m_Format;
	int												m_Width = 0;
	int												m_Height = 0;
	int												m_Layers = 0;
	//int												m_DepthBytes = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUTextureView {

public:

													WebGPUTextureView		( WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect, int layers, int mipmaps );
			 										~WebGPUTextureView		( void );

	WGPUTextureView									m_View = nullptr;
};


} // namespace Neshny