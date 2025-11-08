#pragma once
#include "pch.h"
#include "Board.h"
#include "Player.h"
#include "AIPlayer.h"

// Main application window, coordinates UI, user input, and game flow
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

    HWND hConsole = nullptr;
    void ToggleConsole();

    // AI timer state
    std::atomic<int> aiTimeRemaining{ AI_TIME_LIMIT };
    std::atomic<bool> aiThinking{ false };
    std::atomic<bool> forceAIStop{ false };
    std::thread timerThread;

    bool resetInProgress = false;
    std::chrono::steady_clock::time_point lastResetTime;

    bool player1Turn = true;
    bool singlePlayer = true;
    Difficulty difficulty = Difficulty::MINIMAX;

    bool gameOverShown = false;

    struct MoveRecord {
        int row;
        int col;
        BoardValue color;
        std::vector<std::pair<int, int>> flipped;
    };

    std::vector<MoveRecord> moveHistory;

    void RunTimerThread();

    void TriggerAIMove();
    void Undo();
};

inline Player player1(BoardValue::BLACK);
inline AIPlayer player2(BoardValue::WHITE);