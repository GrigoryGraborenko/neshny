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
    qDebug() << "Starting test" << test.p_Label;
    try {
        test.p_Func();
    } catch(const char* info) {
        qWarning() << "Failure in " << test.p_Label << ": " << info;
        return { false, test.p_Label, info };
    } catch(Test::InfoException info) {
        qWarning() << "Failure in " << test.p_Label << ": " << info.p_Info;
        return { false, test.p_Label, info.p_Info };
    } catch(std::exception excp) {
        qWarning() << "Failure in " << test.p_Label << ": " << excp.what();
        return { false, test.p_Label, excp.what() };
    } catch(...) {
        qWarning() << "Failure in " << test.p_Label;
        return { false, test.p_Label, "Unknown exception" };
    }
    qDebug() << test.p_Label << "passed";
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
