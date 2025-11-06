#include "Player.h"

MoveResult Player::MouseHandler(Board& board, HWND hWnd, RECT clientRect, int x, int y) {
    MoveResult result = { false, -1, -1, {} };

    cellWidth = (clientRect.right - Board::borderX * 2) / Board::MATRIX_SIZE;
	cellHeight = (clientRect.bottom - Board::borderY * 2) / Board::MATRIX_SIZE;
	
	int indexX = x / cellWidth;
	int indexY = y / cellHeight;

    if (ValidCell(board, indexX, indexY, playerColor)) {
        result.valid = true;
        result.row = indexX;
        result.col = indexY;
        result.flipped = board.ApplyMove(hWnd, indexX, indexY, playerColor);
    }

    return result;
}

bool Player::HasValidMove(const Board& board) const {
    for (int x = 0; x < Board::MATRIX_SIZE; x++) {
        for (int y = 0; y < Board::MATRIX_SIZE; y++) {
            if (ValidCell(board, x, y)) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::pair<int, int>> Player::GetValidMoves(const Board& board) {
    std::vector<std::pair<int, int>> moves;
    for (int x = 0; x < Board::MATRIX_SIZE; x++) {
        for (int y = 0; y < Board::MATRIX_SIZE; y++) {
            if (ValidCell(board, x, y)) {
                moves.push_back({ x, y });
            }
        }
    }
    return moves;
}

bool Player::ValidCell(const Board& board, int x, int y) const {
    return ValidCell(board, x, y, playerColor);
}

bool Player::ValidCell(const Board& board, int x, int y, BoardValue color) const {
    if (x < 0 || x >= Board::MATRIX_SIZE || y < 0 || y >= Board::MATRIX_SIZE) { return false; }

    if (board.boardState[x][y] != BoardValue::EMPTY) { return false; }

    for (auto& dir : directions) {
        int dx = dir[0], dy = dir[1];
        int nx = x + dx, ny = y + dy;
        bool foundOpponent = false;

        while (nx >= 0 && nx < Board::MATRIX_SIZE && ny >= 0 && ny < Board::MATRIX_SIZE) {
            BoardValue currentValue = board.boardState[nx][ny];

            if (currentValue == BoardValue::EMPTY) { break; }

            if (currentValue == color) {
                if (foundOpponent) { return true; }
                break;
            }

            foundOpponent = true;
            nx += dx;
            ny += dy;
        }
    }

    return false;
}
