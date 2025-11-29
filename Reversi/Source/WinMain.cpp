#include "WinMain.h"

// Main window constructor
MainWindow::MainWindow(HINSTANCE hInst)
    : hInstance(hInst), hWnd(nullptr), hIcon(nullptr) {

    LoadString(hInstance, IDS_WINDOWNAME, windowTitle, MAX_NAME_STRING);
    LoadString(hInstance, IDS_WINDOWCLASS, windowClass, MAX_NAME_STRING);
    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
}

MainWindow::~MainWindow() {}

// Initialize the window, attack console, register class and create window
bool MainWindow::Init(int nCmdShow) {
    FILE* fp;
    AllocConsole();
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    hConsole = GetConsoleWindow();

    if (!consoleVisible) { ShowWindow(hConsole, SW_HIDE); }

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MainWindow::StaticWndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.hIcon = hIcon;
    wcex.hIconSm = hIcon;
    wcex.lpszClassName = windowClass;

    if (!RegisterClassEx(&wcex)) {
        MessageBox(nullptr, L"Window Registration Failed!", L"Error", MB_OK);
        return false;
    }

    hWnd = CreateWindow(
        windowClass, windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr,
        hInstance, this);

    if (!hWnd) {
        MessageBox(nullptr, L"Failed to create window", L"Error", MB_OK);
        return false;
    }

    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));
    SetMenu(hWnd, hMenu);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return true;
}

int MainWindow::Run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void MainWindow::RunTimerThread() {
    while (aiThinking) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (aiThinking) {
            aiTimeRemaining--;

            if (aiTimeRemaining <= 0) {
                aiTimeRemaining = 0;
                forceAIStop = true;
            }

            InvalidateRect(hWnd, NULL, TRUE);
        }
    }
}

// Trigger AI move if it's AI's turn in single player mode
void MainWindow::TriggerAIMove() {
    if (singlePlayer && !player1Turn && player2.HasValidMove(board) && !aiMoveInProgress) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Start AI computation
        aiMoveInProgress = true;
        aiThinking = true;
        aiTimeRemaining = AI_TIME_LIMIT;
        forceAIStop = false;

        timerThread = std::thread(&MainWindow::RunTimerThread, this);

        // Launch AI in background - THIS DOESN'T BLOCK!
        aiFuture = std::async(std::launch::async, [this]() {
            return player2.move(board, hWnd, difficulty, &forceAIStop);
            });

        // Set a timer to check when AI is done
        SetTimer(hWnd, 2, 50, NULL);
    }
}

void MainWindow::CheckAIComplete() {
    if (!aiMoveInProgress) return;

    // Check if AI is done (non-blocking check)
    if (aiFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        // AI finished! Get the result
        MoveResult aiResult = aiFuture.get();

        // Stop timer
        aiThinking = false;
        aiMoveInProgress = false;

        if (timerThread.joinable()) {
            timerThread.join();
        }

        KillTimer(hWnd, 2); // Stop checking
        aiTimeRemaining = AI_TIME_LIMIT;

        // Process AI move
        if (aiResult.valid) {
            MoveRecord aiRecord;
            aiRecord.row = aiResult.row;
            aiRecord.col = aiResult.col;
            aiRecord.color = player2.playerColor;
            aiRecord.flipped = aiResult.flipped;
            moveHistory.push_back(aiRecord);

            player1Turn = true;
        }

        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
    }
}

void MainWindow::Undo() {
    if (moveHistory.empty()) {
        MessageBox(hWnd, L"Nothing to undo!", L"Undo", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int movesToUndo = singlePlayer ? 2 : 1;
    for (int i = 0; i < movesToUndo && !moveHistory.empty(); i++) {
        MoveRecord& lastMove = moveHistory.back();
        board.UndoMove(lastMove.row, lastMove.col, lastMove.color, lastMove.flipped);
        moveHistory.pop_back();
        player1Turn = !player1Turn;
    }

    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);

    TriggerAIMove();
}

// Handle game over scenario
void MainWindow::GameOver() {
    if (gameOverShown) return;
    gameOverShown = true;

    int whiteCount = 0, blackCount = 0;
    board.CountPieces(whiteCount, blackCount);
    std::wstring result;
    if (whiteCount > blackCount) {
        result = L"White wins!";
    } else if (blackCount > whiteCount) {
        result = L"Black wins!";
    } else {
        result = L"It's a tie!";
    }

    wchar_t buffer[256];
    swprintf(buffer, 256, L"White: %d  Black: %d\n%s\n\nPlay again?", whiteCount, blackCount, result.c_str());

    int response = MessageBox(hWnd, buffer, L"Game Over", MB_YESNO | MB_ICONQUESTION);
    if (response == IDYES) {
        board.reset();
        moveHistory.clear();
        player1Turn = (player1.playerColor == BoardValue::BLACK);
        gameOverShown = false;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);

        TriggerAIMove();
    } else {
        PostQuitMessage(0);
    }
}

LRESULT CALLBACK MainWindow::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->hWnd = hWnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->WindowProc(hWnd, message, wParam, lParam);
    } else {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

static std::wstring DifficultyToWString(Difficulty d) {
    switch (d) {
    case Difficulty::HEURISTIC: return L"Heuristic";
    case Difficulty::MINIMAX:   return L"Minimax";
    default:                    return L"Unknown";
    }
}

void MainWindow::ToggleConsole() {
    if (hConsole) {
        consoleVisible = !consoleVisible;
        ShowWindow(hConsole, consoleVisible ? SW_SHOW : SW_HIDE);
    }
}

LRESULT MainWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            board.LoadResources((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
                MAKEINTRESOURCE(IDB_BOARD), MAKEINTRESOURCE(IDB_BOARDNUMBERED));

            break;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = WINDOW_WIDTH;
            mmi->ptMinTrackSize.y = WINDOW_HEIGHT;
            return 0;
        }

        case WM_TIMER:
            if (wParam == 1 && aiThinking) {
                aiTimeRemaining--;
                if (aiTimeRemaining < 0) {
                    aiTimeRemaining = 0;
                }
                InvalidateRect(hWnd, NULL, TRUE);
            } else if (wParam == 2) {
                CheckAIComplete();
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_VIEW_CONSOLE:
                    ToggleConsole();
                    break;

                case ID_PLAYER_SINGLEPLAYER:
                    singlePlayer = true;
                    board.reset();
                    moveHistory.clear();
                    gameOverShown = false;
                    break;
                case ID_PLAYER_MULTIPLAYER:
                    singlePlayer = false;
                    board.reset();
                    moveHistory.clear();
                    gameOverShown = false;
                    break;
                case ID_PLAYER_COLOR_BLACK:
                    // Player chooses to play as Black
                    player1.playerColor = BoardValue::BLACK;
                    player2.playerColor = BoardValue::WHITE;
                    board.reset();
                    moveHistory.clear();
                    gameOverShown = false;
                    player1Turn = (player1.playerColor == BoardValue::BLACK);
                    break;
                case ID_PLAYER_COLOR_WHITE:
                    // Player chooses to play as White
                    player1.playerColor = BoardValue::WHITE;
                    player2.playerColor = BoardValue::BLACK;
                    board.reset();
                    moveHistory.clear();
                    gameOverShown = false;
                    player1Turn = (player1.playerColor == BoardValue::BLACK);

                    TriggerAIMove();
                    break;

                case ID_BOARD_DEFAULT:
                    board.useNumberedBackground = false;
                    break;
                case ID_BOARD_NUMBERED:
                    board.useNumberedBackground = true;
                    break;

                case ID_DIFFICULTY_HEURISTIC:
                    difficulty = Difficulty::HEURISTIC;
                    board.reset();
                    moveHistory.clear();
                    gameOverShown = false;
                    break;
                case ID_DIFFICULTY_MINIMAX:
                    difficulty = Difficulty::MINIMAX;
                    board.reset();
                    moveHistory.clear();
                    gameOverShown = false;
                    break;

                case ID_UNDO:
                    Undo();
                    break;
                //case ID_DIFFICULTY_LEARNING:
                //    difficulty = Difficulty::LEARNING;
                //    MessageBox(nullptr, L"Using learning", L"Error", MB_OK);
                //    board.reset();
                //    break;
            }

            InvalidateRect(hWnd, NULL, true);
            UpdateWindow(hWnd);
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);

            board.SetDifficultyText(DifficultyToWString(difficulty));

            if (singlePlayer) {
                int timeLeft = aiTimeRemaining.load();
                std::wstring timerText = L"AI Time: " + std::to_wstring(timeLeft) + L"s";

                COLORREF timerColor;
                if (timeLeft <= 5) {
                    timerColor = RGB(255, 0, 0);
                }
                else if (timeLeft <= 10) {
                    timerColor = RGB(255, 165, 0);
                }
                else {
                    timerColor = RGB(255, 255, 255);
                }

                board.SetTimerText(timerText, timerColor);
                board.SetShowSingleplayerText(true);
            }
            else {
                board.SetShowSingleplayerText(false);
            }

            board.Draw(hdc, rect);

            if (!player1.HasValidMove(board) && !player2.HasValidMove(board)) {
                GameOver();
            }

            return 1;
        }

        case WM_LBUTTONDOWN:
        {
            RECT rect;
            GetClientRect(hWnd, &rect);

            if (singlePlayer) {
                if (!player1.HasValidMove(board)) {
                    player1Turn = false;
                    if (player2.HasValidMove(board)) {
                        TriggerAIMove();
                    } else {
                        InvalidateRect(hWnd, NULL, TRUE);
                    }
                    break;
                }

                auto moveResult = player1.MouseHandler(board, hWnd, rect, LOWORD(lParam), HIWORD(lParam));

                if (moveResult.valid) {
                    // Record player move
                    MoveRecord record;
                    record.row = moveResult.row;
                    record.col = moveResult.col;
                    record.color = player1.playerColor;
                    record.flipped = moveResult.flipped;

                    moveHistory.push_back(record);

                    player1Turn = false;

                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateWindow(hWnd);

                    if (player2.HasValidMove(board)) {
                        TriggerAIMove();
                    }
                    else if (player1.HasValidMove(board)) {
                        player1Turn = true;
                    }
                }

                InvalidateRect(hWnd, NULL, TRUE);
                UpdateWindow(hWnd);
            }
            else {
                Player& currentPlayer = player1Turn ? player1 : player2;
                auto moveResult = currentPlayer.MouseHandler(board, hWnd, rect, LOWORD(lParam), HIWORD(lParam));

                if (moveResult.valid) {
                    MoveRecord record;
                    record.row = moveResult.row;
                    record.col = moveResult.col;
                    record.color = currentPlayer.playerColor;
                    record.flipped = moveResult.flipped;
                    moveHistory.push_back(record);

                    player1Turn = !player1Turn;

                    Player& newPlayer = player1Turn ? player1 : player2;
                    if (!newPlayer.HasValidMove(board)) {
                        player1Turn = !player1Turn;
                    }

                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateWindow(hWnd);
                }
                else if (!currentPlayer.HasValidMove(board)) {
                    player1Turn = !player1Turn;
                }
            }
            break;
        }

        case WM_DESTROY:
            forceAIStop = true;
            aiThinking = false;

            // Wait for AI thread if running
            if (aiMoveInProgress && aiFuture.valid()) {
                aiFuture.wait();
            }

            if (timerThread.joinable()) {
                timerThread.join();
            }

            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    MainWindow app(hInstance);
    if (!app.Init(nCmdShow)) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }
    return app.Run();
}