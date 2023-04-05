////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Neshny {

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
void Matrix4UnitTest(void) {

	const float rad = 10;
	for (int i = 0; i < 32; i++) {

		float vals[16] = {
			(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
			,(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
			,(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
			,(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
		};
		float vals_b[16] = {
			(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
			,(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
			,(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
			,(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
		};
		fMatrix4 mine(
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
		fMatrix4 mine_b(
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

		fMatrix4 persp_mine = fMatrix4::Perspective(60.0f, 1.5f, 0.1f, 100.0f);
		QMatrix4x4 persp_theirs;
		persp_theirs.perspective(60.0f, 1.5f, 0.1f, 100.0f);

		fVec3 tst(Random(-rad, rad), Random(-rad, rad), Random(-rad, rad));
		auto my_tst_res = mine_res * fVec4(tst, 1.0);
		auto their_tst_res = fVec3(theirs_res * tst.toVec4());
		fVec3 diff = their_tst_res - my_tst_res.ToVec3();
		double delta = diff.Length();

		auto my_persp_res = persp_mine * fVec4(tst, 1.0);
		auto their_persp_res = fVec3(persp_theirs * tst.toVec4());
		double delta_persp = (my_persp_res.ToVec3() - their_persp_res).Length();;
		assert(delta_persp < 0.001);

		bool theirs_ok = false, mine_ok = false;
		auto theirs_inv = theirs_res.inverted(&theirs_ok);
		auto mine_inv = mine_res.Inverse(&mine_ok);
		assert(theirs_ok == mine_ok);
		if (theirs_ok) {

			auto my_tst_inv_res = mine_inv * tst;
			auto their_tst_inv_res = fVec3(theirs_inv * tst.toVec4());
			fVec3 diff_inv = their_tst_inv_res - my_tst_inv_res;
			double delta_inv = diff_inv.Length();
			assert(delta_inv < 0.001);
		}
		assert(delta < 0.001);
	}
}

void QuatUnitTest(void) {
	const float rad = 10;
	for (int i = 0; i < 32; i++) {

		float vals[4] = {
			(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
		};
		float vals_b[4] = {
			(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
		};
		float vals_c[4] = {
			(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
		};
		float vals_d[3] = {
			(float)Random(-rad, rad), (float)Random(-rad, rad), (float)Random(-rad, rad)
		};
		fQuat mine(vals[0], vals[1], vals[2], vals[3]);
		QQuaternion theirs(vals[0], vals[1], vals[2], vals[3]);

		fQuat mine_other(vals_b[0], vals_b[1], vals_b[2], vals_b[3]);
		QQuaternion theirs_other(vals_b[0], vals_b[1], vals_b[2], vals_b[3]);

		fVec3 mine_vect(vals_d[0], vals_d[1], vals_d[2]);
		QVector3D theirs_vect(vals_d[0], vals_d[1], vals_d[2]);

		fQuat mine_axis(fVec3(vals_c[0], vals_c[1], vals_c[2]), vals_c[3]);
		QQuaternion theirs_axis = QQuaternion::fromAxisAndAngle(vals_c[0], vals_c[1], vals_c[2], vals_c[3]);

		fQuat mine_res = mine * mine_other;
		QQuaternion theirs_res = theirs * theirs_other;

		fVec3 mine_vector_res = mine_res * mine_vect;
		QVector3D theirs_vector_res = theirs_res * theirs_vect;

		fQuat mine_multi_res = mine * mine_axis * mine_other;
		QQuaternion theirs_multi_res = theirs * theirs_axis * theirs_other;

		fVec3 mine_vector_multi = mine_multi_res * mine_vect;
		QVector3D theirs_vector_multi = theirs_multi_res * theirs_vect;

		float delta_vect = (mine_vector_res.toVec() - theirs_vector_res).length();
		assert(delta_vect < 0.5);

		float delta_multi_vect = (mine_vector_multi.toVec() - theirs_vector_multi).length();
		assert(delta_multi_vect < 0.5);
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

} // namespace Neshny