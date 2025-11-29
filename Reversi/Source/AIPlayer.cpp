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

// Counts flips along a single direction with weights
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

// Simulates placing a piece and counts weighted flips for all directions
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

// ===================================================================
// STABLE DISC COUNTER
// ===================================================================
int AIPlayer::CountStableDiscs(const Board& board, BoardValue color) {
    bool stable[8][8] = { false };
    int count = 0;

    // === PHASE 1: Mark corners as stable ===
    auto markStable = [&](int x, int y) -> bool {
        if (board.boardState[x][y] == color && !stable[x][y]) {
            stable[x][y] = true;
            count++;
            return true;
        }
        return false;
        };

    bool corners[4] = {
        markStable(0, 0), markStable(0, 7),
        markStable(7, 0), markStable(7, 7)
    };

    // === PHASE 2: Propagate from corners along edges ===
    auto propagateEdge = [&](int startX, int startY, int dx, int dy) {
        int x = startX + dx, y = startY + dy;
        while (x >= 0 && x < 8 && y >= 0 && y < 8) {
            if (board.boardState[x][y] != color) break;
            if (!stable[x][y]) {
                stable[x][y] = true;
                count++;
            }
            else {
                break;
            }
            x += dx; y += dy;
        }
        };

    if (corners[0]) { // Top-left
        propagateEdge(0, 0, 0, 1);  // Right
        propagateEdge(0, 0, 1, 0);  // Down
    }
    if (corners[1]) { // Top-right
        propagateEdge(0, 7, 0, -1); // Left
        propagateEdge(0, 7, 1, 0);  // Down
    }
    if (corners[2]) { // Bottom-left
        propagateEdge(7, 0, -1, 0); // Up
        propagateEdge(7, 0, 0, 1);  // Right
    }
    if (corners[3]) { // Bottom-right
        propagateEdge(7, 7, -1, 0); // Up
        propagateEdge(7, 7, 0, -1); // Left
    }

    // === PHASE 3: Check for completely filled edges ===
    auto checkFullEdge = [&](int fixed, bool isRow) {
        bool allSame = true;
        for (int i = 0; i < 8; i++) {
            int x = isRow ? fixed : i;
            int y = isRow ? i : fixed;
            if (board.boardState[x][y] != color) {
                allSame = false;
                break;
            }
        }
        if (allSame) {
            for (int i = 0; i < 8; i++) {
                int x = isRow ? fixed : i;
                int y = isRow ? i : fixed;
                if (!stable[x][y]) {
                    stable[x][y] = true;
                    count++;
                }
            }
        }
        };

    checkFullEdge(0, true);  // Top
    checkFullEdge(7, true);  // Bottom
    checkFullEdge(0, false); // Left
    checkFullEdge(7, false); // Right

    // === PHASE 4: Multi-pass interior propagation ===
    const int directions[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };

    bool changed = true;
    int maxPasses = 15;

    while (changed && maxPasses-- > 0) {
        changed = false;

        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                if (stable[x][y] || board.boardState[x][y] != color)
                    continue;

                bool isStable = true;

                // Check all 4 axes
                for (const auto& dir : directions) {
                    int dx = dir[0], dy = dir[1];
                    bool posLocked = false, negLocked = false;

                    // Check positive direction
                    int nx = x + dx, ny = y + dy;
                    while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                        if (board.boardState[nx][ny] != color) {
                            posLocked = true; // Hit edge or opponent
                            break;
                        }
                        if (stable[nx][ny]) {
                            posLocked = true; // Hit stable disc
                            break;
                        }
                        nx += dx; ny += dy;
                    }
                    if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) {
                        posLocked = true; // Hit board edge
                    }

                    // Check negative direction
                    nx = x - dx; ny = y - dy;
                    while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                        if (board.boardState[nx][ny] != color) {
                            negLocked = true;
                            break;
                        }
                        if (stable[nx][ny]) {
                            negLocked = true;
                            break;
                        }
                        nx -= dx; ny -= dy;
                    }
                    if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) {
                        negLocked = true;
                    }

                    // Axis must be locked in both directions
                    if (!posLocked || !negLocked) {
                        isStable = false;
                        break;
                    }
                }

                if (isStable) {
                    stable[x][y] = true;
                    count++;
                    changed = true;
                }
            }
        }
    }

    return count;
}

// ===================================================================
// MOBILITY COUNTER
// ===================================================================
int AIPlayer::CountMoves(const Board& board, BoardValue color) {
    int moveCount = 0;
    BoardValue opponent = board.OpponentColor(color);

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (board.boardState[x][y] != BoardValue::EMPTY) continue;

            // Check all 8 directions
            for (const auto& dir : directions) {
                int dx = dir[0], dy = dir[1];
                int nx = x + dx, ny = y + dy;

                // Must have at least one opponent disc in this direction
                if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8 ||
                    board.boardState[nx][ny] != opponent) {
                    continue;
                }

                // Walk in direction, looking for our color
                nx += dx; ny += dy;
                while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                    if (board.boardState[nx][ny] == BoardValue::EMPTY)
                        break;
                    if (board.boardState[nx][ny] == color) {
                        moveCount++;
                        goto nextSquare; // Valid move found
                    }
                    nx += dx; ny += dy;
                }
            }
        nextSquare:;
        }
    }

    return moveCount;
}

// ===================================================================
// CORNER EVALUATION
// ===================================================================
int AIPlayer::EvaluateCorners(const Board& board, BoardValue myColor, BoardValue oppColor, int pieceCount) {
    int score = 0;

    struct CornerRegion {
        int cx, cy;  // Corner
        int xx, yy;  // X-square (diagonal)
        int c1x, c1y, c2x, c2y;  // C-squares (adjacent)
    };

    CornerRegion regions[4] = {
        {0, 0, 1, 1, 0, 1, 1, 0},  // Top-left
        {0, 7, 1, 6, 0, 6, 1, 7},  // Top-right
        {7, 0, 6, 1, 6, 0, 7, 1},  // Bottom-left
        {7, 7, 6, 6, 6, 7, 7, 6}   // Bottom-right
    };

    // Dynamic penalty based on game phase
    int xPenalty = pieceCount < 25 ? 80 : (pieceCount < 40 ? 50 : 25);
    int cPenalty = pieceCount < 25 ? 40 : (pieceCount < 40 ? 25 : 15);

    for (const auto& r : regions) {
        BoardValue corner = board.boardState[r.cx][r.cy];
        BoardValue xSquare = board.boardState[r.xx][r.yy];
        BoardValue c1 = board.boardState[r.c1x][r.c1y];
        BoardValue c2 = board.boardState[r.c2x][r.c2y];

        // Corner value - massive throughout game
        if (corner == myColor) {
            score += 100;
            // Adjacent squares now stable and valuable
            if (xSquare == myColor) score += 15;
            if (c1 == myColor) score += 15;
            if (c2 == myColor) score += 15;
        }
        else if (corner == oppColor) {
            score -= 100;
            if (xSquare == oppColor) score -= 15;
            if (c1 == oppColor) score -= 15;
            if (c2 == oppColor) score -= 15;
        }
        else {
            // Corner empty - dangerous zone

            // X-square penalty
            if (xSquare == myColor) {
                score -= xPenalty;
            }
            else if (xSquare == oppColor) {
                score += xPenalty;
            }

            // C-square penalty (reduced if wedge forming)
            bool wedgeForming = (c1 == myColor && c2 == myColor);
            int effectiveCPenalty = wedgeForming ? cPenalty / 2 : cPenalty;

            if (c1 == myColor) score -= effectiveCPenalty;
            else if (c1 == oppColor) score += effectiveCPenalty;

            if (c2 == myColor) score -= effectiveCPenalty;
            else if (c2 == oppColor) score += effectiveCPenalty;
        }
    }

    return score;
}

// ===================================================================
// PARITY EVALUATION
// ===================================================================
int AIPlayer::EvaluateParity(const Board& board, int emptySquares, int pieceCount) {
    if (pieceCount < 48 || pieceCount >= 56) return 0;

    // Count empty squares in different regions
    int corners = 0, edges = 0, interior = 0;

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (board.boardState[x][y] != BoardValue::EMPTY) continue;

            bool isCorner = (x == 0 || x == 7) && (y == 0 || y == 7);
            bool isEdge = (x == 0 || x == 7 || y == 0 || y == 7);

            if (isCorner) corners++;
            else if (isEdge) edges++;
            else interior++;
        }
    }

    int score = 0;

    // Odd parity in interior is advantageous (last move advantage)
    if (interior % 2 == 1) score += 15;
    else score -= 15;

    // Edge parity matters too
    if (edges % 2 == 1) score += 10;
    else score -= 10;

    return score;
}

// ===================================================================
// MAIN EVALUATION FUNCTION
// ===================================================================
int AIPlayer::EvaluateBoard(const Board& board) {
    BoardValue opponent = board.OpponentColor(playerColor);
    int score = 0;

    // Count pieces and determine phase
    int myDiscs = 0, oppDiscs = 0, emptySquares = 0;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            BoardValue cell = board.boardState[x][y];
            if (cell == playerColor) myDiscs++;
            else if (cell == opponent) oppDiscs++;
            else emptySquares++;
        }
    }

    int pieceCount = myDiscs + oppDiscs;

    // === ENDGAME: Pure disc count ===
    if (emptySquares <= 10) {
        return (myDiscs - oppDiscs) * 1000;
    }

    // === CORNER EVALUATION ===
    score += EvaluateCorners(board, playerColor, opponent, pieceCount);

    // === PIECE-SQUARE TABLES (early/mid game) ===
    if (pieceCount < 40) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                if (board.boardState[x][y] == playerColor) {
                    score += tileWeights[x][y];
                }
                else if (board.boardState[x][y] == opponent) {
                    score -= tileWeights[x][y];
                }
            }
        }
    }

    // === MOBILITY ===
    int myMoves = CountMoves(board, playerColor);
    int oppMoves = CountMoves(board, opponent);

    int mobilityWeight;
    if (pieceCount < 20)      mobilityWeight = 25;
    else if (pieceCount < 35) mobilityWeight = 18;
    else if (pieceCount < 50) mobilityWeight = 10;
    else                      mobilityWeight = 5;

    score += (myMoves - oppMoves) * mobilityWeight;

    // No-move bonus/penalty
    if (oppMoves == 0 && myMoves > 0) score += 150;
    else if (myMoves == 0 && oppMoves > 0) score -= 150;

    // === POTENTIAL MOBILITY (frontier analysis) ===
    if (pieceCount < 48) {
        int myPotential = 0, oppPotential = 0;

        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                if (board.boardState[x][y] != BoardValue::EMPTY) continue;

                bool adjToMe = false, adjToOpp = false;
                for (const auto& dir : directions) {
                    int nx = x + dir[0], ny = y + dir[1];
                    if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                        if (board.boardState[nx][ny] == playerColor)
                            adjToMe = true;
                        else if (board.boardState[nx][ny] == opponent)
                            adjToOpp = true;
                    }
                }

                if (adjToOpp) myPotential++;
                if (adjToMe) oppPotential++;
            }
        }

        int potentialWeight = pieceCount < 30 ? 5 : 3;
        score += (myPotential - oppPotential) * potentialWeight;
    }

    // === STABLE DISCS ===
    if (pieceCount >= 20) {
        int myStable = CountStableDiscs(board, playerColor);
        int oppStable = CountStableDiscs(board, opponent);

        int stableWeight;
        if (pieceCount < 30)      stableWeight = 10;
        else if (pieceCount < 40) stableWeight = 15;
        else if (pieceCount < 50) stableWeight = 22;
        else                      stableWeight = 30;

        score += (myStable - oppStable) * stableWeight;
    }

    // === PARITY ===
    if (pieceCount >= 48) {
        score += EvaluateParity(board, emptySquares, pieceCount);
    }

    // === FRONTIER DISCS (minimize exposure) ===
    if (pieceCount < 45) {
        int myFrontier = 0, oppFrontier = 0;

        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                BoardValue cell = board.boardState[x][y];
                if (cell == BoardValue::EMPTY) continue;

                bool isFrontier = false;
                for (const auto& dir : directions) {
                    int nx = x + dir[0], ny = y + dir[1];
                    if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8 &&
                        board.boardState[nx][ny] == BoardValue::EMPTY) {
                        isFrontier = true;
                        break;
                    }
                }

                if (isFrontier) {
                    if (cell == playerColor) myFrontier++;
                    else oppFrontier++;
                }
            }
        }

        int frontierWeight = pieceCount < 25 ? 7 : 5;
        score -= (myFrontier - oppFrontier) * frontierWeight;
    }

    // === EDGE CONTROL ===
    if (pieceCount >= 25 && pieceCount < 50) {
        int myEdges = 0, oppEdges = 0;

        for (int i = 1; i < 7; i++) {
            // Exclude corners (already counted)
            if (board.boardState[0][i] == playerColor) myEdges++;
            else if (board.boardState[0][i] == opponent) oppEdges++;

            if (board.boardState[7][i] == playerColor) myEdges++;
            else if (board.boardState[7][i] == opponent) oppEdges++;

            if (board.boardState[i][0] == playerColor) myEdges++;
            else if (board.boardState[i][0] == opponent) oppEdges++;

            if (board.boardState[i][7] == playerColor) myEdges++;
            else if (board.boardState[i][7] == opponent) oppEdges++;
        }

        score += (myEdges - oppEdges) * 12;
    }

    // === DISC COUNT (late game focus) ===
    if (pieceCount >= 45 && emptySquares > 10) {
        score += (myDiscs - oppDiscs) * 8;
    }

    return score;
}

// sort moves by heuristic value
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

    // Early termination checks BEFORE TT lookup
    std::vector<std::pair<int, int>> validMoves = GetValidMoves(board, currentTurn);
    BoardValue nextTurn = board.OpponentColor(currentTurn);

    if (depth == 0) {
        return GetTurnMultiplier(currentTurn) * EvaluateBoard(board);
    }

    if (validMoves.empty()) {
        std::vector<std::pair<int, int>> nextValidMoves = GetValidMoves(board, nextTurn);
        if (nextValidMoves.empty()) {
            return GetTurnMultiplier(currentTurn) * EvaluateBoard(board);
        }
        return -Minimax(board, depth - 1, -beta, -alpha, nextTurn, forceStop);
    }

    // TT lookup
    size_t hash = board.currentHash;
    if (currentTurn == BoardValue::WHITE) {
        hash ^= 0x9E3779B97F4A7C15ULL;
    }

    int alphaOrig = alpha;
    if (transpositionTable.count(hash)) {
        TTEntry& entry = transpositionTable[hash];
        if (entry.depth >= depth) {
            if (entry.flag == 0) {
                return entry.score;
            }
            if (entry.flag == 1) {
                alpha = std::max(alpha, entry.score);
            }
            if (entry.flag == 2) {
                beta = std::min(beta, entry.score);
            }
            if (alpha >= beta) {
                return entry.score;
            }
        }
    }

    int bestScore = -INF;

    // Move ordering
    std::vector<std::pair<int, int>> orderedMoves = OrderMoves(board, validMoves, currentTurn);

    // Minimax loop
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

        if (alpha >= beta) {
            break;
        }
    }

    // Store in TT with CURRENT hash
    TTEntry ttEntry;
    ttEntry.depth = depth;
    ttEntry.score = bestScore;

    if (bestScore <= alphaOrig) {
        ttEntry.flag = 2;
    }
    else if (bestScore >= beta) {
        ttEntry.flag = 1;
    }
    else {
        ttEntry.flag = 0;
    }

    transpositionTable[hash] = ttEntry;

    return bestScore;
}

// evaluate all moves in parallel then choose the best
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
