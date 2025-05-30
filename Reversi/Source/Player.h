#pragma once
class Board;

class Player {
private:
	int cellWidth;
	int cellHeight;
public:
	Player(BoardValue color) : playerColor(color), cellWidth(0), cellHeight(0) {}
	~Player() {}

	bool MouseHandler(Board& board, HWND hWnd, RECT clientRect, int x, int y);

	bool HasValidMove(const Board& board) const;

	std::vector<std::pair<int, int>> GetValidMoves(const Board& board);

	bool ValidCell(const Board& board, int x, int y) const;
	bool ValidCell(const Board& board, int x, int y, BoardValue color) const;

	void FlipPieces(Board& board, int x, int y);

	void FlipPieces(Board& board, int x, int y, BoardValue color);

	BoardValue playerColor;
};