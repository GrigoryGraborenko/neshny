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
                "#define SOME_NAME 7"
                ,"int other = SOME_NAME + 456;"
                ,"/*"
                ,"int other = SOME_NAME + 999;"
                ,"*/"
                ,"int thing = SOME_NAME + 11;"
            },{
                "int other = 7 + 456;"
                ,"/*"
                ,"int other = SOME_NAME + 999;"
                ,"*/"
                ,"int thing = 7 + 11;"
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
            ,{{
                "#define MACRO"
                ,"#ifdef MACRO"
                ,"int thing = 123;"
                ,"#endif"
                ,"int thing = 456;"
                ,"#ifdef MACRO"
                ,"int x = 1;"
                ,"#else"
                ,"int x = 2;"
                ,"#endif"
                ,"int y = 3;"
                ,"#ifdef UNKNOWN"
                ,"int z = 10;"
                ,"#else"
                ,"int z = 20;"
                ,"#endif"
                ,"#ifdef NOTDEF"
                ,"int w = 20;"
                ,"#endif"
            },{
                "int thing = 123;"
                ,"int thing = 456;"
                ,"int x = 1;"
                ,"int y = 3;"
                ,"int z = 20;"
                ,""
            }}
            ,{{
                "#define MACRO 123"
                ,"#define MACRO_LONGER 456"
                ,"int thing1 = MACRO;"
                ,"int thing2 = MACRO_LONGER;"
            },{
                "int thing1 = 123;"
                ,"int thing2 = 456;"
            }},
            {{
                "#include \"include.txt\""
                ,"int abc = BLEG;"
                ,"#include \"include.txt\""
            },{
                "float val = 1.234;"
                ,"int abc = 333;"
                ,""
            }},
            {{
                "#include \"recurse.txt\""
                ,"int abc = BLEG;"
            },{
                "float val = 1.234;"
                ,"float thing = 99.9;"
                ,"int abc = 333;"
            }},
            {{
                "#include \"fail.txt\""
            },{
                "#error file \"fail.txt\" not found"
            }}
            ,{{
                "#define THING"
                ,""
                ,"#ifdef HELLO"
                ,"int i = 1;"
                ,"#elifdef THING1"
                ,"int i = 2;"
                ,"#elifdef THING"
                ,"int i = 3;"
                ,"#elifdef BLEG"
                ,"int i = 4;"
                ,"#endif"
            },{
                ""
                ,"int i = 3;"
                ,""
            }}
            ,{{
                "#define THING 123"
                ,"#define THING 456"
                ,"#define FUNC(x) (x + x)"
                ,"#define FUNC(x) (x * x)"
                ,"int x = THING;"
                ,"int y = FUNC(7);"
            },{
                "int x = 456;"
                ,"int y = (7 * 7);"
            }}
            // TODO: fix me
            //,{{
            //    "#define THING"
            //    ,"#ifdef HELLO"
            //    ,"int i = 1;"
            //    ,"#else"
            //    ,"int i = 2;"
            //    ,"#ifdef GLORB"
            //    ,"int i = 3;"
            //    ,"#endif"
            //    ,"int i = 4;"
            //    ,"#endif"
            //    ,"int i = 5;"
            //},{
            //    ""
            //    ,"int i = 2;"
            //    ,"int i = 5;"
            //}}
        };

        auto loader = [] (std::string_view fname, std::string& err) -> QByteArray {
            if (fname == "recurse.txt") {
                return "#include \"include.txt\"\nfloat thing = 99.9;";
            } else if (fname == "include.txt") {
                return "#define BLEG 333\nfloat val = 1.234;";
            } else if (fname == "normal.txt") {
                return "int file = 1;";
            }
            err = "not found";
            return QByteArray();
        };

        for (int i = 0; i < scenarios.size(); i++) {
            std::string err_msg;
            auto expected = scenarios[i].second.join("\n");
            auto output = Neshny::Preprocess(scenarios[i].first.join("\n"), loader, err_msg);
            Expect(QString("Scenario %1 did not match expected output").arg(i), expected == output);
        }
#endif
    }
}