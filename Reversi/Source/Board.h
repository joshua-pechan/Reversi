#pragma once

class Board {
public:
    static constexpr int MATRIX_SIZE = 8;
    static constexpr int borderX = 10;
    static constexpr int borderY = 8;

    bool useNumberedBackground = false;
private:
    HBITMAP background;
    HBITMAP backgroundNumbered;
    COLORREF white = RGB(255, 255, 255);;
    COLORREF black = RGB(0, 0, 0);;

    int cellWidth;
    int cellHeight;
public:
    Board();
    ~Board();
    Board duplicateBoard() const;

    void reset();
    void LoadResources(HINSTANCE hInstance, LPCWSTR backgroundID, LPCWSTR numberedBackgroundID);
    void Draw(HDC hdc, RECT clientRect);
    void DrawPiece(HDC hdcMem, COLORREF color, int indexX, int indexY);
    void CountPieces(int& whiteCount, int& blackCount) const;
    BoardValue boardState[MATRIX_SIZE][MATRIX_SIZE];
};
