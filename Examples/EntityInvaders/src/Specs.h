////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct SkillSpec {
	enum class Type {
		S_ADD,
		S_POWER,
		S_EXPONENTIAL,
	};

	static double GetAmount(int level, Type type, double base_amount, double increase_amount) {
		if (type == Type::S_ADD) {
			return base_amount + increase_amount * level;
		} else if (type == Type::S_POWER) {
			return base_amount * pow((double)level, increase_amount);
		} else if (type == Type::S_EXPONENTIAL) {
			return base_amount * pow(increase_amount, (double)level);
		}
		return 0.0;
	};

	double GetValue(int level) const {
		return GetAmount(level, p_TypeValue, p_BaseValue, p_IncreaseValue);
	};

	double GetCost(int level) const {
		return GetAmount(level + 1, p_TypeCost, p_BaseCost, p_IncreaseCost);
	};

	std::string	p_Name;
	Type		p_TypeValue;
	Type		p_TypeCost;
	double		p_BaseValue;
	double		p_BaseCost;
	double		p_IncreaseValue;
	double		p_IncreaseCost;
	int			p_LevelCap = -1;
};

std::vector<SkillSpec> g_Skills = {
	{ "Energy Rate", SkillSpec::Type::S_ADD, SkillSpec::Type::S_POWER, 10.0, 10.0, 10.0, 1.2 }
	,{ "Energy Max", SkillSpec::Type::S_ADD, SkillSpec::Type::S_POWER, 10.0, 10.0, 5.0, 1.1 }
};

std::vector<GPUEnemySpec> g_EnemySpecs = {
	//{ 10.0, 100, }
	//{ 0.1, 1.0 }
	{ 0.1, 100.0 }
};