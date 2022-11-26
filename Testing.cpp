////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <UnitTests/Vec2Test.cpp>
#include <UnitTests/SSBOTest.cpp>
#include <UnitTests/UtilsTest.cpp>

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

    std::vector<std::pair<std::function<void()>, QString>> tests;
    tests.push_back({ &Test::Test_LineIntersect2D, "LineIntersect2D" });
    tests.push_back({ &Test::Test_SSBOEnsureSize, "SSBOEnsureSize" });
    tests.push_back({ &Test::Test_RoundPowerTwo, "RoundPowerTwo" });
    

    for (auto& test : tests) {
        QString label = test.second;
        //QString label = "TODO: insert function name";
        //QString label = test.target_type().name();
        try {
            test.first();
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
