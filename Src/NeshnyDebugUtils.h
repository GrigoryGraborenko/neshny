////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct AlgorithmStepper {

	enum class PointType {
		P_NONE
		,P_CIRCLE
		,P_CROSS
		,P_SMALL_FILLED_CIRCLE
		,P_SMALL_CROSS
	};
	struct Point2D {
		Vec2 p_Pos;
		Vec4 p_Color;
		PointType p_Type;
		std::string p_Text;
	};
	struct Point3D {
		Vec3 p_Pos;
		Vec4 p_Color;
		std::string p_Text;
	};
	struct Line2D {
		Vec2 p_Start;
		Vec2 p_End;
		Vec4 p_Color;
	};
	struct Line3D {
		Vec3 p_Start;
		Vec3 p_End;
		Vec4 p_Color;
	};

	struct Step {
		std::string p_Header;
		std::optional<Vec2> p_MainHighlight;
		std::vector<Point2D> p_Main2DPoints;
		std::vector<Point2D> p_Scrap2DPoints;
		std::vector<Point3D> p_Scrap3DPoints;
		std::vector<Line2D> p_Main2DLines;
		std::vector<Line2D> p_Scrap2DLines;
		std::vector<Line3D> p_Scrap3DLines;
		std::vector<std::string> p_Info;
	};

	AlgorithmStepper(float point_size = 6.0) :
		p_PointSize	( point_size )
	{
		p_Steps.push_back({});
		p_DisplayedStep = 0;
	}

	bool HasData() {
		return (p_Steps.size() > 1) || (!p_Steps[0].p_Header.empty()) || (!p_Steps[0].p_Info.empty()) || (!p_Steps[0].p_MainHighlight.has_value())
			|| (!p_Steps[0].p_Main2DPoints.empty())
			|| (!p_Steps[0].p_Scrap2DPoints.empty())
			|| (!p_Steps[0].p_Scrap3DPoints.empty())
			|| (!p_Steps[0].p_Main2DLines.empty())
			|| (!p_Steps[0].p_Scrap2DLines.empty())
			|| (!p_Steps[0].p_Scrap3DLines.empty());
	}

	void EndStep(std::string header = std::string(), std::optional<Vec2> highlight_point = std::nullopt) {
		p_Steps.back().p_Header = header;
		p_Steps.back().p_MainHighlight = highlight_point;
		p_Steps.push_back({});
		p_DisplayedStep = (int)p_Steps.size() - 2;
	}

	void Add2DPointToMain(Vec2 pos, Vec4 color, PointType type = PointType::P_CIRCLE, std::string text = "") {
		p_Steps.back().p_Main2DPoints.push_back({ pos, color, type, text });
	}
	void Add2DPointToScrap(Vec2 pos, Vec4 color, PointType type = PointType::P_CIRCLE, std::string text = "") {
		p_Steps.back().p_Scrap2DPoints.push_back({ pos, color, type, text });
	}
	void Add3DPointToScrap(Vec3 pos, Vec4 color, std::string text = "") {
		p_Steps.back().p_Scrap3DPoints.push_back({ pos, color, text });
	}
	void Add2DLineToMain(Vec2 start, Vec2 end, Vec4 color) {
		p_Steps.back().p_Main2DLines.push_back({ start, end, color });
	}
	void Add2DLineToScrap(Vec2 start, Vec2 end, Vec4 color) {
		p_Steps.back().p_Scrap2DLines.push_back({ start, end, color });
	}
	void Add3DLineToScrap(Vec3 start, Vec3 end, Vec4 color) {
		p_Steps.back().p_Scrap3DLines.push_back({ start, end, color });
	}
	void AddInfo(std::string info) {
		p_Steps.back().p_Info.push_back(info);
	}

	void AdjustCurrentStep(int delta);

	void RenderDialog(bool* close);
	void RenderMain(Vec2 inner_offset, double scale, Vec2 outer_offset, Vec2 min_pos, Vec2 max_pos, std::optional<Vec2> mouse_pos);
	void RenderScrapbooks(void) const;

	std::vector<Step> p_Steps;

	int	p_DisplayedStep = 0;
	float p_PointSize;
};

} // namespace Neshny