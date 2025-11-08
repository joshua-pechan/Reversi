#pragma once

#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <limits>
#include <array>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <future>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#include "../resource.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define MAX_NAME_STRING 256
#define HInstance() GetModuleHandle(NULL)

constexpr int INF = std::numeric_limits<int>::max();

enum class BoardValue { EMPTY, BLACK, WHITE };
enum class Difficulty { HEURISTIC, MINIMAX };

// directions for move checking: N, NE, E, SE, S, SW, W, NW
inline const int directions[8][2] = {
    {-1,  1}, { 1,  1}, { 0,  1},
    {-1,  0},           { 1,  0},
    {-1, -1}, { 0, -1}, { 1, -1},
};

constexpr int DEPTH = 64;
static constexpr int AI_TIME_LIMIT = 30;

inline const int tileWeights[8][8] = {
    { 100, -20,  10,   5,   5,  10, -20, 100},
    { -20, -40,  -5,  -5,  -5,  -5, -40, -20},
    {  10,  -5,   5,   1,   1,   5,  -5,  10},
    {   5,  -5,   1,   0,   0,   1,  -5,   5},
    {   5,  -5,   1,   0,   0,   1,  -5,   5},
    {  10,  -5,   5,   1,   1,   5,  -5,  10},
    { -20, -40,  -5,  -5,  -5,  -5, -40, -20},
    { 100, -20,  10,   5,   5,  10, -20, 100}
};

inline bool consoleVisible = false;