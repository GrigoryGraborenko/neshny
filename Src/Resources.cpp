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
void TextureSkybox::Render(WebGPURTT& rtt, const Matrix4& view_perspective) {
    WebGPUPipeline::RenderParams render_params;
    render_params.p_DepthWriteEnabled = false;
    render_params.p_DepthCompare = WGPUCompareFunction_Always;
    Neshny::EntityPipeline::RenderBuffer(SrcStr(), "SkyBox", Core::GetBuffer("Square"), render_params)
        .SetUniform(view_perspective.ToGPU())
        .AddTexture("SkyBox", &m_Texture)
        .AddSampler("Sampler", Core::GetSampler(WGPUAddressMode_Repeat))
        .Render(&rtt);
}

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
			std::from_chars(str.data(), str.data() + str.size(), result);
			return result;
		};
		auto read_vertex_int = [] (std::string_view str) -> int {
			int result = -1;
			if (str.empty()) {
				return result;
			}
			std::from_chars(str.data(), str.data() + str.size(), result);
			return result - 1;
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

				if (tokens == 4) {
					VertexInfo v0 = get_vertex(line_tokens[1]);
					VertexInfo v1 = get_vertex(line_tokens[2]);
					VertexInfo v2 = get_vertex(line_tokens[3]);
					vertex_info_set.insert_or_assign(v0, -1); vertex_info_set.insert_or_assign(v1, -1); vertex_info_set.insert_or_assign(v2, -1);
					triangles.push_back({ v0, v1, v2, material_index });
				} else {
					Face face;
					for (std::size_t i = 1; i < line_tokens.size(); i++) {
						VertexInfo v = get_vertex(line_tokens[i]);
						vertex_info_set.insert_or_assign(v, -1);
						face.p_Vertices.push_back(v);
					}
					face.p_MaterialIndex = material_index;
					faces.emplace_back(std::move(face));
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
