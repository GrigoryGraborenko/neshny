//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

void Test_LineIntersect2D(void) {

    double f1;
    auto v1 = Vec2(10, 10).NearestToLine(Vec2(11, 11), Vec2(12, 9), true, &f1);
    Expect("First vector should be 11.2, 10.6", (Vec2(11.2, 10.6) - v1).length() <= ALMOST_ZERO);
    Expect("First vector frac 0.2", (f1 - 0.2) <= ALMOST_ZERO);
}

} // namespace Test