////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef SDL_h_

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

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class SoundFile : public FileResource {
public:
	struct Params {};

	virtual				~SoundFile(void) { Mix_FreeChunk(m_Chuck); }
	bool				Init(QString path, Params params, QString& err) { return Load(path, err); }

	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) {
		SDL_RWops* rw = SDL_RWFromMem(data, length);
		m_Chuck = Mix_LoadWAV_RW(rw, 0);
		SDL_RWclose(rw);
		if (m_Chuck == NULL) {
			err = "Could not load sound";
			return false;
		}
		return true;
	};

	void Play(void) {
		Mix_PlayChannel(-1, m_Chuck, 0);
	}
protected:
	Mix_Chunk*	m_Chuck;
};

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

protected:

	GLTexture	m_Texture;
};


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

	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) {

		QByteArray bytes((const char*)data, length);
		QImage im;
		if (!im.loadFromData(bytes)) {
			return false;
		}
		int wid = im.width();
		int hei = im.height();
		m_DepthBytes = im.depth() / 8;
		im = im.mirrored();

		int num_wid = wid / m_Params.p_TileWidth;
		int num_hei = wid / m_Params.p_TileHeight;
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
	inline const int		GetWidth		( void ) const { return m_TileWidth; }
	inline const int		GetHeight		( void ) const { return m_TileHeight; }

protected:

	Params		m_Params;
	GLuint		m_Texture = 0;
	int			m_TileCount = 0;
	int			m_TileWidth = 0;
	int			m_TileHeight = 0;
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

	void Render(const QMatrix4x4& vp, Vec3 cam_pos) {
		
		GLShader* prog = Core::GetShader("Skybox");
		prog->UseProgram();
		GLBuffer* square_buffer = Core::GetBuffer("Cube");
		square_buffer->UseBuffer(prog);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_Texture.GetTexture());
		glUniform1i(prog->GetUniform("uSkybox"), 0);
		glUniform3f(prog->GetUniform("uOffset"), cam_pos.x, cam_pos.y, cam_pos.z);
		glUniformMatrix4fv(prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, vp.data());
		
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