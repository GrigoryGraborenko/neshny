////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define msg(str) message(__FILE__ "(" STRING(__LINE__) ") ____________________ " str " ____________________")

constexpr double RADIANS_TO_DEGREES = 57.29577951;
constexpr double DEGREES_TO_RADIANS = 1.0 / RADIANS_TO_DEGREES;

#define GETSIGN(x) (((x) < 0) ? -1 : (((x) > 0) ? 1 : 0))
#define GETCLAMP(x, min_val, max_val) (((x) < (min_val)) ? (min_val) : (((x) > (max_val)) ? (max_val) : (x)))
#define MIX(a, b, frac) ((a) * (1 - (frac)) + (b) * (frac))

constexpr double PI = 3.14159265359;
constexpr double TWOPI = 2.0 * PI;
#define SIGN(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))
#define STEP(v, step) ((v >= step) ? 1 : 0)
#define ALMOST_ZERO 0.0000001
constexpr double ONE_THIRD = 1.0 / 3.0;
constexpr double INV_255 = 1.0 / 255.0;
constexpr double INV_UINT = 1.0 / std::numeric_limits<unsigned int>::max();

#define GIGA_CONVERT 1000000000
#define MEGA_CONVERT 1000000

#define NANO_CONVERT 0.000000001
#define MICRO_CONVERT 0.000001

constexpr float FLOAT_NAN = std::numeric_limits<float>::quiet_NaN();
constexpr double DOUBLE_NAN = std::numeric_limits<double>::quiet_NaN();

////////////////////////////////////////////////////////////////////////////////
// Permuted congruential generator
class RandomGenerator {
public:
					RandomGenerator	( bool auto_seed = true ) : m_State(0x4d595df4d0f33173) { if(auto_seed) AutoSeed(); }
					RandomGenerator	( uint64_t seed ) : m_State(seed) {}
	void			Seed			( uint64_t seed ) { m_State = seed;}
	void			AutoSeed		( void );
	unsigned int	Next			( void );
private:
	uint64_t	m_State;
};

static RandomGenerator g_GlobalRandom;

void RandomSeed(uint64_t seed);
double Random(void);
double Random(double min_val, double max_val);
int RandomInt(int min_val, int max_val);
int RoundUpPowerTwo(int value);
size_t HashMemory(unsigned char* mem, int size);

////////////////////////////////////////////////////////////////////////////////
template <class T>
void RemoveUnordered(std::vector<T>& vect, typename std::vector<T>::iterator iter) {
	if (vect.empty()) {
		return;
	}
	typename std::vector<T>::iterator back = std::prev(vect.end());
	if (back != iter) {
		*iter = *back;
	}
	vect.resize(vect.size() - 1);
}
////////////////////////////////////////////////////////////////////////////////
template <class T>
void Shuffle(std::vector<T>& vect) {
	int s = (int)vect.size();
	std::vector<std::pair<int, int>> indices;
	indices.reserve(s);
	for (int i = 0; i < s; i++) {
		indices.push_back({ i, rand() + rand() * RAND_MAX });
	}
	std::sort(indices.begin(), indices.end(), [&indices](const std::pair<int, int>& a, const std::pair<int, int>& b) -> bool {
		return a.second < b.second;
		});

	std::vector<T> output;
	output.reserve(s);
	for (const auto& ind : indices) {
		output.push_back(vect[ind.first]);
	}
	vect = std::move(output);
}
////////////////////////////////////////////////////////////////////////////////
uint64_t TimeSinceEpochMilliseconds();
