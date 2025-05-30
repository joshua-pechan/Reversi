#pragma once
#include "pch.h"
#include "Board.h"
#include "Player.h"
#include "AIPlayer.h"

class MainWindow {
public:
    MainWindow(HINSTANCE hInst);
    ~MainWindow();

    bool Init(int nCmdShow);

    int Run();

    void GameOver();

private:
    virtual LRESULT WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    HINSTANCE hInstance;
    HWND      hWnd;
    WCHAR     windowClass[MAX_NAME_STRING];
    WCHAR     windowTitle[MAX_NAME_STRING];
    HICON     hIcon;
    Board     board;

    bool resetInProgress = false;
    std::chrono::steady_clock::time_point lastResetTime;

    bool player1Turn = true;
    bool singlePlayer = true;
    Difficulty difficulty = Difficulty::HEURISTIC;
};

inline Player player1(BoardValue::BLACK);
inline AIPlayer player2(BoardValue::WHITE);
