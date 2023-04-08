////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// this file needs to include the test .cpp files as well as define a list of tests in g_UnitTests
#include "UnitTests.cpp"

void UnitTester::IRender(void) {
    if (!m_Display) {
        return;
    }

    ImGui::Begin("Test results", &m_Display, ImGuiWindowFlags_NoCollapse);
    ImVec2 space_available = ImGui::GetWindowContentRegionMax();

    for (const auto& result : m_Results) {
        auto bytes = result.p_Label.toLocal8Bit();
        ImGui::TextColored(result.p_Success ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), bytes.data());
        if (!result.p_Success) {
            auto err_bytes = result.p_Error.toLocal8Bit();
            ImGui::Indent(20);
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), err_bytes.data());
            ImGui::Unindent(20);
        }
    }

    ImGui::End();
}

void UnitTester::IExecute(void) {

    m_Results.clear();

    for (const auto& test : g_UnitTests) {
        QString label = test.p_Label.c_str();
        try {
            test.p_Func();
        } catch(const char* info) {
            m_Results.push_back({ false, label, info });
            continue;
        } catch(...) {
            m_Results.push_back({ false, label, "Unknown exception" });
            continue;
        }
        m_Results.push_back({ true, label });
    }

    m_Display = true;
}
