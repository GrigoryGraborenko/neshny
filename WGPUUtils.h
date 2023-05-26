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
		QByteArray					m_Message;
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

													WebGPUBuffer		( WGPUBufferUsageFlags flags, int size = 0 );
													WebGPUBuffer		( WGPUBufferUsageFlags flags, unsigned char* data, int size );
			 										~WebGPUBuffer		( void );

	void											EnsureSizeBytes	( int size_bytes, bool clear_after = true );
	void											ClearBuffer		( void );
	std::shared_ptr<unsigned char[]>				MakeCopy		( int max_size = -1 );

	inline WGPUBuffer								Get				( void ) { return m_Buffer; }
	inline int										GetSizeBytes	( void ) const { return m_Size; }
	inline WGPUBufferUsageFlags						GetFlags		( void ) const { return m_Flags; }

	template<class T>
	inline void										SetSingleValue(int index, T value) {
		// TODO: replace
		//glNamedBufferSubData(m_Buffer, index * sizeof(T), sizeof(T), &value);
	}

	template<class T>
	inline void										SetValues(const std::vector<T>& items, int offset = 0) {
		if (items.empty()) {
			return;
		}
		// TODO: replace
		//glNamedBufferSubData(m_Buffer, offset * sizeof(T), items.size() * sizeof(T), &(items[0]));
	}

	template<class T>
	inline T										GetSingleValue(int index) {
		T result;
		// TODO: replace
		//glGetNamedBufferSubData(m_Buffer, index * sizeof(T), sizeof(T), &result);
		return result;
	}

	template<class T>
	inline void										GetValues(std::vector<T>& items, int count, int offset = 0) {
		if (count <= 0) {
			return;
		}
		items.resize(count);
		// TODO: replace
		//glGetNamedBufferSubData(m_Buffer, offset * sizeof(T), count * sizeof(T), &(items[0]));
	}

protected:

	void											Create(int size, unsigned char* data);

	WGPUBufferUsageFlags							m_Flags = 0;
	WGPUBuffer										m_Buffer = nullptr;
	int												m_Size = 0;

};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPURenderBuffer {

public:

	struct VertexFormatItem {
		WGPUVertexFormat	p_Type;
		int					p_Size;
		QString				p_TypeName;
	};


													WebGPURenderBuffer		( WGPUVertexFormat attribute, std::vector<float> vertex_data, std::vector<uint16_t> index_data = {} ): WebGPURenderBuffer(std::vector<WGPUVertexFormat>{ attribute  }, vertex_data, index_data) {}
													WebGPURenderBuffer		( std::vector<WGPUVertexFormat> attributes, std::vector<float> vertex_data, std::vector<uint16_t> index_data );

			 										~WebGPURenderBuffer		( void );

	inline const std::vector<VertexFormatItem>&		GetFormat				( void ) { return m_Attributes; }
	inline WGPUBuffer								GetVertex				( void ) { return m_VertexBuffer->Get(); }
	inline WGPUBuffer								GetIndex				( void ) { return m_IndexBuffer ? m_IndexBuffer->Get() : nullptr; }
	inline WebGPUBuffer*							GetVertexBuffer			( void ) { return m_VertexBuffer; }
	inline WebGPUBuffer*							GetIndexBuffer			( void ) { return m_IndexBuffer; }

protected:

	WebGPUBuffer*									m_VertexBuffer = nullptr;
	WebGPUBuffer*									m_IndexBuffer = nullptr;
	std::vector<VertexFormatItem>					m_Attributes;
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
																			WGPUTextureFormat format = WGPUTextureFormat_BGRA8Unorm,
																			WGPUTextureUsageFlags usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = AUTO_MIPMAPS
																		) { Init(width, height, 1, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }
	void											InitCubeMap			(	int width,
																			int height,
																			WGPUTextureFormat format = WGPUTextureFormat_BGRA8Unorm,
																			WGPUTextureUsageFlags usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = 1
																		) { Init(width, height, 6, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_Cube, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }
	inline void										InitDepth			( int width, int height ) { Init(width, height, 1, WGPUTextureFormat_Depth24Plus, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_DepthOnly, 1); }
	inline void										InitDepthStencil	( int width, int height ) { Init(width, height, 1, WGPUTextureFormat_Depth24PlusStencil8, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_All, 1); }
	void											Init2DArray			(	int width,
																			int height,
																			int layers,
																			WGPUTextureFormat format = WGPUTextureFormat_BGRA8Unorm,
																			WGPUTextureUsageFlags usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = AUTO_MIPMAPS
																		) { Init(width, height, layers, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_2DArray, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }

	void											Init				(	int width, int height, int depth,
																			WGPUTextureFormat format,
																			WGPUTextureDimension dimension,
																			WGPUTextureViewDimension view_dimension,
																			WGPUTextureUsageFlags usage,
																			WGPUTextureAspect aspect,
																			int mip_maps );

	inline void										CopyData2D			( unsigned char* data, int bytes_per_pixel, int bytes_per_row, bool auto_mipmap = true ) { CopyDataLayer(0, data, bytes_per_pixel, bytes_per_row, auto_mipmap); }
	void											CopyDataLayer		( int layer, unsigned char* data, int bytes_per_pixel, int bytes_per_row, bool auto_mipmap = true );
	void											CopyDataLayerMipMap	( int layer, int mip_map, unsigned char* data, int bytes_per_pixel, int bytes_per_row );

	inline WGPUTexture								GetTexture			( void ) const { return m_Texture; }
	inline WGPUTextureView							GetTextureView		( void ) const { return m_View; }
	inline int										GetWidth			( void ) const { return m_Width; }
	inline int										GetHeight			( void ) const { return m_Height; }
	inline int										GetLayers			( void ) const { return m_Layers; }
	inline int										GetMipMaps			( void ) const { return m_MipMaps; }
	inline WGPUTextureFormat						GetFormat			( void ) const { return m_Format; }
	inline int										GetDepthBytes		( void ) const { return m_DepthBytes; }
	inline WGPUTextureViewDimension					GetViewDimension	( void ) const { return m_ViewDimension; }

protected:

	int												GetMipMaps			( int width, int height, int mip_maps );

	WGPUTexture										m_Texture = nullptr;
	WGPUTextureView									m_View = nullptr;
	WGPUTextureFormat								m_Format;
	WGPUTextureViewDimension						m_ViewDimension;
	int												m_Width = 0;
	int												m_Height = 0;
	int												m_Layers = 1;
	int												m_MipMaps = 1;
	int												m_DepthBytes = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUTextureView {

public:

													WebGPUTextureView		( WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect, int layers, int mipmaps );
			 										~WebGPUTextureView		( void );

	inline WGPUTextureView							Get						( void ) const { return m_View; }

protected:

	WGPUTextureView									m_View = nullptr;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUSampler {

public:

													WebGPUSampler		( WGPUAddressMode mode, WGPUFilterMode filter = WGPUFilterMode_Linear, bool linear_mipmaps = true, unsigned int max_anisotropy = 1 );
			 										~WebGPUSampler		( void );

	inline WGPUSampler								Get					( void ) const { return m_Sampler; }

	inline WGPUAddressMode							GetMode				( void ) const { return m_Mode; }
	inline WGPUFilterMode							GetFilter			( void ) const { return m_Filter; }
	inline bool										GetLinearMipMaps	( void ) const { return m_LinearMipMaps; }
	inline unsigned int								GetMaxAnisotropy	( void ) const { return m_MaxAnisotropy; }

protected:

	WGPUSampler										m_Sampler = nullptr;
	WGPUAddressMode									m_Mode;
	WGPUFilterMode									m_Filter;
	bool											m_LinearMipMaps;
	unsigned int									m_MaxAnisotropy;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPURenderPipeline {

public:

	struct Buffer {
		WebGPUBuffer&			p_Buffer;
		bool					p_ReadOnly = true;
	};

								WebGPURenderPipeline	( void ) {}
								~WebGPURenderPipeline	( void );

	WebGPURenderPipeline&		AddBuffer				( WebGPUBuffer& buffer, bool read_only = false ) { m_Buffers.push_back({ buffer, read_only }); return *this; }
	WebGPURenderPipeline&		AddTexture				( const WebGPUTexture& texture ) { m_Textures.push_back(&texture); return *this; }
	WebGPURenderPipeline&		AddSampler				( const WebGPUSampler& sampler ) { m_Samplers.push_back(&sampler); return *this; }

	void						Finalize				( QString shader_name, WebGPURenderBuffer* render_buffer );

	inline WGPURenderPipeline	GetPipeline				( void ) { return m_Pipeline; }
	inline WGPUBindGroup		GetBindGroup			( void ) { return m_BindGroup; }

protected:

	std::vector<Buffer>			m_Buffers;
	std::vector<const WebGPUTexture*>	m_Textures;
	std::vector<const WebGPUSampler*>	m_Samplers;

	WGPURenderPipeline			m_Pipeline;
	WGPUBindGroup				m_BindGroup;

};

} // namespace Neshny