////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void Triple::set(double e0, double e1, double e2) {
	x=e0;y=e1;z=e2;
}

////////////////////////////////////////////////////////////////////////////////
void Triple::operator=(const Triple& t2) {
	x=t2.x;y=t2.y;z=t2.z;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::operator+(const Triple& t2) const {
	Triple ta;
	ta.x=x+t2.x;ta.y=y+t2.y;ta.z=z+t2.z;
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
void Triple::operator+=(const Triple& t2) {
	x = x + t2.x;
	y = y + t2.y;
	z = z + t2.z;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::operator-(const Triple& t2) const {
	Triple ta;
	ta.x=x-t2.x;ta.y=y-t2.y;ta.z=z-t2.z;
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
double Triple::operator*(const Triple& t2) const {
	return x*t2.x + y*t2.y + z*t2.z;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::operator*(double f) const {
	Triple ta;
	ta.x=x*f;ta.y=y*f;ta.z=z*f;
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
void Triple::operator*=(double f) {
	x *= f;
	y *= f;
	z *= f;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::operator/(double f) const {
	Triple ta;
	ta.x = x / f; ta.y = y / f; ta.z = z / f;
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::operator/(const Triple& t2) const {
	Triple ta;
	ta.x = x / t2.x; ta.y = y / t2.y; ta.z = z / t2.z;
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::operator^(const Triple& t2) const {
	Triple ta;
	ta.x = y*t2.z - z*t2.y;
	ta.y = z*t2.x - x*t2.z;
	ta.z = x*t2.y - y*t2.x;
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
bool Triple::operator==(const Triple& t2) const {
	return ((x==t2.x)&&(y==t2.y)&&(z==t2.z));
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::round(void) const {
	Triple ta;
	ta.x = floor(x + 0.5f);
	ta.y = floor(y + 0.5f);
	ta.z = floor(z + 0.5f);
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::sign(void) const {
	return Triple(SIGN(x), SIGN(y), SIGN(z));
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::mult(const Triple& t2) const {
	return Triple(x * t2.x, y * t2.y, z * t2.z);
}

////////////////////////////////////////////////////////////////////////////////
double Triple::lengthSquared(void) const {
	return (x * x + y * y + z * z);
}

////////////////////////////////////////////////////////////////////////////////
double Triple::length(void) const {
	return sqrt(x*x+y*y+z*z);
}

////////////////////////////////////////////////////////////////////////////////
void Triple::normalize(void) {
	double dist = length();
	if(dist == 0)
		dist = ALMOST_ZERO;
	x = x / dist;
	y = y / dist;
	z = z / dist;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::normalizeCopy(void) const {
	Triple res;
	double dist = length();
	if(dist == 0)
		dist = ALMOST_ZERO;
	res.x = x / dist;
	res.y = y / dist;
	res.z = z / dist;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::modify(double new_val, Axis axis) const {
	switch(axis) {
		case Axis::X: return Triple(new_val, y, z);
		case Axis::Y: return Triple(x, new_val, z);
	}
	return Triple(x, y, new_val);
}

////////////////////////////////////////////////////////////////////////////////
Triple Triple::NearestToLine(Triple start, Triple end, bool clamp, double* frac) const {

	Triple p_to_lp0 = (*this) - start;
	Triple lp1_to_lp0 = end - start;

	double numer = p_to_lp0 * lp1_to_lp0;
	double denom = lp1_to_lp0 * lp1_to_lp0;

	if(denom == 0) {
		if(frac) {
			frac = 0;
		}
		return start;
	}
	double u = numer / denom;
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

////////////////////////////////////////////////////////////////////////////////
bool Triple::LineLineIntersect(Triple a0, Triple a1, Triple b0, Triple b1, bool clamp, Triple& out_a, Triple& out_b, double* a_frac, double* b_frac) {

	Triple p43 = b1 - b0;
	if ((fabs(p43.x) < ALMOST_ZERO) && (fabs(p43.y) < ALMOST_ZERO) && (fabs(p43.z) < ALMOST_ZERO)) {
		return false;
	}
	Triple p21 = a1 - a0;
	if ((fabs(p21.x) < ALMOST_ZERO) && (fabs(p21.y) < ALMOST_ZERO) && (fabs(p21.z) < ALMOST_ZERO)) {
		return false;
	}
	Triple p13 = a0 - b0;

	double d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
	double d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
	double d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
	double d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
	double d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

	double denom = d2121 * d4343 - d4321 * d4321;
	if (fabs(denom) < ALMOST_ZERO) {
		return false;
	}
	double numer = d1343 * d4321 - d1321 * d4343;

	double mua = numer / denom;
	double mub = (d1343 + d4321 * (mua)) / d4343;

	if (clamp) {
		mua = GETCLAMP(mua, 0.0, 1.0);
		mub = GETCLAMP(mub, 0.0, 1.0);
	}

	out_a = Triple(a0.x + mua * p21.x, a0.y + mua * p21.y, a0.z + mua * p21.z);
	out_b = Triple(b0.x + mub * p43.x, b0.y + mub * p43.y, b0.z + mub * p43.z);

	if (a_frac) {
		*a_frac = mua;
	}
	if (b_frac) {
		*b_frac = mub;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4 Triple::GetScale(void) const {
	Matrix4 scale = Matrix4::Identity();
	scale.m[0][0] = x;
	scale.m[1][1] = y;
	scale.m[2][2] = z;
	return scale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void Vec2::set(double e0, double e1) {
	x = e0;
	y = e1;
}

////////////////////////////////////////////////////////////////////////////////
void Vec2::operator=(const Vec2& t2) {
	x = t2.x;
	y = t2.y;
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::operator+(const Vec2& t2) const {
	return Vec2(x + t2.x, y + t2.y);
}

////////////////////////////////////////////////////////////////////////////////
void Vec2::operator+=(const Vec2& t2) {
	x += t2.x;
	y += t2.y;
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::operator-(const Vec2& t2) const {
	return Vec2(x - t2.x, y - t2.y);
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::operator*(double f) const {
	return Vec2(x * f, y * f);
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::operator/(double f) const {
	return Vec2(x / f, y / f);
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::operator/(const Vec2& t2) const {
	return Vec2(x / t2.x, y / t2.y);
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::operator*(const Vec2& t2) const {
	return Vec2(x * t2.x, y * t2.y);
}

////////////////////////////////////////////////////////////////////////////////
bool Vec2::operator==(const Vec2& t2) const {
	return ((x == t2.x) && (y == t2.y));
}

////////////////////////////////////////////////////////////////////////////////
void Vec2::normalize(void) {
	double inv_len = 1.0 / length();
	x *= inv_len;
	y *= inv_len;
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::normalizeCopy(void) {
	double inv_len = 1.0 / length();
	return Vec2(x * inv_len, y * inv_len);
}

////////////////////////////////////////////////////////////////////////////////
Vec2 Vec2::NearestToLine(Vec2 A, Vec2 B, bool clamp, double* frac) {

	Vec2 a_p = *this - A;
	Vec2 a_b = B - A;

	double a_b_sqr = a_b.lengthSquared();
	double a_p_dot_a_b = a_b.x * a_p.x + a_b.y * a_p.y;

	double t = a_p_dot_a_b / a_b_sqr;
	if (clamp) {
		t = std::max(0.0, std::min(1.0, t));
	}
	if (frac) {
		*frac = t;
	}
	return A + a_b * t;
}

////////////////////////////////////////////////////////////////////////////////
bool Vec2::LineLineIntersect(Vec2 a0, Vec2 a1, Vec2 b0, Vec2 b1, Vec2& out, double* a_frac, double* b_frac) {

	Vec2 r = a1 - a0;
	Vec2 s = b1 - b0;

	Vec2 qp = b0 - a0;
	double numerator = qp ^ r;
	double denominator = r ^ s;

	// lines are parallel
	if (denominator == 0) {
		return false;
	}

	if ((numerator == 0) && (denominator == 0)) { // they are collinear
		// TODO: check if the lines are overlapping
		return false;
	}

	double u = numerator / denominator;
	double t = (qp ^ s) / denominator;
	if (a_frac) {
		*a_frac = u;
	}
	if (b_frac) {
		*b_frac = t;
	}

	return (t >= 0) && (t <= 1) && (u >= 0) && (u <= 1);
}

// might consider templating this func
////////////////////////////////////////////////////////////////////////////////
bool Vec4::LineLineIntersect(Vec4 a0, Vec4 a1, Vec4 b0, Vec4 b1, Vec4& out_a, Vec4& out_b, double* a_frac, double* b_frac) {

	Vec4 p43 = b1 - b0;
	if ((fabs(p43.x) < ALMOST_ZERO) && (fabs(p43.y) < ALMOST_ZERO) && (fabs(p43.z) < ALMOST_ZERO) && (fabs(p43.w) < ALMOST_ZERO)) {
		return false;
	}
	Vec4 p21 = a1 - a0;
	if ((fabs(p21.x) < ALMOST_ZERO) && (fabs(p21.y) < ALMOST_ZERO) && (fabs(p21.z) < ALMOST_ZERO) && (fabs(p21.w) < ALMOST_ZERO)) {
		return false;
	}
	Vec4 p13 = a0 - b0;

	double d1343 = p13 * p43;
	double d4321 = p43 * p21;
	double d1321 = p13 * p21;
	double d4343 = p43 * p43;
	double d2121 = p21 * p21;

	double denom = d2121 * d4343 - d4321 * d4321;
	if (fabs(denom) < ALMOST_ZERO) {
		return false;
	}
	double numer = d1343 * d4321 - d1321 * d4343;

	double mua = numer / denom;
	double mub = (d1343 + d4321 * (mua)) / d4343;

	out_a = Vec4(a0.x + mua * p21.x, a0.y + mua * p21.y, a0.z + mua * p21.z, a0.w + mua * p21.w);
	out_b = Vec4(b0.x + mub * p43.x, b0.y + mub * p43.y, b0.z + mub * p43.z, b0.w + mub * p43.w);

	if (a_frac) {
		*a_frac = mua;
	}
	if (b_frac) {
		*b_frac = mub;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4::Matrix4(void) {
	for(int a = 0; a < 4; a++) {
		for(int b = 0; b < 4; b++) {
			m[a][b] = 0;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
Matrix3::Matrix3(void) {
	int i = 0;
	while(i < 9)
	{
		m[i/3][i%3] = 0;
		i++;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Matrix3::Matrix3(double e00, double e01, double e02, double e10, double e11, double e12, double e20, double e21, double e22) {
	m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; 
	m[1][0] = e10; m[1][1] = e11; m[1][2] = e12;
	m[2][0] = e20; m[2][1] = e21; m[2][2] = e22;
}

////////////////////////////////////////////////////////////////////////////////
Matrix3::Matrix3(Triple t1, Triple t2, Triple t3) {
	set(
		t1.x, t1.y, t1.z
		,t2.x, t2.y, t2.z
		,t3.x, t3.y, t3.z
	);
}

////////////////////////////////////////////////////////////////////////////////
void Matrix3::set(	double e00, double e01, double e02, double e10, double e11, double e12, double e20, double e21, double e22) {
	m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; 
	m[1][0] = e10; m[1][1] = e11; m[1][2] = e12;
	m[2][0] = e20; m[2][1] = e21; m[2][2] = e22;
}

////////////////////////////////////////////////////////////////////////////////
void Matrix3::operator=(Matrix3 m2) {
	set(
		m2.m[0][0], m2.m[0][1], m2.m[0][2] 
		,m2.m[1][0], m2.m[1][1], m2.m[1][2] 
		,m2.m[2][0], m2.m[2][1], m2.m[2][2]
	);
}

////////////////////////////////////////////////////////////////////////////////
Matrix3 Matrix3::operator+(Matrix3 m2) { 
	Matrix3 ma;
	int i = 0;
	while(i < 9) {
		ma.m[i/3][i%3] = m[i/3][i%3] + m2.m[i/3][i%3];
		i++;
	}
	return ma;
}

////////////////////////////////////////////////////////////////////////////////
Matrix3 Matrix3::operator*(Matrix3 m2) {
	Matrix3 ma;
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			for(int r = 0; r < 3; r++) {
				ma.m[i][j] += m[i][r]*m2.m[r][j];
			}
		}
	}
	return ma;
}

////////////////////////////////////////////////////////////////////////////////
Matrix3 Matrix3::operator*(double m2) { 
	Matrix3 ma;
	int i = 0;
	while(i < 9) {
		ma.m[i/3][i%3] = m2 * m[i/3][i%3];
		i++;
	}
	return ma;
}

////////////////////////////////////////////////////////////////////////////////
Triple Matrix3::operator*(Triple t2) {
	Triple ta;
	ta.x= m[0][0]*t2.x + m[0][1]*t2.y + m[0][2]*t2.z;
	ta.y= m[1][0]*t2.x + m[1][1]*t2.y + m[1][2]*t2.z;
	ta.z= m[2][0]*t2.x + m[2][1]*t2.y + m[2][2]*t2.z;
	return ta;
}

////////////////////////////////////////////////////////////////////////////////
Matrix3 Matrix3::operator~(void) {
	Matrix3 ma;
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			ma.m[i][j] = m[j][i];
		}
	}

	return ma;
}

////////////////////////////////////////////////////////////////////////////////
double Matrix3::determinant(void) {
	return(
		m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - 
		m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
		m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])
	);
}

////////////////////////////////////////////////////////////////////////////////
Matrix3 Matrix3::adjoint(void) {
	Matrix3 ma(	
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

////////////////////////////////////////////////////////////////////////////////
Matrix3 Matrix3::inverse(void) {
	double det = determinant();
	if (det == 0) {
		return Matrix3(1,0,0,0,1,0,0,0,1);
	}

	Matrix3 ma = adjoint();
	ma = ma * (1 / det);
	return ma;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Matrix4::Matrix4(QMatrix4x4 mat) {
	auto ptr = mat.data();
	//m[0][0] = ptr[0]; m[0][1] = ptr[1]; m[0][2] = ptr[2]; m[0][3] = ptr[3];
	//m[1][0] = ptr[4]; m[1][1] = ptr[5]; m[1][2] = ptr[6]; m[1][3] = ptr[7];
	//m[2][0] = ptr[8]; m[2][1] = ptr[9]; m[2][2] = ptr[10]; m[2][3] = ptr[11];
	//m[3][0] = ptr[12]; m[3][1] = ptr[13]; m[3][2] = ptr[14]; m[3][3] = ptr[15];
	m[0][0] = ptr[0]; m[0][1] = ptr[4]; m[0][2] = ptr[8]; m[0][3] = ptr[12];
	m[1][0] = ptr[1]; m[1][1] = ptr[5]; m[1][2] = ptr[9]; m[1][3] = ptr[13];
	m[2][0] = ptr[2]; m[2][1] = ptr[6]; m[2][2] = ptr[10]; m[2][3] = ptr[14];
	m[3][0] = ptr[3]; m[3][1] = ptr[7]; m[3][2] = ptr[11]; m[3][3] = ptr[15];
}

////////////////////////////////////////////////////////////////////////////////
Matrix4::Matrix4(Triple row_a, Triple row_b, Triple row_c, Triple row_d) {
	m[0][0] = row_a.x;
	m[0][1] = row_a.y;
	m[0][2] = row_a.z;
	m[0][3] = 0.0;
	m[1][0] = row_b.x;
	m[1][1] = row_b.y;
	m[1][2] = row_b.z;
	m[1][3] = 0.0;
	m[2][0] = row_c.x;
	m[2][1] = row_c.y;
	m[2][2] = row_c.z;
	m[2][3] = 0.0;
	m[3][0] = row_d.x;
	m[3][1] = row_d.y;
	m[3][2] = row_d.z;
	m[3][3] = 0.0;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4::Matrix4(double e00, double e01, double e02, double e03, double e10, double e11, double e12, double e13, double e20, double e21, double e22, double e23, double e30, double e31, double e32, double e33) {
	m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; m[0][3] = e03;
	m[1][0] = e10; m[1][1] = e11; m[1][2] = e12; m[1][3] = e13;
	m[2][0] = e20; m[2][1] = e21; m[2][2] = e22; m[2][3] = e23;
	m[3][0] = e30; m[3][1] = e31; m[3][2] = e32; m[3][3] = e33;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4	Matrix4::Identity(void) {
	return Matrix4(
		1, 0, 0, 0
		,0, 1, 0, 0
		,0, 0, 1, 0
		,0, 0, 0, 1
	);
}

////////////////////////////////////////////////////////////////////////////////
void Matrix4::set(double e00, double e01, double e02, double e03, double e10, double e11, double e12, double e13, double e20, double e21, double e22, double e23, double e30, double e31, double e32, double e33) {
	m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; m[0][3] = e03;
	m[1][0] = e10; m[1][1] = e11; m[1][2] = e12; m[1][3] = e13;
	m[2][0] = e20; m[2][1] = e21; m[2][2] = e22; m[2][3] = e23;
	m[3][0] = e30; m[3][1] = e31; m[3][2] = e32; m[3][3] = e33;
}

////////////////////////////////////////////////////////////////////////////////
void Matrix4::operator=(Matrix4 m2) {
	for(int a = 0; a < 4; a++) {
		for(int b = 0; b < 4; b++) {
			m[a][b] = m2.m[a][b];
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
Matrix4 Matrix4::operator+(Matrix4 m2) const {
	Matrix4 res;
	for(int a = 0; a < 4; a++) {
		for(int b = 0; b < 4; b++) {
			res.m[a][b] = m[a][b] + m2.m[a][b];
		}
	}
	return res;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4 Matrix4::operator*(Matrix4 m2) const {

	Matrix4 ma;
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			for(int r = 0; r < 4; r++) {
				ma.m[i][j] += m[i][r]*m2.m[r][j];
			}
		}
	}
	return ma;
}

////////////////////////////////////////////////////////////////////////////////
Triple Matrix4::operator*(Triple v) const {
	Triple res(
		m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3]
		,m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3]
		,m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]
	);
	return res;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4 Matrix4::operator*(double m2) const {
	Matrix4 res;
	for(int a = 0; a < 4; a++) {
		for(int b = 0; b < 4; b++) {
			res.m[a][b] = m[a][b] * m2;
		}
	}
	return res;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4 Matrix4::operator~(void) const {
	Matrix4 ma;
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			ma.m[i][j] = m[j][i];
		}
	}

	return ma;
}

////////////////////////////////////////////////////////////////////////////////
double SubDeterminant2x2(const double m[4][4], int col0, int col1, int row0, int row1) {
	return m[col0][row0] * m[col1][row1] - m[col0][row1] * m[col1][row0];
}

////////////////////////////////////////////////////////////////////////////////
double SubDeterminant3x3(const double m[4][4], int col0, int col1, int col2, int row0, int row1, int row2) {
	return m[col0][row0] * SubDeterminant2x2(m, col1, col2, row1, row2)
		- m[col1][row0] * SubDeterminant2x2(m, col0, col2, row1, row2)
		+ m[col2][row0] * SubDeterminant2x2(m, col0, col1, row1, row2);
}

////////////////////////////////////////////////////////////////////////////////
double Matrix4::determinant(void) const {
	double det;
	det  = m[0][0] * SubDeterminant3x3(m, 1, 2, 3, 1, 2, 3);
	det -= m[1][0] * SubDeterminant3x3(m, 0, 2, 3, 1, 2, 3);
	det += m[2][0] * SubDeterminant3x3(m, 0, 1, 3, 1, 2, 3);
	det -= m[3][0] * SubDeterminant3x3(m, 0, 1, 2, 1, 2, 3);
	return det;
}

////////////////////////////////////////////////////////////////////////////////
Matrix4 Matrix4::inverse(bool* ok) const {

	Matrix4 inv;
	double det = determinant();
	if (det == 0.0f) {
		if (ok) {
			*ok = false;
		}
		return inv;
	}
	det = 1.0f / det;

	inv.m[0][0] =  SubDeterminant3x3(m, 1, 2, 3, 1, 2, 3) * det;
	inv.m[0][1] = -SubDeterminant3x3(m, 0, 2, 3, 1, 2, 3) * det;
	inv.m[0][2] =  SubDeterminant3x3(m, 0, 1, 3, 1, 2, 3) * det;
	inv.m[0][3] = -SubDeterminant3x3(m, 0, 1, 2, 1, 2, 3) * det;
	inv.m[1][0] = -SubDeterminant3x3(m, 1, 2, 3, 0, 2, 3) * det;
	inv.m[1][1] =  SubDeterminant3x3(m, 0, 2, 3, 0, 2, 3) * det;
	inv.m[1][2] = -SubDeterminant3x3(m, 0, 1, 3, 0, 2, 3) * det;
	inv.m[1][3] =  SubDeterminant3x3(m, 0, 1, 2, 0, 2, 3) * det;
	inv.m[2][0] =  SubDeterminant3x3(m, 1, 2, 3, 0, 1, 3) * det;
	inv.m[2][1] = -SubDeterminant3x3(m, 0, 2, 3, 0, 1, 3) * det;
	inv.m[2][2] =  SubDeterminant3x3(m, 0, 1, 3, 0, 1, 3) * det;
	inv.m[2][3] = -SubDeterminant3x3(m, 0, 1, 2, 0, 1, 3) * det;
	inv.m[3][0] = -SubDeterminant3x3(m, 1, 2, 3, 0, 1, 2) * det;
	inv.m[3][1] =  SubDeterminant3x3(m, 0, 2, 3, 0, 1, 2) * det;
	inv.m[3][2] = -SubDeterminant3x3(m, 0, 1, 3, 0, 1, 2) * det;
	inv.m[3][3] =  SubDeterminant3x3(m, 0, 1, 2, 0, 1, 2) * det;
	if (ok) {
		*ok = true;
	}
	return inv;
}

////////////////////////////////////////////////////////////////////////////////
QMatrix4x4 Matrix4::toQMatrix4x4(void) const {
	return QMatrix4x4(
		(float)m[0][0], (float)m[0][1], (float)m[0][2], (float)m[0][3]
		,(float)m[1][0], (float)m[1][1], (float)m[1][2], (float)m[1][3]
		,(float)m[2][0], (float)m[2][1], (float)m[2][2], (float)m[2][3]
		,(float)m[3][0], (float)m[3][1], (float)m[3][2], (float)m[3][3]
	);
}

////////////////////////////////////////////////////////////////////////////////
void Matrix4UnitTest(void) {

	const double rad = 10;
	for (int i = 0; i < 32; i++) {

		double vals[16] = {
			Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
			,Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
			,Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
			,Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
		};
		double vals_b[16] = {
			Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
			,Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
			,Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
			,Random(-rad, rad), Random(-rad, rad), Random(-rad, rad), Random(-rad, rad)
		};
		Matrix4 mine(
			vals[0], vals[1], vals[2], vals[3]
			,vals[4], vals[5], vals[6], vals[7]
			,vals[8], vals[9], vals[10], vals[11]
			,vals[12], vals[13], vals[14], vals[15]
		);
		QMatrix4x4 theirs(
			vals[0], vals[1], vals[2], vals[3]
			,vals[4], vals[5], vals[6], vals[7]
			,vals[8], vals[9], vals[10], vals[11]
			,vals[12], vals[13], vals[14], vals[15]
		);
		Matrix4 mine_b(
			vals_b[0], vals_b[1], vals_b[2], vals_b[3]
			,vals_b[4], vals_b[5], vals_b[6], vals_b[7]
			,vals_b[8], vals_b[9], vals_b[10], vals_b[11]
			,vals_b[12], vals_b[13], vals_b[14], vals_b[15]
		);
		QMatrix4x4 theirs_b(
			vals_b[0], vals_b[1], vals_b[2], vals_b[3]
			,vals_b[4], vals_b[5], vals_b[6], vals_b[7]
			,vals_b[8], vals_b[9], vals_b[10], vals_b[11]
			,vals_b[12], vals_b[13], vals_b[14], vals_b[15]
		);
		auto theirs_res = theirs* theirs_b;
		auto mine_res = mine * mine_b;

		Triple tst(Random(-rad, rad), Random(-rad, rad), Random(-rad, rad));
		auto my_tst_res = mine_res * tst;
		auto their_tst_res = Triple(theirs_res * tst.toVec4());
		Triple diff = their_tst_res - my_tst_res;
		double delta = diff.length();

		bool theirs_ok = false, mine_ok = false;
		auto theirs_inv = theirs_res.inverted(&theirs_ok);
		auto mine_inv = mine_res.inverse(&mine_ok);
		assert(theirs_ok == mine_ok);
		if (theirs_ok) {

			auto my_tst_inv_res = mine_inv * tst;
			auto their_tst_inv_res = Triple(theirs_inv * tst.toVec4());
			Triple diff_inv = their_tst_inv_res - my_tst_inv_res;
			double delta_inv = diff_inv.length();
			assert(delta_inv < 0.001);
		}
		assert(delta < 0.001);
	}
}

#pragma msg("unify these two functions with template")
////////////////////////////////////////////////////////////////////////////////
bool IntersectLineCircle(double cx, double cy, double radius_sqr, double x0, double y0, double x1, double y1, double& result_t0, double& result_t1) {

	double dx = x1 - x0;
	double dy = y1 - y0;

	double fx = x0 - cx;
	double fy = y0 - cy;

	double a = dx * dx + dy * dy;
	double b = 2.0 * (dx * fx + dy * fy);
	double c = (fx * fx + fy * fy) - radius_sqr;

	double det = b * b - 4 * a * c;
	if (det < 0) {
		return false;
	}
	det = sqrt(det);
	double inv_denom = 0.5 / a;
	result_t0 = (-b - det) * inv_denom;
	result_t1 = (-b + det) * inv_denom;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool IntersectLineSphere(Triple centre, double radius_sqr, Triple p0, Triple p1, double& result_t0, double& result_t1) {

	Triple delta = p1 - p0;
	Triple f = p0 - centre;

	double a = delta * delta;
	double b = 2.0 * (delta * f);
	double c = (f * f) - radius_sqr;

	double det = b * b - 4 * a * c;
	if (det < 0) {
		return false;
	}
	det = sqrt(det);
	double inv_denom = 0.5 / a;
	result_t0 = (-b - det) * inv_denom;
	result_t1 = (-b + det) * inv_denom;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CircumSphere(Triple p0, Triple p1, Triple p2, Triple& out_centre) {

	Triple d10 = p1 - p0;
	Triple d20 = p2 - p0;

	Triple cross = d10 ^ d20;
	double clensqr = cross.lengthSquared();
	if (clensqr < 0.01) {
		return false;
	}

	Triple double_cross = cross ^ d10;
	Triple alt_double_cross = cross ^ d20;
	Triple mid01 = (p0 + p1) * 0.5;
	Triple mid02 = (p0 + p2) * 0.5;

	Triple intersect_a, intersect_b;
	if (!Triple::LineLineIntersect(mid01, mid01 + double_cross, mid02, mid02 + alt_double_cross, false, intersect_a, intersect_b)) {
		return false;
	}
	out_centre = (intersect_a + intersect_b) * 0.5;
	return true;
}
