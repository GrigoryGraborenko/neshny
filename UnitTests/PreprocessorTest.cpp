//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

    void UnitTest_Preprocessor(void) {

#ifdef NESHNY_PREPROCESS
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
                ,"int other = 123 + 456 + 123555555123;"
                ,"int last = 99 + THING + 555;"
            }}
            ,{{
                "#define THING(x) ((x)*(x))"
                ,"#define FUNC( name ,size, thing) ((name) + (sizesize) + name)"
                ,"#define BLEG 555"
                ,"int other = THING(8 + BLEG);"
                ,"int plonk = FUNC('steve', 123  , '1234');"
            },{
                "int other = ((8 + 555)*(8 + 555));"
                ,"int plonk = (('steve') + (123123) + 'steve');"
            }}
            ,{{
                "#define Thing_SET(base, index, value) b_OutputThing[(base) + (index)] = (value)"
                ,"Thing_SET(base, 1, item.Int);"
                ,"//Thing_SET(base, 1, item.Int);"
                ,"Thing_SET(base, 2, bitcast<i32>(item.Float));"
                ,"Thing_SET(base, 3, bitcast<i32>(item.TwoDim.x)); Thing_SET(base, 4, bitcast<i32>(item.TwoDim.y));"
            },{
                "b_OutputThing[(base) + (1)] = (item.Int);"
                ,"//Thing_SET(base, 1, item.Int);"
                ,"b_OutputThing[(base) + (2)] = (bitcast<i32>(item.Float));"
                ,"b_OutputThing[(base) + (3)] = (bitcast<i32>(item.TwoDim.x)); b_OutputThing[(base) + (4)] = (bitcast<i32>(item.TwoDim.y));"
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
#endif
    }
}