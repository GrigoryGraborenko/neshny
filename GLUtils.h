////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class GLShader {

public:

	struct SourceInfo {
		QString		m_Type;
		QString		m_Source;
		QString		m_Error;
	};

													GLShader			( void );
													~GLShader			( void);

	bool											Init				( QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString vertex_filename, QString fragment_filename, QString geometry_filename, QString insertion );
	bool											InitCompute			( QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString shader_filename, QString insertion );

	void											UseProgram			( void );
	GLuint											GetAttribute		( QString name );
	GLint											GetUniform			( QString name );

	inline const std::vector<SourceInfo>&			GetSources			( void ) const { return m_Sources; }
	inline bool										IsValid				( void ) const { return m_Program > 0; }

	inline int										GetLocalSizeX		( void ) const { return m_LocalSizeX; }
	inline int										GetLocalSizeY		( void ) const { return m_LocalSizeY; }
	inline int										GetLocalSizeZ		( void ) const { return m_LocalSizeZ; }

private:

	GLuint											CreateProgram		( QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString vertex_shader_filename, QString fragment_shader_filename, QString geometry_shader_filename, QString insertion );
	GLuint											CreateShader		( QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString filename, GLenum type, QString insertion );

	GLuint											m_Program;
	std::vector<SourceInfo>							m_Sources;
	int												m_LocalSizeX = 8;
	int												m_LocalSizeY = 8;
	int												m_LocalSizeZ = 8;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class GLBuffer {

public:

													GLBuffer		( void );
	virtual 										~GLBuffer		( void );

	bool											Init			( unsigned int vert_size, GLenum mode, std::vector<GLfloat> vertices );
	bool											Init			( unsigned int vert_size, GLenum mode, std::vector<GLfloat> vertices, std::vector<GLuint> indices );
	bool											Init			( std::vector<unsigned int> sizes, GLenum mode, std::vector<GLfloat> vertices, std::vector<GLuint> indices );

	void											UseBuffer		( GLShader* program );
	void											Draw			( void );
	void											DrawInstanced	( int count );

protected:

	std::vector<unsigned int>						m_VertexSizes;
	unsigned int									m_Indices;
	GLenum											m_RenderMode;
	GLuint											m_VertexBuffer;
	GLuint											m_IndexBuffer;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class GLTexture {

public:

													GLTexture		( void );
	virtual 										~GLTexture		( void );

	bool											Init			( int width, int height, int depth_bytes = 4, GLint wrap_mode = GL_REPEAT );
	bool											Init			( QByteArray data, GLint wrap_mode = GL_REPEAT );
	bool											InitSkybox		( QString filename, QString& err );

	inline GLuint									GetTexture		( void ) const { return m_Texture; }
	inline int										GetWidth		( void ) const { return m_Width; }
	inline int										GetHeight		( void ) const { return m_Height; }
	inline int										GetDepthBytes	( void ) const { return m_DepthBytes; }

protected:

	void											Common2DInit	( GLint wrap_mode, unsigned char* data = nullptr );

	GLuint											m_Texture = 0;
	int												m_Width = 0;
	int												m_Height = 0;
	int												m_DepthBytes = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class GLSSBO {
public:
													GLSSBO			( int size = 0 );
													GLSSBO			( int size, unsigned char* data );
	virtual 										~GLSSBO			( void );

	void											EnsureSize		( int size, bool clear_after = true );
	void											Bind			( int index );
	void											ClearBuffer		( void );
	std::shared_ptr<unsigned char[]>				MakeCopy		( int max_size = -1 );

	inline GLuint									Get				( void ) { return m_Buffer; }
	inline int										GetSizeBytes	( void ) { return m_Size; }

	template<class T>
	inline void										SetSingleValue(int index, T value) {
		glNamedBufferSubData(m_Buffer, index * sizeof(T), sizeof(T), &value);
	}

	template<class T>
	inline void										SetValues(const std::vector<T>& items, int offset = 0) {
		if (items.empty()) {
			return;
		}
		glNamedBufferSubData(m_Buffer, offset * sizeof(T), items.size() * sizeof(T), &(items[0]));
	}

	template<class T>
	inline T										GetSingleValue(int index) {
		T result;
		glGetNamedBufferSubData(m_Buffer, index * sizeof(T), sizeof(T), &result);
		return result;
	}

	template<class T>
	inline void										GetValues(std::vector<T>& items, int count, int offset = 0) {
		if (count <= 0) {
			return;
		}
		items.resize(count);
		glGetNamedBufferSubData(m_Buffer, offset * sizeof(T), count * sizeof(T), &(items[0]));
	}

#ifdef SSBO_DEBUG
	inline int										GetNumResizes	( void ) { return m_NumberResizes; }
	inline QByteArray								GetInfo			( void ) { return QString("%1 [%2]").arg(m_Size).arg(m_NumberResizes).toLocal8Bit(); }
#endif // SSBO_DEBUG

protected:

	GLuint											m_Buffer = 0;
	int												m_Size = 0;

#ifdef SSBO_DEBUG
	int												m_NumberResizes = 0;
#endif // SSBO_DEBUG

};

} // namespace Neshny