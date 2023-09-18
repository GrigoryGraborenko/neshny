////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define MINIMUM_DELTA_NANOSECONDS 15000000

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class Engine : public IEngine {

public:
							Engine						( void );
							~Engine						( void );

	virtual void			MouseButton					( int button, bool is_down );
	virtual void			MouseMove					( Vec2 delta, bool occluded );
	virtual void			MouseWheel					( bool up );
	virtual void			Key							( int key, bool is_down );

	virtual bool			Init						( void );
	virtual void			ExitSignal					( void ) { m_ShouldExit = true; }
	virtual bool			ShouldExit					( void ) { return m_ShouldExit; }
	virtual bool			Tick						( qint64 delta_nanoseconds, int tick );
	virtual void			Render						( int width, int height );

	inline int				GetWidth					( void ) { return m_Width; }
	inline int				GetHeight					( void ) { return m_Height; }

private:

	bool					m_ShouldExit = false;

	qint64					m_AccumilatedNanoseconds = 0;

	//SDL_Window* sdl_window;

	//int	m_Width = 1920;
	//int	m_Height = 1080;
	int	m_Width = 1280;
	int	m_Height = 720;
	//int	m_Width = 1060;
	//int	m_Height = 600;
};