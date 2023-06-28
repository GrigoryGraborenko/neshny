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
	void											Read			( unsigned char* buffer, int offset = 0, int size = -1 );
	std::shared_ptr<unsigned char[]>				MakeCopy		( int max_size = -1 );

	inline WGPUBuffer								Get				( void ) { return m_Buffer; }
	inline int										GetSizeBytes	( void ) const { return m_Size; }
	inline WGPUBufferUsageFlags						GetFlags		( void ) const { return m_Flags; }

	template<class T>
	inline void										SetSingleValue(int index, T value) {
		wgpuQueueWriteBuffer(GetCoreQueue(), m_Buffer, index * sizeof(T), (unsigned char*)&value, sizeof(T));
	}

	template<class T>
	inline void										SetValues(const std::vector<T>& items, int offset = 0) {
		if (items.empty()) {
			return;
		}
		wgpuQueueWriteBuffer(GetCoreQueue(), m_Buffer, offset, (unsigned char*)&items[0], items.size() * sizeof(T));
	}

	template<class T>
	inline T										GetSingleValue(int index) {
		T result;
		Read((unsigned char*)&result, index * sizeof(T), sizeof(T));
		return result;
	}

	template<class T>
	inline void										GetValues(std::vector<T>& items, int count, int offset = 0) {
		if (count <= 0) {
			return;
		}
		items.resize(count);
		Read((unsigned char*)&(items[0]), offset * sizeof(T), count * sizeof(T));
	}

protected:

	void											Init(void);
	void											Create(int size, unsigned char* data);

	WGPUBufferUsageFlags							m_Flags = 0;
	WGPUBuffer										m_Buffer = nullptr;
	int												m_Size = 0;

private:

	WGPUQueue										GetCoreQueue(void);
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


													WebGPURenderBuffer		( void ) {}

	void											Init					( WGPUVertexFormat attribute, WGPUPrimitiveTopology topology, unsigned char* vertex_data, int vertex_data_size, std::vector<uint16_t> index_data = {} ) { Init(std::vector<WGPUVertexFormat>{ attribute }, topology, vertex_data, vertex_data_size, index_data); }
	void											Init					( std::vector<WGPUVertexFormat> attributes, WGPUPrimitiveTopology topology, unsigned char* vertex_data, int vertex_data_size, std::vector<uint16_t> index_data = {} );

			 										~WebGPURenderBuffer		( void );

	inline const std::vector<VertexFormatItem>&		GetFormat				( void ) { return m_Attributes; }
	inline WGPUBuffer								GetVertex				( void ) { return m_VertexBuffer->Get(); }
	inline WGPUBuffer								GetIndex				( void ) { return m_IndexBuffer ? m_IndexBuffer->Get() : nullptr; }
	inline WebGPUBuffer*							GetVertexBuffer			( void ) { return m_VertexBuffer; }
	inline WebGPUBuffer*							GetIndexBuffer			( void ) { return m_IndexBuffer; }
	inline WGPUPrimitiveTopology					GetTopology				( void ) { return m_Topology; }
	inline int										GetNumVertices			( void ) { return m_NumVertices; }
	inline int										GetNumIndices			( void ) { return m_NumIndices; }

protected:

	WebGPUBuffer*									m_VertexBuffer = nullptr;
	WebGPUBuffer*									m_IndexBuffer = nullptr;
	WGPUPrimitiveTopology							m_Topology;
	int												m_NumVertices = -1;
	int												m_NumIndices = -1;
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
		WGPUShaderStageFlags	p_VisibilityFlags;
		bool					p_ReadOnly;
	};

								WebGPURenderPipeline	( void ) {}
								~WebGPURenderPipeline	( void );

	WebGPURenderPipeline&		AddBuffer				( WebGPUBuffer& buffer, WGPUShaderStageFlags visibility_flags, bool read_only ) { m_Buffers.push_back({ buffer, visibility_flags, read_only }); return *this; }
	WebGPURenderPipeline&		AddTexture				( const WebGPUTexture& texture ) { m_Textures.push_back(&texture); return *this; }
	WebGPURenderPipeline&		AddSampler				( const WebGPUSampler& sampler ) { m_Samplers.push_back(&sampler); return *this; }

	void						Finalize				( QString shader_name, WebGPURenderBuffer& render_buffer );
	void						RefreshBindings			( void );
	void						Render					( WGPURenderPassEncoder pass, int instances = 1 );

	inline WGPURenderPipeline	GetPipeline				( void ) { return m_Pipeline; }
	inline WGPUBindGroup		GetBindGroup			( void ) { return m_BindGroup; }

protected:

	std::vector<Buffer>			m_Buffers;
	std::vector<const WebGPUTexture*>	m_Textures;
	std::vector<const WebGPUSampler*>	m_Samplers;
	WebGPURenderBuffer*			m_RenderBuffer = nullptr;

	WGPURenderPipeline			m_Pipeline = nullptr;
	WGPUBindGroupLayout			m_BindGroupLayout = nullptr;
	WGPUBindGroup				m_BindGroup = nullptr;

};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPURTT {

public:

	enum class Mode {
		RGBA
		//,RGBA_FLOAT32
		//,RGBA_FLOAT16
	};

									WebGPURTT		( void );
									~WebGPURTT		( void ) { Destroy(); }

	Token							Activate		( std::vector<Mode> color_attachments, bool capture_depth_stencil, int width, int height, bool clear = true );
	Token							Activate		( std::vector<WGPUTextureView> color_attachments, WGPUTextureView depth_tex, bool clear = true );

	void							Render			( WebGPURenderPipeline* pipeline, int instances = 1 );
	Token							RenderPassToken	( WGPURenderPassEncoder& pass );

	inline void						ClearBeforeNextRender	( void ) { m_ClearNext = true; }
	inline WGPUTextureView			GetColorTex				( int index ) { return index >= m_ColorTextures.size() ? nullptr : m_ColorTextures[index]->GetTextureView(); }
	inline WGPUTextureView			GetDepthTex				( void ) { return m_DepthTex ? m_DepthTex->GetTextureView() : nullptr; }

private:

	void							Destroy		( void );

	std::vector<Mode>				m_Modes = {};
	bool							m_CaptureDepthStencil = false;
	int								m_Width = 0;
	int								m_Height = 0;
	std::vector<WebGPUTexture*>		m_ColorTextures;
	WebGPUTexture*					m_DepthTex = nullptr;

	std::vector<WGPURenderPassColorAttachment>	m_ColorDescriptors;
	WGPURenderPassDepthStencilAttachment		m_DepthDesc;
	WGPURenderPassDescriptor		m_PassDescriptor;
	WGPUCommandEncoder				m_ActiveEncoder = nullptr;
	bool							m_ClearNext = true;
};

typedef WebGPURTT RTT;

} // namespace Neshny