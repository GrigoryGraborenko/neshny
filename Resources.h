////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class FileResource : public Resource {
public:
	struct Params {};

	virtual				~FileResource(void) {}
	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) = 0;

	bool				Load(QString path, QString& err) {
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			err = file.errorString();
			return false;
		}
		auto data = file.readAll();
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
	bool				Init(QString path, Params params, QString& err) { return Load(path, err); }

	virtual qint64		GetMemoryEstimate		( void ) const { return m_Chunk->alen; }
	virtual qint64		GetGPUMemoryEstimate	( void ) const { return 0; }

	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) {
		SDL_RWops* rw = SDL_RWFromMem(data, length);
		m_Chunk = Mix_LoadWAV_RW(rw, 0);
		SDL_RWclose(rw);
		if (m_Chunk == NULL) {
			err = "Could not load sound";
			return false;
		}
		return true;
	};

	void Play(void) {
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
	struct Params {}; // todo: add stuff like linear interp, mipmaps, etc

	virtual				~Texture2D(void) {}
	bool				Init(QString path, Params params, QString& err) { return Load(path, err); }

	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) {
		QByteArray arr((const char*)data, length);
		return m_Texture.Init(arr);
	};

	inline const GLTexture& Get(void) const { return m_Texture; }

	bool Save(QString filename) {
		int size = m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes();
		unsigned char* data = new unsigned char[size];
		glGetTextureImage(m_Texture.GetTexture(), 0, GL_RGBA, GL_UNSIGNED_BYTE, size, data);
		QImage im(data, m_Texture.GetWidth(), m_Texture.GetHeight(), QImage::Format_RGBA8888);
		im = im.mirrored();
		bool result = im.save(filename);
		delete[] data;
		return result;
	}

	virtual qint64		GetMemoryEstimate		( void ) const { return 0; }
	virtual qint64		GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes(); }

protected:
	GLTexture	m_Texture;
};

#elif defined(NESHNY_WEBGPU)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Texture2D : public Resource {
public:
	struct Params {}; // todo: add stuff like linear interp, mipmaps, etc

	virtual				~Texture2D(void) {}
	bool				Init(QString path, Params params, QString& err) { 

		QImage im(path);
		if (im.isNull()) {
			err = "Could not load image " + path;
			return false;
		}
		int depth = im.depth() / 8;
		if (depth != 4) {
			err = "Can only load RGBA images";
			return false;
		}
		m_Texture.Init2D(im.width(), im.height());
		auto sync_token = Core::Singleton().SyncWithMainThread();
		m_Texture.CopyData2D((unsigned char*)im.bits(), depth, im.bytesPerLine());
		return true;
	}

	const WebGPUTexture&	Get						( void ) const { m_Texture; }
	WGPUTextureView			GetTextureView			( void ) const { return m_Texture.GetTextureView(); }

	virtual qint64			GetMemoryEstimate		( void ) const { return 0; }
	virtual qint64			GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes(); }

protected:
	WebGPUTexture	m_Texture;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class TextureSkybox : public Resource {
public:
	struct Params {};

	virtual				~TextureSkybox(void) {}
	virtual bool		Init(QString path, Params params, QString& err) {
		std::vector<QString> names = { "right", "left", "top", "bottom", "front", "back" };
		for (int i = 0; i < 6; i++) {
			QString fullname = path;
			fullname.replace('*', names[i]);
			QImage im(fullname);
			if (im.isNull()) {
				err = "Could not load " + fullname;
				return false;
			}
			if (i == 0) {
				m_Texture.InitCubeMap(im.width(), im.height());
			}
			auto sync_token = Core::Singleton().SyncWithMainThread();
			m_Texture.CopyDataLayer(i, (unsigned char*)im.bits(), 4, im.bytesPerLine(), false);
		}
		return true;
	};

	const WebGPUTexture&	Get						( void ) const { return m_Texture; }
	WGPUTextureView			GetTextureView			( void ) const { return m_Texture.GetTextureView(); }

	virtual qint64		GetMemoryEstimate		( void ) const { return 0; }
	virtual qint64		GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes() * 6; }

protected:
	WebGPUTexture	m_Texture;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class TextureTileset : public Resource {
public:
	struct Params {
		int		p_TileHeight;
		int		p_TileWidth;
		bool	p_Mipmaps;
	};

	virtual				~TextureTileset(void) {}

	virtual qint64		GetMemoryEstimate		( void ) const { return 0; }
	virtual qint64		GetGPUMemoryEstimate	( void ) const { return m_FullWidth * m_FullHeight * m_Texture.GetDepthBytes(); }

	bool				Init(QString path, Params params, QString& err) {
		m_Params = params;

		QImage im(path);
		if (im.isNull()) {
			err = "Could not load image " + path;
			return false;
		}
		int depth = im.depth() / 8;
		if (depth != 4) {
			err = "Can only load RGBA images";
			return false;
		}

		m_FullWidth = im.width();
		m_FullHeight = im.height();
		int num_wid = m_FullWidth / m_Params.p_TileWidth;
		int num_hei = m_FullHeight / m_Params.p_TileHeight;
		m_TileCount = num_wid * num_hei;

		m_Texture.Init2DArray(m_Params.p_TileWidth, m_Params.p_TileHeight, m_TileCount);

		auto sync_token = Core::Singleton().SyncWithMainThread();
		for (int i = 0; i < m_TileCount; i++) {
			int x = (i % num_wid) * m_Params.p_TileWidth;
			int y = (i / num_hei) * m_Params.p_TileHeight;
			unsigned char* data = (unsigned char*)im.bits();
			m_Texture.CopyDataLayer(i, data + ((y * m_FullWidth + x) * depth), depth, im.bytesPerLine(), params.p_Mipmaps);
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
	struct Params {
		int		p_TileHeight;
		int		p_TileWidth;
		GLint	p_WrapMode;
		bool	p_LinearInterp;
		bool	p_Mipmaps;
	};

	virtual				~TextureTileset(void) {}
	bool				Init(QString path, Params params, QString& err) { m_Params = params; return Load(path, err); }

	virtual qint64		GetMemoryEstimate		( void ) const { return 0; }
	virtual qint64		GetGPUMemoryEstimate	( void ) const { return m_FullWidth * m_FullHeight * m_DepthBytes; }

	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) {

		QByteArray bytes((const char*)data, length);
		QImage im;
		if (!im.loadFromData(bytes)) {
			return false;
		}
		m_FullWidth = im.width();
		m_FullHeight = im.height();
		m_DepthBytes = im.depth() / 8;
		im = im.mirrored();

		int num_wid = m_FullWidth / m_Params.p_TileWidth;
		int num_hei = m_FullHeight / m_Params.p_TileHeight;
		m_TileCount = num_wid * num_hei;

		const unsigned char* image_data = im.bits();

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
			int ty = num_hei - (i / num_wid) - 1;

			for (int r = 0; r < m_Params.p_TileHeight; r++) {
				int src_offset = (tx * row_size + ty * tile_size * num_wid) + r * row_size * num_wid;
				memcpy(layer_data + r * row_size, image_data + src_offset, row_size);
			}

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, m_Params.p_TileWidth, m_Params.p_TileHeight, 1, GL_BGRA, GL_UNSIGNED_BYTE, layer_data);
		}
		delete[] layer_data;

		if (m_Params.p_Mipmaps) {
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		}

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
	struct Params {};

	virtual				~TextureSkybox(void) {}
	virtual bool		Init(QString path, Params params, QString& err) {
		return m_Texture.InitSkybox(path, err);
	};

	inline const GLTexture& Get(void) { return m_Texture; }

	virtual qint64		GetMemoryEstimate		( void ) const { return 0; }
	virtual qint64		GetGPUMemoryEstimate	( void ) const { return m_Texture.GetWidth() * m_Texture.GetHeight() * m_Texture.GetDepthBytes() * 6; }

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
