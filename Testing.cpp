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
        auto bytes = result.p_Label.toLocal8Bit();
        ImGui::PushID(bytes.data());
        ImGui::TextColored(result.p_Success ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), bytes.data());
        ImGui::SameLine();
        if (ImGui::Button("Rerun")) {
            result = ExecuteTest(g_UnitTests[index]);
        }
        if (!result.p_Success) {
            auto err_bytes = result.p_Error.toLocal8Bit();
            ImGui::Indent(20);
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), err_bytes.data());
            ImGui::Unindent(20);
        }
        ImGui::PopID();
        index++;
    }

    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
UnitTester::TestResult UnitTester::ExecuteTest(const UnitTest& test) {
    QString label = test.p_Label.c_str();
    qDebug() << "Starting test" << label;
    try {
        test.p_Func();
    } catch(const char* info) {
        qWarning() << "Failure in " << label << ": " << info;
        return { false, label, info };
    } catch(Test::InfoException info) {
        qWarning() << "Failure in " << label << ": " << info.p_Info;
        return { false, label, info.p_Info };
    } catch(std::exception excp) {
        qWarning() << "Failure in " << label << ": " << excp.what();
        return { false, label, excp.what() };
    } catch(...) {
        qWarning() << "Failure in " << label;
        return { false, label, "Unknown exception" };
    }
    qDebug() << label << "passed";
    return { true, label };
}

////////////////////////////////////////////////////////////////////////////////
void UnitTester::IExecute(void) {

    m_Results.clear();
    for (const auto& test : g_UnitTests) {
        m_Results.push_back(ExecuteTest(test));
    }

    m_Display = true;
}
