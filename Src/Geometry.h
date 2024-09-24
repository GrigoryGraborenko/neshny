////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Grid2DStepCursor {

	Grid2DStepCursor(Vec2 start, Vec2 end) {
		m_Start = start;
		m_Delta = end - start;
		iVec2 grid_pos = start.Floor().ToIVec2();

		m_GridDirs = m_Delta.Sign().ToIVec2();
		m_Amounts = Vec2(fabs(1.0 / m_Delta.x), fabs(1.0 / m_Delta.y));

		Vec2 left_neg((double(grid_pos.x) - start.x) / m_Delta.x, (double(grid_pos.y) - start.y) / m_Delta.y);
		Vec2 left_pos((double(grid_pos.x) + 1.0 - start.x) / m_Delta.x, (double(grid_pos.y) + 1.0 - start.y) / m_Delta.y);

		m_Left = Vec2(std::max(left_neg.x, left_pos.x), std::max(left_neg.y, left_pos.y));

		m_StepDist = 0;
		m_CurrentGrid = grid_pos;
	}

	bool HasNext() {
		return m_StepDist < 1.0;
	}

	void Next() {

		float move_dist = std::min(m_Left.x, m_Left.y);
		m_StepDist += move_dist;
		m_Left += Vec2(-move_dist, -move_dist);
		Vec2 hit_res = (m_Left * -1).Step(0);
		m_Left += hit_res * m_Amounts;
		m_CurrentGrid = m_CurrentGrid + hit_res.ToIVec2() * m_GridDirs;
	}

	inline iVec2 CurrentGridPos() { return m_CurrentGrid; }

	inline Vec2 CurrentPos() { return m_Start + m_Delta * m_StepDist; }

private:
	Vec2 m_Start;
	Vec2 m_Delta;
	iVec2 m_GridDirs;
	Vec2 m_Amounts;
	Vec2 m_Left;

	float m_StepDist;
	iVec2 m_CurrentGrid;
};

std::optional<Vec2> GetInterceptPosition(Vec2 target_pos, Vec2 target_vel, Vec2 start_pos, double intercept_speed, double* time_mult = nullptr);

////////////////////////////////////////////////////////////////////////////////
template<typename V>
bool RaySphere(V sphere_pos, double sphere_rad, V ray_origin, V ray_end, V& hit_pos, double& hit_frac, V* normal = nullptr) {

	V d = ray_end - ray_origin;
	V f = ray_origin - sphere_pos;

	double sqr_size = sphere_rad * sphere_rad;
	double a = d | d;
	double b = 2.0 * (f | d);
	double c = (f | f) - sqr_size;

	double det = b * b - 4.0 * a * c;
	if(det < 0.0) {
		return false;
	}

	det = sqrt(det);
	double t = (-b - det) / (2.0 * a); // assume smallest frac - doesn't handle exit point
	hit_frac = t;
	hit_pos = d * t + ray_origin;

	if (normal) {
		*normal = (hit_pos - sphere_pos).NormalizeCopy();
	}
	return true;
}

template<typename T>
class Grid2DCPUCache {

public:

	Grid2DCPUCache		( Vec2 min_pos, Vec2 max_pos, double cell_size, std::function<Vec2(const T& item)> get_position ) :
		p_MinPos		( min_pos )
		,p_InvCellSize	( 1.0 / cell_size )
		,p_GridRange	( ((max_pos - min_pos) * p_InvCellSize).Ceil() )
		,p_GetPosFunc	( get_position )
	{
		p_Cells.resize(p_GridRange.x * p_GridRange.y);
	}

	void	AddItem			( T& item ) {
		GetCell(p_GetPosFunc(item)).push_back(&item);
	}

	void	AddItems		( std::vector<T>& items ) {
		for (auto& item : items) {
			AddItem(item);
		}
	}

	void	Reset			( void ) { for(auto& cell: p_Cells) { cell.clear(); } }

	bool	AnyWithin		( Vec2 pos, double radius ) {
		auto min_g = GetGridPos(pos - Vec2(radius, radius));
		auto max_g = GetGridPos(pos + Vec2(radius, radius));
		double rad_sqr = radius * radius;
		for (int x = min_g.x; x <= max_g.x; x++) {
			for (int y = min_g.y; y <= max_g.y; y++) {
				int index = y * p_GridRange.x + x;
				auto& cell = p_Cells[index];
				for (auto& item : cell) {
					Vec2 item_pos = p_GetPosFunc(*item);
					double dist_sqr = (pos - item_pos).LengthSquared();
					if (dist_sqr < rad_sqr) {
						return true;
					}
				}
			}
		}
		return false;
	}

	void	Iterate			( Vec2 from_pos, Vec2 to_pos, std::function<void(T* item)> item_callback ) {
		auto min_g = GetGridPos(from_pos);
		auto max_g = GetGridPos(to_pos);
		for (int x = min_g.x; x <= max_g.x; x++) {
			for (int y = min_g.y; y <= max_g.y; y++) {
				int index = y * p_GridRange.x + x;
				auto& cell = p_Cells[index];
				for (auto& item : cell) {
					item_callback(item);
				}
			}
		}
	}

private:

	iVec2	GetGridPos		( Vec2 pos ) {
		return iVec2::Max(iVec2(0, 0), iVec2::Min(p_GridRange + iVec2(-1, -1), iVec2(((pos - p_MinPos) * p_InvCellSize).Floor())));
	}

	std::vector<T*>&	GetCell	( Vec2 pos ) {
		auto gpos = GetGridPos(pos);
		int index = gpos.y * p_GridRange.x + gpos.x;
		return p_Cells[index];
	}

	Vec2							p_MinPos;
	double							p_CellSize;
	double							p_InvCellSize;
	iVec2							p_GridRange;
	std::function<Vec2(const T&)>	p_GetPosFunc;
	std::vector<std::vector<T*>>	p_Cells;
};

} // namespace Neshny