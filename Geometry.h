////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct Grid2DStepCursor {

	Grid2DStepCursor(Vec2 start, Vec2 end) {
		m_Start = start;
		m_Delta = end - start;
		m_GridPos = IVec2(start.Floor());

		m_GridDirs = IVec2(m_Delta.Sign());
		m_Amounts = Vec2(fabs(1.0 / m_Delta.x), fabs(1.0 / m_Delta.y));

		Vec2 left_neg((double(m_GridPos.x) - start.x) / m_Delta.x, (double(m_GridPos.y) - start.y) / m_Delta.y);
		Vec2 left_pos((double(m_GridPos.x) + 1.0 - start.x) / m_Delta.x, (double(m_GridPos.y) + 1.0 - start.y) / m_Delta.y);

		m_Left = Vec2(std::max(left_neg.x, left_pos.x), std::max(left_neg.y, left_pos.y));

		m_StepDist = 0;
		m_CurrentGrid = m_GridPos;
	}

	bool Next() {

		float move_dist = std::min(m_Left.x, m_Left.y);
		m_StepDist += move_dist;
		m_Left += Vec2(-move_dist, -move_dist);
		Vec2 hit_res = (m_Left * -1).Step(0);
		m_Left += hit_res * m_Amounts;
		m_CurrentGrid = m_CurrentGrid + IVec2(hit_res) * m_GridDirs;

		return m_StepDist < 1.0;
	}

	inline IVec2 CurrentGridPos() { return m_CurrentGrid; }

	inline Vec2 CurrentPos() { return m_Start + m_Delta * m_StepDist; }

private:
	Vec2 m_Start;
	Vec2 m_Delta;
	IVec2 m_GridPos;
	IVec2 m_GridDirs;
	Vec2 m_Amounts;
	Vec2 m_Left;

	float m_StepDist;
	IVec2 m_CurrentGrid;
};