#include "WinMain.h"

MainWindow::MainWindow(HINSTANCE hInst)
    : hInstance(hInst), hWnd(nullptr), hIcon(nullptr) {

    LoadString(hInstance, IDS_WINDOWNAME, windowTitle, MAX_NAME_STRING);
    LoadString(hInstance, IDS_WINDOWCLASS, windowClass, MAX_NAME_STRING);
    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
}

MainWindow::~MainWindow() {}

bool MainWindow::Init(int nCmdShow) {
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

void MainWindow::GameOver() {
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
        player1Turn = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        InvalidateRect(hWnd, NULL, TRUE);
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

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_PLAYER_SINGLEPLAYER:
                    singlePlayer = true;
                    board.reset();
                    break;
                case ID_PLAYER_MULTIPLAYER:
                    singlePlayer = false;
                    board.reset();
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
                    break;
                case ID_DIFFICULTY_MINIMAX:
                    difficulty = Difficulty::MINIMAX;
                    board.reset();
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
        {
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hWnd, &rect);
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
                // Player 1 move
                if (player1.HasValidMove(board)) {
                    if (player1.MouseHandler(board, hWnd, rect, LOWORD(lParam), HIWORD(lParam))) {
                        InvalidateRect(hWnd, NULL, true);
                        UpdateWindow(hWnd);

                        std::this_thread::sleep_for(std::chrono::milliseconds(500));

                        // AI move
                        if (player2.HasValidMove(board)) {
                            player2.move(board, hWnd, difficulty);
                            InvalidateRect(hWnd, NULL, true);
                            UpdateWindow(hWnd);
                        }
                    }
                }
                else {
                    if (player2.HasValidMove(board)) {
                        // AI move
                        player2.move(board, hWnd, difficulty);
                        InvalidateRect(hWnd, NULL, true);
                        UpdateWindow(hWnd);
                    }
                }
            } else {
                Player& currentPlayer = player1Turn ? player1 : player2;

                bool switchTurn = !currentPlayer.HasValidMove(board)
                    || currentPlayer.MouseHandler(board, hWnd, rect, LOWORD(lParam), HIWORD(lParam));
            
                if (switchTurn) player1Turn = !player1Turn;
            }

            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MainWindow app(hInstance);
    if (!app.Init(nCmdShow)) {
        return 0;
    }
    return app.Run();
}