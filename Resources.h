////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef SDL_h_

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class SoundFile : Resource {
public:
	virtual				~SoundFile(void) {
		Mix_FreeChunk(m_Chuck);
	}
	virtual bool		Init(unsigned char* data, int length, QString& err) {
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

#endif