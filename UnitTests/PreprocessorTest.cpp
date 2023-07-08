//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

    void UnitTest_Preprocessor(void) {

        std::vector<std::pair<QByteArrayList, QByteArrayList>> scenarios = {
            {{
                "int thing = THING;"
                ,"#define   THING   123   "
                ,"#define BLEG 555"
                ,"int other = THING + 456 + THINGBLEGBLEGTHING;"
                ,"#undef THING"
                ,"int last = 99 + THING + BLEG;"
            },{
                "int thing = THING;"
                ,"   "
                ,""
                ,"int other = 123 + 456 + 123555555123;"
                ,""
                ,"int last = 99 + THING + 555;"
            }}
        };

        auto loader = [] (QString fname, QString& err) -> QByteArray {
            return "int file = 1;";
        };

        for (int i = 0; i < scenarios.size(); i++) {
            QString err_msg;
            auto expected = scenarios[i].second.join("\n");
            auto output = Neshny::Preprocess(scenarios[i].first.join("\n"), loader, err_msg);
            Expect(QString("Scenario %1 did not match expected output").arg(i), expected == output);
        }
    }
}