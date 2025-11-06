#pragma once
#include "pch.h"
#include "Board.h"

class Board;

struct MoveResult {
	bool valid;
	int row;
	int col;
	std::vector<std::pair<int, int>> flipped;
};

class Player {
private:
	int cellWidth;
	int cellHeight;
public:
	Player(BoardValue color) : playerColor(color), cellWidth(0), cellHeight(0) {}
	~Player() {}

	MoveResult MouseHandler(Board& board, HWND hWnd, RECT clientRect, int x, int y);

	bool HasValidMove(const Board& board) const;

	std::vector<std::pair<int, int>> GetValidMoves(const Board& board);

	bool ValidCell(const Board& board, int x, int y) const;
	bool ValidCell(const Board& board, int x, int y, BoardValue color) const;

	BoardValue playerColor;
};