#pragma once
#include "pch.h"
#include "Player.h"

// Represents the game board and handles game logic and rendering
class Board {
public:
    static constexpr int MATRIX_SIZE = 8;
    static constexpr int borderX = 10;
    static constexpr int borderY = 8;

    bool useNumberedBackground = false;

    static std::array<std::array<std::array<uint64_t, 2>, 8>, 8> zobristTable;

    uint64_t currentHash = 0;
private:
    HBITMAP background;
    HBITMAP backgroundNumbered;
    COLORREF white = RGB(255, 255, 255);;
    COLORREF black = RGB(0, 0, 0);;

    // Overlay text
    std::wstring difficultyText;
    std::wstring timerText;
    COLORREF timerColor = RGB(255, 255, 255);
    bool setShowSingleplayerText = false;

    int cellWidth;
    int cellHeight;

    void InitZobrist();

    void UpdateHash(int x, int y, BoardValue oldValue, BoardValue newValue);
public:
    Board();
    ~Board();

    Board duplicateBoard() const;
    static BoardValue OpponentColor(BoardValue c);

    // For drawing text overlays
    void SetDifficultyText(const std::wstring& text) { difficultyText = text; }
    void SetTimerText(const std::wstring& text, COLORREF color) {
        timerText = text;
        timerColor = color;
    }
    void SetShowSingleplayerText(bool show) { setShowSingleplayerText = show; }

	// Game operations
    std::vector<std::pair<int, int>> ApplyMove(int row, int col, BoardValue color);
    std::vector<std::pair<int, int>> ApplyMove(HWND hWnd, int row, int col, BoardValue color);
    void UndoMove(int row, int col, BoardValue color, const std::vector<std::pair<int, int>>& flipped);
    void reset();
    void LoadResources(HINSTANCE hInstance, LPCWSTR backgroundID, LPCWSTR numberedBackgroundID);
    
    // Rendering
    void Draw(HDC hdc, RECT clientRect);
    void DrawPiece(Graphics* graphics, Color color, int indexX, int indexY);
    
    // Utilities
    void CountPieces(int& whiteCount, int& blackCount) const;
    void PrintBoard() const;
    
    BoardValue boardState[MATRIX_SIZE][MATRIX_SIZE];
};
