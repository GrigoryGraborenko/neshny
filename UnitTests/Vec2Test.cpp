//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

void UnitTest_LineIntersect2D(void) {

    double f1;
    auto v1 = Neshny::Vec2(10, 10).NearestToLine(Neshny::Vec2(11, 11), Neshny::Vec2(12, 9), true, &f1);
    Expect("First vector should be 11.2, 10.6", (Neshny::Vec2(11.2, 10.6) - v1).Length() <= ALMOST_ZERO);
    Expect("First vector frac 0.2", (f1 - 0.2) <= ALMOST_ZERO);
}

} // namespace Test