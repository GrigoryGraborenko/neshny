////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

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

private:

	bool					m_ShouldExit = false;

};