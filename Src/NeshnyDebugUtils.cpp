////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "NeshnyDebugUtils.h"

namespace Neshny {

///////////////////////////////////////////////////////////////////////////////
void AlgorithmStepper::RenderDialog(bool* close) {
	ImGui::Begin("Stepper", close, ImGuiWindowFlags_NoCollapse);

	int step_count = p_Steps.size();
	ImGui::SetNextItemWidth(500);
	ImGui::SliderInt("Step", &p_DisplayedStep, 0, step_count - 2);

	if (p_DisplayedStep < p_Steps.size()) {
		const auto& step = p_Steps[p_DisplayedStep];
		if (!step.p_Header.empty()) {
			ImGui::Text(step.p_Header.data());
			ImGui::NewLine();
		}
		for (auto& info : step.p_Info) {
			ImGui::Text(info.data());
		}
	}

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
void AlgorithmStepper::AdjustCurrentStep(int delta) {
	p_DisplayedStep = std::max(0, std::min((int)p_Steps.size() - 2, p_DisplayedStep + delta));
}

///////////////////////////////////////////////////////////////////////////////
void AlgorithmStepper::RenderMain(Vec2 inner_offset, double scale, Vec2 outer_offset, Vec2 min_pos, Vec2 max_pos, std::optional<Vec2> mouse_pos) {

	if (mouse_pos.has_value()) {
		std::optional<int> hover_step = std::nullopt;
		double min_dist_sqr = -1;
		for (int i = 0; i < p_Steps.size(); i++) {
			if (!p_Steps[i].p_MainHighlight.has_value()) {
				continue;
			}
			double dist_sqr = (p_Steps[i].p_MainHighlight.value() - mouse_pos.value()).LengthSquared();
			if ((!hover_step.has_value()) || (dist_sqr < min_dist_sqr)) {
				hover_step = i;
				min_dist_sqr = dist_sqr;
			}
		}
		if (hover_step.has_value()) {
			p_DisplayedStep = *hover_step;
		}
	}

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	const double small_rad = p_PointSize;
	const double rad = p_PointSize * 2.0;

	if (p_DisplayedStep >= p_Steps.size()) {
		return;
	}
	const auto& step = p_Steps[p_DisplayedStep];
	for (const auto& point : step.p_Main2DPoints) {

		Vec2 screen_pos = (point.p_Pos - inner_offset) * scale + outer_offset;
		if ((screen_pos.x < min_pos.x) || (screen_pos.x >= max_pos.x) || (screen_pos.y < min_pos.y) || (screen_pos.y >= max_pos.y)) {
			continue;
		}
		ImVec2 screen(screen_pos.x, screen_pos.y);

		float mult = 1.0;
		if (point.p_Type != PointType::P_SMALL_FILLED_CIRCLE) {
			mult = Random();
		}
		auto col = IM_COL32(point.p_Color.x * 255 * mult, point.p_Color.y * 255 * mult, point.p_Color.z * 255 * mult, point.p_Color.w * 255);

		if (point.p_Type == PointType::P_CIRCLE) {
			draw_list->AddCircle(screen, rad, col);
		} else if (point.p_Type == PointType::P_CROSS) {
			draw_list->AddLine(ImVec2(screen.x - rad, screen.y - rad), ImVec2(screen.x + rad, screen.y + rad), col);
			draw_list->AddLine(ImVec2(screen.x - rad, screen.y + rad), ImVec2(screen.x + rad, screen.y - rad), col);
		} else if (point.p_Type == PointType::P_SMALL_CROSS) {
			draw_list->AddLine(ImVec2(screen.x - small_rad, screen.y - small_rad), ImVec2(screen.x + small_rad, screen.y + small_rad), col);
			draw_list->AddLine(ImVec2(screen.x - small_rad, screen.y + small_rad), ImVec2(screen.x + small_rad, screen.y - small_rad), col);
		} else if (point.p_Type == PointType::P_SMALL_FILLED_CIRCLE) {
			draw_list->AddCircleFilled(screen, small_rad, col);
			draw_list->AddCircle(screen, small_rad, IM_COL32(0, 0, 0, 255));
		}
		if (!point.p_Text.empty()) {
			ImGui::SetCursorPos(ImVec2(screen.x + 5, screen.y + 5));
			ImGui::TextColored(ImVec4(1, Random(), 0, 1.0), point.p_Text.data());
		}
	}
	for (const auto& line : step.p_Main2DLines) {

		Vec2 screen_pos_a = (line.p_Start - inner_offset) * scale + outer_offset;
		Vec2 screen_pos_b = (line.p_End - inner_offset) * scale + outer_offset;

		if (
			((screen_pos_a.x < min_pos.x) || (screen_pos_a.x >= max_pos.x) || (screen_pos_a.y < min_pos.y) || (screen_pos_a.y >= max_pos.y)) &&
			((screen_pos_b.x < min_pos.x) || (screen_pos_b.x >= max_pos.x) || (screen_pos_b.y < min_pos.y) || (screen_pos_b.y >= max_pos.y))
			) {
			continue;
		}

		ImVec2 screen_a(screen_pos_a.x, screen_pos_a.y);
		ImVec2 screen_b(screen_pos_b.x, screen_pos_b.y);

		float mult = 1.0;
		auto col = IM_COL32(line.p_Color.x * 255 * mult, line.p_Color.y * 255 * mult, line.p_Color.z * 255 * mult, line.p_Color.w * 255);

		draw_list->AddLine(screen_a, screen_b, IM_COL32(0, 0, 0, 255), 2);
		draw_list->AddLine(screen_a, screen_b, col, 1);
	}

}

///////////////////////////////////////////////////////////////////////////////
void AlgorithmStepper::RenderScrapbooks(void) const {

	if (p_DisplayedStep >= p_Steps.size()) {
		return;
	}

	const auto& step = p_Steps[p_DisplayedStep];

	{ // Scrapbook 2D
		const double tiny_rad = p_PointSize;
		const double small_rad = p_PointSize * 2.0;
		const double rad = p_PointSize * 3;

		for (const auto& point : step.p_Scrap2DPoints) {

			float mult = 1.0;
			if (point.p_Type != PointType::P_SMALL_FILLED_CIRCLE) {
				mult = Random();
			}
			Vec4 col = point.p_Color * mult;

			if (point.p_Type == PointType::P_CIRCLE) {
				Scrapbook2D::Circle(point.p_Pos, rad, col);
				Scrapbook2D::Circle(point.p_Pos, small_rad, col);
				Scrapbook2D::Circle(point.p_Pos, tiny_rad, col);
			} else if (point.p_Type == PointType::P_CROSS) {
				Scrapbook2D::Line(point.p_Pos + Vec2(-rad, -rad), point.p_Pos + Vec2(rad, rad), col);
				Scrapbook2D::Line(point.p_Pos + Vec2(-rad, rad), point.p_Pos + Vec2(rad, -rad), col);
			} else if (point.p_Type == PointType::P_SMALL_CROSS) {
				Scrapbook2D::Line(point.p_Pos + Vec2(-small_rad, -small_rad), point.p_Pos + Vec2(small_rad, small_rad), col);
				Scrapbook2D::Line(point.p_Pos + Vec2(-small_rad, small_rad), point.p_Pos + Vec2(small_rad, -small_rad), col);
			} else if (point.p_Type == PointType::P_SMALL_FILLED_CIRCLE) {
				Scrapbook2D::Circle(point.p_Pos, small_rad, col);
			}
			if (!point.p_Text.empty()) {
				Scrapbook2D::Text(point.p_Text, point.p_Pos, point.p_Color);
			}
		}

		for (const auto& line : step.p_Scrap2DLines) {
			float mult = 1.0;
			Vec4 col = line.p_Color * mult;
			Scrapbook2D::Line(line.p_Start, line.p_End, col);
		}
	}

	{ // Scrapbook 3D
		for (const auto& point : step.p_Scrap3DPoints) {
			Scrapbook3D::Point(point.p_Pos, point.p_Color);
		}

		for (const auto& line : step.p_Scrap3DLines) {
			float mult = 1.0;
			Vec4 col = line.p_Color * mult;
			Scrapbook3D::Line(line.p_Start, line.p_End, col);
		}
	}
}

} // namespace Neshny