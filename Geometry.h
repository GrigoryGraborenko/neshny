////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

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

	bool Next() {

		float move_dist = std::min(m_Left.x, m_Left.y);
		m_StepDist += move_dist;
		m_Left += Vec2(-move_dist, -move_dist);
		Vec2 hit_res = (m_Left * -1).Step(0);
		m_Left += hit_res * m_Amounts;
		m_CurrentGrid = m_CurrentGrid + hit_res.ToIVec2() * m_GridDirs;

		return m_StepDist < 1.0;
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