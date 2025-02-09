////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

const auto g_CorrectSDLFormat = SDL_PIXELFORMAT_ARGB8888;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class FileResource : public Resource {
public:
	virtual				~FileResource(void) {}
	virtual bool		FileInit(std::string_view path, unsigned char* data, int length, std::string& err) = 0;

	bool				Load(std::string_view path, std::string& err) {
		std::ifstream file(std::string(path), std::ios::in | std::ios::binary);
		if (!file.is_open()) {
			err = std::format("Could not open {} for reading", path);
			return false;
		}
		std::ostringstream data_stream;
		data_stream << file.rdbuf();
		std::string data = data_stream.str();

		return FileInit(path, (unsigned char*)data.data(), data.size(), err);
	};
};

#ifdef SDL_h_
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class SoundFile : public FileResource {
public:
	struct Params {};

	virtual				~SoundFile(void) { Mix_FreeChunk(m_Chunk); }
	bool				Init(std::string_view path, Params params, std::string& err) { return Load(path, err); }

	virtual uint64_t	GetMemoryEstimate		( void ) const { return m_Chunk->alen; }
	virtual uint64_t	GetGPUMemoryEstimate	( void ) const { return 0; }

	virtual bool		FileInit(std::string_view path, unsigned char* data, int length, std::string& err) {
		SDL_RWops* rw = SDL_RWFromMem(data, length);
		m_Chunk = Mix_LoadWAV_RW(rw, 0);
		SDL_RWclose(rw);
		if (m_Chunk == nullptr) {
			err = "Could not load sound";
			return false;
		}
		return true;
	};

	const Mix_Chunk*	GetChunk				( void ) const { return m_Chunk; }

	void				Play					( void ) {
		Mix_PlayChannel(-1, m_Chunk, 0);
	}

protected:
	Mix_Chunk*	m_Chunk;
};
#endif


#if defined(NESHNY_GL)
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Texture2D : public FileResource {
public:
#pragma pack(push)
#pragma pack (1)
	struct Params {}; // todo: add stuff like linear interp, mipmaps, etc
#pragma pack(pop)

	virtual				~Texture2D(void) {}
	bool				Init(std::string_view path, Params params, std::string& err) { return Load(path, err); }

	virtual bool		FileInit(std::string_view path, unsigned char* data, int length, std::string& err) {
		return m_Texture.Init(std::span<unsigned char>(data, length));
	};

	inline const GLTexture& Get(void) const { return m_Texture; }

	virtual uint64_t	GetMemoryEstimate		( void ) const { return 0; }
	virtual uint64_t	GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes(); }

protected:
	GLTexture	m_Texture;
};

#elif defined(NESHNY_WEBGPU)

const auto g_WebGPUFormat = WGPUTextureFormat_BGRA8Unorm; // not sure why this is the correct combo with g_CorrectSDLFormat

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Texture2D : public Resource {
public:
#pragma pack(push)
#pragma pack (1)
	struct Params {}; // todo: add stuff like linear interp, mipmaps, etc
#pragma pack(pop)

	virtual				~Texture2D(void) {}
	bool				Init(std::string_view path, Params params, std::string& err) {

		SDL_Surface* surface = IMG_Load(path.data());
		if (!surface) {
			err = std::format("Could not load image {}", path);
			return false;
		}

		if (surface->format->format != g_CorrectSDLFormat) {
			SDL_Surface* converted_surface = SDL_ConvertSurfaceFormat(surface, g_CorrectSDLFormat, 0);
			SDL_FreeSurface(surface);
			surface = converted_surface;
		}

		m_Texture.Init2D(surface->w, surface->h, g_WebGPUFormat);
		auto sync_token = Core::Singleton().SyncWithMainThread();
		m_Texture.CopyData2D((unsigned char*)surface->pixels, surface->format->BytesPerPixel, surface->pitch);

		SDL_FreeSurface(surface);
		return true;
	}

	const WebGPUTexture&	Get						( void ) const { return m_Texture; }
	WGPUTextureView			GetTextureView			( void ) const { return m_Texture.GetTextureView(); }

	virtual uint64_t		GetMemoryEstimate		( void ) const { return 0; }
	virtual uint64_t		GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes(); }

protected:
	WebGPUTexture	m_Texture;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class TextureSkybox : public Resource {
public:
#pragma pack(push)
#pragma pack (1)
	struct Params {};
#pragma pack(pop)

	virtual					~TextureSkybox			( void ) {}
	virtual bool			Init					( std::string_view path, Params params, std::string& err );

	void					Render					( WebGPURTT& rtt, const Matrix4& view_perspective );

	const WebGPUTexture&	Get						( void ) const { return m_Texture; }
	WGPUTextureView			GetTextureView			( void ) const { return m_Texture.GetTextureView(); }

	virtual uint64_t		GetMemoryEstimate		( void ) const { return 0; }
	virtual uint64_t		GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes() * 6; }

protected:
	WebGPUTexture	m_Texture;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class TextureTileset : public Resource {
public:
#pragma pack(push)
#pragma pack (1)
	struct Params {
		int		p_TileHeight;
		int		p_TileWidth;
		bool	p_Mipmaps;
	};
#pragma pack(pop)

	virtual				~TextureTileset(void) {}

	virtual uint64_t	GetMemoryEstimate		( void ) const { return 0; }
	virtual uint64_t	GetGPUMemoryEstimate	( void ) const { return m_FullWidth * m_FullHeight * m_Texture.GetDepthBytes(); }

	bool				Init(std::string_view path, Params params, std::string& err) {
		m_Params = params;

		SDL_Surface* surface = IMG_Load(path.data());
		if (!surface) {
			err = std::format("Could not load image {}", path);
			return false;
		}

		if (surface->format->format != g_CorrectSDLFormat) {
			SDL_Surface* converted_surface = SDL_ConvertSurfaceFormat(surface, g_CorrectSDLFormat, 0);
			SDL_FreeSurface(surface);
			surface = converted_surface;
		}

		m_FullWidth = surface->w;
		m_FullHeight = surface->h;
		int depth = surface->format->BytesPerPixel;
		int num_wid = m_FullWidth / m_Params.p_TileWidth;
		int num_hei = m_FullHeight / m_Params.p_TileHeight;
		m_TileCount = num_wid * num_hei;

		m_Texture.Init2DArray(m_Params.p_TileWidth, m_Params.p_TileHeight, m_TileCount);

		auto sync_token = Core::Singleton().SyncWithMainThread();
		for (int i = 0; i < m_TileCount; i++) {
			int x = (i % num_wid) * m_Params.p_TileWidth;
			int y = (i / num_hei) * m_Params.p_TileHeight;
			unsigned char* data = (unsigned char*)surface->pixels;
			m_Texture.CopyDataLayer(i, data + ((y * m_FullWidth + x) * depth), depth, surface->pitch, params.p_Mipmaps);
		}
		return true;
	};

	const WebGPUTexture&		Get				( void ) const { return m_Texture; }
	WGPUTextureView				GetTextureView	( void ) const { return m_Texture.GetTextureView(); }
	inline const int			GetFullWidth	( void ) const { return m_FullWidth; }
	inline const int			GetFullHeight	( void ) const { return m_FullHeight; }

protected:

	Params			m_Params;
	WebGPUTexture	m_Texture;
	int				m_TileCount = 0;
	int				m_FullWidth = 0;
	int				m_FullHeight = 0;
};

#endif

#if defined(NESHNY_GL)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class TextureTileset : public FileResource {
public:
#pragma pack(push)
#pragma pack (1)
	struct Params {
		int		p_TileHeight;
		int		p_TileWidth;
		GLint	p_WrapMode;
		bool	p_LinearInterp;
		bool	p_Mipmaps;
	};
#pragma pack(pop)

	virtual				~TextureTileset(void) {}
	bool				Init(std::string_view path, Params params, std::string& err) { m_Params = params; return Load(path, err); }

	virtual uint64_t	GetMemoryEstimate		( void ) const { return 0; }
	virtual uint64_t	GetGPUMemoryEstimate	( void ) const { return m_FullWidth * m_FullHeight * m_DepthBytes; }

	virtual bool		FileInit(std::string_view path, unsigned char* data, int length, std::string& err) {

		SDL_RWops* read_buffer = SDL_RWFromMem(data, length);
		SDL_Surface* surface = IMG_Load_RW(read_buffer, 0);
		if (!surface) {
			SDL_FreeRW(read_buffer);
			return false;
		}

		if (surface->format->format != g_CorrectSDLFormat) {
			SDL_Surface* converted_surface = SDL_ConvertSurfaceFormat(surface, g_CorrectSDLFormat, 0);
			SDL_FreeSurface(surface);
			surface = converted_surface;
		}
		m_FullWidth = surface->w;
		m_FullHeight = surface->h;
		m_DepthBytes = surface->format->BytesPerPixel;
		const unsigned char* image_data = (const unsigned char*)surface->pixels;

		int num_wid = m_FullWidth / m_Params.p_TileWidth;
		int num_hei = m_FullHeight / m_Params.p_TileHeight;
		m_TileCount = num_wid * num_hei;

		int levels = m_Params.p_Mipmaps ? int(ceil(log(double(std::max(m_Params.p_TileWidth, m_Params.p_TileHeight))) / log(2))) : 1;

		glGenTextures(1, &m_Texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_Texture);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, levels, GL_RGBA8, m_Params.p_TileWidth, m_Params.p_TileHeight, m_TileCount);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, m_Params.p_WrapMode);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, m_Params.p_WrapMode);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, m_Params.p_LinearInterp ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, m_Params.p_LinearInterp ? (m_Params.p_Mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR) : GL_NEAREST);

		int row_size = m_Params.p_TileWidth * m_DepthBytes;
		int tile_size = m_Params.p_TileHeight * row_size;
		unsigned char* layer_data = new unsigned char[tile_size];
		for (int i = 0; i < m_TileCount; i++) {

			int tx = i % num_wid;
			int ty = i / num_wid;

			for (int r = 0; r < m_Params.p_TileHeight; r++) {
				int src_offset = (r + ty * m_Params.p_TileHeight) * surface->pitch + tx * row_size;
				memcpy(layer_data + r * row_size, image_data + src_offset, row_size);
			}

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, m_Params.p_TileWidth, m_Params.p_TileHeight, 1, GL_BGRA, GL_UNSIGNED_BYTE, layer_data);
		}
		delete[] layer_data;

		if (m_Params.p_Mipmaps) {
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		}

		SDL_FreeSurface(surface);
		SDL_FreeRW(read_buffer);

		return true;
	};

	inline const GLuint&	GetTexture		( void ) const { return m_Texture; }
	inline const int		GetFullWidth	( void ) const { return m_FullWidth; }
	inline const int		GetFullHeight	( void ) const { return m_FullHeight; }

protected:

	Params		m_Params;
	GLuint		m_Texture = 0;
	int			m_TileCount = 0;
	int			m_FullWidth = 0;
	int			m_FullHeight = 0;
	int			m_DepthBytes = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class TextureSkybox : public Resource {
public:
#pragma pack(push)
#pragma pack (1)
	struct Params {};
#pragma pack(pop)

	virtual				~TextureSkybox(void) {}
	virtual bool		Init(std::string_view path, Params params, std::string& err) {
		return m_Texture.InitSkybox(path, err);
	};

	inline const GLTexture& Get(void) { return m_Texture; }

	virtual uint64_t	GetMemoryEstimate		( void ) const { return 0; }
	virtual uint64_t	GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes() * 6; }

	void Render(const fMatrix4& vp, Vec3 cam_pos) {
		
		GLShader* prog = Core::GetShader("Skybox");
		prog->UseProgram();
		GLBuffer* square_buffer = Core::GetBuffer("Cube");
		square_buffer->UseBuffer(prog);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_Texture.GetTexture());
		glUniform1i(prog->GetUniform("uSkybox"), 0);
		glUniform3f(prog->GetUniform("uOffset"), cam_pos.x, cam_pos.y, cam_pos.z);
		glUniformMatrix4fv(prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, vp.Data());
		
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		square_buffer->Draw();
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
	}

protected:

	GLTexture	m_Texture;
};
#endif

} // namespace Neshny
