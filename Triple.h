////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct BaseVec3 {

	T x, y, z;

	enum class Axis {
		X, Y, Z
	};

						BaseVec3		( void ) : x(0), y(0), z(0) {}
						BaseVec3		( T e0, T e1, T e2 ) : x(e0), y(e1), z(e2) {}
						BaseVec3		( QVector3D v ) : x(v.x()), y(v.y()), z(v.z()) {}
						BaseVec3		( QVector4D v ) : x(v.x()), y(v.y()), z(v.z()) {}
						BaseVec3		( Axis ax, double v ) : x(ax == Axis::X ? v : 0), y(ax == Axis::Y ? v : 0), z(ax == Axis::Z ? v : 0) {}

	inline void			Set				( T e0, T e1, T e2 ) { x = e0; y = e1; z = e2; }

	inline void			operator=		( const BaseVec3<T>& t2 ) { x = t2.x; y = t2.y; z = t2.z; }
	inline BaseVec3<T>	operator+		( const BaseVec3<T>& t2 ) const { return BaseVec3<T>(x + t2.x, y + t2.y, z + t2.z); }
	inline void			operator+=		( const BaseVec3<T>& t2 ) { x = x + t2.x; y = y + t2.y; z = z + t2.z; }
	inline BaseVec3<T>	operator-		( const BaseVec3<T>& t2 ) const { return BaseVec3<T>(x - t2.x, y - t2.y, z - t2.z); }
	inline void			operator-=		( const BaseVec3<T>& t2 ) { x = x - t2.x; y = y - t2.y; z = z - t2.z; }
	inline BaseVec3<T>	operator*		( T f ) const { return BaseVec3<T>(x * f, y * f, z * f); };
	inline void			operator*=		( T f ) { x *= f; y *= f; z *= f; }
	inline BaseVec3<T>	operator/		( T f ) const { return BaseVec3<T>(x / f, y / f, z / f); };
	inline void			operator/=		( T f ) { x /= f; y /= f; z /= f; }
	//inline T			operator*		( const Triple& t2 ) const;
	inline BaseVec3<T>	operator*		( const BaseVec3<T>& t2 ) const { return BaseVec3<T>(x * t2.x, y * t2.y, z * t2.z); }
	inline BaseVec3<T>	operator/		( const BaseVec3<T>& t2 ) const { return BaseVec3<T>(x / t2.x, y / t2.y, z / t2.z); }
	inline bool			operator==		( const  BaseVec3<T>& t2 ) const { return (x == t2.x) && (y == t2.y) && (z == t2.z); }

	inline T			Dot				( BaseVec3<T> t2 ) const { return x * t2.x + y * t2.y + z * t2.z; }
	inline BaseVec3<T>	Cross			( BaseVec3<T> t2 ) const { return BaseVec3<T>(y * t2.z - z * t2.y, z * t2.x - x * t2.z, x * t2.y - y * t2.x); }

	inline T			operator|		( const BaseVec3<T>& t2 ) const { return Dot(t2); }
	inline BaseVec3<T>	operator^		( const BaseVec3<T>& t2 ) const { return Cross(t2); }

	inline BaseVec3<T>	Round			( void ) const { return BaseVec3<T>(floor(x + (T)0.5), floor(y + (T)0.5), floor(z + (T)0.5)); };
	inline BaseVec3<T>	Ceil			( void ) const { return BaseVec3<T>(ceil(x), ceil(y), ceil(z)); };
	inline BaseVec3<T>	Floor			( void ) const { return BaseVec3<T>(floor(x), floor(y), floor(z)); };
	inline BaseVec3<T>	Abs				( void ) const { return BaseVec3<T>(fabs(x), fabs(y), fabs(z)); };
	inline BaseVec3<T>	Inv				( void ) const { return BaseVec3<T>(1.0 / x, 1.0 / y, 1.0 / z); };

	inline BaseVec3<T>	Sign			( void ) const { return BaseVec3<T>(SIGN(x), SIGN(y), SIGN(z)); };

	//inline BaseVec3<T>			mult			( const Triple& t2 ) const;
	inline T			LengthSquared	( void ) const { return x * x + y * y + z * z; }
	inline T			Length			( void ) const { return sqrt(x * x + y * y + z * z); }
	inline void			Normalize		( void ) {
		T dist = Length();
		dist = dist == 0 ? ALMOST_ZERO : dist;
		x = x / dist; y = y / dist; z = z / dist;
	}
	inline BaseVec3<T>	NormalizeCopy	( void ) const {
		T dist = Length();
		dist = dist == 0 ? ALMOST_ZERO : dist;
		return BaseVec3<T>(x / dist, y / dist, z / dist);
	}
	inline BaseVec3<T>	Modify			( T new_val, Axis axis ) const {
		switch(axis) {
			case Axis::X: return BaseVec3<T>(new_val, y, z);
			case Axis::Y: return BaseVec3<T>(x, new_val, z);
		}
		return BaseVec3<T>(x, y, new_val);
	}
	inline bool			Nearby			( const BaseVec3<T>& other, T tolerance ) const { return (fabs(other.x - x) < tolerance) && (fabs(other.y - y) < tolerance) && (fabs(other.z - z) < tolerance); };
	inline T			MinVal			( void ) const { return std::min(x, std::min(y, z)); }
	inline T			MaxVal			( void ) const { return std::max(x, std::max(y, z)); }

	BaseVec3<T>			NearestToLine	( BaseVec3<T> start, BaseVec3<T> end, bool clamp, T* frac = nullptr ) const {
		BaseVec3<T> p_to_lp0 = (*this) - start;
		BaseVec3<T> lp1_to_lp0 = end - start;

		T numer = p_to_lp0 | lp1_to_lp0;
		T denom = lp1_to_lp0 | lp1_to_lp0;

		if(denom == 0) {
			if(frac) {
				frac = 0;
			}
			return start;
		}
		T u = numer / denom;
		if(clamp) {
			if(u < 0) {
				u = 0;
			} else if(u > 1) {
				u = 1;
			}
		}
		if(frac) {
			*frac = u;
		}

		return start + (lp1_to_lp0 * u);
	}

	static bool			LineLineIntersect ( BaseVec3<T> a0, BaseVec3<T> a1, BaseVec3<T> b0, BaseVec3<T> b1, bool clamp, BaseVec3<T>& out_a, BaseVec3<T>& out_b, T* a_frac = nullptr, T* b_frac = nullptr ) {
		BaseVec3<T> p43 = b1 - b0;
		if ((fabs(p43.x) < ALMOST_ZERO) && (fabs(p43.y) < ALMOST_ZERO) && (fabs(p43.z) < ALMOST_ZERO)) {
			return false;
		}
		BaseVec3<T> p21 = a1 - a0;
		if ((fabs(p21.x) < ALMOST_ZERO) && (fabs(p21.y) < ALMOST_ZERO) && (fabs(p21.z) < ALMOST_ZERO)) {
			return false;
		}
		BaseVec3<T> p13 = a0 - b0;

		T d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
		T d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
		T d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
		T d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
		T d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

		T denom = d2121 * d4343 - d4321 * d4321;
		if (fabs(denom) < ALMOST_ZERO) {
			return false;
		}
		T numer = d1343 * d4321 - d1321 * d4343;

		T mua = numer / denom;
		T mub = (d1343 + d4321 * (mua)) / d4343;

		if (clamp) {
			mua = GETCLAMP(mua, 0.0, 1.0);
			mub = GETCLAMP(mub, 0.0, 1.0);
		}

		out_a = BaseVec3<T>(a0.x + mua * p21.x, a0.y + mua * p21.y, a0.z + mua * p21.z);
		out_b = BaseVec3<T>(b0.x + mub * p43.x, b0.y + mub * p43.y, b0.z + mub * p43.z);

		if (a_frac) {
			*a_frac = mua;
		}
		if (b_frac) {
			*b_frac = mub;
		}
		return true;
	}

	QVector3D		toVec			( void ) const { return QVector3D(x, y, z); }
	QVector4D		toVec4			( void ) const { return QVector4D(x, y, z, 1.0); }

	struct Matrix4	GetScale		( void ) const;

	inline static BaseVec3<T>	Min				( BaseVec3<T> a, BaseVec3<T> b ) { return BaseVec3<T>(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)); }
	inline static BaseVec3<T>	Max				( BaseVec3<T> a, BaseVec3<T> b ) { return BaseVec3<T>(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)); }
	inline static Axis			IntToAxis		( int ax ) { return ax == 0 ? Axis::X : (ax == 1 ? Axis::Y : Axis::Z); }
};

//using Vec3 = BaseVec3<double>;
using Triple = BaseVec3<double>;

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
	Vec2			( Triple p ) : x(p.x), y(p.y) {}

	void			set				( double e0, double e1 );

	inline QPointF		ToPointF	( void ) { return QPointF(x, y); }
	inline QVector2D	ToQVec		( void ) { return QVector2D(x, y); }
	inline Triple		ToTriple	( double z = 0.0 ) { return Triple(x, y, z); }

	Vec2			Ceil(void) const { return Vec2(ceil(x), ceil(y)); };
	Vec2			Floor(void) const { return Vec2(floor(x), floor(y)); };
	Vec2			Sign(void) const { return Vec2(SIGN(x), SIGN(y)); }
	Vec2			Step(double step) const { return Vec2(STEP(x, step), STEP(y, step)); }
	Vec2			Step(Vec2 step) const { return Vec2(STEP(x, step.x), STEP(y, step.y)); }

	void			operator=		( const Vec2& t2 );
	Vec2			operator+		( const Vec2& t2 ) const;
	void			operator+=		( const Vec2& t2 );
	Vec2			operator-		( const Vec2& t2 ) const;
	Vec2			operator*		( double f ) const;
	//void			operator*=		( double f );
	Vec2			operator/		( double f ) const;
	Vec2			operator/		( const Vec2& t2 ) const;
	Vec2			operator*		( const Vec2& t2 ) const;
	double			operator^		( const Vec2& t2 ) const { return x * t2.y - y * t2.x; } // cross product
	bool			operator==		( const Vec2& t2 ) const;

	inline double	lengthSquared	( void ) { return (x * x + y * y); }
	double			length			( void ) { return sqrt(x * x + y * y); }
	void			normalize		( void );
	Vec2			normalizeCopy	( void );

	Vec2			NearestToLine	( Vec2 p0, Vec2 p1, bool clamp = true, double* frac = nullptr );

	static bool		LineLineIntersect( Vec2 a0, Vec2 a1, Vec2 b0, Vec2 b1, Vec2& out, double* a_frac = nullptr, double* b_frac = nullptr );
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct IVec2 {
	int x, y;

	IVec2(void) : x(0), y(0) {}
	IVec2(int e0, int e1) : x(e0), y(e1) {}
	IVec2(Vec2 v) : x(floor(v.x)), y(floor(v.y)) {}
	Vec2			ToVec2(void) const { return Vec2(x, y); }

	IVec2			operator*		(const IVec2& t2) const { return IVec2(x * t2.x, y * t2.y); }

	IVec2			operator+		(const IVec2& t2) const { return IVec2(x + t2.x, y + t2.y); }

	bool			operator==		(const IVec2& v2) const { return ((x == v2.x) && (y == v2.y)); };
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

inline QMatrix3x3 ConvertTo3x3(const QMatrix4x4& mat) {
	auto d = mat.data();
	float v[9] = {
		d[0], d[4], d[8]
		,d[1], d[5], d[9]
		,d[2], d[6], d[10]
	};
	return QMatrix3x3(v);
}

template<typename T>
Matrix4 BaseVec3<T>::GetScale(void) const {
	Matrix4 scale = Matrix4::Identity();
	scale.m[0][0] = x;
	scale.m[1][1] = y;
	scale.m[2][2] = z;
	return scale;
}
