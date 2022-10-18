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
	virtual void			MouseMove					( QVector2D delta, bool occluded );
	virtual void			MouseWheel					( bool up );
	virtual void			Key							( int key, bool is_down );

	virtual void			Init						( void );
	virtual void			ExitSignal					( void );
	virtual bool			ShouldExit					( void );
	virtual bool			Tick						( qint64 delta_nanoseconds, int tick );
	virtual void			Render						( int width, int height );

private:

	bool					m_ShouldExit = false;

};