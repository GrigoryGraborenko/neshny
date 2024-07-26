////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <algorithm>

namespace Neshny {

struct HexLayout;
struct HexFractional;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Hex {

    Hex(void): q(0), r(0), s(0) {}
    Hex(int q_, int r_): q(q_), r(r_), s(-q_ - r_) {}
    Hex(int q_, int r_, int s_): q(q_), r(r_), s(s_) {}

    Hex			operator+		    ( const Hex& b ) const;
    Hex			operator-		    ( const Hex& b ) const;
    Hex			operator*		    ( const double scale ) const;
    bool		operator==		    ( const Hex& b ) const;

    Hex         RotateLeft          ( void ) const;
    Hex         RotateRight         ( void ) const;
    Hex         Clamp               ( int distance, int* dist_moved = nullptr ) const;

    Vec2        HexToPixel          ( const HexLayout& layout ) const;

    Hex         Neighbor            ( int direction ) const;
    Hex         DiagonalNeighbor    ( int direction ) const;
    int         Length              ( void ) const;
    void        AllNeighbors        ( Hex* neighbors ) const;

    static int              Distance            ( Hex a, Hex b );
    static std::vector<Hex> Linedraw            ( Hex a, Hex b );

    int q;
    int r;
    int s;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct HexEdge {

                                HexEdge             ( void ) : dir(0) {}
                                HexEdge             ( Hex hex, int _dir );

    bool                        operator==		    ( const HexEdge& b ) const { return (b.pos == pos) && (b.dir == dir); };

    void                        AllNeighbors        ( HexEdge* neighbors );
    void                        AllNeighborHex      ( Hex* neighbors );

    std::pair<Vec2, Vec2>       GetStartEnd         ( const HexLayout& layout );

    static int                  Distance            ( const HexEdge& a, const HexEdge& b );

    Hex pos;
    int dir;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct HexFractional {

    HexFractional(double q_, double r_): q(q_), r(r_), s(-q_ - r_) {}

    HexFractional			operator+		    ( const HexFractional& b) const { return HexFractional(q + b.q, r + b.r); }
    HexFractional			operator-		    ( const HexFractional& b ) const { return HexFractional(q - b.q, r - b.r); }

    Hex                     Round               ( void ) const;
    int                     ClosestDirection    ( const HexLayout& layout ) const;
    inline HexEdge          ClosestEdge         ( const HexLayout& layout ) const;
    Vec2                    HexToPixel          ( const HexLayout& layout ) const;

    static HexFractional    Lerp    ( HexFractional a, HexFractional b, double t );

    const double q;
    const double r;
    const double s;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct HexOrientation {

    HexOrientation(double f0_, double f1_, double f2_, double f3_, double b0_, double b1_, double b2_, double b3_, double start_angle_): f0(f0_), f1(f1_), f2(f2_), f3(f3_), b0(b0_), b1(b1_), b2(b2_), b3(b3_), start_angle(start_angle_) {}

    const double f0;
    const double f1;
    const double f2;
    const double f3;
    const double b0;
    const double b1;
    const double b2;
    const double b3;
    const double start_angle;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct HexLayout {

    HexLayout(HexOrientation orientation_, Vec2 size_, Vec2 origin_): orientation(orientation_), size(size_), origin(origin_) {}

    const HexOrientation orientation;
    const Vec2 size;
    const Vec2 origin;
};

static const std::vector<Hex> HexDirections = { Hex(1, 0), Hex(1, -1), Hex(0, -1), Hex(-1, 0), Hex(-1, 1), Hex(0, 1) };
static const std::vector<Hex> HexDiagonals = { Hex(2, -1), Hex(1, -2), Hex(-1, -1), Hex(-2, 1), Hex(-1, 2), Hex(1, 1) };

HexFractional       PixelToHex      ( const HexLayout& layout, Vec2 p );
Vec2                CornerOffset    ( const HexLayout& layout, int corner );
std::vector<Vec2>   PolygonCorners  ( const HexLayout& layout, Hex h );

} // namespace Neshny