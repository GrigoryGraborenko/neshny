//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
GLShader::GLShader(void) :
	m_Program	( 0 )
{
}

////////////////////////////////////////////////////////////////////////////////
GLShader::~GLShader(void) {
	glDeleteProgram(m_Program);
}

////////////////////////////////////////////////////////////////////////////////
bool GLShader::Init(QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString vertex_filename, QString fragment_filename, QString geometry_filename, QString insertion) {
	m_Program = CreateProgram(err_msg, loader, vertex_filename, fragment_filename, geometry_filename, insertion);
	return (m_Program != 0);
}

////////////////////////////////////////////////////////////////////////////////
bool GLShader::InitCompute(QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString shader_filename, QString insertion) {

	auto shader = CreateShader(err_msg, loader, shader_filename, GL_COMPUTE_SHADER, insertion);
	if (!shader) {
		err_msg = "Could not create " + shader_filename + ": " + err_msg;
		return false;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, shader);
	glLinkProgram(program);
	GLint linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char err_buff[512];
		int len;
		glGetProgramInfoLog(program, 512, &len, err_buff);
		err_msg = QString(QByteArray(err_buff, len));
		this->m_Sources[0].m_Error = err_msg;
		return false;
	}

	glDetachShader(program, shader);
	glDeleteShader(shader);

	m_Program = program;
	return (m_Program != 0);
}

////////////////////////////////////////////////////////////////////////////////
void GLShader::UseProgram(void) {
	glUseProgram(m_Program);
}

////////////////////////////////////////////////////////////////////////////////
GLuint GLShader::GetAttribute(QString name) {
#pragma msg("cache these for later?")
	return glGetAttribLocation(m_Program, name.toLocal8Bit().data());
}

////////////////////////////////////////////////////////////////////////////////
GLint GLShader::GetUniform(QString name) {
	GLint location = glGetUniformLocation(m_Program, name.toLocal8Bit().data());
	//assert(location >= 0);
	return location;
}

////////////////////////////////////////////////////////////////////////////////
GLuint GLShader::CreateProgram(QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString vertex_shader_filename, QString fragment_shader_filename, QString geometry_shader_filename, QString insertion) {

	GLuint vertex_shader = 0;
	if (vertex_shader_filename != "") {
		vertex_shader = CreateShader(err_msg, loader, vertex_shader_filename, GL_VERTEX_SHADER, insertion);
		if (!vertex_shader) {
			err_msg = "Could not create " + vertex_shader_filename + ": " + err_msg;
			qDebug() << err_msg;
			return 0;
		}
	}
	GLuint fragment_shader = CreateShader(err_msg, loader, fragment_shader_filename, GL_FRAGMENT_SHADER, insertion);
	if (!fragment_shader) {
		err_msg = "Could not create " + fragment_shader_filename + ": " + err_msg;
		qDebug() << err_msg;
		return 0;
	}

	GLuint geom_shader = 0;
	if (geometry_shader_filename != "") {
		geom_shader = CreateShader(err_msg, loader, geometry_shader_filename, GL_GEOMETRY_SHADER, insertion);
		if (!geom_shader) {
			err_msg = "Could not create " + geometry_shader_filename + ": " + err_msg;
			qDebug() << err_msg;
			return 0;
		}
	}

	GLuint program = glCreateProgram();
	if (!program) {
		err_msg = "Could not create program";
		return 0;
	}

	if (vertex_shader) {
		glAttachShader(program, vertex_shader);
	}
	glAttachShader(program, fragment_shader);
	if (geom_shader) {
		glAttachShader(program, geom_shader);
	}

	glLinkProgram(program);

	GLint linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		err_msg = "Could not link program";
		return 0;
	}

	if (vertex_shader) {
		glDetachShader(program, vertex_shader);
		glDeleteShader(vertex_shader);
	}
	glDetachShader(program, fragment_shader);
	glDeleteShader(fragment_shader);
	if (geom_shader) {
		glDetachShader(program, geom_shader);
		glDeleteShader(geom_shader);
	}

	return program;
}

////////////////////////////////////////////////////////////////////////////////
GLuint GLShader::CreateShader(QString& err_msg, const std::function<QByteArray(QString, QString&)>& loader, QString filename, GLenum type, QString insertion) {

	QByteArray arr = loader(filename, err_msg);
	if (arr.isNull()) {
		err_msg = "File error - " + err_msg; // TODO: better error handling than just last file error
		return 0;
	}

	{ // replace all #include
		QString search_term = "#include \"";
		QRegularExpression regex(search_term + "(?<fileName>[\\w.\\w]+)\"");
		QRegularExpressionMatchIterator includes = regex.globalMatch(arr);
		std::vector<std::pair<QByteArray, QByteArray>> replacements;
		while (includes.hasNext()) {
			QRegularExpressionMatch match = includes.next();
			QString include_filename = match.captured(1);
			QByteArray include_data = (search_term + include_filename + "\"").toLocal8Bit();

			bool found = false;
			for (auto it = replacements.begin(); it != replacements.end(); it++) {
				if (it->first == include_data) {
					found = true;
					break;
				}
			}
			if (found) {
				continue;
			}

			QByteArray loaded_data = loader(include_filename, err_msg);
			if (loaded_data.isEmpty() || loaded_data.isNull()) {
				err_msg = "Include error, cannot open " + include_filename + " - " + err_msg;
				return 0;
			}

			replacements.push_back({ include_data, loaded_data });
		}
		int replacePos = 0;
		for (auto it = replacements.begin(); it != replacements.end(); it++) {
			int foundPos = arr.indexOf(it->first, replacePos);
			if (foundPos < 0) {
				continue;
			}
			arr.replace(foundPos, it->first.length(), it->second);
			replacePos = foundPos + it->second.length();
		}
	}

	QString source_type = "Compute";
	QString preamble = "#version 450\nprecision highp float;\nprecision highp int;\n";
	if (type == GL_VERTEX_SHADER) {
		preamble += "#define IS_VERTEX_SHADER\n";
		source_type = "Vertex";
	} else if (type == GL_FRAGMENT_SHADER) {
		preamble += "#define IS_FRAGMENT_SHADER\n";
		source_type = "Fragment";
	} else if (type == GL_GEOMETRY_SHADER) {
		preamble += "#define IS_GEOMETRY_SHADER\n";
		source_type = "Geometry";
	}
	preamble += insertion + "\n";
	arr = preamble.toLocal8Bit() + arr;

	unsigned char* start = (unsigned char*)arr.data();
	int len = arr.count();

	const GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const char**)(&start), (&len));
	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		const GLsizei max_err = 512;
		char err_buff[max_err];
		GLsizei len;
		glGetShaderInfoLog(shader, max_err, &len, err_buff);
		err_msg = QString(QByteArray(err_buff, len));
		m_Sources.push_back({ source_type, arr, err_msg });
		return 0;
	}
	m_Sources.push_back({ source_type, arr });
	return shader;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
GLBuffer::GLBuffer(void) :
	m_IndexBuffer	( 0 )
{
}

////////////////////////////////////////////////////////////////////////////////
GLBuffer::~GLBuffer(void) {
	glDeleteBuffers(1, &m_VertexBuffer);
	if (m_IndexBuffer != 0) {
		glDeleteBuffers(1, &m_IndexBuffer);
	}
}

////////////////////////////////////////////////////////////////////////////////
bool GLBuffer::Init(unsigned int vert_size, GLenum mode, std::vector<GLfloat> vertices) {

	m_VertexSizes = std::vector<unsigned int>{ vert_size };
	m_Indices = (unsigned int)vertices.size() / vert_size;
	m_RenderMode = mode;

	glGenBuffers(1, &m_VertexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GLBuffer::Init(unsigned int vert_size, GLenum mode, std::vector<GLfloat> vertices, std::vector<GLuint> indices) {

	return Init(std::vector<unsigned int>{vert_size}, mode, vertices, indices);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GLBuffer::Init(std::vector<unsigned int> sizes, GLenum mode, std::vector<GLfloat> vertices, std::vector<GLuint> indices) {

	m_VertexSizes = sizes;
	m_Indices = (unsigned int)indices.size();
	m_RenderMode = mode;

	glGenBuffers(1, &m_VertexBuffer);
	glGenBuffers(1, &m_IndexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void GLBuffer::UseBuffer(GLShader* program) {

	if (m_VertexSizes.size() == 1) {
		GLuint attribute = program->GetAttribute("aPosition");
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
		glVertexAttribPointer(attribute, m_VertexSizes[0], GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(attribute);
		glVertexAttribDivisor(attribute, 0);
	} else {
		unsigned int total_size = 0;
		for (std::vector<unsigned int>::iterator size = m_VertexSizes.begin(); size != m_VertexSizes.end(); size++) {
			total_size += *size;
		}

		unsigned int current_size = 0;
		for (std::vector<unsigned int>::iterator size = m_VertexSizes.begin(); size != m_VertexSizes.end(); size++) {

			int index = size - m_VertexSizes.begin();
			qint64 offset = sizeof(GL_FLOAT) * current_size;
			current_size += *size;
			int stride = sizeof(GL_FLOAT) * total_size;
			if (m_VertexSizes.size() == 1) {
				stride = 0;
			}
			QString name = "aPosition";
			if (index > 0) {
				name += QString("%1").arg(index);
			}
			GLuint attribute = program->GetAttribute(name);
			glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
			glVertexAttribPointer(attribute, *size, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offset);
			glEnableVertexAttribArray(attribute);
			glVertexAttribDivisor(attribute, 0);
		}
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
}

////////////////////////////////////////////////////////////////////////////////
void GLBuffer::Draw(void) {

	if (m_IndexBuffer == 0) {
		glDrawArrays(m_RenderMode, 0, m_Indices);
	} else {
		glDrawElements(m_RenderMode, m_Indices, GL_UNSIGNED_INT, 0);
	}
}

////////////////////////////////////////////////////////////////////////////////
void GLBuffer::DrawInstanced(int count) {

	if (m_IndexBuffer == 0) {
		glDrawArraysInstanced(m_RenderMode, 0, m_Indices, count);
	} else {
		glDrawElementsInstanced(m_RenderMode, m_Indices, GL_UNSIGNED_INT, 0, count);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
GLTexture::GLTexture(void) :
	m_Texture		( 0 )
	,m_Width		( 0 )
	,m_Height		( 0 )
	,m_DepthBytes	( 0 )
{
}

////////////////////////////////////////////////////////////////////////////////
GLTexture::~GLTexture(void) {
	glDeleteTextures(1, &m_Texture);
}

////////////////////////////////////////////////////////////////////////////////
bool GLTexture::Init(int width, int height, int depth_bytes, GLint wrap_mode) {
	m_Width = width;
	m_Height = height;
	m_DepthBytes = depth_bytes;
	Common2DInit(wrap_mode);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GLTexture::Init(QByteArray data, GLint wrap_mode) {

	QImage im;
	if (!im.loadFromData(data)) {
		return false;
	}
	m_Width = im.width();
	m_Height = im.height();
	m_DepthBytes = im.depth() / 8;
	im = im.mirrored();

	Common2DInit(wrap_mode, im.bits());

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void GLTexture::Common2DInit(GLint wrap_mode, unsigned char* data) {
	auto internal_format = GL_RGB;
	auto format = GL_BGR;
	if (m_DepthBytes == 4) {
		internal_format = GL_RGBA;
		format = GL_BGRA;
		//format = GL_BGRA;
		//format = GL_AGB;
	}

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
}

////////////////////////////////////////////////////////////////////////////////
bool GLTexture::InitSkybox(QString filename, QString& err) {

	std::vector<QString> names = { "right", "left", "top", "bottom", "front", "back" };
	std::list<QImage> images;

	for (auto name : names) {
		images.push_back(QImage());
		QString fullname = filename;
		fullname.replace('*', name);
		if (!images.back().load(fullname)) {
			err = "Could not load " + fullname;
			return false;
		}
	}

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_Texture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	int index = 0;
	for (auto& im : images) {
		auto format = im.depth() == 24 ? GL_RGB : GL_RGBA;
		auto f = im.format();
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index++, 0, format, im.width(), im.height(), 0, format, GL_UNSIGNED_BYTE, im.bits());
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
GLSSBO::GLSSBO(int size) {
	if (size > 0) {
		EnsureSize(size);
	}
}

////////////////////////////////////////////////////////////////////////////////
GLSSBO::GLSSBO(int size, unsigned char* data) {
	glGenBuffers(1, &m_Buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
	m_Size = size;
}

////////////////////////////////////////////////////////////////////////////////
GLSSBO::~GLSSBO(void) {
	glDeleteBuffers(1, &m_Buffer);
	m_Buffer = 0;
}

////////////////////////////////////////////////////////////////////////////////
void GLSSBO::EnsureSize(int size, bool clear_after) {

	// ensure size doubles each time
	size = RoundUpPowerTwo(size);

	if (m_Size >= size) {
		if (clear_after) {
			ClearBuffer();
		}
		return;
	}

	GLuint old_buffer = m_Buffer;
	glGenBuffers(1, &m_Buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	
	ClearBuffer();
	if (!clear_after) {
		glCopyNamedBufferSubData(old_buffer, m_Buffer, 0, 0, m_Size);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	if (old_buffer) {
		glDeleteBuffers(1, &old_buffer);
	}

	m_Size = size;
#ifdef SSBO_DEBUG
	m_NumberResizes++;
#endif
}

////////////////////////////////////////////////////////////////////////////////
void GLSSBO::Bind(int index) {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_Buffer);
}

////////////////////////////////////////////////////////////////////////////////
void GLSSBO::ClearBuffer() {
#ifdef TIMING_DEBUG
	DebugTiming dt0("GLSSBO::ClearBuffer");
#endif

	GLubyte val = 0;
	glClearNamedBufferData(m_Buffer, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &val);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<unsigned char[]> GLSSBO::MakeCopy(int max_size) {
	max_size = max_size >= 0 ? max_size : m_Size;
	if (max_size <= 0) {
		return std::shared_ptr<unsigned char[]>(nullptr);
	}
	unsigned char* ptr = new unsigned char[max_size];
	glGetNamedBufferSubData(m_Buffer, 0, max_size, ptr);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	return std::shared_ptr<unsigned char[]>(ptr);
}
