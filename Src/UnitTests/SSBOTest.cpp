//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

    void UnitTest_SSBOEnsureSize(void) {
#if defined(NESHNY_GL)
        Neshny::SSBO buffer;
#elif defined(NESHNY_WEBGPU)
        Neshny::SSBO buffer(WGPUBufferUsage_None);
#endif

        buffer.EnsureSizeBytes(5 * sizeof(int));

        buffer.SetSingleValue<int>(1, -456);
        buffer.SetSingleValue<int>(3, 123);

        std::vector<int> existing;
        buffer.GetValues<int>(existing, buffer.GetSizeBytes() / sizeof(int), 0);

        std::vector<int> expected = { 0, -456, 0, 123, 0, 0, 0, 0 };
        Expect("SSBO has unexpected values after setting single value", expected == existing);

        buffer.EnsureSizeBytes(3 * sizeof(int), false);
        buffer.GetValues<int>(existing, buffer.GetSizeBytes() / sizeof(int), 0);

        Expect("SSBO has unexpected values ensuring smaller size", expected == existing);

        buffer.EnsureSizeBytes(9 * sizeof(int), false);
        buffer.GetValues<int>(existing, buffer.GetSizeBytes() / sizeof(int), 0);
        expected = { 0, -456, 0, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

        Expect("SSBO has unexpected values ensuring greater size with preserver data", expected == existing);

        int existing_single = buffer.GetSingleValue<int>(1);
        Expect("SSBO has unexpected single value", -456 == existing_single);
    }

} // namespace Test