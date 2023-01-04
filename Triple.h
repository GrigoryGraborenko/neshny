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
						BaseVec3		( QVector3D v ) : x(v.x()), y(v.y()), z(v.z()) {} // TODO: clean up any refs to QT maths
						BaseVec3		( QVector4D v ) : x(v.x()), y(v.y()), z(v.z()) {}
						BaseVec3		( const BaseVec3<int>& t2 ) : x((T)t2.x), y((T)t2.y), z((T)t2.z) {}
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
	inline BaseVec3<T>	Step			( T step ) const { return BaseVec3<T>(STEP(x, step), STEP(y, step), STEP(z, step)); }
	inline BaseVec3<T>	Step			( BaseVec3<T> step ) const { return BaseVec3<T>(STEP(x, step.x), STEP(y, step.y), STEP(z, step.z)); }

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
	inline T			GetAxis			( Axis axis ) const {
		switch(axis) {
			case Axis::X: return x;
			case Axis::Y: return y;
		}
		return z;
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

using Vec3 = BaseVec3<double>;
using fVec3 = BaseVec3<float>;
using iVec3 = BaseVec3<int>;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct BaseVec2 {

	T x, y;

	enum class Axis {
		X, Y
	};

						BaseVec2		( void ) : x(0), y(0) {}
						BaseVec2		( T e0, T e1 ) : x(e0), y(e1) {}
						BaseVec2		( const BaseVec2<int>& t2 ) : x((T)t2.x), y((T)t2.y) {}
						BaseVec2		( const BaseVec2<float>& t2 ) : x((T)t2.x), y((T)t2.y) {}
						BaseVec2		( const BaseVec2<double>& t2 ) : x((T)t2.x), y((T)t2.y) {}
						BaseVec2		( Axis ax, double v ) : x(ax == Axis::X ? v : 0), y(ax == Axis::Y ? v : 0) {}

	inline void			Set				( T e0, T e1 ) { x = e0; y = e1; }
	template <typename G>
	inline BaseVec3<G>	ToVec3			( G z = 0.0 ) { return BaseVec3<G>(x, y, z); }
	inline BaseVec3<T>	ToVec3			( T z = 0.0 ) { return BaseVec3<T>(x, y, z); }
	inline BaseVec2<int>	ToIVec2			( void ) { return BaseVec2<int>(x, y); }
	inline BaseVec2<double>	ToVec2		( void ) { return BaseVec2<double>(x, y); }

	inline void			operator=		( const BaseVec2<T>& t2 ) { x = t2.x; y = t2.y; }
	inline BaseVec2<T>	operator+		( const BaseVec2<T>& t2 ) const { return BaseVec2<T>(x + t2.x, y + t2.y); }
	inline void			operator+=		( const BaseVec2<T>& t2 ) { x = x + t2.x; y = y + t2.y; }
	inline BaseVec2<T>	operator-		( const BaseVec2<T>& t2 ) const { return BaseVec2<T>(x - t2.x, y - t2.y); }
	inline void			operator-=		( const BaseVec2<T>& t2 ) { x = x - t2.x; y = y - t2.y; }
	inline BaseVec2<T>	operator*		( T f ) const { return BaseVec2<T>(x * f, y * f); };
	inline void			operator*=		( T f ) { x *= f; y *= f; }
	inline BaseVec2<T>	operator/		( T f ) const { return BaseVec2<T>(x / f, y / f); };
	inline void			operator/=		( T f ) { x /= f; y /= f;; }
	inline BaseVec2<T>	operator*		( const BaseVec2<T>& t2 ) const { return BaseVec2<T>(x * t2.x, y * t2.y); }
	inline BaseVec2<T>	operator/		( const BaseVec2<T>& t2 ) const { return BaseVec2<T>(x / t2.x, y / t2.y); }
	inline bool			operator==		( const  BaseVec2<T>& t2 ) const { return (x == t2.x) && (y == t2.y); }

	inline T			Dot				( BaseVec2<T> t2 ) const { return x * t2.x + y * t2.y; }

	inline T			operator|		( const BaseVec2<T>& t2 ) const { return Dot(t2); }

	inline BaseVec2<T>	Round			( void ) const { return BaseVec2<T>(floor(x + (T)0.5), floor(y + (T)0.5)); };
	inline BaseVec2<T>	Ceil			( void ) const { return BaseVec2<T>(ceil(x), ceil(y)); };
	inline BaseVec2<T>	Floor			( void ) const { return BaseVec2<T>(floor(x), floor(y)); };
	inline BaseVec2<T>	Abs				( void ) const { return BaseVec2<T>(fabs(x), fabs(y)); };
	inline BaseVec2<T>	Inv				( void ) const { return BaseVec2<T>(1.0 / x, 1.0 / y); };

	inline BaseVec2<T>	Sign			( void ) const { return BaseVec2<T>(SIGN(x), SIGN(y)); };
	inline BaseVec2<T>	Step			( T step ) const { return BaseVec2<T>(STEP(x, step), STEP(y, step)); }
	inline BaseVec2<T>	Step			( BaseVec2<T> step ) const { return BaseVec2<T>(STEP(x, step.x), STEP(y, step.y)); }

	inline T			LengthSquared	( void ) const { return x * x + y * y; }
	inline T			Length			( void ) const { return sqrt(x * x + y * y); }
	inline void			Normalize		( void ) {
		T dist = Length();
		dist = dist == 0 ? ALMOST_ZERO : dist;
		x = x / dist; y = y / dist;
	}
	inline BaseVec2<T>	NormalizeCopy	( void ) const {
		T dist = Length();
		dist = dist == 0 ? ALMOST_ZERO : dist;
		return BaseVec2<T>(x / dist, y / dist);
	}
	inline BaseVec2<T>	Modify			( T new_val, Axis axis ) const {
		if (axis == Axis::X) {
			return BaseVec2<T>(new_val, y);
		}
		return BaseVec2<T>(x, new_val);
	}
	inline bool			Nearby			( const BaseVec2<T>& other, T tolerance ) const { return (fabs(other.x - x) < tolerance) && (fabs(other.y - y) < tolerance); };
	inline T			MinVal			( void ) const { return std::min(x, y); }
	inline T			MaxVal			( void ) const { return std::max(x, y); }

	BaseVec2<T>			NearestToLine	( BaseVec2<T> A, BaseVec2<T> B, bool clamp, T* frac = nullptr ) const {
		BaseVec2<T> a_p = *this - A;
		BaseVec2<T> a_b = B - A;

		T a_b_sqr = a_b.LengthSquared();
		T a_p_dot_a_b = a_b.x * a_p.x + a_b.y * a_p.y;

		T t = a_p_dot_a_b / a_b_sqr;
		if (clamp) {
			t = std::max(0.0, std::min(1.0, t));
		}
		if (frac) {
			*frac = t;
		}
		return A + a_b * t;
	}
	
	static bool			LineLineIntersect ( BaseVec2<T> a0, BaseVec2<T> a1, BaseVec2<T> b0, BaseVec2<T> b1, bool clamp, BaseVec2<T>& out_a, BaseVec2<T>& out_b, T* a_frac = nullptr, T* b_frac = nullptr ) {
		BaseVec2<T> r = a1 - a0;
		BaseVec2<T> s = b1 - b0;

		BaseVec2<T> qp = b0 - a0;
		T numerator = qp ^ r;
		T denominator = r ^ s;

		// lines are parallel
		if (denominator == 0) {
			return false;
		}

		if ((numerator == 0) && (denominator == 0)) { // they are collinear
			// TODO: check if the lines are overlapping
			return false;
		}

		T u = numerator / denominator;
		T t = (qp ^ s) / denominator;
		if (a_frac) {
			*a_frac = u;
		}
		if (b_frac) {
			*b_frac = t;
		}

		return (t >= 0) && (t <= 1) && (u >= 0) && (u <= 1);
	}

	inline static BaseVec2<T>	Min				( BaseVec2<T> a, BaseVec2<T> b ) { return BaseVec2<T>(std::min(a.x, b.x), std::min(a.y, b.y)); }
	inline static BaseVec2<T>	Max				( BaseVec2<T> a, BaseVec2<T> b ) { return BaseVec2<T>(std::max(a.x, b.x), std::max(a.y, b.y)); }
	inline static Axis			IntToAxis		( int ax ) { return ax == 0 ? Axis::X : Axis::Y; }
};

using Vec2 = BaseVec2<double>;
using fVec2 = BaseVec2<float>;
using iVec2 = BaseVec2<int>;

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
					Matrix3			( Vec3 t1, Vec3 t2, Vec3 t3 );

	void			set				( double e00, double e01, double e02, double e10, double e11, double e12, double e20, double e21, double e22 );

	void			operator=		( Matrix3 m2 );
	Matrix3			operator+		( Matrix3 m2 );
	Matrix3			operator*		( Matrix3 m2 );
	Matrix3			operator*		( double m2 );
	Vec3			operator*		( Vec3 t2 );
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
					Matrix4			( Vec3 row_a, Vec3 row_b, Vec3 row_c, Vec3 row_d );
					Matrix4			( double e00, double e01, double e02, double e03, double e10, double e11, double e12, double e13, double e20, double e21, double e22, double e23, double e30, double e31, double e32, double e33 );

	void			set				( double e00, double e01, double e02, double e03, double e10, double e11, double e12, double e13, double e20, double e21, double e22, double e23, double e30, double e31, double e32, double e33 );

	void			operator=		( Matrix4 m2 );
	Matrix4			operator+		( Matrix4 m2 ) const;
	Matrix4			operator*		( Matrix4 m2 ) const;
	Matrix4			operator*		( double m2 ) const;
	Vec3			operator*		( Vec3 v ) const;
	Matrix4			operator~		( void ) const;

	double			determinant		( void ) const;
	Matrix4			inverse			( bool* ok = nullptr ) const;
	QMatrix4x4		toQMatrix4x4	( void ) const;

	static Matrix4	Identity		( void );
};

void Matrix4UnitTest(void);

bool IntersectLineCircle(double cx, double cy, double radius_sqr, double x0, double y0, double x1, double y1, double& result_t0, double& result_t1);
bool IntersectLineSphere(Vec3 centre, double radius_sqr, Vec3 p0, Vec3 p1, double& result_t0, double& result_t1);
bool CircumSphere(Vec3 p0, Vec3 p1, Vec3 p2, Vec3& out_centre);

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
