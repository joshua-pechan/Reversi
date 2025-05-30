#include "pch.h"
#include "Player.h"
#include "AIPlayer.h"
#include "Board.h"

std::vector<std::pair<int, int>> AIPlayer::GetValidMoves(const Board& board) {
    return GetValidMoves(board, playerColor);
}

std::vector<std::pair<int, int>> AIPlayer::GetValidMoves(const Board& board, BoardValue color) {
    std::vector<std::pair<int, int>> moves;
    for (int x = 0; x < Board::MATRIX_SIZE; x++) {
        for (int y = 0; y < Board::MATRIX_SIZE; y++) {
            if (ValidCell(board, x, y, color)) {
                moves.push_back({ x, y });
            }
        }
    }
    return moves;
}

int AIPlayer::WeightedCountFlips(const Board& board, int x, int y, int dx, int dy) {
    int flips = 0;
    int nx = x + dx;
    int ny = y + dy;
    bool foundOpponent = false;

    while (nx >= 0 && nx < Board::MATRIX_SIZE && ny >= 0 && ny < Board::MATRIX_SIZE) {
        BoardValue currentValue = board.boardState[nx][ny];

        if (currentValue == BoardValue::EMPTY) { return 0; }

        if (currentValue == playerColor) {
            return (foundOpponent ? flips : 0);
        }

        foundOpponent = true;
        flips += tileWeights[x][y];
        nx += dx;
        ny += dy;
    }

    return 0;
}

int AIPlayer::WeightedSimulateFlips(const Board& board, int x, int y) {
    if (board.boardState[x][y] != BoardValue::EMPTY) { return 0; }

    int totalFlips = 0;

    for (const auto& dir : directions) {
        totalFlips += WeightedCountFlips(board, x, y, dir[0], dir[1]);
    }

    return totalFlips;
}

// board heuristic
std::pair<int, int> AIPlayer::ChooseHeuristic(const Board& board, std::vector<std::pair<int, int>> validMoves) {
    std::pair<int, int> bestMove = { -1, -1 };
    int bestScore = -INF;

    for (const std::pair<int, int>& move : validMoves) {
        int score = WeightedSimulateFlips(board, move.first, move.second);
        if (score > bestScore) {
            bestScore = score;
            bestMove = { move.first, move.second };
        }
    }

    return bestMove;
}

// Minimax (negamax)
int AIPlayer::GetTurnMultiplier(BoardValue currentTurn) {
    return (playerColor == currentTurn) ? 1 : -1;
}

int AIPlayer::EvaluateBoard(const Board& board) {
    int score = 0;
    for (int x = 0; x < Board::MATRIX_SIZE; ++x) {
        for (int y = 0; y < Board::MATRIX_SIZE; ++y) {
            if (board.boardState[x][y] == playerColor)
                score += tileWeights[x][y];
            else if (board.boardState[x][y] != BoardValue::EMPTY)
                score -= tileWeights[x][y];
        }
    }
    return score;
}

int AIPlayer::Minimax(const Board& board, int depth, int alpha, int beta, BoardValue currentTurn) {
    std::vector<std::pair<int, int>> validMoves = GetValidMoves(board, currentTurn);
    BoardValue nextTurn = (currentTurn == BoardValue::BLACK) ? BoardValue::WHITE : BoardValue::BLACK;

    if (depth == 0) {
        return GetTurnMultiplier(currentTurn) * EvaluateBoard(board);
    }

    if (validMoves.empty()) {
        std::vector<std::pair<int, int>> nextValidMoves = GetValidMoves(board, nextTurn);

        if (nextValidMoves.empty()) {
            return GetTurnMultiplier(currentTurn) * EvaluateBoard(board);
        }

        return -Minimax(board, depth - 1, -beta, -alpha, nextTurn);
    }

    int bestScore = -INF;
    for (const auto& move : validMoves) {
        Board newBoard = board.duplicateBoard();
        FlipPieces(newBoard, move.first, move.second);
        int score = -Minimax(newBoard, depth - 1, -beta, -alpha, nextTurn);

        bestScore = std::max(bestScore, score);
        alpha = std::max(alpha, score);

        if (alpha >= beta) { break; }
    }

    return bestScore;
}

std::pair<int, int> AIPlayer::ChooseMinimax(const Board& board, std::vector<std::pair<int, int>> validMoves) {
    std::pair<int, int> bestMove = { -1, -1 };
    int bestScore = -INF;
    BoardValue nextTurn = (playerColor == BoardValue::BLACK) ? BoardValue::WHITE : BoardValue::BLACK;

    for (const auto& move : validMoves) {
        Board newBoard = board.duplicateBoard();

        FlipPieces(newBoard, move.first, move.second);

        int score = -Minimax(newBoard, minimaxLevelDepth, -INF, INF, nextTurn);

        if (score > bestScore) {
            bestScore = score;
            bestMove = { move.first, move.second };
        }
    }

    return bestMove;
}

void AIPlayer::move(Board& board, HWND hWnd, Difficulty difficulty) {
    auto validMoves = GetValidMoves(board);

    std::pair<int, int> move;
    switch (difficulty) {
        case Difficulty::HEURISTIC:
            move = ChooseHeuristic(board, validMoves);
            break;
        case Difficulty::MINIMAX:
            move = ChooseMinimax(board, validMoves);
            break;
    }

    if (move.first == -1 && move.second == -1) {
        MessageBox(nullptr, L"Something Broke in AI code", L"Error", MB_OK);
    }

    FlipPieces(board, move.first, move.second);
    InvalidateRect(hWnd, NULL, true);
    UpdateWindow(hWnd);
}
