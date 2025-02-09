////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

#if defined(NESHNY_GL)
#elif defined(NESHNY_WEBGPU)

// TODO: move all appropriate code out of .h into here

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

} // namespace Neshny
