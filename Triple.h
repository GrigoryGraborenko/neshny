////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Triple {

	double x, y, z;

	enum class Axis {
		X, Y, Z
	};

					Triple			( void ) : x(0), y(0), z(0) {}
					Triple			( double e0, double e1, double e2 ) : x(e0), y(e1), z(e2) {}
					Triple			( QVector3D v ) : x(v.x()), y(v.y()), z(v.z()) {}
					Triple			( QVector4D v ) : x(v.x()), y(v.y()), z(v.z()) {}
					Triple			( Axis ax, double v ) : x(ax == Axis::X ? v : 0), y(ax == Axis::Y ? v : 0), z(ax == Axis::Z ? v : 0) {}

	void			set				( double e0, double e1, double e2 );

	void			operator=		( const Triple& t2 );
	Triple			operator+		( const Triple& t2 ) const;
	void			operator+=		( const Triple& t2 );
	Triple			operator-		( const Triple& t2 ) const;
	double			operator*		( const Triple& t2 ) const;
	Triple			operator*		( double f ) const;
	void			operator*=		( double f );
	Triple			operator/		( double f ) const;
	Triple			operator/		( const Triple& t2 ) const;
	Triple			operator^		( const Triple& t2 ) const; // cross product
	bool			operator==		( const Triple& t2 ) const;

	Triple			round			( void ) const;
	Triple			Ceil			( void ) const { return Triple(ceil(x), ceil(y), ceil(z)); };
	Triple			Floor			( void ) const { return Triple(floor(x), floor(y), floor(z)); };
	Triple			sign			( void ) const;
	Triple			abs				( void ) const { return Triple(fabs(x), fabs(y), fabs(z)); };
	Triple			inv				( void ) const { return Triple(1.0 / x, 1.0 / y, 1.0 / z); };
	Triple			mult			( const Triple& t2 ) const;
	double			lengthSquared	( void ) const;
	double			length			( void ) const;
	void			normalize		( void );
	Triple			normalizeCopy	( void ) const;
	Triple			modify			( double new_val, Axis axis ) const;
	bool			nearby			( const Triple& other, double tolerance ) const { return (fabs(other.x - x) < tolerance) && (fabs(other.y - y) < tolerance) && (fabs(other.z - z) < tolerance); };
	double			MinVal			( void ) const { return std::min(x, std::min(y, z)); }
	double			MaxVal			( void ) const { return std::max(x, std::max(y, z)); }
	Triple			NearestToLine	( Triple start, Triple end, bool clamp, double* frac = nullptr ) const;

	static bool		LineLineIntersect ( Triple a0, Triple a1, Triple b0, Triple b1, Triple& out_a, Triple& out_b, double* a_frac = nullptr, double* b_frac = nullptr );

	QVector3D		toVec			( void ) const { return QVector3D(x, y, z); }
	QVector4D		toVec4			( void ) const { return QVector4D(x, y, z, 1.0); }

	struct Matrix4	GetScale		( void ) const;

	static Triple	Min				( Triple a, Triple b ) { return Triple(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)); }
	static Triple	Max				( Triple a, Triple b ) { return Triple(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)); }
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct ITriple {
	int x, y, z;

					ITriple			( void ) : x(0), y(0), z(0) {}
					ITriple			( int e0, int e1, int e2 ) : x(e0), y(e1), z(e2) {}
	Triple			ToTriple		( void ) const { return Triple(x, y, z); }

	bool			operator==		(const ITriple& t2) const { return ((x == t2.x) && (y == t2.y) && (z == t2.z)); };
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Vec2 {

public:

	double x, y;

	Vec2			( void ) : x(0), y(0) {}
	Vec2			( double e0, double e1 ) : x(e0), y(e1) {}
	Vec2			( QPointF p ) : x(p.x()), y(p.y()) {}
	Vec2			( QPoint p ) : x(p.x()), y(p.y()) {}

	void			set				( double e0, double e1 );

	inline QPointF	toPointF		( void) { return QPointF(x, y); }
	//inline QVec2 toVec2		( void ) { return QVec2(x, y); }

	void			operator=		( const Vec2& t2 );
	Vec2			operator+		( const Vec2& t2 ) const;
	void			operator+=		( const Vec2& t2 );
	Vec2			operator-		( const Vec2& t2 ) const;
	Vec2			operator*		( double f ) const;
	//void			operator*=		( double f );
	Vec2			operator/		( double f ) const;
	Vec2			operator/		( const Vec2& t2 ) const;
	Vec2			operator*		( const Vec2& t2 ) const;
	bool			operator==		( const Vec2& t2 ) const;

	inline double	lengthSquared	( void ) { return (x * x + y * y); }
	double			length			( void ) { return sqrt(x * x + y * y); }
	void			normalize		( void );
	Vec2			normalizeCopy	( void );
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Vec4 {

public:

	double x, y, z, w;

	Vec4(void) : x(0), y(0), z(0), w(0) {}
	Vec4(double e0, double e1, double e2, double e3) : x(e0), y(e1), z(e2), w(e3) {}

	void			operator=		(const Vec4& t2) { x = t2.x; y = t2.y; z = t2.z; w = t2.w; }
	Vec4			operator+		(const Vec4& t2) const { return Vec4(x + t2.x, y + t2.y, z + t2.z, w + t2.w); }
	void			operator+=		(const Vec4& t2) { x += t2.x; y += t2.y; z += t2.z; w += t2.w; }
	Vec4			operator-		(const Vec4& t2) const { return Vec4(x - t2.x, y - t2.y, z - t2.z, w - t2.w); }
	Vec4			operator*		(double f) const { return Vec4(x * f, y * f, z * f, w * f); }
	double			operator*		(const Vec4& t2) const { return (x * t2.x + y * t2.y + z * t2.z + w * t2.w); }
	bool			operator==		(const Vec4& t2) const { return ((x == t2.x) && (y == t2.y) && (z == t2.z) && (w == t2.w)); }

	inline double	lengthSquared(void) { return (x * x + y * y + z * z + w * w); }
	inline double	length(void) { return sqrt(lengthSquared()); }
	void			normalize(void) {
		double inv_len = 1.0 / length();
		x *= inv_len; y *= inv_len; z *= inv_len; w *= inv_len;
	}
	Vec4			normalizeCopy(void) {
		double inv_len = 1.0 / length();
		return Vec4(x * inv_len, y * inv_len, z * inv_len, w * inv_len);
	}

	static bool		LineLineIntersect(Vec4 a0, Vec4 a1, Vec4 b0, Vec4 b1, Vec4& out_a, Vec4& out_b, double* a_frac = nullptr, double* b_frac = nullptr);
};

////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
struct Matrix3{

	double m[3][3];

					Matrix3			( void );
					Matrix3			( double e00, double e01, double e02, double e10, double e11, double e12, double e20, double e21, double e22 );
					Matrix3			( Triple t1, Triple t2, Triple t3 );

	void			set				( double e00, double e01, double e02, double e10, double e11, double e12, double e20, double e21, double e22 );

	void			operator=		( Matrix3 m2 );
	Matrix3			operator+		( Matrix3 m2 );
	Matrix3			operator*		( Matrix3 m2 );
	Matrix3			operator*		( double m2 );
	Triple			operator*		( Triple t2 );
	Matrix3			operator~		( void );

	double			determinant		( void );
	Matrix3			adjoint			( void );
	Matrix3			inverse			( void );
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Matrix4{

	double m[4][4];

					Matrix4			( void );
					Matrix4			( QMatrix4x4 mat );
					Matrix4			( Triple row_a, Triple row_b, Triple row_c, Triple row_d );
					Matrix4			( double e00, double e01, double e02, double e03, double e10, double e11, double e12, double e13, double e20, double e21, double e22, double e23, double e30, double e31, double e32, double e33 );

	void			set				( double e00, double e01, double e02, double e03, double e10, double e11, double e12, double e13, double e20, double e21, double e22, double e23, double e30, double e31, double e32, double e33 );

	void			operator=		( Matrix4 m2 );
	Matrix4			operator+		( Matrix4 m2 ) const;
	Matrix4			operator*		( Matrix4 m2 ) const;
	Matrix4			operator*		( double m2 ) const;
	Triple			operator*		( Triple v ) const;
	Matrix4			operator~		( void ) const;

	double			determinant		( void ) const;
	Matrix4			inverse			( bool* ok = nullptr ) const;
	QMatrix4x4		toQMatrix4x4	( void ) const;

	static Matrix4	Identity		( void );
};

void Matrix4UnitTest(void);

bool IntersectLineCircle(double cx, double cy, double radius_sqr, double x0, double y0, double x1, double y1, double& result_t0, double& result_t1);
bool IntersectLineSphere(Triple centre, double radius_sqr, Triple p0, Triple p1, double& result_t0, double& result_t1);
bool CircumSphere(Triple p0, Triple p1, Triple p2, Triple& out_centre);

QMatrix3x3 ConvertTo3x3(const QMatrix4x4& mat) {
	auto d = mat.data();
	float v[9] = {
		d[0], d[4], d[8]
		,d[1], d[5], d[9]
		,d[2], d[6], d[10]
	};
	return QMatrix3x3(v);
}
