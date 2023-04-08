//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

    void UnitTest_RoundPowerTwo(void) {
        Expect("RoundUpPowerTwo of 0 should be 0", RoundUpPowerTwo(0) == 0);
        Expect("RoundUpPowerTwo of 1 should be 1", RoundUpPowerTwo(1) == 1);
        Expect("RoundUpPowerTwo of 2 should be 2", RoundUpPowerTwo(2) == 2);
        Expect("RoundUpPowerTwo of 3 should be 4", RoundUpPowerTwo(3) == 4);
        Expect("RoundUpPowerTwo of 4 should be 4", RoundUpPowerTwo(4) == 4);
        Expect("RoundUpPowerTwo of 5 should be 8", RoundUpPowerTwo(5) == 8);
        Expect("RoundUpPowerTwo of 31 should be 32", RoundUpPowerTwo(31) == 32);
        Expect("RoundUpPowerTwo of 45 should be 64", RoundUpPowerTwo(45) == 64);
        Expect("RoundUpPowerTwo of 64 should be 64", RoundUpPowerTwo(64) == 64);
    }

} // namespace Test