#include "pch.h"
#include "Player.h"
#include "Board.h"

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

    return newBoard;
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
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if (boardState[i][j] == BoardValue::WHITE) {
                whiteCount++;
            } else if (boardState[i][j] == BoardValue::BLACK) {
                blackCount++;
            }
        }
    }
}