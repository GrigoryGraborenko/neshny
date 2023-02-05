////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// might consider templating this func
////////////////////////////////////////////////////////////////////////////////
bool LineLineIntersect(Vec4 a0, Vec4 a1, Vec4 b0, Vec4 b1, Vec4& out_a, Vec4& out_b, double* a_frac, double* b_frac) {

	Vec4 p43 = b1 - b0;
	if ((fabs(p43.x) < ALMOST_ZERO) && (fabs(p43.y) < ALMOST_ZERO) && (fabs(p43.z) < ALMOST_ZERO) && (fabs(p43.w) < ALMOST_ZERO)) {
		return false;
	}
	Vec4 p21 = a1 - a0;
	if ((fabs(p21.x) < ALMOST_ZERO) && (fabs(p21.y) < ALMOST_ZERO) && (fabs(p21.z) < ALMOST_ZERO) && (fabs(p21.w) < ALMOST_ZERO)) {
		return false;
	}
	Vec4 p13 = a0 - b0;

	double d1343 = p13 | p43;
	double d4321 = p43 | p21;
	double d1321 = p13 | p21;
	double d4343 = p43 | p43;
	double d2121 = p21 | p21;

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
Matrix3::Matrix3(Vec3 t1, Vec3 t2, Vec3 t3) {
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
Vec3 Matrix3::operator*(Vec3 t2) {
	Vec3 ta;
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
Matrix4::Matrix4(Vec3 row_a, Vec3 row_b, Vec3 row_c, Vec3 row_d) {
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
Vec3 Matrix4::operator*(Vec3 v) const {
	Vec3 res(
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

		Vec3 tst(Random(-rad, rad), Random(-rad, rad), Random(-rad, rad));
		auto my_tst_res = mine_res * tst;
		auto their_tst_res = Vec3(theirs_res * tst.toVec4());
		Vec3 diff = their_tst_res - my_tst_res;
		double delta = diff.Length();

		bool theirs_ok = false, mine_ok = false;
		auto theirs_inv = theirs_res.inverted(&theirs_ok);
		auto mine_inv = mine_res.inverse(&mine_ok);
		assert(theirs_ok == mine_ok);
		if (theirs_ok) {

			auto my_tst_inv_res = mine_inv * tst;
			auto their_tst_inv_res = Vec3(theirs_inv * tst.toVec4());
			Vec3 diff_inv = their_tst_inv_res - my_tst_inv_res;
			double delta_inv = diff_inv.Length();
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
bool IntersectLineSphere(Vec3 centre, double radius_sqr, Vec3 p0, Vec3 p1, double& result_t0, double& result_t1) {

	Vec3 delta = p1 - p0;
	Vec3 f = p0 - centre;

	double a = delta | delta;
	double b = 2.0 * (delta | f);
	double c = (f | f) - radius_sqr;

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
bool CircumSphere(Vec3 p0, Vec3 p1, Vec3 p2, Vec3& out_centre) {

	Vec3 d10 = p1 - p0;
	Vec3 d20 = p2 - p0;

	Vec3 cross = d10 ^ d20;
	double clensqr = cross.LengthSquared();
	if (clensqr < 0.01) {
		return false;
	}

	Vec3 double_cross = cross ^ d10;
	Vec3 alt_double_cross = cross ^ d20;
	Vec3 mid01 = (p0 + p1) * 0.5;
	Vec3 mid02 = (p0 + p2) * 0.5;

	Vec3 intersect_a, intersect_b;
	if (!Vec3::LineLineIntersect(mid01, mid01 + double_cross, mid02, mid02 + alt_double_cross, false, intersect_a, intersect_b)) {
		return false;
	}
	out_centre = (intersect_a + intersect_b) * 0.5;
	return true;
}
