////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

#if defined(NESHNY_GL)
#elif defined(NESHNY_WEBGPU)

// TODO: move all appropriate code out of .h into here

////////////////////////////////////////////////////////////////////////////////
bool TextureSkybox::Init(std::string_view path, Params params, std::string& err) {
	std::vector<std::string> names = { "right", "left", "top", "bottom", "front", "back" };
	for (int i = 0; i < 6; i++) {

		std::string fullname = ReplaceAll(path, "*", names[i]);

		SDL_Surface* surface = IMG_Load(fullname.data());
		if (!surface) {
			err = std::format("Could not load image {}", path);
			return false;
		}

		if (surface->format->format != g_CorrectSDLFormat) {
			SDL_Surface* converted_surface = SDL_ConvertSurfaceFormat(surface, g_CorrectSDLFormat, 0);
			SDL_FreeSurface(surface);
			surface = converted_surface;
		}

		if (i == 0) {
			m_Texture.InitCubeMap(surface->w, surface->h);
		}
		auto sync_token = Core::Singleton().SyncWithMainThread();
		m_Texture.CopyDataLayer(i, (unsigned char*)surface->pixels, surface->format->BytesPerPixel, surface->pitch, false);
	}
	return true;
};

////////////////////////////////////////////////////////////////////////////////
void TextureSkybox::Render(WebGPURTT& rtt, const Matrix4& inverse_view_perspective) {
    WebGPUPipeline::RenderParams render_params;
    render_params.p_DepthWriteEnabled = false;
    render_params.p_DepthCompare = WGPUCompareFunction_Always;
    Neshny::EntityPipeline::RenderBuffer(SrcStr(), "SkyBox", Core::GetBuffer("Square"), render_params)
        .SetUniform(inverse_view_perspective.ToGPU())
        .AddTexture("SkyBox", &m_Texture)
        .AddSampler("Sampler", Core::GetSampler(WGPUAddressMode_Repeat))
        .Render(&rtt);
}

////////////////////////////////////////////////////////////////////////////////
bool TextureTileset::Init(std::string_view path, Params params, std::string& err) {
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
	std::vector<unsigned char> normal_data;
	if (params.p_GenerateNormalTexture) {
		m_NormalTexture = WebGPUTexture();
		m_NormalTexture->Init2DArray(m_Params.p_TileWidth, m_Params.p_TileHeight, m_TileCount);
		normal_data.resize(m_Params.p_TileWidth * m_Params.p_TileHeight * 4);
	}
	auto sync_token = Core::Singleton().SyncWithMainThread();
	for (int i = 0; i < m_TileCount; i++) {
		int x = (i % num_wid) * m_Params.p_TileWidth;
		int y = (i / num_hei) * m_Params.p_TileHeight;
		unsigned char* data = (unsigned char*)surface->pixels;
		m_Texture.CopyDataLayer(i, data + ((y * m_FullWidth + x) * depth), depth, surface->pitch, params.p_Mipmaps);

		if (params.p_GenerateNormalTexture) {
			for (int tx = 0; tx < m_Params.p_TileWidth; tx++) {
				for (int ty = 0; ty < m_Params.p_TileHeight; ty++) {

					int ind = 0;
					float intensities[9];
					for (int dy = -1; dy <= 1; dy++) {
						for (int dx = -1; dx <= 1; dx++) {
							int final_x = (tx + m_Params.p_TileWidth + dx) % m_Params.p_TileWidth;
							int final_y = (ty + m_Params.p_TileHeight + dy) % m_Params.p_TileHeight;
							unsigned char* pixel = data + (((y + final_y) * m_FullWidth + x + final_x) * depth);
							float intensity = pixel[0] * 0.299 + pixel[1] * 0.587 + pixel[2] * 0.114;
							intensities[ind++] = intensity;
						}
					}

					float dx = intensities[0] - intensities[2] + 2.0 * intensities[3] - 2.0 * intensities[5] + intensities[6] - intensities[8];
					float dy = -intensities[0] - 2.0 * intensities[1] - intensities[2] + intensities[6] + 2.0 * intensities[7] + intensities[8];
					fVec3 normal(-dx, dy, 64);
					normal.Normalize();

					unsigned char* output_pix = normal_data.data() + (ty * m_Params.p_TileWidth + tx) * 4;
					output_pix[0] = (unsigned char)std::max(0.0, std::min(255.0, normal.z * 255.0));
					output_pix[1] = (unsigned char)std::max(0.0, std::min(255.0, (normal.y + 0.5) * 255));
					output_pix[2] = (unsigned char)std::max(0.0, std::min(255.0, (normal.x + 0.5) * 255));
					output_pix[3] = 255;
				}
			}
			m_NormalTexture->CopyDataLayer(i, normal_data.data(), 4, m_Params.p_TileWidth * 4, params.p_Mipmaps);
		}
	}
	return true;
};

#endif

////////////////////////////////////////////////////////////////////////////////
bool ObjModelFile::FileInit(std::string_view path, unsigned char* data, int length, std::string& err) {
	DebugTiming debug_timing("ObjModelFile::Init");

	std::string_view directory = path;
	{
		auto last_forward = path.find_last_of('/');
		auto last_back = path.find_last_of('\\');
		if ((last_forward != std::string_view::npos) && (last_back != std::string_view::npos)) {
			directory = path.substr(0, std::max(last_forward, last_back) + 1);
		} else if (last_forward != std::string_view::npos) {
			directory = path.substr(0, last_forward + 1);
		} else if (last_back != std::string_view::npos) {
			directory = path.substr(0, last_back + 1);
		}
	}

	struct VertexInfo {
		int	p_PositionIndex = -1;
		int	p_TextureIndex = -1;
		int	p_NormalIndex = -1;

		inline bool operator<(const VertexInfo& other) const {
			if (p_PositionIndex == other.p_PositionIndex) {
				if (p_TextureIndex == other.p_TextureIndex) {
					return p_NormalIndex < other.p_NormalIndex;
				}
				return p_TextureIndex < other.p_TextureIndex;
			}
			return p_PositionIndex < other.p_PositionIndex;
		}
	};

	struct Triangle {
		VertexInfo p_Vertices[3];
		int p_MaterialIndex = -1;
	};

	struct Face {
		std::vector<VertexInfo> p_Vertices;
		int p_MaterialIndex = -1;
	};

	std::vector<Triangle> triangles;
	std::vector<Face> faces; // todo: not used yet

	std::vector<fVec4> vertex_positions;
	std::vector<fVec4> vertex_textures;
	std::vector<fVec3> vertex_normals;

	std::vector<std::pair<std::string, std::string>> materials;
	std::map<VertexInfo, int> vertex_info_set;
	if (path.ends_with(".obj")) {

		auto read_float = [] (std::string_view str) -> float {
			float result = 0;
			if (str.empty()) {
				return result;
			}
#ifdef __EMSCRIPTEN__
			return std::stof(std::string(str));
#else
			std::from_chars(str.data(), str.data() + str.size(), result);
#endif
			return result;
		};
		auto read_vertex_int = [] (std::string_view str) -> int {
			int result = -1;
			if (str.empty()) {
				return result;
			}
#ifdef __EMSCRIPTEN__
			return std::stoi(std::string(str)) - 1;
#else
			std::from_chars(str.data(), str.data() + str.size(), result);
			return result - 1;
#endif
		};
		auto get_vertex = [&read_vertex_int] (std::string_view str) -> VertexInfo {
			int first_slash = -1;
			int second_slash = -1;
			for (int i = 0; i < str.length(); i++) {
				bool is_slash = str.data()[i] == '/';
				if (is_slash && (first_slash >= 0)) {
					second_slash = i;
				} else if (is_slash) {
					first_slash = i;
				}
			}
			if (first_slash < 0) {
				return VertexInfo(read_vertex_int(str), -1, -1);
			} else if (second_slash < 0) {
				return VertexInfo(read_vertex_int(str.substr(0, first_slash)), read_vertex_int(str.substr(first_slash + 1)), -1);
			}
			return VertexInfo(read_vertex_int(str.substr(0, first_slash)), read_vertex_int(str.substr(first_slash + 1, second_slash - first_slash - 1)), read_vertex_int(str.substr(second_slash + 1)));
		};

		auto read_file = [] (unsigned char* file_data, int file_length, std::function<void(const std::vector<std::string_view>& line_tokens)> callback) {
			int start_token = 0;
			bool comment = false;
			std::vector<std::string_view> line_tokens;
			for (int i = 0; i < file_length; i++) {
				unsigned char c = file_data[i];
				if (c == '#') {
					comment = true;
				} else if ((c == ' ') || (c == '\r') || (c == '\t') || (c == '\n')) { // whitespace
					int token_len = i - start_token;
					if (token_len && (!comment)) {
						line_tokens.push_back(std::string_view((char*)(file_data + start_token), token_len));
					}
					start_token = i + 1;
					if (c == '\n') {
						if (!line_tokens.empty()) { // flush line
							callback(line_tokens);
						}
						comment = false;
						line_tokens.clear();
					}
				}
			}
		};

		int material_index = -1;
		read_file(data, length, [&] (const std::vector<std::string_view>& line_tokens) {
			auto tokens = line_tokens.size();
			if ((line_tokens[0] == "v") && (tokens >= 4)) {
				fVec4 pos(read_float(line_tokens[1]), read_float(line_tokens[2]), read_float(line_tokens[3]), 1.0);
				if (tokens >= 5) {
					pos.w = read_float(line_tokens[4]);
				}
				vertex_positions.push_back(pos);
			} else if ((line_tokens[0] == "vn") && (tokens >= 4)) {
				vertex_normals.push_back(fVec3(read_float(line_tokens[1]), read_float(line_tokens[2]), read_float(line_tokens[3])));
			} else if ((line_tokens[0] == "vt") && (tokens >= 3)) {
				fVec4 uv(read_float(line_tokens[1]), read_float(line_tokens[2]), 0.0, -1);
				if (tokens >= 4) {
					uv.z = read_float(line_tokens[3]);
				}
				vertex_textures.push_back(uv);
			} else if ((line_tokens[0] == "f") && (tokens >= 4)) {

				std::vector<VertexInfo> face_vertices;
				face_vertices.reserve(line_tokens.size() - 1);
				for (std::size_t i = 1; i < line_tokens.size(); i++) {
					VertexInfo v = get_vertex(line_tokens[i]);
					vertex_info_set.insert_or_assign(v, -1);
					face_vertices.push_back(v);
				}
				if (face_vertices.size() == 4) {
					triangles.push_back({ face_vertices[0], face_vertices[1], face_vertices[2], material_index });
					triangles.push_back({ face_vertices[2], face_vertices[3], face_vertices[0], material_index });
				} else {
					int num_tri = face_vertices.size() - 2;
					for (int ind = 0; ind < num_tri; ind++) {
						triangles.push_back({ face_vertices[ind], face_vertices[ind + 1], face_vertices[ind + 2], material_index });
					}
				}
			} else if ((line_tokens[0] == "mtllib") && (tokens >= 2)) {
				std::ifstream file(std::format("{}{}", directory, line_tokens[1]), std::ios::in | std::ios::binary);
				if (file.is_open()) {
					std::ostringstream data_stream;
					data_stream << file.rdbuf();
					std::string sub_file_data = data_stream.str();

					std::string new_material;
					read_file((unsigned char*)sub_file_data.data(), sub_file_data.length(), [&] (const std::vector<std::string_view>& sub_line_tokens) {
						if ((sub_line_tokens[0] == "newmtl") && (sub_line_tokens.size() >= 2)) {
							new_material = std::string(sub_line_tokens[1]);
						} else if ((sub_line_tokens[0] == "map_Kd") && (sub_line_tokens.size() >= 2)) {
							materials.push_back({ std::move(new_material), std::format("{}{}", directory, sub_line_tokens[1]) });
						}
					});
				}
			} else if ((line_tokens[0] == "usemtl") && (tokens >= 2)) {
				for (int i = 0; i < materials.size(); i++) {
					if (materials[i].first == line_tokens[1]) {
						material_index = i;
						break;
					}
				}
			}
		});
	} else {
		err = "Not supported yet";
		return false;
	}

#ifdef SDL_h_
	for (int layer = 0; layer < materials.size(); layer++) {
		const auto& mat = materials[layer];
		SDL_Surface* surface = IMG_Load(mat.second.data());
		if (!surface) {
			err = std::format("Could not load image at {}", mat.second);
			return false;
		}
		if (surface->format->format != g_CorrectSDLFormat) {
			SDL_Surface* converted_surface = SDL_ConvertSurfaceFormat(surface, g_CorrectSDLFormat, 0);
			SDL_FreeSurface(surface);
			surface = converted_surface;
		}

#if defined(NESHNY_GL)
		// todo
#elif defined(NESHNY_WEBGPU)
		if (layer == 0) {
			m_Texture = std::make_unique<WebGPUTexture>();
			m_Texture->Init2DArray(surface->w, surface->h, materials.size());
			m_GPUSize = surface->w * surface->h * materials.size() * surface->format->BytesPerPixel;
		} else if ((m_Texture->GetWidth() != surface->w) || (m_Texture->GetHeight() != surface->h)) {
			err = "Textures are not all the same resolution";
			return false;
		}
		auto sync_token = Core::Singleton().SyncWithMainThread();
		m_Texture->CopyDataLayer(layer, (unsigned char*)surface->pixels, surface->format->BytesPerPixel, surface->pitch);
#endif
	}
#endif

	for (const auto& triangle : triangles) {
		for (int i = 0; i < 3; i++) {
			int tex_ind = triangle.p_Vertices[i].p_TextureIndex;
			if (tex_ind >= 0) {
				int existing = vertex_textures[tex_ind].w;
				if ((existing >= 0) && (existing != triangle.p_MaterialIndex)) {
					err = "UV points reused across different materials";
					return false;
				}
				vertex_textures[tex_ind].w = triangle.p_MaterialIndex;
			}
		}
	}

	int v_index = 0;
	for (auto& vertex_info : vertex_info_set) {
		vertex_info.second = v_index++;
		int pos_ind = vertex_info.first.p_PositionIndex;
		int tex_ind = vertex_info.first.p_TextureIndex;
		int norm_ind = vertex_info.first.p_NormalIndex;
		m_Vertices.push_back({
			pos_ind < 0 ? fVec4(0.0, 0.0, 0.0, 0.0) : vertex_positions[pos_ind],
			tex_ind < 0 ? fVec4(0.0, 0.0, 0.0, 0.0) : vertex_textures[tex_ind],
			norm_ind < 0 ? fVec3(1.0, 0.0, 0.0) : vertex_normals[norm_ind]
		});
	}

#if defined(NESHNY_GL)
	// todo
#elif defined(NESHNY_WEBGPU)
	m_RenderBuffer = std::make_unique<WebGPURenderBuffer>();

	std::vector<uint32_t> indices;
	for (const auto& triangle : triangles) {
		for (int i = 0; i < 3; i++) {
			int index = vertex_info_set.find(triangle.p_Vertices[i])->second;
			indices.push_back(index);
		}
	}
	int buff_size = (int)m_Vertices.size() * sizeof(Vertex);
	auto sync_token = Core::Singleton().SyncWithMainThread();
	m_RenderBuffer->Init({ WGPUVertexFormat_Float32x4, WGPUVertexFormat_Float32x4, WGPUVertexFormat_Float32x3 }, WGPUPrimitiveTopology_TriangleList, (unsigned char*)m_Vertices.data(), buff_size, indices);
	m_GPUSize += buff_size;

#endif

	return true;
};

} // namespace Neshny
