////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <vector>
#include <list>
#include <set>
#include <deque>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <optional>
#include <thread>
#include <mutex>

#include "glad/glad.h"

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define msg(str) message(__FILE__ "(" STRING(__LINE__) ") ____________________ " str " ____________________")

#define RADIANS_TO_DEGREES 57.29577951

#define GETSIGN(x) (((x) < 0) ? -1 : (((x) > 0) ? 1 : 0))
#define GETCLAMP(x, min_val, max_val) (((x) < (min_val)) ? (min_val) : (((x) > (max_val)) ? (max_val) : (x)))
#define MIX(a, b, frac) ((a) * (1 - (frac)) + (b) * (frac))

#define PI 3.14159265359
constexpr double TWOPI = 2.0 * PI;
#define SIGN(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))
#define ALMOST_ZERO 0.0000001
constexpr double ONE_THIRD = 1.0 / 3.0;
constexpr double INV_255 = 1.0 / 255.0;

#define GIGA_CONVERT 1000000000
#define MEGA_CONVERT 1000000

#define NANO_CONVERT 0.000000001
#define MICRO_CONVERT 0.000001

constexpr float FLOAT_NAN = std::numeric_limits<float>::quiet_NaN();
constexpr double DOUBLE_NAN = std::numeric_limits<double>::quiet_NaN();

////////////////////////////////////////////////////////////////////////////////
double Random(void);
double Random(double min_val, double max_val);
int RandomInt(int min_val, int max_val);

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

#include "GLUtils.h"
#include "Triple.h"
#include "Serialization.h"
#include "Core.h"
#include "Resources.h"
#include "GPUEntity.h"
#include "EditorViewers.h"
#include "Pipeline.h"
#include "Testing.h"
