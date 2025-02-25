////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_WEBGPU

namespace Neshny {

// TODO: create global state for device, etc rather than using core, to make core optional
WGPUDevice GlobalWebGPUDevice();

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUShader {

public:

	struct Error {
		WGPUCompilationMessageType	m_Type;
		std::string					m_Message;
		uint64_t					m_LineNum = 0;
		uint64_t					m_LinePos = 0;
	};

													WebGPUShader			( void );
													~WebGPUShader			( void );

	bool											Init					( const std::function<std::string(std::string_view, std::string&)>& loader, std::string_view filename, std::string_view start_insert, std::string_view end_insert );

	inline WGPUShaderModule							Get						( void ) const { return m_Shader; }
	inline bool										IsValid					( void ) const { return (m_Shader != nullptr) && m_Errors.empty(); }

	inline const std::vector<Error>&				GetErrors				( void ) const { return m_Errors; }
	inline std::string_view							GetSource				( void ) const { return m_Source; }
	inline std::string_view							GetRawSource			( void ) const { return m_SourcePrePreProcessor; }

private:

	static void										CompilationInfoCallback ( WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const* compilationInfo, void* userdata );

	WGPUShaderModule					m_Shader;
	std::string							m_Source;
	std::string							m_SourcePrePreProcessor;
	std::vector<Error>					m_Errors;

};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPUBuffer {

public:

	template<typename T>
	struct AsyncToken {
		static AsyncToken Empty() { AsyncToken token; token.m_Internals->m_Finished = true; return token; }

		bool IsFinished() { return m_Internals->m_Finished; }
		bool IsError() { return m_Internals->m_Error; }
		std::shared_ptr<T> GetPayload(void) { return m_Internals ? (m_Internals->m_Payload ? m_Internals->m_Payload : nullptr) : nullptr; }
		AsyncToken(const AsyncToken& other): m_Internals(other.m_Internals) {}
		bool operator==(const AsyncToken& other) const { return other.m_Internals.get() == m_Internals.get(); }
		void Wait() {
			while (!m_Internals->m_Finished) {
#ifndef __EMSCRIPTEN__
				wgpuDeviceTick(GlobalWebGPUDevice());
#else
				emscripten_sleep(0);
#endif
			}
		}
	private:
		struct Internals {
			bool					m_Finished = false;
			bool					m_Error = false;
			std::shared_ptr<T>		m_Payload = nullptr;
		};
		AsyncToken() : m_Internals(new Internals{}) {}
		std::shared_ptr<Internals>	m_Internals = nullptr;
		friend class WebGPUBuffer;
	};

													WebGPUBuffer			( WGPUBufferUsage flags, int size = 0 );
													WebGPUBuffer			( WGPUBufferUsage flags, unsigned char* data, int size );
			 										~WebGPUBuffer			( void );

	void											EnsureSizeBytes			( int size_bytes, bool clear_after = true );
	void											ClearBuffer				( void );
	void											ReadSync				( unsigned char* buffer, int offset = 0, int size = -1 );

	static void										CopyBufferToBuffer		( WGPUBuffer source, WGPUBuffer destination, int source_offset_bytes, int dest_offset_bytes, int size, WGPUCommandEncoder existing_encoder = nullptr );

	template<typename T>
	AsyncToken<T>									Read					( int offset, int size, std::function<std::shared_ptr<T>(unsigned char* data, int size, AsyncToken<T> token)>&& callback ) {
		if ((offset + size) > m_Size) {
			throw std::invalid_argument("Attempt to read past end of buffer");
		}
		WGPUBufferDescriptor desc = {};
		desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
		desc.size = size;
		desc.nextInChain = nullptr;
		desc.label = nullptr;
		desc.mappedAtCreation = false;
		WGPUBuffer copy_buffer = wgpuDeviceCreateBuffer(GlobalWebGPUDevice(), &desc);
		CopyBufferToBuffer(m_Buffer, copy_buffer, offset, 0, size);

		struct AsyncInfo {
			WGPUBuffer p_Buffer;
			std::function<std::shared_ptr<T>(unsigned char*, int, AsyncToken<T>)> p_Callback;
			int p_Size;
			AsyncToken<T> p_Token;
		};
		AsyncToken<T> token;
		AsyncInfo* info = new AsyncInfo{
			copy_buffer,
			std::move(callback),
			size,
			token
		};

		// since you are copying from a temp buffer that already took the offset into account, offset must be set to zero
		wgpuBufferMapAsync(copy_buffer, WGPUMapMode_Read, 0, size, [](WGPUBufferMapAsyncStatus status, void* user_data) {
			auto info = (AsyncInfo*)user_data;
			if (status == WGPUMapAsyncStatus_Success) {
				auto buffer_data = wgpuBufferGetConstMappedRange(info->p_Buffer, 0, info->p_Size);
				info->p_Token.m_Internals->m_Payload = info->p_Callback((unsigned char*)buffer_data, info->p_Size, info->p_Token);
 				wgpuBufferUnmap(info->p_Buffer);
			} else {
				info->p_Token.m_Internals->m_Error = true;
			}
			wgpuBufferDestroy(info->p_Buffer);
			info->p_Token.m_Internals->m_Finished = true;
			delete info;
		}, info);
		return token;
	}
	void											Write					( unsigned char* buffer, int offset, int size );
	std::shared_ptr<unsigned char[]>				MakeCopy				( int max_size = -1 );

	inline WGPUBuffer								Get						( void ) { return m_Buffer; }
	inline int										GetSizeBytes			( void ) const { return m_Size; }
	inline WGPUBufferUsage							GetUsageFlags			( void ) const { return m_Flags; }

	template<class T>
	inline void										SetSingleValue(int index, const T& value) {
		Write((unsigned char*)&value, index * sizeof(T), sizeof(T));
	}

	template<class T>
	inline void										SetValues(const std::vector<T>& items, int offset = 0) {
		if (items.empty()) {
			return;
		}
		Write((unsigned char*)&items[0], offset * sizeof(T), items.size() * sizeof(T));
	}

	template<class T>
	inline T										GetSingleValue(int index) {
		T result;
		ReadSync((unsigned char*)&result, index * sizeof(T), sizeof(T));
		return result;
	}

	template<class T>
	inline void										GetValues(std::vector<T>& items, int count, int offset = 0) {
		if (count <= 0) {
			return;
		}
		items.resize(count);
		ReadSync((unsigned char*)&(items[0]), offset * sizeof(T), count * sizeof(T));
	}

	template<class T>
	inline void										GetAllValues(std::vector<T>& items) {
		int count = m_Size / sizeof(T);
		items.resize(count);
		ReadSync((unsigned char*)&(items[0]), 0, m_Size);
	}

protected:

	void											Init(void);
	void											Create(int size, unsigned char* data);

	WGPUBufferUsage									m_Flags = 0;
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
		std::string			p_TypeName;
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
																			WGPUTextureUsage usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = AUTO_MIPMAPS
																		) { Init(width, height, 1, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }
	void											InitCubeMap			(	int width,
																			int height,
																			WGPUTextureFormat format = WGPUTextureFormat_BGRA8Unorm,
																			WGPUTextureUsage usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = 1
																		) { Init(width, height, 6, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_Cube, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }
	inline void										InitDepth			( int width, int height ) { Init(width, height, 1, WGPUTextureFormat_Depth24Plus, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_DepthOnly, 1); }
	inline void										InitDepthStencil	( int width, int height ) { Init(width, height, 1, WGPUTextureFormat_Depth24PlusStencil8, WGPUTextureDimension_2D, WGPUTextureViewDimension_2D, WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_All, 1); }
	void											Init2DArray			(	int width,
																			int height,
																			int layers,
																			WGPUTextureFormat format = WGPUTextureFormat_BGRA8Unorm,
																			WGPUTextureUsage usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
																			int mip_maps = AUTO_MIPMAPS
																		) { Init(width, height, layers, format, WGPUTextureDimension_2D, WGPUTextureViewDimension_2DArray, usage, WGPUTextureAspect_All, GetMipMaps(width, height, mip_maps)); }

	void											Init				(	int width, int height, int depth,
																			WGPUTextureFormat format,
																			WGPUTextureDimension dimension,
																			WGPUTextureViewDimension view_dimension,
																			WGPUTextureUsage usage,
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
class WebGPUPipeline {

	struct Buffer {
		WebGPUBuffer*			p_Buffer;
		WGPUShaderStage			p_VisibilityFlags;
		bool					p_ReadOnly;
		WGPUBuffer				p_LastSeenBuffer = nullptr;
	};

	struct Texture {
		WGPUTextureViewDimension	p_Dimensions;
		WGPUTextureView				p_TextureView = nullptr;
		WGPUTextureView				p_LastSeenTextureView = nullptr;
	};

public:

	enum class Type {
		UNKNOWN,
		RENDER,
		COMPUTE
	};

	enum class AttachmentMode {
		RGBA
		//,RGBA_FLOAT32
		//,RGBA_FLOAT16
	};

	struct RenderParams {
		RenderParams() {};

		WGPUBlendComponent		p_ColorBlend = { WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha };
		WGPUBlendComponent		p_AlphaBlend = { WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha };
		bool					p_DepthWriteEnabled = true;

		WGPUCompareFunction		p_DepthCompare = WGPUCompareFunction_Less;
		WGPUStencilFaceState	p_StencilFront = { WGPUCompareFunction_Always, WGPUStencilOperation_Keep, WGPUStencilOperation_Keep, WGPUStencilOperation_Keep };
		WGPUStencilFaceState	p_StencilBack = { WGPUCompareFunction_Always, WGPUStencilOperation_Keep, WGPUStencilOperation_Keep, WGPUStencilOperation_Keep };
		uint32_t				p_StencilReadMask = 0;
		uint32_t				p_StencilWriteMask = 0;
		int32_t					p_DepthBias = 0;
		float					p_DepthBiasSlopeScale = 0;
		float					p_DepthBiasClamp = 0;

		WGPUFrontFace			p_FrontFace = WGPUFrontFace_CCW;
		WGPUCullMode			p_CullMode = WGPUCullMode_None;

		std::vector<WebGPUPipeline::AttachmentMode>	p_Attachments = { AttachmentMode::RGBA };
	};

								WebGPUPipeline			( void ) : m_Type(Type::UNKNOWN) {}
								~WebGPUPipeline			( void );
	void						Reset					( void );

	WebGPUPipeline&				AddBuffer				( WebGPUBuffer& buffer, WGPUShaderStage visibility_flags, bool read_only ) { m_Buffers.push_back({ &buffer, visibility_flags, read_only, buffer.Get() }); return *this; }
	WebGPUPipeline&				AddTexture				( WGPUTextureViewDimension texture_dimension, WGPUTextureView view ) { m_Textures.push_back({ texture_dimension, view }); return *this; }
	WebGPUPipeline&				AddSampler				( const WebGPUSampler& sampler ) { m_Samplers.push_back(&sampler); return *this; }

	void						FinalizeRender			( std::string_view shader_name, WebGPURenderBuffer& render_buffer, RenderParams params = {}, std::string_view insertion = std::string_view(), std::string_view end_insertion = std::string_view() );
	void						FinalizeCompute			( std::string_view shader_name, std::string_view insertion = std::string_view(), std::string_view end_insertion = std::string_view() );
	void						RefreshBindings			( void );
	void						ReplaceBuffer			( WebGPUBuffer& original, WebGPUBuffer& replacement );
	void						ReplaceBuffer			( int index, WebGPUBuffer& replacement );
	void						ReplaceTexture			( WGPUTextureView original, WGPUTextureView replacement );
	void						ReplaceTexture			( int index, WGPUTextureView replacement );
	void						Render					( WGPURenderPassEncoder pass, int instances = 1 );
	void						Compute					( int calls, Neshny::iVec3 workgroup_size = Neshny::iVec3(-1, -1, -1), std::optional<std::function<void(WGPUCommandEncoder encoder)>> pre_execute = std::nullopt );

	inline Type					GetType					( void ) { return m_Type; }
	inline WGPURenderPipeline	GetRenderPipeline		( void ) { return m_RenderPipeline; }
	inline WGPUBindGroup		GetBindGroup			( void ) { return m_BindGroup; }

protected:

	void						CheckBuffersUpToDate	( void );
	void						CreateBindGroupLayout	( void );

	Type						m_Type;
	WebGPURenderBuffer*			m_RenderBuffer = nullptr;
	std::vector<Buffer>			m_Buffers;
	std::vector<Texture>		m_Textures;
	
	std::vector<const WebGPUSampler*>	m_Samplers;

	WGPURenderPipeline			m_RenderPipeline = nullptr;
	WGPUComputePipeline			m_ComputePipeline = nullptr;
	WGPUBindGroupLayout			m_BindGroupLayout = nullptr;
	WGPUBindGroup				m_BindGroup = nullptr;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class WebGPURTT {

public:

									WebGPURTT		( void );
									~WebGPURTT		( void ) { Destroy(); }

	Token							Activate		( std::vector<WebGPUPipeline::AttachmentMode> color_attachments, bool capture_depth_stencil, int width, int height, bool clear = true, WGPUTextureView existing_depth_tex = nullptr );
	Token							Activate		( std::vector<WGPUTextureView> color_attachments, WGPUTextureView depth_tex, bool clear = true );

	void							Render			( WebGPUPipeline* pipeline, int instances = 1 );
	Token							RenderPassToken	( WGPURenderPassEncoder& pass );

	inline void						ClearBeforeNextRender	( void ) { m_ClearNext = true; }
	inline WebGPUTexture*			GetColorTex				( int index ) { return index >= m_ColorTextures.size() ? nullptr : m_ColorTextures[index]; }
	inline WebGPUTexture*			GetDepthTex				( void ) { return m_DepthTex ? m_DepthTex : nullptr; }
	inline WGPUTextureView			GetColorTexView			( int index ) { return index >= m_ColorTextures.size() ? nullptr : m_ColorTextures[index]->GetTextureView(); }
	inline WGPUTextureView			GetDepthTexView			( void ) { return m_DepthTex ? m_DepthTex->GetTextureView() : nullptr; }
	inline auto						GetAttachmentModes		( void ) { return m_Modes; }

private:

	void							Destroy		( void );

	bool							m_CaptureDepthStencil = false;
	int								m_Width = 0;
	int								m_Height = 0;
	std::vector<WebGPUTexture*>		m_ColorTextures;
	WebGPUTexture*					m_DepthTex = nullptr;

	std::vector<WebGPUPipeline::AttachmentMode>	m_Modes = {};
	std::vector<WGPURenderPassColorAttachment>	m_ColorDescriptors;
	WGPURenderPassDepthStencilAttachment		m_DepthDesc;
	WGPURenderPassDescriptor		m_PassDescriptor;
	WGPUCommandEncoder				m_ActiveEncoder = nullptr;
	bool							m_ClearNext = true;
};

typedef WebGPUShader Shader;
typedef WebGPURenderBuffer RenderableBuffer;
typedef WebGPUBuffer SSBO;
typedef WebGPURTT RTT;

} // namespace Neshny