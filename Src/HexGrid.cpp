////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "HexGrid.h"

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
Hex Hex::operator+(const Hex& b) const {
    return Hex(q + b.q, r + b.r);
}

////////////////////////////////////////////////////////////////////////////////
Hex Hex::operator-(const Hex& b) const {
    return Hex(q - b.q, r - b.r);
}

////////////////////////////////////////////////////////////////////////////////
Hex Hex::operator*(const double scale) const {
    return Hex(q * scale, r * scale);
}

////////////////////////////////////////////////////////////////////////////////
bool Hex::operator==(const Hex& b) const {
    return (q == b.q) && (r == b.r) && (s == b.s);
}

////////////////////////////////////////////////////////////////////////////////
Hex Hex::RotateLeft(void) const {
    return Hex(-s, -q);
}

////////////////////////////////////////////////////////////////////////////////
Hex Hex::RotateRight(void) const {
    return Hex(-r, -s);
}

////////////////////////////////////////////////////////////////////////////////
Hex Hex::Clamp(int distance, int* dist_moved) const {

    int aq = abs(q);
    int ar = abs(r);
    int as = abs(s);
    int max = std::max(std::max(aq, ar), as);
    if (max <= distance) {
        if (dist_moved) {
            *dist_moved = 0;
        }
        return *this;
    }
    if (dist_moved) {
        *dist_moved = max - distance;
    }
    float frac = distance / (float)max;

    if (max == as) {
        int ns = s * frac;
        int nq = q * frac;
        return Hex(nq, -nq - ns, ns);
    }
    return Hex( q * frac, r * frac );
}

////////////////////////////////////////////////////////////////////////////////
Hex Hex::Neighbor(int direction) const {
    return (*this) + HexDirections[direction];
}

////////////////////////////////////////////////////////////////////////////////
Hex Hex::DiagonalNeighbor(int direction) const {
    return (*this) + HexDiagonals[direction];
}

////////////////////////////////////////////////////////////////////////////////
void Hex::AllNeighbors(Hex* neighbors) const {
    for (int i = 0; i < 6; i++) {
        *(neighbors + i) = Neighbor(i);
    }
}

////////////////////////////////////////////////////////////////////////////////
int Hex::Length(void) const {
    return std::max(abs(q), std::max(abs(r), abs(s)));
}

////////////////////////////////////////////////////////////////////////////////
int Hex::Distance(Hex a, Hex b) {
    return (a - b).Length();
}

////////////////////////////////////////////////////////////////////////////////
HexEdge::HexEdge(Hex hex, int _dir) :
    pos(hex)
    ,dir(_dir)
{
    if (dir >= 3) {
        pos = pos.Neighbor(dir);
        dir -= 3;
    }
}

////////////////////////////////////////////////////////////////////////////////
void HexEdge::AllNeighbors(HexEdge* neighbors) {
    neighbors[0] = HexEdge(pos, (dir + 1) % 6);
    neighbors[1] = HexEdge(pos.Neighbor(dir), (dir + 4) % 6);
    neighbors[2] = HexEdge(pos, (dir + 5) % 6);
    neighbors[3] = HexEdge(pos.Neighbor(dir), (dir + 2) % 6);
}

////////////////////////////////////////////////////////////////////////////////
void HexEdge::AllNeighborHex(Hex* neighbors) {
    neighbors[0] = pos;
    neighbors[1] = pos.Neighbor(dir);
    neighbors[2] = pos.Neighbor((dir + 1) % 6);
    neighbors[3] = pos.Neighbor((dir + 5) % 6);
}

////////////////////////////////////////////////////////////////////////////////
std::pair<Vec2, Vec2> HexEdge::GetStartEnd(const HexLayout& layout) {

    Vec2 offset = pos.HexToPixel(layout);
    Vec2 start = offset + CornerOffset(layout, (dir - 1) % 6);
    Vec2 end = offset + CornerOffset(layout, dir);
    return { start, end };
}

////////////////////////////////////////////////////////////////////////////////
int HexEdge::Distance(const HexEdge& a, const HexEdge& b) {

    if (a == b) {
        return 0;
    }

    Hex delta = b.pos - a.pos;
    int dist = delta.Length();
    int simple_dist = dist * 2;

    if (b.dir < a.dir) {
        /// swapping directions is as simple as flipping sign of all delta q r s
        delta = Hex(-delta.q, -delta.r, -delta.s);
    }

    int min_dir = std::min(a.dir, b.dir);
    int max_dir = std::max(a.dir, b.dir);

    if ((a.dir == 0) && (b.dir == 0)) {
        if ((abs(delta.q) == dist) && (abs(delta.s) == dist)) {
            simple_dist++;
        }
    } else if ((a.dir == 1) && (b.dir == 1)) {
        if ((abs(delta.q) == dist) && (abs(delta.r) == dist)) {
            simple_dist++;
        }
    } else if ((a.dir == 2) && (b.dir == 2)) {
        if ((abs(delta.r) == dist) && (abs(delta.s) == dist)) {
            simple_dist++;
        }
    } else if ((min_dir == 0) && (max_dir == 1)) {
        if ((delta.s == dist) || (-delta.r == dist)) {
            simple_dist++;
        } else if (abs(delta.q) == dist) {
            /// no-op
        } else if ((delta.r == dist) || (-delta.s == dist)) {
            simple_dist--;
        }
    } else if ((min_dir == 0) && (max_dir == 2)) {
        if (delta.s == dist) {
            simple_dist += 2;
        } else if ((-delta.q == dist) || (-delta.r == dist)) {
            simple_dist++;
        } else if ((delta.r == dist) || (delta.q == dist)) {
            simple_dist--;
        } else if (-delta.s == dist) {
            simple_dist -= 2;
        }
    } else if ((min_dir == 1) && (max_dir == 2)) {
        if ((delta.s == dist) || (-delta.q == dist)) {
            simple_dist++;
        } else if (abs(delta.r) == dist) {
            /// no-op
        } else if ((delta.q == dist) || (-delta.s == dist)) {
            simple_dist--;
        }
    }

    return simple_dist;
}

////////////////////////////////////////////////////////////////////////////////
Hex HexFractional::Round(void) const {
    int qi = int(round(q));
    int ri = int(round(r));
    int si = int(round(s));
    double q_diff = abs(qi - q);
    double r_diff = abs(ri - r);
    double s_diff = abs(si - s);
    if ((q_diff > r_diff) && (q_diff > s_diff)) {
        qi = -ri - si;
    } else if (r_diff > s_diff) {
        ri = -qi - si;
    }
    return Hex(qi, ri);
}

////////////////////////////////////////////////////////////////////////////////
int HexFractional::ClosestDirection(const HexLayout& layout) const {
    Hex rounded = Round();
    HexFractional center(rounded.q, rounded.r);

    HexFractional offset = *this - center;
    Vec2 dir = offset.HexToPixel(layout);

    //double degrees = atan2(dir.y(), dir.x()) * RADIANS_TO_DEGREES;// -90;
    //double degrees = 60 - atan2(dir.y(), dir.x()) * RADIANS_TO_DEGREES;
    double degrees = -atan2(dir.y, dir.x) * RADIANS_TO_DEGREES;

    int direction = (int)floor(degrees / 60.0) + 1;
    if (direction < 0) {
        direction += 6;
    }
    return direction;
}

////////////////////////////////////////////////////////////////////////////////
HexEdge HexFractional::ClosestEdge(const HexLayout& layout) const {
    return HexEdge(Round(), ClosestDirection(layout));
}

////////////////////////////////////////////////////////////////////////////////
HexFractional HexFractional::Lerp(HexFractional a, HexFractional b, double t) {
    return HexFractional(a.q * (1.0 - t) + b.q * t, a.r * (1.0 - t) + b.r * t);
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Hex> Hex::Linedraw(Hex a, Hex b) {
    int N = Distance(a, b);
    HexFractional a_nudge = HexFractional(a.q + 1e-06, a.r + 1e-06);
    HexFractional b_nudge = HexFractional(b.q + 1e-06, b.r + 1e-06);
    std::vector<Hex> results = {};
    double step = 1.0 / std::max(N, 1);
    for (int i = 0; i <= N; i++)
    {
        results.push_back(HexFractional::Lerp(a_nudge, b_nudge, step * i).Round());
    }
    return results;
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Hex::HexToPixel(const HexLayout& layout) const {
    HexOrientation M = layout.orientation;
    Vec2 size = layout.size;
    Vec2 origin = layout.origin;
    double x = (M.f0 * q + M.f1 * r) * size.x;
    double y = (M.f2 * q + M.f3 * r) * size.y;
    return Vec2(x + origin.x, y + origin.y);
}

////////////////////////////////////////////////////////////////////////////////
Vec2 HexFractional::HexToPixel(const HexLayout& layout) const {
    HexOrientation M = layout.orientation;
    Vec2 size = layout.size;
    Vec2 origin = layout.origin;
    double x = (M.f0 * q + M.f1 * r) * size.x;
    double y = (M.f2 * q + M.f3 * r) * size.y;
    return Vec2(x + origin.x, y + origin.y);
}

////////////////////////////////////////////////////////////////////////////////
HexFractional PixelToHex(const HexLayout& layout, Vec2 p) {
    HexOrientation M = layout.orientation;
    Vec2 size = layout.size;
    Vec2 origin = layout.origin;
    Vec2 pt = Vec2((p.x - origin.x) / size.x, (p.y - origin.y) / size.y);
    double q = M.b0 * pt.x + M.b1 * pt.y;
    double r = M.b2 * pt.x + M.b3 * pt.y;
    return HexFractional(q, r);
}

////////////////////////////////////////////////////////////////////////////////
Vec2 CornerOffset(const HexLayout& layout, int corner) {
    HexOrientation M = layout.orientation;
    Vec2 size = layout.size;
    double angle = 2.0 * PI * (M.start_angle - corner) / 6.0;
    return Vec2(size.x * cos(angle), size.y * sin(angle));
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vec2> PolygonCorners(const HexLayout& layout, Hex h) {
    std::vector<Vec2> corners = {};
    Vec2 center = h.HexToPixel(layout);
    for (int i = 0; i < 6; i++) {
        Vec2 offset = CornerOffset(layout, i);
        corners.push_back(Vec2(center.x + offset.x, center.y + offset.y));
    }
    return corners;
}

} // namespace Neshny