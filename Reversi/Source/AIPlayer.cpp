#include "pch.h"
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
std::pair<int, int> AIPlayer::ChooseHeuristic(const Board& board, std::vector<std::pair<int, int>> validMoves, std::atomic<bool>* forceStop) {
    std::pair<int, int> bestMove = { -1, -1 };
    int bestScore = -INF;

    for (const std::pair<int, int>& move : validMoves) {
        if (forceStop && forceStop->load()) {
            return (bestMove.first != -1) ? bestMove : validMoves[0];
        }

        int score = WeightedSimulateFlips(board, move.first, move.second, playerColor);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }

    return bestMove;
}

// Minimax (negamax)
int AIPlayer::GetTurnMultiplier(BoardValue currentTurn) {
    return (playerColor == currentTurn) ? 1 : -1;
}
int AIPlayer::CountStableDiscs(const Board& board, BoardValue color) {
    bool stable[8][8] = { false };
    int count = 0;

    // Corners are always stable
    auto checkCorner = [&](int x, int y) {
        if (board.boardState[x][y] == color) {
            stable[x][y] = true;
            count++;
            return true;
        }
        return false;
        };

    checkCorner(0, 0);
    checkCorner(0, 7);
    checkCorner(7, 0);
    checkCorner(7, 7);

    // Propagate stability from corners along edges
    // Top edge from top-left corner
    if (stable[0][0]) {
        for (int y = 1; y < 8; y++) {
            if (board.boardState[0][y] == color) {
                stable[0][y] = true;
                count++;
            }
            else break;
        }
    }
    // Similar for other edges...

    return count;
}

int AIPlayer::EvaluateBoard(const Board& board) {
    BoardValue opponent = board.OpponentColor(playerColor);
    int score = 0;

    // Count pieces to determine game phase
    int pieceCount = 0;
    int myDiscs = 0, oppDiscs = 0;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (board.boardState[x][y] == playerColor) {
                myDiscs++;
                pieceCount++;
            }
            else if (board.boardState[x][y] == opponent) {
                oppDiscs++;
                pieceCount++;
            }
        }
    }

    // ENDGAME: Only disc count matters
    if (pieceCount >= 54) {
        return (myDiscs - oppDiscs) * 100;
    }

    // === OPENING/MIDGAME ===

    // 1. CORNERS - Most important (can never be flipped)
    int cornerScore = 0;
    int corners[4][2] = { {0,0}, {0,7}, {7,0}, {7,7} };
    for (auto& c : corners) {
        if (board.boardState[c[0]][c[1]] == playerColor)
            cornerScore += 100;
        else if (board.boardState[c[0]][c[1]] == opponent)
            cornerScore -= 100;
    }
    score += cornerScore;

    // 2. AVOID X-SQUARES and C-SQUARES (gift corners to opponent!)
    auto penalizeNearCorner = [&](int cx, int cy, const std::vector<std::pair<int, int>>& dangerSquares) {
        if (board.boardState[cx][cy] == BoardValue::EMPTY) {
            for (auto& sq : dangerSquares) {
                if (board.boardState[sq.first][sq.second] == playerColor)
                    score -= 50;
                else if (board.boardState[sq.first][sq.second] == opponent)
                    score += 50;
            }
        }
        };

    penalizeNearCorner(0, 0, { {0,1}, {1,0}, {1,1} });
    penalizeNearCorner(0, 7, { {0,6}, {1,7}, {1,6} });
    penalizeNearCorner(7, 0, { {6,0}, {7,1}, {6,1} });
    penalizeNearCorner(7, 7, { {6,7}, {7,6}, {6,6} });

    // 3. MOBILITY - Having more moves is crucial
    int myMoves = GetValidMoves(board, playerColor).size();
    int oppMoves = GetValidMoves(board, opponent).size();

    // Early game: mobility is VERY important
    if (pieceCount < 40) {
        score += (myMoves - oppMoves) * 10;
    }
    else {
        score += (myMoves - oppMoves) * 5;
    }

    // 4. EDGE STABILITY
    int edgeScore = 0;

    auto isEdgeStable = [&](int x, int y) -> bool {
        if (x == 0) {
            if ((y == 0) || (y == 7)) return true;
            if (board.boardState[0][0] == board.boardState[x][y]) return true;
            if (board.boardState[0][7] == board.boardState[x][y]) return true;
        }
        if (x == 7) {
            if ((y == 0) || (y == 7)) return true;
            if (board.boardState[7][0] == board.boardState[x][y]) return true;
            if (board.boardState[7][7] == board.boardState[x][y]) return true;
        }
        if (y == 0) {
            if (board.boardState[0][0] == board.boardState[x][y]) return true;
            if (board.boardState[7][0] == board.boardState[x][y]) return true;
        }
        if (y == 7) {
            if (board.boardState[0][7] == board.boardState[x][y]) return true;
            if (board.boardState[7][7] == board.boardState[x][y]) return true;
        }
        return false;
        };

    for (int i = 2; i < 6; i++) {
        if (board.boardState[0][i] == playerColor && isEdgeStable(0, i)) edgeScore += 10;
        else if (board.boardState[0][i] == opponent && isEdgeStable(0, i)) edgeScore -= 10;

        if (board.boardState[7][i] == playerColor && isEdgeStable(7, i)) edgeScore += 10;
        else if (board.boardState[7][i] == opponent && isEdgeStable(7, i)) edgeScore -= 10;

        if (board.boardState[i][0] == playerColor && isEdgeStable(i, 0)) edgeScore += 10;
        else if (board.boardState[i][0] == opponent && isEdgeStable(i, 0)) edgeScore -= 10;

        if (board.boardState[i][7] == playerColor && isEdgeStable(i, 7)) edgeScore += 10;
        else if (board.boardState[i][7] == opponent && isEdgeStable(i, 7)) edgeScore -= 10;
    }
    score += edgeScore;

    // 5. PARITY (who gets the last move)
    if (pieceCount >= 45 && pieceCount < 54) {
        int emptyCount = 64 - pieceCount;
        if (emptyCount % 2 == 0) {
            score += 5;
        }
        else {
            score -= 5;
        }
    }

    // 6. FRONTIER DISCS
    int myFrontier = 0, oppFrontier = 0;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (board.boardState[x][y] == BoardValue::EMPTY) continue;

            bool isFrontier = false;
            for (auto& dir : directions) {
                int nx = x + dir[0], ny = y + dir[1];
                if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8 &&
                    board.boardState[nx][ny] == BoardValue::EMPTY) {
                    isFrontier = true;
                    break;
                }
            }

            if (isFrontier) {
                if (board.boardState[x][y] == playerColor) myFrontier++;
                else oppFrontier++;
            }
        }
    }
    // Fewer frontier discs is better
    if (pieceCount < 40) {
        score -= (myFrontier - oppFrontier) * 3;
    }

    return score;
}

std::vector<std::pair<int, int>> AIPlayer::OrderMoves(const Board& board, const std::vector<std::pair<int, int>>& moves, BoardValue color) {
    std::vector<std::pair<int, int>> ordered = moves;
    std::sort(ordered.begin(), ordered.end(), [&](const auto& a, const auto& b) {
        return WeightedSimulateFlips(board, a.first, a.second, color) >
            WeightedSimulateFlips(board, b.first, b.second, color);
    });

    return ordered;
}

int AIPlayer::Minimax(Board& board, int depth, int alpha, int beta, BoardValue currentTurn, std::atomic<bool>* forceStop) {
    if (forceStop && forceStop->load()) {
        return 0;
    }

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
    BoardValue nextTurn = board.OpponentColor(currentTurn);

    if (depth == 0) return GetTurnMultiplier(currentTurn) * EvaluateBoard(board);

    if (validMoves.empty()) {
        std::vector<std::pair<int, int>> nextValidMoves = GetValidMoves(board, nextTurn);
        if (nextValidMoves.empty()) {
            return GetTurnMultiplier(currentTurn) * EvaluateBoard(board);
        }
        return -Minimax(board, depth - 1, -beta, -alpha, nextTurn, forceStop);
    }

    int bestScore = -INF;

    // --- Move ordering ---
    std::vector<std::pair<int, int>> orderedMoves = OrderMoves(board, validMoves, currentTurn);

    // --- Minimax loop ---
    for (const auto& move : orderedMoves) {
        if (forceStop && forceStop->load()) {
            return bestScore > -INF ? bestScore : 0;
        }

        auto flipped = board.ApplyMove(move.first, move.second, currentTurn);
        if (flipped.empty()) continue;

        int score = -Minimax(board, depth - 1, -beta, -alpha, nextTurn, forceStop);

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

std::pair<int, int> AIPlayer::ChooseMinimax(Board& board, const std::vector<std::pair<int, int>>& validMoves, std::atomic<bool>* forceStop) {
    std::pair<int, int> bestMove = { -1, -1 };
    int bestScore = -INF;
    BoardValue nextTurn = board.OpponentColor(playerColor);

    // Pre-sort moves by heuristic for better ordering
    auto orderedMoves = OrderMoves(board, validMoves, playerColor);

    // Launch all move evaluations in parallel
    std::vector<std::future<int>> futures;
    futures.reserve(orderedMoves.size());

    for (const auto& move : orderedMoves) {
        if (forceStop && forceStop->load()) {
            break;
        }

        futures.push_back(std::async(std::launch::async, [this, &board, move, nextTurn, forceStop]() {
            Board boardCopy = board.duplicateBoard();
            auto flipped = boardCopy.ApplyMove(move.first, move.second, playerColor);
            if (flipped.empty()) return -INF;

            return -Minimax(boardCopy, DEPTH, -INF, INF, nextTurn, forceStop);
        }));
    }

    // Collect results and find best move
    for (size_t i = 0; i < futures.size(); ++i) {
        int score = futures[i].get();
        if (score > bestScore) {
            bestScore = score;
            bestMove = orderedMoves[i];
        }
    }

    return bestMove;
}

MoveResult AIPlayer::move(Board& board, HWND hWnd, Difficulty difficulty, std::atomic<bool>* forceStop) {
    MoveResult result = { false, -1, -1, {} };

    auto validMoves = GetValidMoves(board);
    std::pair<int, int> move;

    switch (difficulty) {
        case Difficulty::HEURISTIC:
            move = ChooseHeuristic(board, validMoves, forceStop);
            break;
        case Difficulty::MINIMAX:
            move = ChooseMinimax(board, validMoves, forceStop);
            break;
    }

    if (move.first == -1 && move.second == -1) {
        if (!validMoves.empty()) {
            move = validMoves[0];
        } else {
            MessageBox(nullptr, L"Something Broke in AI code", L"Error", MB_OK);
            return result;
        }
    }

    result.valid = true;
    result.row = move.first;
    result.col = move.second;
    result.flipped = board.ApplyMove(hWnd, move.first, move.second, playerColor);

    //if (consoleVisible) {
    //    std::cout << "(" << Board::MATRIX_SIZE - result.col - 1 << ", " << result.row << ")" << std::endl;
    //}

    return result;
}
