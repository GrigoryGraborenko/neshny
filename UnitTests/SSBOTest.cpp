//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

    void UnitTest_SSBOEnsureSize(void) {

        Neshny::GLSSBO buffer;
        buffer.EnsureSizeBytes(5 * sizeof(int));
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        buffer.SetSingleValue<int>(1, -456);
        buffer.SetSingleValue<int>(3, 123);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::vector<int> existing;
        buffer.GetValues<int>(existing, buffer.GetSizeBytes() / sizeof(int), 0);

        std::vector<int> expected = { 0, -456, 0, 123, 0, 0, 0, 0 };
        Expect("SSBO has unexpected values after setting single value", expected == existing);

        buffer.EnsureSizeBytes(3 * sizeof(int), false);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        buffer.GetValues<int>(existing, buffer.GetSizeBytes() / sizeof(int), 0);

        Expect("SSBO has unexpected values ensuring smaller size", expected == existing);

        buffer.EnsureSizeBytes(9 * sizeof(int), false);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        buffer.GetValues<int>(existing, buffer.GetSizeBytes() / sizeof(int), 0);
        expected = { 0, -456, 0, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

        Expect("SSBO has unexpected values ensuring greater size with preserver data", expected == existing);
    }

} // namespace Test