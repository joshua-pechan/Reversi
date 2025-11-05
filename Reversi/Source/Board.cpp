#include "pch.h"
#include "Player.h"
#include "Board.h"

std::array<std::array<std::array<uint64_t, 2>, 8>, 8> Board::zobristTable;

void Board::InitZobrist() {
    std::mt19937_64 rng(0xF00D);
    std::uniform_int_distribution<uint64_t> dist;

    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            zobristTable[x][y][0] = dist(rng);
            zobristTable[x][y][1] = dist(rng);
        }
    }
}

void Board::UpdateHash(int row, int col, BoardValue oldValue, BoardValue newValue) {
    if (oldValue == BoardValue::BLACK) currentHash ^= zobristTable[row][col][0];
    else if (oldValue == BoardValue::WHITE) currentHash ^= zobristTable[row][col][1];

    if (newValue == BoardValue::BLACK) currentHash ^= zobristTable[row][col][0];
    else if (newValue == BoardValue::WHITE) currentHash ^= zobristTable[row][col][1];
}

Board::Board() : background(nullptr), cellWidth(), cellHeight() {
    reset();
}

Board::~Board() {
    if (background) { DeleteObject(background); }
}

Board Board::duplicateBoard() const {
    Board newBoard;

    for (int row = 0; row < Board::MATRIX_SIZE; row++) {
        for (int col = 0; col < Board::MATRIX_SIZE; col++) {
            newBoard.boardState[row][col] = boardState[row][col];
        }
    }

    newBoard.currentHash = currentHash;
    return newBoard;
}

BoardValue Board::OpponentColor(BoardValue c) {
    return (c == BoardValue::BLACK) ? BoardValue::WHITE : BoardValue::BLACK;
}

static std::vector<std::pair<int, int>> CollectFlipsInDirection(
    BoardValue boardState[Board::MATRIX_SIZE][Board::MATRIX_SIZE],
    int row, int col, int drow, int dcol, BoardValue color)
{
    std::vector<std::pair<int, int>> flips;
    int r = row + drow;
    int c = col + dcol;

    while (r >= 0 && r < Board::MATRIX_SIZE && c >= 0 && c < Board::MATRIX_SIZE) {
        BoardValue v = boardState[r][c];
        if (v == BoardValue::EMPTY) {
            flips.clear();
            return flips;
        }
        if (v == color) {
            return flips;
        }
        flips.emplace_back(r, c);
        r += drow;
        c += dcol;
    }

    flips.clear();
    return flips;
}

std::vector<std::pair<int, int>> Board::ApplyMove(int row, int col, BoardValue color) {
    std::vector<std::pair<int, int>> totalFlipped;

    if (row < 0 || row >= MATRIX_SIZE || col < 0 || col >= MATRIX_SIZE) return totalFlipped;
    if (boardState[row][col] != BoardValue::EMPTY) return totalFlipped;

    totalFlipped.reserve(64);

    for (const auto& d : directions) {
        auto flips = CollectFlipsInDirection(boardState, row, col, d[0], d[1], color);
        if (!flips.empty()) {
            totalFlipped.insert(totalFlipped.end(), flips.begin(), flips.end());
        }
    }

    if (totalFlipped.empty()) {
        return totalFlipped;
    }

    BoardValue oldValue = boardState[row][col];
    boardState[row][col] = color;
    UpdateHash(row, col, oldValue, color);

    for (const auto& p : totalFlipped) {
        BoardValue oldColor = boardState[p.first][p.second];
        boardState[p.first][p.second] = color;
        UpdateHash(p.first, p.second, oldColor, color);
    }

    return totalFlipped;
}

std::vector<std::pair<int, int>> Board::ApplyMove(HWND hWnd, int row, int col, BoardValue color)
{
    const std::vector<std::pair<int, int>> totalFlipped = ApplyMove(row, col, color);
    InvalidateRect(hWnd, NULL, true);
    UpdateWindow(hWnd);
    return totalFlipped;
}

void Board::UndoMove(int row, int col, BoardValue color, const std::vector<std::pair<int, int>>& flipped) {
    if (row >= 0 && row < MATRIX_SIZE && col >= 0 && col < MATRIX_SIZE) {
        boardState[row][col] = BoardValue::EMPTY;
        UpdateHash(row, col, color, BoardValue::EMPTY);
    }

    BoardValue opp = OpponentColor(color);
    for (const auto& p : flipped) {
        if (p.first >= 0 && p.first < MATRIX_SIZE && p.second >= 0 && p.second < MATRIX_SIZE) {
            boardState[p.first][p.second] = opp;
            UpdateHash(p.first, p.second, color, opp);
        }
    }
}

void Board::reset() {
    std::fill(&boardState[0][0], &boardState[0][0] + MATRIX_SIZE * MATRIX_SIZE, BoardValue::EMPTY);
    boardState[3][3] = BoardValue::WHITE;
    boardState[4][4] = BoardValue::WHITE;
    boardState[3][4] = BoardValue::BLACK;
    boardState[4][3] = BoardValue::BLACK;
}

void Board::LoadResources(HINSTANCE hInstance, LPCWSTR backgroundID, LPCWSTR numberedBackgroundID) {
    background = (HBITMAP)LoadImage(hInstance, backgroundID, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    backgroundNumbered = (HBITMAP)LoadImage(hInstance, numberedBackgroundID, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
}

void Board::Draw(HDC hdc, RECT clientRect) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmapMem = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);

    HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmapMem);

    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdcMem, &clientRect, hBrush);
    DeleteObject(hBrush);

    HDC hdcMemOriginal = CreateCompatibleDC(hdc);
    BITMAP bitmap;

    if (useNumberedBackground) {
        SelectObject(hdcMemOriginal, backgroundNumbered);

        GetObject(backgroundNumbered, sizeof(BITMAP), &bitmap);
    } else {
        SelectObject(hdcMemOriginal, background);

        GetObject(background, sizeof(BITMAP), &bitmap);
    }

    StretchBlt(hdcMem, 0, 0, clientRect.right, clientRect.bottom, hdcMemOriginal, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);

    cellWidth = (clientRect.right - borderX * 2) / MATRIX_SIZE;
    cellHeight = (clientRect.bottom - borderY * 2) / MATRIX_SIZE;

    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            switch (boardState[i][j]) {
                case BoardValue::WHITE:
                    Board::DrawPiece(hdcMem, white, i, j);
                    break;
                case BoardValue::BLACK:
                    Board::DrawPiece(hdcMem, black, i, j);
                    break;
            }
        }
    }

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmapMem);
    DeleteDC(hdcMemOriginal);
    DeleteDC(hdcMem);
}

void Board::DrawPiece(HDC hdcMem, COLORREF color, int indexX, int indexY) {
    int x = borderX + indexX * cellWidth;
    int y = borderY + indexY * cellHeight;

    float paddingRatio = 0.1f;
    int paddingX = static_cast<int>(cellWidth * paddingRatio);
    int paddingY = static_cast<int>(cellHeight * paddingRatio);

    int offsetX = 1;
    int offsetY = 1;

    HBRUSH hBrush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdcMem, hBrush);

    HPEN hPen = (HPEN)SelectObject(hdcMem, GetStockObject(NULL_PEN));

    Ellipse(hdcMem, x + paddingX + offsetX, y + paddingY + offsetY, x + cellWidth - paddingX + offsetX, y + cellHeight - paddingY + offsetY);

    SelectObject(hdcMem, oldBrush);
    SelectObject(hdcMem, hPen);
    DeleteObject(hBrush);
}

void Board::CountPieces(int& whiteCount, int& blackCount) const {
    whiteCount = 0;
    blackCount = 0;
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if (boardState[i][j] == BoardValue::WHITE) ++whiteCount;
            else if (boardState[i][j] == BoardValue::BLACK) ++blackCount;
        }
    }
}