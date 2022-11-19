////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef SDL_h_

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class FileResource : public Resource {
public:
	virtual				~FileResource(void) {}
	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) = 0;
	virtual bool		Init(QString path, QString& err) {
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
	virtual				~SoundFile(void) {
		Mix_FreeChunk(m_Chuck);
	}
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
private:
	Mix_Chunk*	m_Chuck;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Texture2D : public FileResource {
public:
	virtual				~Texture2D(void) {}
	virtual bool		FileInit(QString path, unsigned char* data, int length, QString& err) {
		QByteArray arr((const char*)data, length);
		return m_Texture.Init(arr);
	};

	inline const GLTexture& Get(void) const { return m_Texture; }

private:

	GLTexture	m_Texture;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class TextureSkybox : public Resource {
public:
	virtual				~TextureSkybox(void) {}
	virtual bool		Init(QString path, QString& err) {
		return m_Texture.InitSkybox(path, err);
	};

	inline const GLTexture& Get(void) { return m_Texture; }

	void Render(const QMatrix4x4& vp, Vec3 cam_pos) {
		
		GLShader* prog = Neshny::GetShader("Skybox");
		prog->UseProgram();
		GLBuffer* square_buffer = Neshny::GetBuffer("Cube");
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

private:

	GLTexture	m_Texture;
};

#endif