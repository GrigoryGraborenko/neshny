//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

void UnitTest_TextureRead(void) {

#if defined(NESHNY_WEBGPU) && !defined(__EMSCRIPTEN__)

    int width = 128;
    int height = 99;
    int depth_bytes = 4;
    int size = width * height * depth_bytes;

    std::vector<unsigned char> pix(size);
    for (int i = 0; i < size; i++) {
        pix[i] = i % 255;
    }

    Neshny::WebGPUTexture texture;
    texture.Init2D(width, height, WGPUTextureFormat_BGRA8Unorm);
    texture.CopyData2D(pix.data(), depth_bytes, width * depth_bytes);

    std::vector<unsigned char> copied;
    texture.Read<void>([&](unsigned char* data, int size, Neshny::WebGPUBuffer::AsyncToken<void> token) -> std::shared_ptr<void> {
        copied.resize(size);
		memcpy(copied.data(), data, size);
		return nullptr;
	}).Wait().GetPayload();

    Expect("Texture read values do not match size", pix.size() == copied.size());
    for (int i = 0; i < size; i++) {
        Expect("Texture read values do not match", pix[i] == copied[i]);
    }

    Neshny::iVec3 offset(10, 20, 0);
    Neshny::iVec3 rsize(64, 5, 1);
    copied.resize(0);
    texture.ReadRegion<void>(offset.x, offset.y, offset.z, rsize.x, rsize.y, rsize.z, [&](unsigned char* data, int size, Neshny::WebGPUBuffer::AsyncToken<void> token) -> std::shared_ptr<void> {
        copied.resize(size);
		memcpy(copied.data(), data, size);
		return nullptr;
	}).Wait().GetPayload();

    Expect("Texture region read values do not match size", copied.size() == (rsize.x * rsize.y * rsize.z * depth_bytes));
    for (int x = 0; x < rsize.x; x++) {
        for (int y = 0; y < rsize.y; y++) {
            int dest_ind = (x + y * rsize.x) * depth_bytes;
            int src_ind = (offset.x + x + (offset.y + y) * width) * depth_bytes;
            Expect("Texture region read values do not match", pix[src_ind] == copied[dest_ind]);
        }
    }

#endif
}

} // namespace Test