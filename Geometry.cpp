////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
std::optional<Vec2> GetInterceptPosition(Vec2 target_pos, Vec2 target_vel, Vec2 start_pos, double intercept_speed, double* time_mult) {

	Vec2 delta = start_pos - target_pos;

	double dist = delta.Length();
	double a_speed = target_vel.Length();
	double r = intercept_speed / a_speed;
	double a = r * r - 1.0;

	// use law of cosins: c^2 = a^2 + b^2 - 2abCos(y)
	// where y is angle between position delta vector and target velocity
	double cos_a = (delta | target_vel) / (a_speed * dist);
	double b = 2 * dist * cos_a;
	double c = -(dist * dist);

	double det = b * b - 4.0 * a * c;
	if (det < 0.0) {
		return std::nullopt;
	}
	double a_dist = (-b + sqrt(det)) / (2.0 * a);
	double t = a_dist / a_speed;
	if (t < 0.0) {
		return std::nullopt;
	}

	if (time_mult) {
		*time_mult = t;
	}

	return target_pos + target_vel * t;
}