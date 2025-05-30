#pragma once

#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <limits>
#include <algorithm>

#include "../resource.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define MAX_NAME_STRING 256
#define HInstance() GetModuleHandle(NULL)

#define INF std::numeric_limits<int>::max()

enum class BoardValue {
	EMPTY,
	BLACK,
	WHITE,
};

enum class Difficulty {
	HEURISTIC,
	MINIMAX,
	LEARNING
};

const int directions[8][2] = {
	{-1,  1}, { 1,  1}, { 0,  1},
	{-1,  0},           { 1,  0},
	{-1, -1}, { 0, -1}, { 1, -1},
};
