////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

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
						BaseVec3		( const BaseVec3<int>& t2 ) : x((T)t2.x), y((T)t2.y), z((T)t2.z) {}
						BaseVec3		( Axis ax, T v ) : x(ax == Axis::X ? v : 0), y(ax == Axis::Y ? v : 0), z(ax == Axis::Z ? v : 0) {}

	inline BaseVec3<float>	ToFloat3( void ) const { return BaseVec3<float>(float(x), float(y), float(z)); }
	inline BaseVec3<int>	ToInt3	( void ) const { return BaseVec3<int>(int(floor(x)), int(floor(y)), int(floor(z))); }

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
	inline BaseVec3<T>	operator*		( const BaseVec3<T>& t2 ) const { return BaseVec3<T>(x * t2.x, y * t2.y, z * t2.z); }
	inline BaseVec3<T>	operator/		( const BaseVec3<T>& t2 ) const { return BaseVec3<T>(x / t2.x, y / t2.y, z / t2.z); }
	inline bool			operator==		( const BaseVec3<T>& t2 ) const { return (x == t2.x) && (y == t2.y) && (z == t2.z); }

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
						BaseVec2		( Axis ax, T v ) : x(ax == Axis::X ? v : 0), y(ax == Axis::Y ? v : 0) {}

	inline void			Set				( T e0, T e1 ) { x = e0; y = e1; }
	template <typename G>
	inline BaseVec3<G>	ToVec3			( G z = 0.0 ) { return BaseVec3<G>(x, y, z); }
	inline BaseVec3<T>	ToVec3			( T z = 0.0 ) { return BaseVec3<T>(x, y, z); }
	inline BaseVec2<int>	ToIVec2		( void ) { return BaseVec2<int>(x, y); }
	inline BaseVec2<double>	ToDoubleVec2	( void ) { return BaseVec2<double>(x, y); }

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
template<typename T>
struct BaseVec4 {

	T x, y, z, w;

	enum class Axis {
		X, Y, Z, W
	};

	BaseVec4(void) : x(0), y(0), z(0), w(0) {}
	BaseVec4(T e0, T e1, T e2, T e3) : x(e0), y(e1), z(e2), w(e3) {}
	BaseVec4(const BaseVec4<int>& t2) : x((T)t2.x), y((T)t2.y), z((T)t2.z), w((T)t2.w) {}
	BaseVec4(const BaseVec3<T>& t2, T w_val) : x((T)t2.x), y((T)t2.y), z((T)t2.z), w(w_val) {}
	BaseVec4(Axis ax, T v) : x(ax == Axis::X ? v : 0), y(ax == Axis::Y ? v : 0), z(ax == Axis::Z ? v : 0), w(ax == Axis::W ? v : 0) {}

	inline void			Set(T e0, T e1, T e2, T e3) { x = e0; y = e1; z = e2; w = e3; }

	inline void			operator=		(const BaseVec4<T>& t2) { x = t2.x; y = t2.y; z = t2.z; w = t2.w; }
	inline BaseVec4<T>	operator+		(const BaseVec4<T>& t2) const { return BaseVec4<T>(x + t2.x, y + t2.y, z + t2.z, w + t2.w); }
	inline void			operator+=		(const BaseVec4<T>& t2) { x = x + t2.x; y = y + t2.y; z = z + t2.z; w = w + t2.w; }
	inline BaseVec4<T>	operator-		(const BaseVec4<T>& t2) const { return BaseVec4<T>(x - t2.x, y - t2.y, z - t2.z, w - t2.w); }
	inline void			operator-=		(const BaseVec4<T>& t2) { x = x - t2.x; y = y - t2.y; z = z - t2.z; w = w - t2.w; }
	inline BaseVec4<T>	operator*		(T f) const { return BaseVec4<T>(x * f, y * f, z * f, w * f); };
	inline void			operator*=		(T f) { x *= f; y *= f; z *= f; w *= f; }
	inline BaseVec4<T>	operator/		(T f) const { return BaseVec4<T>(x / f, y / f, z / f, w / f); };
	inline void			operator/=		(T f) { x /= f; y /= f; z /= f; w /= f; }
	inline BaseVec4<T>	operator*		(const BaseVec4<T>& t2) const { return BaseVec4<T>(x * t2.x, y * t2.y, z * t2.z, w * t2.w); }
	inline BaseVec4<T>	operator/		(const BaseVec4<T>& t2) const { return BaseVec4<T>(x / t2.x, y / t2.y, z / t2.z, w / t2.w); }
	inline bool			operator==		(const  BaseVec4<T>& t2) const { return (x == t2.x) && (y == t2.y) && (z == t2.z) && (w == t2.w); }

	inline T			Dot(BaseVec4<T> t2) const { return x * t2.x + y * t2.y + z * t2.z + w * t2.w; }
	inline T			operator|		(const BaseVec4<T>& t2) const { return Dot(t2); }

	inline BaseVec4<T>	Round(void) const { return BaseVec4<T>(floor(x + (T)0.5), floor(y + (T)0.5), floor(z + (T)0.5), floor(w + (T)0.5)); };
	inline BaseVec4<T>	Ceil(void) const { return BaseVec4<T>(ceil(x), ceil(y), ceil(z), ceil(w)); };
	inline BaseVec4<T>	Floor(void) const { return BaseVec4<T>(floor(x), floor(y), floor(z), floor(w)); };
	inline BaseVec4<T>	Abs(void) const { return BaseVec4<T>(fabs(x), fabs(y), fabs(z), fabs(w)); };
	inline BaseVec4<T>	Inv(void) const { return BaseVec4<T>(1.0 / x, 1.0 / y, 1.0 / z, 1.0 / w); };

	inline BaseVec4<T>	Sign(void) const { return BaseVec4<T>(SIGN(x), SIGN(y), SIGN(z), SIGN(w)); };
	inline BaseVec4<T>	Step(T step) const { return BaseVec4<T>(STEP(x, step), STEP(y, step), STEP(z, step), STEP(w, step)); }
	inline BaseVec4<T>	Step(BaseVec4<T> step) const { return BaseVec4<T>(STEP(x, step.x), STEP(y, step.y), STEP(z, step.z), STEP(w, step.w)); }

	inline T			LengthSquared(void) const { return x * x + y * y + z * z + w * w; }
	inline T			Length(void) const { return sqrt(x * x + y * y + z * z + w * w); }
	inline void			Normalize(void) {
		T dist = Length();
		dist = dist == 0 ? ALMOST_ZERO : dist;
		x = x / dist; y = y / dist; z = z / dist; w = w / dist;
	}
	inline BaseVec4<T>	NormalizeCopy(void) const {
		T dist = Length();
		dist = dist == 0 ? ALMOST_ZERO : dist;
		return BaseVec4<T>(x / dist, y / dist, z / dist, w / dist);
	}
	inline BaseVec4<T>	Modify(T new_val, Axis axis) const {
		switch (axis) {
		case Axis::X: return BaseVec4<T>(new_val, y, z, w);
		case Axis::Y: return BaseVec4<T>(x, new_val, z, w);
		case Axis::Z: return BaseVec4<T>(x, y, new_val, w);
		}
		return BaseVec4<T>(x, y, z, new_val);
	}
	inline T			GetAxis(Axis axis) const {
		switch (axis) {
		case Axis::X: return x;
		case Axis::Y: return y;
		case Axis::Z: return z;
		}
		return w;
	}
	inline T			MinVal(void) const { return std::min(x, std::min(y, std::min(z, w))); }
	inline T			MaxVal(void) const { return std::max(x, std::max(y, std::min(z, w))); }

	inline BaseVec3<T> ToVec3() const { return BaseVec3<T>(x, y, z); }

	inline static BaseVec4<T>	Min(BaseVec4<T> a, BaseVec4<T> b) { return BaseVec4<T>(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w)); }
	inline static BaseVec4<T>	Max(BaseVec4<T> a, BaseVec4<T> b) { return BaseVec4<T>(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w)); }
	inline static Axis			IntToAxis(int ax) { return ax == 0 ? Axis::X : (ax == 1 ? Axis::Y : (ax == 2 ? Axis::Z : Axis::W)); }
};

using Vec4 = BaseVec4<double>;
using fVec4 = BaseVec4<float>;
using iVec4 = BaseVec4<int>;

bool		LineLineIntersect(Vec4 a0, Vec4 a1, Vec4 b0, Vec4 b1, Vec4& out_a, Vec4& out_b, double* a_frac = nullptr, double* b_frac = nullptr);

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct BaseMatrix3 {

	T m[3][3];

					BaseMatrix3		( void ) { m[0][0] = 0; m[0][1] = 0; m[0][2] = 0; m[1][0] = 0; m[1][1] = 0; m[1][2] = 0; m[2][0] = 0; m[2][1] = 0; m[2][2] = 0; }
					BaseMatrix3		( T e00, T e01, T e02, T e10, T e11, T e12, T e20, T e21, T e22 ) { m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; m[1][0] = e10; m[1][1] = e11; m[1][2] = e12; m[2][0] = e20; m[2][1] = e21; m[2][2] = e22; }
					BaseMatrix3		( BaseVec3<T> t1, BaseVec3<T> t2, BaseVec3<T> t3 ) { set(t1.x, t1.y, t1.z, t2.x, t2.y, t2.z, t3.x, t3.y, t3.z); }

	void			Set				( T e00, T e01, T e02, T e10, T e11, T e12, T e20, T e21, T e22 ) { m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; m[1][0] = e10; m[1][1] = e11; m[1][2] = e12; m[2][0] = e20; m[2][1] = e21; m[2][2] = e22; }

	void			operator=		( BaseMatrix3<T> m2 ) { Set(m2.m[0][0], m2.m[0][1], m2.m[0][2], m2.m[1][0], m2.m[1][1], m2.m[1][2], m2.m[2][0], m2.m[2][1], m2.m[2][2]); }
	//BaseMatrix3<T>	operator+		( BaseMatrix3<T> m2 );
	BaseMatrix3<T>	operator*		( BaseMatrix3<T> m2 ) {
		BaseMatrix3<T> ma;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				for (int r = 0; r < 3; r++) {
					ma.m[i][j] += m[i][r] * m2.m[r][j];
				}
			}
		}
		return ma;
	}

	BaseMatrix3<T>	operator*		( T m2 ) {
		BaseMatrix3<T> ma;
		int i = 0;
		while (i < 9) {
			ma.m[i / 3][i % 3] = m2 * m[i / 3][i % 3];
			i++;
		}
		return ma;
	}

	BaseVec3<T>			operator*	( BaseVec3<T> t2 ) {
		BaseVec3<T> ta;
		ta.x = m[0][0] * t2.x + m[0][1] * t2.y + m[0][2] * t2.z;
		ta.y = m[1][0] * t2.x + m[1][1] * t2.y + m[1][2] * t2.z;
		ta.z = m[2][0] * t2.x + m[2][1] * t2.y + m[2][2] * t2.z;
		return ta;
	}

	BaseMatrix3<T>	operator~		( void ) const {
		BaseMatrix3<T> ma;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				ma.m[i][j] = m[j][i];
			}
		}
		return ma;
	}

	T				Determinant		( void ) {
		return(
			m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - 
			m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
			m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])
		);
	}

	BaseMatrix3<T>	Adjoint			( void ) {
		BaseMatrix3<T> ma(
			(m[1][1] * m[2][2]) - (m[2][1] * m[1][2])
			,-((m[1][0] * m[2][2]) - (m[2][0] * m[1][2]))
			,(m[1][0] * m[2][1]) - (m[2][0] * m[1][1])

			,-((m[0][1] * m[2][2]) - (m[2][1] * m[0][2]))
			,(m[0][0] * m[2][2]) - (m[2][0] * m[0][2])
			,-((m[0][0] * m[2][1]) - (m[2][0] * m[0][1]))

			,(m[0][1] * m[1][2]) - (m[1][1] * m[0][2])
			,-((m[0][0] * m[1][2]) - (m[1][0] * m[0][2]))
			,(m[0][0] * m[1][1]) - (m[1][0] * m[0][1])
		);
		ma = ~ma;
		return ma;
	}

	BaseMatrix3<T>	Inverse			( void ) {
		T det = Determinant();
		if (det == 0) {
			return BaseMatrix3<T>(1, 0, 0, 0, 1, 0, 0, 0, 1);
		}
		BaseMatrix3<T> ma = Adjoint();
		ma = ma * (1 / det);
		return ma;
	}
};

using Matrix3 = BaseMatrix3<double>;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct BaseQuat {

	enum class Axis {
		X, Y, Z
	};

	T m[4];

	BaseQuat(void) { m[0] = 0; m[1] = 0; m[2] = 0; m[3] = 0; }
	BaseQuat(const BaseVec3<T>& dir) {
		T a1 = atan2(-dir.z, dir.x) * RADIANS_TO_DEGREES;
		const T xzd = sqrt(dir.x * dir.x + dir.z * dir.z);
		T a2 = atan2(dir.y, xzd) * RADIANS_TO_DEGREES;
		if (!dir.x) {
			a1 = 0;
		}
		if (!xzd) {
			a2 = 0;
		}
		operator=(BaseQuat<T>(a1, Axis::Y) * BaseQuat<T>(a2, Axis::Z));
	}
	BaseQuat(BaseVec3<T> axis, T degrees) {
		const T rads = degrees * DEGREES_TO_RADIANS * 0.5;
		const T si = sin(rads);
		axis.Normalize();
		m[0] = cos(rads);
		m[1] = axis.x * si;
		m[2] = axis.y * si;
		m[3] = axis.z * si;
		Normalize();
	}
	BaseQuat(T e0, T e1, T e2, T e3) { m[0] = e0; m[1] = e1; m[2] = e2; m[3] = e3; }
	BaseQuat(T degrees, Axis axis) {
		const T ang = degrees * DEGREES_TO_RADIANS * 0.5;
		if (axis == Axis::X) {
			Set(cos(ang), sin(ang), 0, 0);
		} else if (axis == Axis::Y) {
			Set(cos(ang), 0, sin(ang), 0);
		} else if (axis == Axis::Z) {
			Set(cos(ang), 0, 0, sin(ang));
		} else {
			Set(1, 0, 0, 0);
		}
		Normalize();
	}
	BaseQuat(BaseMatrix3<T> rm) {
		const T tr = rm.m[0][0] + rm.m[1][1] + rm.m[2][2];
		if (tr > 0) {
			T S = sqrt(tr + 1.0) * 2; // S=4*qw 
			m[3] = 0.25 * S;
			m[0] = (rm.m[2][1] - rm.m[1][2]) / S;
			m[1] = (rm.m[0][2] - rm.m[2][0]) / S;
			m[2] = (rm.m[1][0] - rm.m[0][1]) / S;
		} else if ((rm.m[0][0] > rm.m[1][1]) & (rm.m[0][0] > rm.m[2][2])) {
			T S = sqrt(1.0 + rm.m[0][0] - rm.m[1][1] - rm.m[2][2]) * 2; // S=4*qx 
			m[3] = (rm.m[2][1] - rm.m[1][2]) / S;
			m[0] = 0.25 * S;
			m[1] = (rm.m[0][1] + rm.m[1][0]) / S;
			m[2] = (rm.m[0][2] + rm.m[2][0]) / S;
		} else if (rm.m[1][1] > rm.m[2][2]) {
			T S = sqrt(1.0 + rm.m[1][1] - rm.m[0][0] - rm.m[2][2]) * 2; // S=4*qy
			m[3] = (rm.m[0][2] - rm.m[2][0]) / S;
			m[0] = (rm.m[0][1] + rm.m[1][0]) / S;
			m[1] = 0.25 * S;
			m[2] = (rm.m[1][2] + rm.m[2][1]) / S;
		} else {
			T S = sqrt(1.0 + rm.m[2][2] - rm.m[0][0] - rm.m[1][1]) * 2; // S=4*qz
			m[3] = (rm.m[1][0] - rm.m[0][1]) / S;
			m[0] = (rm.m[0][2] + rm.m[2][0]) / S;
			m[1] = (rm.m[1][2] + rm.m[2][1]) / S;
			m[2] = 0.25 * S;
		}
	}

	void Set(T e0, T e1, T e2, T e3) { m[0] = e0; m[1] = e1; m[2] = e2; m[3] = e3; }
	void operator=(const BaseQuat& q2) { m[0] = q2.m[0]; m[1] = q2.m[1]; m[2] = q2.m[2]; m[3] = q2.m[3]; }
	BaseQuat operator*(const BaseQuat& q2) const {
		BaseQuat qa;
		qa.m[0] = m[0] * q2.m[0] - m[1] * q2.m[1] - m[2] * q2.m[2] - m[3] * q2.m[3];
		qa.m[1] = m[0] * q2.m[1] + m[1] * q2.m[0] + m[2] * q2.m[3] - m[3] * q2.m[2];
		qa.m[2] = m[0] * q2.m[2] - m[1] * q2.m[3] + m[2] * q2.m[0] + m[3] * q2.m[1];
		qa.m[3] = m[0] * q2.m[3] + m[1] * q2.m[2] - m[2] * q2.m[1] + m[3] * q2.m[0];
		return qa;
	}
	BaseVec3<T> operator*(BaseVec3<T> vec) const {
		BaseQuat result = (*this) * BaseQuat(0.0, vec.x, vec.y, vec.z) * BaseQuat(m[0], -m[1], -m[2], -m[3]);
		return BaseVec3<T>(result.m[1], result.m[2], result.m[3]);
	}
	BaseQuat operator*(T f) const {
		BaseQuat qa;
		qa.m[0] = m[0] * f;
		qa.m[1] = m[1] * f;
		qa.m[2] = m[2] * f;
		qa.m[3] = m[3] * f;
		return qa;
	}
	BaseQuat operator+(const BaseQuat& q2) const {
		BaseQuat qa;
		qa.m[0] = m[0] + q2.m[0];
		qa.m[1] = m[1] + q2.m[1];
		qa.m[2] = m[2] + q2.m[2];
		qa.m[3] = m[3] + q2.m[3];
		return qa;
	}
	BaseQuat operator/(const BaseQuat& q2) const {
		BaseQuat qa;
		T denom = q2.m[0] * q2.m[0] + q2.m[1] * q2.m[1] + q2.m[2] * q2.m[2] + q2.m[3] * q2.m[3];
		if (denom == 0) {
			denom = ALMOST_ZERO;
		}
		T inv_denom = 1.0 / denom;
		qa.m[0] = (q2.m[0] * m[0] + q2.m[1] * m[1] + q2.m[2] * m[2] + q2.m[3] * m[3]) * inv_denom;
		qa.m[1] = (q2.m[0] * m[1] - q2.m[1] * m[0] - q2.m[2] * m[3] + q2.m[3] * m[2]) * inv_denom;
		qa.m[2] = (q2.m[0] * m[2] + q2.m[1] * m[3] - q2.m[2] * m[0] - q2.m[3] * m[1]) * inv_denom;
		qa.m[3] = (q2.m[0] * m[3] - q2.m[1] * m[2] + q2.m[2] * m[1] - q2.m[3] * m[0]) * inv_denom;
		return qa;
	}
	inline T Dot(const BaseQuat q2) const { return m[0] * q2.m[0] + m[1] * q2.m[1] + m[2] * q2.m[2] + m[3] * q2.m[3]; }
	inline T operator|(const BaseQuat& q2) const { return Dot(q2); }

	BaseQuat Inverse(void) const {
		BaseQuat qa;
		T denom = m[0] * m[0] + m[1] * m[1] + m[2] * m[2] + m[3] * m[3];
		if (denom == 0) {
			denom = ALMOST_ZERO;
		}
		T inv_denom = 1.0 / denom;
		qa.m[0] = m[0] * inv_denom;
		qa.m[1] = -m[1] * inv_denom;
		qa.m[2] = -m[2] * inv_denom;
		qa.m[3] = -m[3] * inv_denom;
		return qa;
	}
	void Normalize(void) {
		T dist = sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2] + m[3] * m[3]);
		if (dist == 0) {
			dist = (T)ALMOST_ZERO;
		}
		T inv_dist = 1.0 / dist;
		m[0] = m[0] * inv_dist;
		m[1] = m[1] * inv_dist;
		m[2] = m[2] * inv_dist;
		m[3] = m[3] * inv_dist;
	}
	BaseMatrix3<T> ToRotation(void) {
		return BaseMatrix3<T>(
			1 - 2 * m[2] * m[2] - 2 * m[3] * m[3], 2 * m[1] * m[2] - 2 * m[0] * m[3], 2 * m[1] * m[3] + 2 * m[0] * m[2],
			2 * m[1] * m[2] + 2 * m[0] * m[3], 1 - 2 * m[1] * m[1] - 2 * m[3] * m[3], 2 * m[2] * m[3] - 2 * m[0] * m[1],
			2 * m[1] * m[3] - 2 * m[0] * m[2], 2 * m[2] * m[3] + 2 * m[0] * m[1], 1 - 2 * m[1] * m[1] - 2 * m[2] * m[2]
		);
	}

	static BaseQuat Slerp(BaseQuat q1, BaseQuat q2, T frac) {
		T angle = acos(q1 | q2);
		T denom = sin(angle);
		if (denom == 0) {
			denom = ALMOST_ZERO;
		}
		return (q1 * sin((1.0 - frac) * angle) + q2 * sin(frac * angle)) * (1.0 / denom);
	}
};

using Quat = BaseQuat<double>;
using fQuat = BaseQuat<float>;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct BaseMatrix4 {

	T m[4][4];

	BaseMatrix4(void) { m[0][0] = 0; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0; m[1][0] = 0; m[1][1] = 0; m[1][2] = 0; m[1][3] = 0; m[2][0] = 0; m[2][1] = 0; m[2][2] = 0; m[2][3] = 0; m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 0; }
	BaseMatrix4(BaseVec3<T> row_a, BaseVec3<T> row_b, BaseVec3<T> row_c, BaseVec3<T> row_d) {
		m[0][0] = row_a.x; m[0][1] = row_a.y; m[0][2] = row_a.z; m[0][3] = 0.0;
		m[1][0] = row_b.x; m[1][1] = row_b.y; m[1][2] = row_b.z; m[1][3] = 0.0;
		m[2][0] = row_c.x; m[2][1] = row_c.y; m[2][2] = row_c.z; m[2][3] = 0.0;
		m[3][0] = row_d.x; m[3][1] = row_d.y; m[3][2] = row_d.z; m[3][3] = 0.0;
	}
	BaseMatrix4(T e00, T e01, T e02, T e03, T e10, T e11, T e12, T e13, T e20, T e21, T e22, T e23, T e30, T e31, T e32, T e33) {
		m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; m[0][3] = e03;
		m[1][0] = e10; m[1][1] = e11; m[1][2] = e12; m[1][3] = e13;
		m[2][0] = e20; m[2][1] = e21; m[2][2] = e22; m[2][3] = e23;
		m[3][0] = e30; m[3][1] = e31; m[3][2] = e32; m[3][3] = e33;
	}

	void Set(T e00, T e01, T e02, T e03, T e10, T e11, T e12, T e13, T e20, T e21, T e22, T e23, T e30, T e31, T e32, T e33) {
		m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; m[0][3] = e03;
		m[1][0] = e10; m[1][1] = e11; m[1][2] = e12; m[1][3] = e13;
		m[2][0] = e20; m[2][1] = e21; m[2][2] = e22; m[2][3] = e23;
		m[3][0] = e30; m[3][1] = e31; m[3][2] = e32; m[3][3] = e33;
	}

	BaseMatrix4<float> ToOpenGL(void) const {
		return BaseMatrix4<float>(
			(float)m[0][0], (float)m[1][0], (float)m[2][0], (float)m[3][0]
			,(float)m[0][1], (float)m[1][1], (float)m[2][1], (float)m[3][1]
			,(float)m[0][2], (float)m[1][2], (float)m[2][2], (float)m[3][2]
			,(float)m[0][3], (float)m[1][3], (float)m[2][3], (float)m[3][3]
		);
	}
	const T* Data(void) const {
		return m[0];
	}

	void operator=(const BaseMatrix4<T>& m2) {
		for (int a = 0; a < 4; a++) {
			for (int b = 0; b < 4; b++) {
				m[a][b] = m2.m[a][b];
			}
		}
	}
	BaseMatrix4<T> operator+(const BaseMatrix4<T>& m2) const {
		BaseMatrix4<T> res;
		for (int a = 0; a < 4; a++) {
			for (int b = 0; b < 4; b++) {
				res.m[a][b] = m[a][b] + m2.m[a][b];
			}
		}
		return res;
	}
	BaseMatrix4<T> operator*(const BaseMatrix4<T>& m2) const {
		BaseMatrix4<T> ma;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				for (int r = 0; r < 4; r++) {
					ma.m[i][j] += m[i][r] * m2.m[r][j];
				}
			}
		}
		return ma;
	}
	void operator*=( const BaseMatrix4<T>& m2 ) {
		operator=(operator*(m2));
	}

	BaseMatrix4<T> operator*		( T m2 ) const {
		BaseMatrix4<T> res;
		for (int a = 0; a < 4; a++) {
			for (int b = 0; b < 4; b++) {
				res.m[a][b] = m[a][b] * m2;
			}
		}
		return res;
	}
	BaseVec3<T> operator*		( BaseVec3<T> v ) const {
		BaseVec3<T> res(
			m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3]
			,m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3]
			,m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]
		);
		return res;
	}
	BaseVec4<T> operator*		( BaseVec4<T> v ) const {
		BaseVec4<T> res(
			m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w
			,m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w
			,m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w
			,m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
		);
		return res;
	}

	BaseMatrix4<T> operator~( void ) const {
		BaseMatrix4<T> ma;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				ma.m[i][j] = m[j][i];
			}
		}
		return ma;
	}

	static BaseMatrix4<T> Ortho(T left, T right, T bottom, T top, T near_plane, T far_plane) {
		BaseMatrix4<T> ma;
		const T inv_width = 1.0 / (right - left);
		const T inv_height = 1.0 / (top - bottom);
		const T inv_clip = 1.0 / (far_plane - near_plane);
		ma.m[0][0] = 2.0 * inv_width; ma.m[1][0] = 0.0; ma.m[2][0] = 0.0; ma.m[3][0] = 0.0;
		ma.m[0][1] = 0.0; ma.m[1][1] = 2.0 * inv_height; ma.m[2][1] = 0.0; ma.m[3][1] = 0.0;
		ma.m[0][2] = 0.0; ma.m[1][2] = 0.0; ma.m[2][2] = -2.0 * inv_clip; ma.m[3][2] = 0.0;

		ma.m[0][3] = -(left + right) * inv_width;
		ma.m[1][3] = -(top + bottom) * inv_height;
		ma.m[2][3] = -(near_plane + far_plane) * inv_clip;
		ma.m[3][3] = 1.0;
		return ma;
	}

	static BaseMatrix4<T> Perspective(T vertical_angle, T aspect_ratio, T near_plane, T far_plane) {
		BaseMatrix4<T> ma;
		const T radians = vertical_angle * 0.5 * DEGREES_TO_RADIANS;
		const T sin_rad = sin(radians);
		const T cotan = cos(radians) / sin_rad;
		const T inv_clip = 1.0 / (far_plane - near_plane);
		ma.m[0][0] = cotan / aspect_ratio; ma.m[0][1] = 0.0; ma.m[0][2] = 0.0; ma.m[0][3] = 0.0;
		ma.m[1][0] = 0.0; ma.m[1][1] = cotan; ma.m[1][2] = 0.0; ma.m[1][3] = 0.0;

		ma.m[2][0] = 0.0;
		ma.m[2][1] = 0.0;
		ma.m[2][2] = -(near_plane + far_plane) * inv_clip;
		ma.m[2][3] = -(2.0 * near_plane * far_plane) * inv_clip;

		ma.m[3][0] = 0.0; ma.m[3][1] = 0.0; ma.m[3][2] = -1.0; ma.m[3][3] = 0.0;
		return ma;
	}

	static BaseMatrix4<T> Rotation(const BaseQuat<T>& quat) {
		BaseMatrix4<T> ma;
		const T f2x = quat.m[1] + quat.m[1];
		const T f2y = quat.m[2] + quat.m[2];
		const T f2z = quat.m[3] + quat.m[3];
		const T f2xw = f2x * quat.m[0];
		const T f2yw = f2y * quat.m[0];
		const T f2zw = f2z * quat.m[0];
		const T f2xx = f2x * quat.m[1];
		const T f2xy = f2x * quat.m[2];
		const T f2xz = f2x * quat.m[3];
		const T f2yy = f2y * quat.m[2];
		const T f2yz = f2y * quat.m[3];
		const T f2zz = f2z * quat.m[3];

		ma.m[0][0] = 1.0 - (f2yy + f2zz);
		ma.m[0][1] = f2xy - f2zw;
		ma.m[0][2] = f2xz + f2yw;
		ma.m[0][3] = 0.0;

		ma.m[1][0] = f2xy + f2zw;
		ma.m[1][1] = 1.0 - (f2xx + f2zz);
		ma.m[1][2] = f2yz - f2xw;
		ma.m[1][3] = 0.0;

		ma.m[2][0] = f2xz - f2yw;
		ma.m[2][1] = f2yz + f2xw;
		ma.m[2][2] = 1.0 - (f2xx + f2yy);
		ma.m[2][3] = 0.0;

		ma.m[3][0] = 0.0; ma.m[3][1] = 0.0; ma.m[3][2] = 0.0; ma.m[3][3] = 1.0;
		return ma;
	}

	static BaseMatrix4<T> Rotation(const T degrees, BaseVec3<T> axis) {
		axis.Normalize();

		BaseMatrix4<T> ma;
		const T rads = degrees * DEGREES_TO_RADIANS;
		const T co = cos(rads), si = sin(rads);
		const T inv_co = 1.0 - co;

		ma.m[0][0] = axis.x * axis.x * inv_co + co;
		ma.m[0][1] = axis.x * axis.y * inv_co - axis.z * si;
		ma.m[0][2] = axis.x * axis.z * inv_co + axis.y * si;
		ma.m[0][3] = 0.0;

		ma.m[1][0] = axis.y * axis.x * inv_co + axis.z * si;
		ma.m[1][1] = axis.y * axis.y * inv_co + co;
		ma.m[1][2] = axis.y * axis.z * inv_co - axis.x * si;
		ma.m[1][3] = 0.0;

		ma.m[2][0] = axis.x * axis.z * inv_co - axis.y * si;
		ma.m[2][1] = axis.y * axis.z * inv_co + axis.x * si;
		ma.m[2][2] = axis.z * axis.z * inv_co + co;
		ma.m[2][3] = 0.0;

		ma.m[3][0] = 0.0; ma.m[3][1] = 0.0; ma.m[3][2] = 0.0; ma.m[3][3] = 1.0;
		return ma;
	}

	void Translate(BaseVec3<T> vect) {
		m[0][3] += m[0][0] * vect.x + m[0][1] * vect.y + m[0][2] * vect.z;
		m[1][3] += m[1][0] * vect.x + m[1][1] * vect.y + m[1][2] * vect.z;
		m[2][3] += m[2][0] * vect.x + m[2][1] * vect.y + m[2][2] * vect.z;
		m[3][3] += m[3][0] * vect.x + m[3][1] * vect.y + m[3][2] * vect.z;
	}

	T SubDeterminant2x2(const T m[4][4], int col0, int col1, int row0, int row1) const {
		return m[col0][row0] * m[col1][row1] - m[col0][row1] * m[col1][row0];
	}

	T SubDeterminant3x3(const T m[4][4], int col0, int col1, int col2, int row0, int row1, int row2) const {
		return m[col0][row0] * SubDeterminant2x2(m, col1, col2, row1, row2) - m[col1][row0] * SubDeterminant2x2(m, col0, col2, row1, row2) + m[col2][row0] * SubDeterminant2x2(m, col0, col1, row1, row2);
	}

	T Determinant		( void ) const {
		T det;
		det = m[0][0] * SubDeterminant3x3(m, 1, 2, 3, 1, 2, 3);
		det -= m[1][0] * SubDeterminant3x3(m, 0, 2, 3, 1, 2, 3);
		det += m[2][0] * SubDeterminant3x3(m, 0, 1, 3, 1, 2, 3);
		det -= m[3][0] * SubDeterminant3x3(m, 0, 1, 2, 1, 2, 3);
		return det;
	}

	BaseMatrix4<T> Inverse			( bool* ok = nullptr ) const {
		BaseMatrix4<T> inv;
		T det = Determinant();
		if (det == 0.0) {
			if (ok) {
				*ok = false;
			}
			return inv;
		}
		det = 1.0 / det;

		inv.m[0][0] = SubDeterminant3x3(m, 1, 2, 3, 1, 2, 3) * det;
		inv.m[0][1] = -SubDeterminant3x3(m, 0, 2, 3, 1, 2, 3) * det;
		inv.m[0][2] = SubDeterminant3x3(m, 0, 1, 3, 1, 2, 3) * det;
		inv.m[0][3] = -SubDeterminant3x3(m, 0, 1, 2, 1, 2, 3) * det;
		inv.m[1][0] = -SubDeterminant3x3(m, 1, 2, 3, 0, 2, 3) * det;
		inv.m[1][1] = SubDeterminant3x3(m, 0, 2, 3, 0, 2, 3) * det;
		inv.m[1][2] = -SubDeterminant3x3(m, 0, 1, 3, 0, 2, 3) * det;
		inv.m[1][3] = SubDeterminant3x3(m, 0, 1, 2, 0, 2, 3) * det;
		inv.m[2][0] = SubDeterminant3x3(m, 1, 2, 3, 0, 1, 3) * det;
		inv.m[2][1] = -SubDeterminant3x3(m, 0, 2, 3, 0, 1, 3) * det;
		inv.m[2][2] = SubDeterminant3x3(m, 0, 1, 3, 0, 1, 3) * det;
		inv.m[2][3] = -SubDeterminant3x3(m, 0, 1, 2, 0, 1, 3) * det;
		inv.m[3][0] = -SubDeterminant3x3(m, 1, 2, 3, 0, 1, 2) * det;
		inv.m[3][1] = SubDeterminant3x3(m, 0, 2, 3, 0, 1, 2) * det;
		inv.m[3][2] = -SubDeterminant3x3(m, 0, 1, 3, 0, 1, 2) * det;
		inv.m[3][3] = SubDeterminant3x3(m, 0, 1, 2, 0, 1, 2) * det;
		if (ok) {
			*ok = true;
		}
		return inv;
	}

	static BaseMatrix4<T> GetScale( BaseVec3<T> vec ) {
		BaseMatrix4<T> scale;
		scale.m[0][0] = vec.x;
		scale.m[1][1] = vec.y;
		scale.m[2][2] = vec.z;
		return scale;
	}

	static BaseMatrix4<T> Identity(void) {
		return BaseMatrix4<T>(
			1, 0, 0, 0
			,0, 1, 0, 0
			,0, 0, 1, 0
			,0, 0, 0, 1
		);
	}
};

using Matrix4 = BaseMatrix4<double>;
using fMatrix4 = BaseMatrix4<float>;

//void Matrix4UnitTest(void);
//void QuatUnitTest(void);

bool IntersectLineCircle(double cx, double cy, double radius_sqr, double x0, double y0, double x1, double y1, double& result_t0, double& result_t1);
bool IntersectLineSphere(Vec3 centre, double radius_sqr, Vec3 p0, Vec3 p1, double& result_t0, double& result_t1);
bool CircumSphere(Vec3 p0, Vec3 p1, Vec3 p2, Vec3& out_centre);

} // namespace Neshny