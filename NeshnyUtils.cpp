////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "NeshnyUtils.h"

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
void RandomGenerator::AutoSeed(void) {
    Seed(TimeSinceEpochMilliseconds());
}

////////////////////////////////////////////////////////////////////////////////
unsigned int RandomGenerator::Next(void) {
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xor_shifted = ((m_State >> 18) ^ m_State) >> 27;
    uint32_t rot = m_State >> 59;
    // Advance internal state
    m_State = m_State * 6364136223846793005ULL + 1442695040888963407ULL;
    return (xor_shifted >> rot) | (xor_shifted << (((rot ^ 0xFFFFFFFF) + 1) & 31));
}

////////////////////////////////////////////////////////////////////////////////
void RandomSeed(uint64_t seed) {
    g_GlobalRandom.Seed(seed);
    g_GlobalRandom.Next();
}

////////////////////////////////////////////////////////////////////////////////
double Random(void) {
    return g_GlobalRandom.Next() * INV_UINT;
}

////////////////////////////////////////////////////////////////////////////////
double Random(double min_val, double max_val) {
    return Random() * (max_val - min_val) + min_val;
}

////////////////////////////////////////////////////////////////////////////////
int RandomInt(int min_val, int max_val) {
    return int(floor(Random(min_val, max_val))); // don't use modulo to avoid bias
}

////////////////////////////////////////////////////////////////////////////////
int RoundUpPowerTwo(int value) {
    if (value <= 0) {
        return 0;
    }
    int pow_two = int(ceil(log2(double(value))));
    return 1 << pow_two;
}

////////////////////////////////////////////////////////////////////////////////
size_t HashMemory(unsigned char* mem, int size) {
    return std::hash<std::string_view>()(std::string_view((char*)mem, size));
}

////////////////////////////////////////////////////////////////////////////////
uint64_t TimeSinceEpochMilliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

////////////////////////////////////////////////////////////////////////////////
bool NearlyEqual(double a, double b) {
    return fabs(a - b) < ALMOST_ZERO;
}

////////////////////////////////////////////////////////////////////////////////
std::string ReplaceAll(std::string_view str, std::string_view before, std::string_view after) {
    std::string result(str);
    if (before.empty()) {
        return result;
    }
    size_t start_pos = 0;
    while ((start_pos = result.find(before, start_pos)) != std::string::npos) {
        result.replace(start_pos, before.length(), after);
        start_pos += after.length();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
std::string JoinStrings(const std::vector<std::string>& list, std::string_view insert_between) {
    std::string result;
    int last_index = (int)list.size() - 1;
    for (int i = 0; i <= last_index; i++) {
        if (i == last_index) {
            result += list[i];
        } else {
            result += std::format("{}{}", list[i], insert_between);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
std::string JoinStrings(const std::list<std::string>& list, std::string_view insert_between) {
    std::string result;
    bool first = true;
    for (const auto& str: list) {
        if (first) {
            result += str;
            first = false;
        } else {
            result += std::format("{}{}", insert_between, str);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
bool StringContains(std::string_view str, std::string_view substr, bool case_insensitive) {

    auto case_insensitive_compare = [] (char a, char b) -> bool {
        return std::tolower(a) == std::tolower(b);
    };
    auto case_sensitive_compare = [] (char a, char b) -> bool {
        return a == b;
    };

    auto it = std::search(str.begin(), str.end(), substr.begin(), substr.end(), case_insensitive ? case_insensitive_compare : case_sensitive_compare);
    return it != str.end();
}

} // namespace Neshny