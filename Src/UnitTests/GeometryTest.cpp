//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

    void UnitTest_RadiansDiff(void) {

        struct Angles { double first; double second; double difference; };
        std::vector<Angles> angle_sets = {
            { 180, 170, -10 }
            ,{ 170, 180, 10 }
            ,{ 10, 350, -20 }
            ,{ 350, 10, 20 }
            ,{ -50, 115, 165 }
            ,{ 115, -50, -165 }
        };

        for (const auto& angles : angle_sets) {
            double diff_angle = Neshny::RadiansDiff(angles.first * Neshny::DEGREES_TO_RADIANS, angles.second * Neshny::DEGREES_TO_RADIANS) * Neshny::RADIANS_TO_DEGREES;
            Expect("Radian diff should match", Neshny::NearlyEqual(diff_angle, angles.difference));
        }
    }

}