#pragma once
#include "pch.h"
#include "Player.h"

// transposition table entry
struct TTEntry {
	int depth;
	int score;
	int flag;
};

class AIPlayer : public Player {
private:
	std::unordered_map<uint64_t, TTEntry> transpositionTable;
public:
	AIPlayer(BoardValue color) : Player(color) {}

	std::vector<std::pair<int, int>> GetValidMoves(const Board& board);
	std::vector<std::pair<int, int>> GetValidMoves(const Board& board, BoardValue color);

	// heuristic
	int WeightedCountFlips(const Board& board, int x, int y, int dx, int dy, BoardValue color);
	int WeightedSimulateFlips(const Board& board, int x, int y, BoardValue color);
	std::pair<int, int> ChooseHeuristic(const Board& board, std::vector<std::pair<int, int>> validMoves, std::atomic<bool>* forceStop = nullptr);

	// minimax
	int GetTurnMultiplier(BoardValue currentTurn);
	int CountStableDiscs(const Board& board, BoardValue color);
	int EvaluateBoard(const Board& board);

	std::vector<std::pair<int, int>> OrderMoves(const Board& board, const std::vector<std::pair<int, int>>& moves, BoardValue color);

	int Minimax(Board& board, int depth, int alpha, int beta, BoardValue currentTurn, std::atomic<bool>* forceStop = nullptr);
	std::pair<int, int> ChooseMinimax(Board& board, const std::vector<std::pair<int, int>>& validMoves, std::atomic<bool>* forceStop = nullptr);

	MoveResult move(Board& board, HWND hWnd, Difficulty difficulty, std::atomic<bool>* forceStop = nullptr);
};

