//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

#pragma pack(push)
#pragma pack (1)

	enum Option {
		TERRIBLE = 0,
		OK = 1,
		DECENT = 2,
		GREAT = 3,
		GOAT = 4
	};

	struct Small {
		int				p_Int;
		float			p_Float;
		double			p_Double;
		Option			p_Quality;

		bool operator==(const Small& other) const {
			return (p_Int == other.p_Int) && (p_Float == other.p_Float) && (p_Double == other.p_Double) && (p_Quality == other.p_Quality);
		}
	};

	struct Composite {
		int				p_Int;
		float			p_Float;
		double			p_Double;
		Neshny::fVec2	p_TwoDim;
		Neshny::fVec3	p_ThreeDim;
		Neshny::fVec4	p_FourDim;
		Neshny::iVec2	p_IntTwoDim;
		Neshny::iVec3	p_IntThreeDim;
		Neshny::iVec4	p_IntFourDim;
		Neshny::Vec2	p_DTwoDim;
		Neshny::Vec3	p_DThreeDim;
		Neshny::Vec4	p_DFourDim;
		std::string		p_StrOne;
		std::string		p_StrTwo;
		std::vector<Small>						p_SmallItems;
		std::list<Neshny::fVec2>				p_Vectors;
		std::unordered_map<std::string, Small>	p_Map;

		bool operator==(const Composite& other) const {
			return	(p_Int == other.p_Int) && (p_Float == other.p_Float) && (p_Double == other.p_Double) &&
				(p_TwoDim == other.p_TwoDim) &&
				(p_ThreeDim == other.p_ThreeDim) &&
				(p_FourDim == other.p_FourDim) &&
				(p_IntTwoDim == other.p_IntTwoDim) &&
				(p_IntThreeDim == other.p_IntThreeDim) &&
				(p_IntFourDim == other.p_IntFourDim) &&
				(p_DTwoDim == other.p_DTwoDim) &&
				(p_DThreeDim == other.p_DThreeDim) &&
				(p_DFourDim == other.p_DFourDim) &&
				(p_StrOne == other.p_StrOne) &&
				(p_StrTwo == other.p_StrTwo) &&
				(p_SmallItems == other.p_SmallItems) &&
				(p_Vectors == other.p_Vectors) &&
				(p_Map == other.p_Map);
		}
	};

	struct EdgeCases {
		int					p_Int;
		std::optional<int>	p_OpInt;
		std::optional<int>	p_OpIntB;
		std::optional<std::string>	p_OpStrA;
		std::optional<std::string>	p_OpStrB;
		std::vector<bool>			p_ListBools;
		std::optional<Small>		p_OpStruct;

		bool operator==(const EdgeCases& other) const {
			return (p_Int == other.p_Int) &&
				(p_OpInt == other.p_OpInt) && (p_OpIntB == other.p_OpIntB) &&
				(p_OpStrA == other.p_OpStrA) && (p_OpStrB == other.p_OpStrB) &&
				(p_ListBools == other.p_ListBools) &&
				(p_OpStruct == other.p_OpStruct);
		}
	};

#pragma pack(pop)
}

namespace meta {
	template<> inline auto registerMembers<Test::Small>() {
		return members(
			member("Int", &Test::Small::p_Int)
			,member("Float", &Test::Small::p_Float)
			,member("Double", &Test::Small::p_Double)
			,member("Quality", &Test::Small::p_Quality)
		);
	}
	template<> inline auto registerMembers<Test::Composite>() {
		return members(
			member("Int", &Test::Composite::p_Int)
			,member("Float", &Test::Composite::p_Float)
			,member("Double", &Test::Composite::p_Double)
			,member("TwoDim", &Test::Composite::p_TwoDim)
			,member("ThreeDim", &Test::Composite::p_ThreeDim)
			,member("FourDim", &Test::Composite::p_FourDim)
			,member("IntTwoDim", &Test::Composite::p_IntTwoDim)
			,member("IntThreeDim", &Test::Composite::p_IntThreeDim)
			,member("IntFourDim", &Test::Composite::p_IntFourDim)
			,member("DTwoDim", &Test::Composite::p_DTwoDim)
			,member("DThreeDim", &Test::Composite::p_DThreeDim)
			,member("DFourDim", &Test::Composite::p_DFourDim)
			,member("StrOne", &Test::Composite::p_StrOne)
			,member("StrTwo", &Test::Composite::p_StrTwo)
			,member("SmallItems", &Test::Composite::p_SmallItems)
			,member("Vectors", &Test::Composite::p_Vectors)
			,member("Map", &Test::Composite::p_Map)
		);
	}
	template<> inline auto registerMembers<Test::EdgeCases>() {
		return members(
			member("Int", &Test::EdgeCases::p_Int)
			,member("OpInt", &Test::EdgeCases::p_OpInt)
			,member("OpIntB", &Test::EdgeCases::p_OpIntB)
			,member("OpStrA", &Test::EdgeCases::p_OpStrA)
			,member("OpStrB", &Test::EdgeCases::p_OpStrB)
			,member("ListBools", &Test::EdgeCases::p_ListBools)
			,member("OpStruct", &Test::EdgeCases::p_OpStruct)
		);
	}

}

namespace Test {

	const Small g_TestItemSmall{ 123, 0.123, 123000.00123, Option::GREAT };

	const Composite g_TestItemComposite{
		456, 0.456, 456000.00456,
		Neshny::fVec2(1.1, 2.2), Neshny::fVec3(3.3, 4.4, 5.5), Neshny::fVec4(6.6, 7.7, 8.8, 9.9),
		Neshny::iVec2(1, 2), Neshny::iVec3(3, 4, 5), Neshny::iVec4(6, 7, 8, 9),
		Neshny::Vec2(1.0000001, 2.0000002), Neshny::Vec3(3.0000003, 4.0000004, 5.0000005), Neshny::Vec4(6.0000006, 7.0000007, 8.0000008, 9.0000009),
		"hello there", "this is a second, longer string",
		{
			{ 123, 0.123, 123000.00123, Option::GREAT },
			{ 1, -10.01, -100.0, Option::OK },
			{ -10000, 10.01, -0.0099, Option::DECENT },
		},
		{}, // empty array
		{
			{ "hello", { 13, 0.13, 13000.0013, Option::TERRIBLE }},
			{ "there", { -666, -0.666, -666000.00666, Option::GOAT }}
		}
	};

	const EdgeCases g_TestItemEdgeCases{ -321, std::nullopt, -654, "string that is set", std::nullopt, { true, false, false, true, false }, Small{ 0, -450.1, 10.0999, Option::TERRIBLE } };

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_JSONSimple(void) {

		Neshny::Json::ParseError err;
		std::string data = Neshny::Json::ToJson<Small>(g_TestItemSmall, err);
		Expect("JSON serialization for simple struct", std::string(R"({"Double":123000.00123,"Float":0.12300000339746475,"Int":123,"Quality":3})") == data);
		
		Small output;
		Neshny::Json::FromJson(data, output, err);
		Expect("JSON deserialization for simple struct", output == g_TestItemSmall);
	}

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_JSONComposite(void) {

		Neshny::Json::ParseError err;
		std::string data = Neshny::Json::ToJson<Composite>(g_TestItemComposite, err);
		Expect("JSON serialization for composite struct", std::string(R"({"DFourDim":{"w":9.0000009,"x":6.0000006,"y":7.0000007,"z":8.0000008},"DThreeDim":{"x":3.0000003,"y":4.0000004,"z":5.0000005},"DTwoDim":{"x":1.0000001,"y":2.0000002},"Double":456000.00456,"Float":0.4560000002384186,"FourDim":{"w":9.899999618530273,"x":6.599999904632568,"y":7.699999809265137,"z":8.800000190734863},"Int":456,"IntFourDim":{"w":9,"x":6,"y":7,"z":8},"IntThreeDim":{"x":3,"y":4,"z":5},"IntTwoDim":{"x":1,"y":2},"Map":{"hello":{"Double":13000.0013,"Float":0.12999999523162842,"Int":13,"Quality":0},"there":{"Double":-666000.00666,"Float":-0.6660000085830688,"Int":-666,"Quality":4}},"SmallItems":[{"Double":123000.00123,"Float":0.12300000339746475,"Int":123,"Quality":3},{"Double":-100.0,"Float":-10.010000228881836,"Int":1,"Quality":1},{"Double":-0.0099,"Float":10.010000228881836,"Int":-10000,"Quality":2}],"StrOne":"hello there","StrTwo":"this is a second, longer string","ThreeDim":{"x":3.299999952316284,"y":4.400000095367432,"z":5.5},"TwoDim":{"x":1.100000023841858,"y":2.200000047683716},"Vectors":[]})") == data);
		
		Composite output;
		Neshny::Json::FromJson(data, output, err);
		Expect("JSON deserialization for composite struct", output == g_TestItemComposite);
	}

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_JSONEdgeCases(void) {

		Neshny::Json::ParseError err;
		std::string data = Neshny::Json::ToJson<EdgeCases>(g_TestItemEdgeCases, err);
		Expect("JSON serialization for edge cases struct", std::string(R"({"Int":-321,"ListBools":[true,false,false,true,false],"OpIntB":-654,"OpStrA":"string that is set","OpStruct":{"Double":10.0999,"Float":-450.1000061035156,"Int":0,"Quality":0}})") == data);
		
		EdgeCases output;
		Neshny::Json::FromJson(data, output, err);
		Expect("JSON deserialization for edge cases struct", output == g_TestItemEdgeCases);
	}

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_BinarySimple(void) {

		Neshny::Binary::ParseError err;
		auto binary = Neshny::Binary::ToBinary(g_TestItemSmall, err);

		Small output;
		Neshny::Binary::FromBinary(binary, output, err);

		Expect("Binary serialization and deserialization for simple struct", output == g_TestItemSmall);
	}

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_BinaryComposite(void) {

		Neshny::Binary::ParseError err;
		auto binary = Neshny::Binary::ToBinary(g_TestItemComposite, err);

		Composite output;
		Neshny::Binary::FromBinary(binary, output, err);

		Expect("Binary serialization and deserialization for composite struct", output == g_TestItemComposite);
	}

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_BinaryEdgeCases(void) {

		Neshny::Binary::ParseError err;
		auto binary = Neshny::Binary::ToBinary(g_TestItemEdgeCases, err);

		EdgeCases output;
		Neshny::Binary::FromBinary(binary, output, err);

		Expect("Binary serialization and deserialization for edge cases struct", output == g_TestItemEdgeCases);
	}

}