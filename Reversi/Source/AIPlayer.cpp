#include "pch.h"
#include "Player.h"
#include "AIPlayer.h"
#include "Board.h"

uint64_t AIPlayer::HashBoard(const Board& board) const {
    uint64_t hash = 0;
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            if (board.boardState[x][y] == BoardValue::BLACK)
                hash ^= Board::zobristTable[x][y][0];
            else if (board.boardState[x][y] == BoardValue::WHITE)
                hash ^= Board::zobristTable[x][y][1];
        }
    }
    return hash;
}

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

int AIPlayer::WeightedCountFlips(const Board& board, int x, int y, int dx, int dy, BoardValue color) {
    int flips = 0;
    int nx = x + dx;
    int ny = y + dy;
    bool foundOpponent = false;

    while (nx >= 0 && nx < Board::MATRIX_SIZE && ny >= 0 && ny < Board::MATRIX_SIZE) {
        BoardValue currentValue = board.boardState[nx][ny];

        if (currentValue == BoardValue::EMPTY) { return 0; }

        if (currentValue == color) {
            return (foundOpponent ? flips : 0);
        }

        foundOpponent = true;
        flips += tileWeights[nx][ny];
        nx += dx;
        ny += dy;
    }

    return 0;
}

int AIPlayer::WeightedSimulateFlips(const Board& board, int x, int y, BoardValue color) {
    if (board.boardState[x][y] != BoardValue::EMPTY) { return 0; }

    int totalFlips = 0;

    for (const auto& dir : directions) {
        totalFlips += WeightedCountFlips(board, x, y, dir[0], dir[1], color);
    }
    
    totalFlips += tileWeights[x][y];

    return totalFlips;
}

// board heuristic
std::pair<int, int> AIPlayer::ChooseHeuristic(const Board& board, std::vector<std::pair<int, int>> validMoves) {
    std::pair<int, int> bestMove = { -1, -1 };
    int bestScore = -INF;

    for (const std::pair<int, int>& move : validMoves) {
        int score = WeightedSimulateFlips(board, move.first, move.second, playerColor);
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
    int pieceCount = 0;

    // Count total pieces to determine game phase
    for (int x = 0; x < Board::MATRIX_SIZE; ++x) {
        for (int y = 0; y < Board::MATRIX_SIZE; ++y) {
            if (board.boardState[x][y] != BoardValue::EMPTY) {
                pieceCount++;
            }
        }
    }

    // Early/Mid game: focus on position and mobility
    if (pieceCount < 50) {
        // Positional score
        for (int x = 0; x < Board::MATRIX_SIZE; ++x) {
            for (int y = 0; y < Board::MATRIX_SIZE; ++y) {
                if (board.boardState[x][y] == playerColor)
                    score += tileWeights[x][y];
                else if (board.boardState[x][y] != BoardValue::EMPTY)
                    score -= tileWeights[x][y];
            }
        }

        // Mobility (very important in mid-game)
        BoardValue opponent = (playerColor == BoardValue::BLACK) ? BoardValue::WHITE : BoardValue::BLACK;
        int myMoves = GetValidMoves(board, playerColor).size();
        int oppMoves = GetValidMoves(board, opponent).size();
        score += (myMoves - oppMoves) * 10;  // High weight on mobility

    }
    else {
        // End game: focus on disc count
        int myDiscs = 0;
        int oppDiscs = 0;

        for (int x = 0; x < Board::MATRIX_SIZE; ++x) {
            for (int y = 0; y < Board::MATRIX_SIZE; ++y) {
                if (board.boardState[x][y] == playerColor)
                    myDiscs++;
                else if (board.boardState[x][y] != BoardValue::EMPTY)
                    oppDiscs++;
            }
        }

        score = (myDiscs - oppDiscs) * 10;
    }

    return score;
}

int AIPlayer::Minimax(Board& board, int depth, int alpha, int beta, BoardValue currentTurn) {
    //std::cout << depth << std::endl;
    size_t hash = board.currentHash;
    int alphaOrig = alpha;

    // Check TT
    if (transpositionTable.count(hash)) {
        TTEntry& entry = transpositionTable[hash];
        if (entry.depth >= depth) {
            if (entry.flag == 0) return entry.score;
            if (entry.flag == 1) alpha = std::max(alpha, entry.score);
            if (entry.flag == 2) beta = std::min(beta, entry.score);
            if (alpha >= beta) return entry.score;
        }
    }

    std::vector<std::pair<int, int>> validMoves = GetValidMoves(board, currentTurn);
    BoardValue nextTurn = (currentTurn == BoardValue::BLACK) ? BoardValue::WHITE : BoardValue::BLACK;

    if (depth == 0) return GetTurnMultiplier(currentTurn) * EvaluateBoard(board);

    if (validMoves.empty()) {
        std::vector<std::pair<int, int>> nextValidMoves = GetValidMoves(board, nextTurn);
        if (nextValidMoves.empty()) {
            return GetTurnMultiplier(currentTurn) * EvaluateBoard(board); // game over
        }
        return -Minimax(board, depth - 1, -beta, -alpha, nextTurn); // skip turn
    }

    int bestScore = -INF;

    // --- Move ordering ---
    std::vector<std::pair<std::pair<int, int>, int>> scored;
    scored.reserve(validMoves.size());
    for (const auto& m : validMoves)
        scored.push_back({ m, WeightedSimulateFlips(board, m.first, m.second, currentTurn) });
    std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) { return a.second > b.second; });

    // --- Minimax loop ---
    for (const auto& entry : scored) {
        const auto& move = entry.first;

        auto flipped = board.ApplyMove(move.first, move.second, currentTurn);
        if (flipped.empty()) continue;

        int score = -Minimax(board, depth - 1, -beta, -alpha, nextTurn);

        board.UndoMove(move.first, move.second, currentTurn, flipped);

        bestScore = std::max(bestScore, score);
        alpha = std::max(alpha, score);
        if (alpha >= beta) break;
    }

    // --- Store in TT ---
    TTEntry ttEntry;
    ttEntry.depth = depth;
    ttEntry.score = bestScore;

    if (bestScore <= alphaOrig) ttEntry.flag = 2;
    else if (bestScore >= beta) ttEntry.flag = 1;
    else ttEntry.flag = 0;

    transpositionTable[hash] = ttEntry;

    return bestScore;
}

std::pair<int, int> AIPlayer::ChooseMinimax(Board& board, const std::vector<std::pair<int, int>>& validMoves) {
    std::pair<int, int> bestMove = { -1, -1 };
    int bestScore = -INF;
    BoardValue nextTurn = (playerColor == BoardValue::BLACK) ? BoardValue::WHITE : BoardValue::BLACK;
    
    // Pre-sort moves by heuristic for better ordering
    std::vector<std::pair<std::pair<int, int>, int>> scored;
    scored.reserve(validMoves.size());
    for (const auto& m : validMoves) {
        int s = WeightedSimulateFlips(board, m.first, m.second, playerColor);
        scored.push_back({ m, s });
    }
    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
        });

    const unsigned int numThreads = std::min(std::thread::hardware_concurrency(),
        static_cast<unsigned int>(scored.size()));
    std::vector<std::future<std::pair<std::pair<int, int>, int>>> futures;

    // Launch all move evaluations at full depth
    for (const auto& entry : scored) {
        const auto move = entry.first;

        futures.push_back(std::async(std::launch::async, [this, &board, move, nextTurn]() {
            Board boardCopy = board.duplicateBoard();

            auto flipped = boardCopy.ApplyMove(move.first, move.second, playerColor);
            if (flipped.empty()) return std::make_pair(move, -INF);

            // Search at full depth specified by minimaxLevelDepth
            int score = -Minimax(boardCopy, minimaxLevelDepth, -INF, INF, nextTurn);

            return std::make_pair(move, score);
            }));
    }

    // Collect all results
    for (auto& f : futures) {
        auto [moveFinished, scoreFinished] = f.get();
        if (scoreFinished > bestScore) {
            bestScore = scoreFinished;
            bestMove = moveFinished;
        }
    }

    return bestMove;
}

MoveResult AIPlayer::move(Board& board, HWND hWnd, Difficulty difficulty) {
    MoveResult result = { false, -1, -1, {} };

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
        return result;
    }

    result.valid = true;
    result.row = move.first;
    result.col = move.second;
    result.flipped = board.ApplyMove(hWnd, move.first, move.second, playerColor);

    return result;
}
