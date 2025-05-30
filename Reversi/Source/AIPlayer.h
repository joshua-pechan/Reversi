#pragma once

class AIPlayer : public Player {
private:
	const int tileWeights[8][8] = {
		{ 120, -20,  20,   5,   5,  20, -20, 120 },
		{ -20, -40,  -5,  -5,  -5,  -5, -40, -20 },
		{  20,  -5,  15,   3,   3,  15,  -5,  20 },
		{   5,  -5,   3,   3,   3,   3,  -5,   5 },
		{   5,  -5,   3,   3,   3,   3,  -5,   5 },
		{  20,  -5,  15,   3,   3,  15,  -5,  20 },
		{ -20, -40,  -5,  -5,  -5,  -5, -40, -20 },
		{ 120, -20,  20,   5,   5,  20, -20, 120 }
	};

	const int minimaxLevelDepth = 6;
public:
	AIPlayer(BoardValue color) : Player(color) {}

	std::vector<std::pair<int, int>> GetValidMoves(const Board& board);
	std::vector<std::pair<int, int>> GetValidMoves(const Board& board, BoardValue color);

	// heuristic
	int WeightedCountFlips(const Board& board, int x, int y, int dx, int dy);
	int WeightedSimulateFlips(const Board& board, int x, int y);
	std::pair<int, int> ChooseHeuristic(const Board& board, std::vector<std::pair<int, int>> validMoves);

	// minimax
	int GetTurnMultiplier(BoardValue currentTurn);
	int EvaluateBoard(const Board& board);
	int Minimax(const Board& board, int depth, int alpha, int beta, BoardValue currentTurn);
	std::pair<int, int> ChooseMinimax(const Board& board, std::vector<std::pair<int, int>> validMoves);

	void move(Board& board, HWND hWnd, Difficulty difficulty);
};

