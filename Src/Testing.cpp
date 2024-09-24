////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// this file needs to include the test .cpp files as well as define a list of tests in g_UnitTests
#include <UnitTestList.cpp>

////////////////////////////////////////////////////////////////////////////////
void UnitTester::IRender(void) {
    if (!m_Display) {
        return;
    }

    ImGui::Begin("Test results", &m_Display, ImGuiWindowFlags_NoCollapse);
    ImVec2 space_available = ImGui::GetWindowContentRegionMax();

    int index = 0;
    for (auto& result : m_Results) {
        ImGui::PushID(result.p_Label.c_str());
        ImGui::TextColored(result.p_Success ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", result.p_Label.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Rerun")) {
            result = ExecuteTest(g_UnitTests[index]);
        }
        if (!result.p_Success) {
            ImGui::Indent(20);
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", result.p_Error.c_str());
            ImGui::Unindent(20);
        }
        ImGui::PopID();
        index++;
    }

    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
UnitTester::TestResult UnitTester::ExecuteTest(const UnitTest& test) {
    Neshny::Core::Log(std::format("Starting test {}", test.p_Label));
    const ImVec4 fail_col(1.0f, 0.25f, 0.25f, 1);
    try {
        test.p_Func();
    } catch(const char* info) {
        Neshny::Core::Log(std::format("Failure in {}: {}", test.p_Label, info), fail_col);
        return { false, test.p_Label, info };
    } catch(Test::InfoException info) {
        Neshny::Core::Log(std::format("Failure in {}: {}", test.p_Label, info.p_Info), fail_col);
        return { false, test.p_Label, info.p_Info };
    } catch(std::exception excp) {
        Neshny::Core::Log(std::format("Failure in {}: {}", test.p_Label, excp.what()), fail_col);
        return { false, test.p_Label, excp.what() };
    } catch(...) {
        Neshny::Core::Log(std::format("Failure in {}", test.p_Label), fail_col);
        return { false, test.p_Label, "Unknown exception" };
    }
    Neshny::Core::Log(std::format("{} passed", test.p_Label), ImVec4(0.5f, 0.8f, 0.2f, 1));
    return { true, test.p_Label };
}

////////////////////////////////////////////////////////////////////////////////
void UnitTester::IExecute(void) {

    m_Results.clear();
    for (const auto& test : g_UnitTests) {
        m_Results.push_back(ExecuteTest(test));
    }

    m_Display = true;
}
