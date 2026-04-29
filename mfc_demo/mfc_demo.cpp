// mfc_demo.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "mfc_demo.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>

#define MAX_LOADSTRING 100

enum ChessPieceType {
    EMPTY = 0,
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
};

enum ChessColor {
    WHITE = 0,
    BLACK,
    NO_COLOR
};

struct ChessPiece {
    ChessPieceType type;
    ChessColor color;
    bool hasMoved;

    ChessPiece() : type(EMPTY), color(NO_COLOR), hasMoved(false) {}
    ChessPiece(ChessPieceType t, ChessColor c) : type(t), color(c), hasMoved(false) {}
};

struct Position {
    int row;
    int col;

    Position() : row(0), col(0) {}
    Position(int r, int c) : row(r), col(c) {}

    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
};

enum GameMode {
    MODE_TWO_PLAYER,
    MODE_AI
};

enum GameState {
    GAME_NOT_STARTED,
    GAME_PLAYING,
    GAME_CHECK_WHITE,
    GAME_CHECK_BLACK,
    GAME_WHITE_WINS,
    GAME_BLACK_WINS,
    GAME_CHECKMATE_WHITE,
    GAME_CHECKMATE_BLACK,
    GAME_STALEMATE
};

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

const int BOARD_SIZE = 8;
const int CELL_SIZE = 60;
const int BOARD_OFFSET = 30;

ChessPiece g_board[BOARD_SIZE][BOARD_SIZE];
ChessColor g_currentTurn;
GameMode g_gameMode;
GameState g_gameState;
Position g_selectedPos;
bool g_hasSelection;
std::vector<Position> g_validMoves;
HBITMAP g_hBitmapBuffer;
HDC g_hdcBuffer;
bool g_bBufferCreated;
Position g_lastMovedFrom;
Position g_lastMovedTo;
bool g_hasLastMove;
Position g_whiteKingPos;
Position g_blackKingPos;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ConfigDlgProc(HWND, UINT, WPARAM, LPARAM);

void InitGame();
void ResetBoard();
void DrawBoard(HDC hdc);
void DrawPiece(HDC hdc, int row, int col, ChessPiece piece);
void DrawValidMoves(HDC hdc);
bool IsValidPosition(int row, int col);
ChessPiece GetPiece(int row, int col);
void SetPiece(int row, int col, ChessPiece piece);
void GetValidMoves(int row, int col, std::vector<Position>& moves);
void GetValidMoves_Internal(int row, int col, std::vector<Position>& moves, bool checkKingSafety);
void GetPawnMoves(int row, int col, std::vector<Position>& moves);
void GetRookMoves(int row, int col, std::vector<Position>& moves);
void GetKnightMoves(int row, int col, std::vector<Position>& moves);
void GetBishopMoves(int row, int col, std::vector<Position>& moves);
void GetQueenMoves(int row, int col, std::vector<Position>& moves);
void GetKingMoves(int row, int col, std::vector<Position>& moves);
bool IsPathClear(int fromRow, int fromCol, int toRow, int toCol);
bool IsKingInCheck(ChessColor color);
bool IsCheckmate(ChessColor color);
bool IsStalemate(ChessColor color);
bool IsMoveValid(int fromRow, int fromCol, int toRow, int toCol);
bool MakeMove(int fromRow, int fromCol, int toRow, int toCol);
void UpdateGameState();
void CreateBuffer(HWND hWnd);
void DestroyBuffer();
bool AITurn();
int EvaluatePosition();
int EvaluatePiece(ChessPieceType type);
bool CanPromote(int row, ChessPieceType type, ChessColor color);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    srand((unsigned int)time(NULL));

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MFCDEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MFCDEMO));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MFCDEMO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MFCDEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   int windowWidth = BOARD_OFFSET * 2 + CELL_SIZE * BOARD_SIZE + 40;
   int windowHeight = BOARD_OFFSET * 2 + CELL_SIZE * BOARD_SIZE + 80;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   g_bBufferCreated = false;
   g_gameMode = MODE_TWO_PLAYER;
   g_gameState = GAME_NOT_STARTED;
   g_hasSelection = false;
   g_hasLastMove = false;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void InitGame()
{
    ResetBoard();
    g_currentTurn = WHITE;
    g_gameState = GAME_PLAYING;
    g_hasSelection = false;
    g_hasLastMove = false;
    UpdateGameState();
}

void ResetBoard()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            g_board[i][j] = ChessPiece();
        }
    }

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        g_board[1][i] = ChessPiece(PAWN, BLACK);
        g_board[6][i] = ChessPiece(PAWN, WHITE);
    }

    ChessPieceType backRow[] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        g_board[0][i] = ChessPiece(backRow[i], BLACK);
        g_board[7][i] = ChessPiece(backRow[i], WHITE);
    }

    g_whiteKingPos = Position(7, 4);
    g_blackKingPos = Position(0, 4);
}

bool IsValidPosition(int row, int col)
{
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

ChessPiece GetPiece(int row, int col)
{
    if (IsValidPosition(row, col))
        return g_board[row][col];
    return ChessPiece();
}

void SetPiece(int row, int col, ChessPiece piece)
{
    if (IsValidPosition(row, col))
    {
        g_board[row][col] = piece;
        if (piece.type == KING)
        {
            if (piece.color == WHITE)
                g_whiteKingPos = Position(row, col);
            else if (piece.color == BLACK)
                g_blackKingPos = Position(row, col);
        }
    }
}

void GetValidMoves(int row, int col, std::vector<Position>& moves)
{
    moves.clear();
    ChessPiece piece = GetPiece(row, col);
    if (piece.type == EMPTY)
        return;

    GetValidMoves_Internal(row, col, moves, false);
}

void GetValidMoves_Internal(int row, int col, std::vector<Position>& moves, bool checkKingSafety)
{
    ChessPiece piece = GetPiece(row, col);
    if (piece.type == EMPTY)
        return;

    std::vector<Position> tempMoves;

    switch (piece.type)
    {
    case PAWN:
        GetPawnMoves(row, col, tempMoves);
        break;
    case ROOK:
        GetRookMoves(row, col, tempMoves);
        break;
    case KNIGHT:
        GetKnightMoves(row, col, tempMoves);
        break;
    case BISHOP:
        GetBishopMoves(row, col, tempMoves);
        break;
    case QUEEN:
        GetQueenMoves(row, col, tempMoves);
        break;
    case KING:
        GetKingMoves(row, col, tempMoves);
        break;
    }

    if (checkKingSafety)
    {
        for (size_t i = 0; i < tempMoves.size(); i++)
        {
            Position to = tempMoves[i];
            if (IsMoveValid(row, col, to.row, to.col))
            {
                moves.push_back(to);
            }
        }
    }
    else
    {
        moves = tempMoves;
    }
}

void GetPawnMoves(int row, int col, std::vector<Position>& moves)
{
    ChessPiece piece = GetPiece(row, col);
    if (piece.type != PAWN)
        return;

    int direction = (piece.color == WHITE) ? -1 : 1;
    int startRow = (piece.color == WHITE) ? 6 : 1;

    int newRow = row + direction;
    if (IsValidPosition(newRow, col))
    {
        ChessPiece ahead = GetPiece(newRow, col);
        if (ahead.type == EMPTY)
        {
            moves.push_back(Position(newRow, col));

            if (row == startRow)
            {
                int doubleRow = row + 2 * direction;
                ChessPiece doubleAhead = GetPiece(doubleRow, col);
                if (doubleAhead.type == EMPTY)
                {
                    moves.push_back(Position(doubleRow, col));
                }
            }
        }
    }

    int captureCols[] = {col - 1, col + 1};
    for (int i = 0; i < 2; i++)
    {
        int newCol = captureCols[i];
        if (IsValidPosition(newRow, newCol))
        {
            ChessPiece target = GetPiece(newRow, newCol);
            if (target.type != EMPTY && target.color != piece.color)
            {
                moves.push_back(Position(newRow, newCol));
            }
        }
    }
}

void GetRookMoves(int row, int col, std::vector<Position>& moves)
{
    ChessPiece piece = GetPiece(row, col);
    int directions[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

    for (int d = 0; d < 4; d++)
    {
        int newRow = row + directions[d][0];
        int newCol = col + directions[d][1];

        while (IsValidPosition(newRow, newCol))
        {
            ChessPiece target = GetPiece(newRow, newCol);
            if (target.type == EMPTY)
            {
                moves.push_back(Position(newRow, newCol));
            }
            else
            {
                if (target.color != piece.color)
                {
                    moves.push_back(Position(newRow, newCol));
                }
                break;
            }
            newRow += directions[d][0];
            newCol += directions[d][1];
        }
    }
}

void GetKnightMoves(int row, int col, std::vector<Position>& moves)
{
    ChessPiece piece = GetPiece(row, col);
    int offsets[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1},
                          {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};

    for (int i = 0; i < 8; i++)
    {
        int newRow = row + offsets[i][0];
        int newCol = col + offsets[i][1];

        if (IsValidPosition(newRow, newCol))
        {
            ChessPiece target = GetPiece(newRow, newCol);
            if (target.type == EMPTY || target.color != piece.color)
            {
                moves.push_back(Position(newRow, newCol));
            }
        }
    }
}

void GetBishopMoves(int row, int col, std::vector<Position>& moves)
{
    ChessPiece piece = GetPiece(row, col);
    int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int d = 0; d < 4; d++)
    {
        int newRow = row + directions[d][0];
        int newCol = col + directions[d][1];

        while (IsValidPosition(newRow, newCol))
        {
            ChessPiece target = GetPiece(newRow, newCol);
            if (target.type == EMPTY)
            {
                moves.push_back(Position(newRow, newCol));
            }
            else
            {
                if (target.color != piece.color)
                {
                    moves.push_back(Position(newRow, newCol));
                }
                break;
            }
            newRow += directions[d][0];
            newCol += directions[d][1];
        }
    }
}

void GetQueenMoves(int row, int col, std::vector<Position>& moves)
{
    GetRookMoves(row, col, moves);
    GetBishopMoves(row, col, moves);
}

void GetKingMoves(int row, int col, std::vector<Position>& moves)
{
    ChessPiece piece = GetPiece(row, col);
    int directions[8][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0},
                             {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int d = 0; d < 8; d++)
    {
        int newRow = row + directions[d][0];
        int newCol = col + directions[d][1];

        if (IsValidPosition(newRow, newCol))
        {
            ChessPiece target = GetPiece(newRow, newCol);
            if (target.type == EMPTY || target.color != piece.color)
            {
                moves.push_back(Position(newRow, newCol));
            }
        }
    }

    if (!piece.hasMoved)
    {
        int baseRow = (piece.color == WHITE) ? 7 : 0;

        ChessPiece leftRook = GetPiece(baseRow, 0);
        if (leftRook.type == ROOK && leftRook.color == piece.color && !leftRook.hasMoved)
        {
            if (GetPiece(baseRow, 1).type == EMPTY &&
                GetPiece(baseRow, 2).type == EMPTY &&
                GetPiece(baseRow, 3).type == EMPTY)
            {
                moves.push_back(Position(baseRow, 2));
            }
        }

        ChessPiece rightRook = GetPiece(baseRow, 7);
        if (rightRook.type == ROOK && rightRook.color == piece.color && !rightRook.hasMoved)
        {
            if (GetPiece(baseRow, 5).type == EMPTY &&
                GetPiece(baseRow, 6).type == EMPTY)
            {
                moves.push_back(Position(baseRow, 6));
            }
        }
    }
}

bool IsPathClear(int fromRow, int fromCol, int toRow, int toCol)
{
    if (fromRow != toRow && fromCol != toCol && abs(fromRow - toRow) != abs(fromCol - toCol))
        return true;

    int dRow = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
    int dCol = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;

    int currRow = fromRow + dRow;
    int currCol = fromCol + dCol;

    while (currRow != toRow || currCol != toCol)
    {
        if (GetPiece(currRow, currCol).type != EMPTY)
            return false;
        currRow += dRow;
        currCol += dCol;
    }

    return true;
}

bool IsKingInCheck(ChessColor color)
{
    Position kingPos;
    if (color == WHITE)
        kingPos = g_whiteKingPos;
    else
        kingPos = g_blackKingPos;

    ChessColor enemyColor = (color == WHITE) ? BLACK : WHITE;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            ChessPiece piece = GetPiece(row, col);
            if (piece.color == enemyColor && piece.type != EMPTY)
            {
                std::vector<Position> moves;
                GetValidMoves_Internal(row, col, moves, false);

                for (size_t i = 0; i < moves.size(); i++)
                {
                    if (moves[i] == kingPos)
                    {
                        if (piece.type == KNIGHT || piece.type == KING)
                        {
                            return true;
                        }
                        else
                        {
                            if (IsPathClear(row, col, kingPos.row, kingPos.col))
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool IsMoveValid(int fromRow, int fromCol, int toRow, int toCol)
{
    ChessPiece fromPiece = GetPiece(fromRow, fromCol);
    if (fromPiece.type == EMPTY)
        return false;

    ChessPiece toPiece = GetPiece(toRow, toCol);
    if (toPiece.type != EMPTY && toPiece.color == fromPiece.color)
        return false;

    if (fromPiece.type == PAWN)
    {
        int direction = (fromPiece.color == WHITE) ? -1 : 1;
        if (toRow == fromRow + direction)
        {
            if (toCol == fromCol - 1 || toCol == fromCol + 1)
            {
                if (toPiece.type == EMPTY)
                    return false;
            }
            else if (toCol == fromCol)
            {
                if (toPiece.type != EMPTY)
                    return false;
            }
        }
        else if (toRow == fromRow + 2 * direction)
        {
            if (toCol != fromCol)
                return false;
            int startRow = (fromPiece.color == WHITE) ? 6 : 1;
            if (fromRow != startRow)
                return false;
        }
    }

    if (fromPiece.type == KING && abs(toCol - fromCol) == 2)
    {
        if (IsKingInCheck(fromPiece.color))
            return false;

        int baseRow = (fromPiece.color == WHITE) ? 7 : 0;
        if (fromRow != baseRow)
            return false;

        ChessColor enemyColor = (fromPiece.color == WHITE) ? BLACK : WHITE;
        int midCol = (fromCol + toCol) / 2;

        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                ChessPiece enemy = GetPiece(row, col);
                if (enemy.color == enemyColor && enemy.type != EMPTY)
                {
                    std::vector<Position> moves;
                    GetValidMoves_Internal(row, col, moves, false);
                    for (size_t i = 0; i < moves.size(); i++)
                    {
                        if (moves[i].row == baseRow && moves[i].col == midCol)
                        {
                            if (enemy.type != KNIGHT && enemy.type != KING)
                            {
                                if (IsPathClear(row, col, baseRow, midCol))
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool MakeMove(int fromRow, int fromCol, int toRow, int toCol)
{
    ChessPiece piece = GetPiece(fromRow, fromCol);
    ChessPiece captured = GetPiece(toRow, toCol);
    bool capturedKing = (captured.type == KING);

    if (piece.type == KING && abs(toCol - fromCol) == 2)
    {
        int baseRow = (piece.color == WHITE) ? 7 : 0;
        if (toCol == 2)
        {
            ChessPiece rook = GetPiece(baseRow, 0);
            rook.hasMoved = true;
            SetPiece(baseRow, 3, rook);
            SetPiece(baseRow, 0, ChessPiece());
        }
        else if (toCol == 6)
        {
            ChessPiece rook = GetPiece(baseRow, 7);
            rook.hasMoved = true;
            SetPiece(baseRow, 5, rook);
            SetPiece(baseRow, 7, ChessPiece());
        }
    }

    if (CanPromote(toRow, piece.type, piece.color))
    {
        piece.type = QUEEN;
    }

    piece.hasMoved = true;
    SetPiece(toRow, toCol, piece);
    SetPiece(fromRow, fromCol, ChessPiece());

    g_lastMovedFrom = Position(fromRow, fromCol);
    g_lastMovedTo = Position(toRow, toCol);
    g_hasLastMove = true;

    return capturedKing;
}

bool CanPromote(int row, ChessPieceType type, ChessColor color)
{
    if (type != PAWN)
        return false;
    if (color == WHITE && row == 0)
        return true;
    if (color == BLACK && row == 7)
        return true;
    return false;
}

bool IsCheckmate(ChessColor color)
{
    if (!IsKingInCheck(color))
        return false;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            ChessPiece piece = GetPiece(row, col);
            if (piece.color == color && piece.type != EMPTY)
            {
                std::vector<Position> moves;
                GetValidMoves(row, col, moves);
                if (!moves.empty())
                    return false;
            }
        }
    }

    return true;
}

bool IsStalemate(ChessColor color)
{
    if (IsKingInCheck(color))
        return false;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            ChessPiece piece = GetPiece(row, col);
            if (piece.color == color && piece.type != EMPTY)
            {
                std::vector<Position> moves;
                GetValidMoves(row, col, moves);
                if (!moves.empty())
                    return false;
            }
        }
    }

    return true;
}

void UpdateGameState()
{
    if (g_gameState == GAME_NOT_STARTED)
        return;

    ChessColor nextTurn = (g_currentTurn == WHITE) ? BLACK : WHITE;

    if (IsKingInCheck(nextTurn))
    {
        g_gameState = (nextTurn == WHITE) ? GAME_CHECK_WHITE : GAME_CHECK_BLACK;
    }
    else
    {
        g_gameState = GAME_PLAYING;
    }
}

void CreateBuffer(HWND hWnd)
{
    if (g_bBufferCreated)
        DestroyBuffer();

    RECT rect;
    GetClientRect(hWnd, &rect);

    HDC hdc = GetDC(hWnd);
    g_hdcBuffer = CreateCompatibleDC(hdc);
    g_hBitmapBuffer = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
    SelectObject(g_hdcBuffer, g_hBitmapBuffer);
    ReleaseDC(hWnd, hdc);

    g_bBufferCreated = true;
}

void DestroyBuffer()
{
    if (g_bBufferCreated)
    {
        DeleteObject(g_hBitmapBuffer);
        DeleteDC(g_hdcBuffer);
        g_bBufferCreated = false;
    }
}

void DrawBoard(HDC hdc)
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            int x = BOARD_OFFSET + col * CELL_SIZE;
            int y = BOARD_OFFSET + row * CELL_SIZE;

            COLORREF color;
            if ((row + col) % 2 == 0)
                color = RGB(240, 217, 181);
            else
                color = RGB(181, 136, 99);

            HBRUSH hBrush = CreateSolidBrush(color);
            RECT rect = {x, y, x + CELL_SIZE, y + CELL_SIZE};
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);
        }
    }

    if (g_hasLastMove)
    {
        int fromX = BOARD_OFFSET + g_lastMovedFrom.col * CELL_SIZE + CELL_SIZE / 2;
        int fromY = BOARD_OFFSET + g_lastMovedFrom.row * CELL_SIZE + CELL_SIZE / 2;
        int toX = BOARD_OFFSET + g_lastMovedTo.col * CELL_SIZE + CELL_SIZE / 2;
        int toY = BOARD_OFFSET + g_lastMovedTo.row * CELL_SIZE + CELL_SIZE / 2;

        COLORREF highlight = RGB(255, 255, 128);
        for (int i = 0; i < 2; i++)
        {
            Position pos = (i == 0) ? g_lastMovedFrom : g_lastMovedTo;
            int x = BOARD_OFFSET + pos.col * CELL_SIZE;
            int y = BOARD_OFFSET + pos.row * CELL_SIZE;

            HBRUSH hBrush = CreateSolidBrush(highlight);
            SetBkMode(hdc, TRANSPARENT);

            int alpha = 100;
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, alpha, 0};

            HDC hdcTemp = CreateCompatibleDC(hdc);
            HBITMAP hBitmapTemp = CreateCompatibleBitmap(hdc, CELL_SIZE, CELL_SIZE);
            SelectObject(hdcTemp, hBitmapTemp);

            SetBkColor(hdcTemp, highlight);
            ExtTextOut(hdcTemp, 0, 0, ETO_OPAQUE, NULL, NULL, 0, NULL);

            AlphaBlend(hdc, x, y, CELL_SIZE, CELL_SIZE, hdcTemp, 0, 0, CELL_SIZE, CELL_SIZE, blend);

            DeleteObject(hBitmapTemp);
            DeleteDC(hdcTemp);
            DeleteObject(hBrush);
        }

        HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 100, 255));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        MoveToEx(hdc, fromX, fromY, NULL);
        LineTo(hdc, toX, toY);

        double angle = atan2((double)(toY - fromY), (double)(toX - fromX));
        int arrowLength = 15;
        int arrowWidth = 8;

        double angle1 = angle + 3.14159 - 0.4;
        double angle2 = angle + 3.14159 + 0.4;

        POINT arrowPoints[3];
        arrowPoints[0].x = toX;
        arrowPoints[0].y = toY;
        arrowPoints[1].x = toX + (int)(arrowLength * cos(angle1));
        arrowPoints[1].y = toY + (int)(arrowLength * sin(angle1));
        arrowPoints[2].x = toX + (int)(arrowLength * cos(angle2));
        arrowPoints[2].y = toY + (int)(arrowLength * sin(angle2));

        Polygon(hdc, arrowPoints, 3);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        TCHAR colLabel[2] = {_T('a') + i, 0};
        TCHAR rowLabel[2] = {_T('8') - i, 0};

        int x = BOARD_OFFSET + i * CELL_SIZE + CELL_SIZE / 2 - 5;
        int yTop = BOARD_OFFSET - 20;
        int yBottom = BOARD_OFFSET + BOARD_SIZE * CELL_SIZE + 5;
        int xLeft = BOARD_OFFSET - 20;
        int xRight = BOARD_OFFSET + BOARD_SIZE * CELL_SIZE + 5;

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        TextOut(hdc, x, yTop, colLabel, 1);
        TextOut(hdc, x, yBottom, colLabel, 1);
        TextOut(hdc, xLeft, BOARD_OFFSET + i * CELL_SIZE + CELL_SIZE / 2 - 7, rowLabel, 1);
        TextOut(hdc, xRight, BOARD_OFFSET + i * CELL_SIZE + CELL_SIZE / 2 - 7, rowLabel, 1);
    }
}

void DrawPiece(HDC hdc, int row, int col, ChessPiece piece)
{
    if (piece.type == EMPTY)
        return;

    int x = BOARD_OFFSET + col * CELL_SIZE;
    int y = BOARD_OFFSET + row * CELL_SIZE;

    const TCHAR* symbol = _T("");
    COLORREF textColor;
    COLORREF bgColor;

    switch (piece.type)
    {
    case KING:
        symbol = _T("王");
        break;
    case QUEEN:
        symbol = _T("后");
        break;
    case ROOK:
        symbol = _T("车");
        break;
    case BISHOP:
        symbol = _T("象");
        break;
    case KNIGHT:
        symbol = _T("马");
        break;
    case PAWN:
        symbol = _T("兵");
        break;
    }

    if (piece.color == WHITE)
    {
        textColor = RGB(200, 0, 0);
        bgColor = RGB(255, 240, 220);
    }
    else
    {
        textColor = RGB(0, 80, 0);
        bgColor = RGB(220, 240, 255);
    }

    int circleRadius = CELL_SIZE / 2 - 5;
    int centerX = x + CELL_SIZE / 2;
    int centerY = y + CELL_SIZE / 2;

    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    Ellipse(hdc, centerX - circleRadius, centerY - circleRadius,
            centerX + circleRadius, centerY + circleRadius);

    HFONT hFont = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              GB2312_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, _T("宋体"));

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    SIZE textSize;
    GetTextExtentPoint32(hdc, symbol, 1, &textSize);

    int textX = centerX - textSize.cx / 2;
    int textY = centerY - textSize.cy / 2;

    TextOut(hdc, textX, textY, symbol, 1);

    SelectObject(hdc, hOldFont);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);

    DeleteObject(hFont);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void DrawValidMoves(HDC hdc)
{
    if (!g_hasSelection)
        return;

    int selX = BOARD_OFFSET + g_selectedPos.col * CELL_SIZE;
    int selY = BOARD_OFFSET + g_selectedPos.row * CELL_SIZE;

    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 128, 255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, selX + 2, selY + 2, selX + CELL_SIZE - 2, selY + CELL_SIZE - 2);

    for (size_t i = 0; i < g_validMoves.size(); i++)
    {
        Position pos = g_validMoves[i];
        int x = BOARD_OFFSET + pos.col * CELL_SIZE;
        int y = BOARD_OFFSET + pos.row * CELL_SIZE;

        ChessPiece piece = GetPiece(pos.row, pos.col);
        if (piece.type != EMPTY)
        {
            HPEN hCapPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
            SelectObject(hdc, hCapPen);
            Rectangle(hdc, x + 2, y + 2, x + CELL_SIZE - 2, y + CELL_SIZE - 2);
            DeleteObject(hCapPen);
        }
        else
        {
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 128, 255));
            SelectObject(hdc, hBrush);
            Ellipse(hdc, x + CELL_SIZE / 2 - 8, y + CELL_SIZE / 2 - 8,
                    x + CELL_SIZE / 2 + 8, y + CELL_SIZE / 2 + 8);
            DeleteObject(hBrush);
        }
    }

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateBuffer(hWnd);
        break;

    case WM_SIZE:
        CreateBuffer(hWnd);
        break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_GAME_START:
                InitGame();
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            case IDM_GAME_CONFIG:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_MFCDEMO_DIALOG), hWnd, ConfigDlgProc);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_LBUTTONDOWN:
        {
            if (g_gameState == GAME_CHECKMATE_WHITE ||
                g_gameState == GAME_CHECKMATE_BLACK ||
                g_gameState == GAME_STALEMATE ||
                g_gameState == GAME_WHITE_WINS ||
                g_gameState == GAME_BLACK_WINS ||
                g_gameState == GAME_NOT_STARTED)
            {
                break;
            }

            if (g_gameMode == MODE_AI && g_currentTurn == BLACK)
            {
                break;
            }

            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            int col = (x - BOARD_OFFSET) / CELL_SIZE;
            int row = (y - BOARD_OFFSET) / CELL_SIZE;

            if (!IsValidPosition(row, col))
            {
                g_hasSelection = false;
                g_validMoves.clear();
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            }

            ChessPiece clickedPiece = GetPiece(row, col);

            if (g_hasSelection)
            {
                bool isValidMove = false;
                for (size_t i = 0; i < g_validMoves.size(); i++)
                {
                    if (g_validMoves[i] == Position(row, col))
                    {
                        isValidMove = true;
                        break;
                    }
                }

                if (isValidMove)
                {
                    bool capturedKing = MakeMove(g_selectedPos.row, g_selectedPos.col, row, col);
                    g_hasSelection = false;
                    g_validMoves.clear();

                    if (capturedKing)
                    {
                        g_gameState = (g_currentTurn == WHITE) ? GAME_WHITE_WINS : GAME_BLACK_WINS;
                        InvalidateRect(hWnd, NULL, FALSE);
                        if (g_currentTurn == WHITE)
                        {
                            MessageBox(hWnd, _T("白方获胜！黑方王被吃掉。"), _T("游戏结束"), MB_OK | MB_ICONINFORMATION);
                        }
                        else
                        {
                            MessageBox(hWnd, _T("黑方获胜！白方王被吃掉。"), _T("游戏结束"), MB_OK | MB_ICONINFORMATION);
                        }
                        break;
                    }

                    g_currentTurn = (g_currentTurn == WHITE) ? BLACK : WHITE;
                    UpdateGameState();

                    InvalidateRect(hWnd, NULL, FALSE);

                    if (g_gameMode == MODE_AI && g_currentTurn == BLACK)
                    {
                        bool aiCapturedKing = AITurn();
                        g_currentTurn = WHITE;
                        UpdateGameState();
                        InvalidateRect(hWnd, NULL, FALSE);

                        if (aiCapturedKing)
                        {
                            g_gameState = GAME_BLACK_WINS;
                            MessageBox(hWnd, _T("黑方获胜！白方王被吃掉。"), _T("游戏结束"), MB_OK | MB_ICONINFORMATION);
                        }
                    }
                }
                else if (clickedPiece.type != EMPTY && clickedPiece.color == g_currentTurn)
                {
                    g_selectedPos = Position(row, col);
                    GetValidMoves(row, col, g_validMoves);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                else
                {
                    g_hasSelection = false;
                    g_validMoves.clear();
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            else
            {
                if (clickedPiece.type != EMPTY && clickedPiece.color == g_currentTurn)
                {
                    g_selectedPos = Position(row, col);
                    g_hasSelection = true;
                    GetValidMoves(row, col, g_validMoves);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            if (!g_bBufferCreated)
                CreateBuffer(hWnd);

            RECT rect;
            GetClientRect(hWnd, &rect);

            HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            FillRect(g_hdcBuffer, &rect, hBrush);
            DeleteObject(hBrush);

            DrawBoard(g_hdcBuffer);

            for (int row = 0; row < BOARD_SIZE; row++)
            {
                for (int col = 0; col < BOARD_SIZE; col++)
                {
                    ChessPiece piece = GetPiece(row, col);
                    if (piece.type != EMPTY)
                    {
                        DrawPiece(g_hdcBuffer, row, col, piece);
                    }
                }
            }

            DrawValidMoves(g_hdcBuffer);

            if (g_gameState != GAME_NOT_STARTED)
            {
                TCHAR statusText[256];
                if (g_gameState == GAME_CHECK_WHITE || g_gameState == GAME_CHECK_BLACK)
                {
                    StringCchPrintf(statusText, 256, _T("%s回合 - 将军！"),
                        (g_currentTurn == WHITE) ? _T("白方") : _T("黑方"));
                }
                else
                {
                    StringCchPrintf(statusText, 256, _T("%s回合"),
                        (g_currentTurn == WHITE) ? _T("白方") : _T("黑方"));
                }

                SetTextColor(g_hdcBuffer, RGB(0, 0, 0));
                SetBkMode(g_hdcBuffer, TRANSPARENT);

                HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                          DEFAULT_PITCH | FF_DONTCARE, _T("MS Shell Dlg"));
                HFONT hOldFont = (HFONT)SelectObject(g_hdcBuffer, hFont);

                TextOut(g_hdcBuffer, BOARD_OFFSET, BOARD_OFFSET + BOARD_SIZE * CELL_SIZE + 10,
                        statusText, (int)_tcslen(statusText));

                SelectObject(g_hdcBuffer, hOldFont);
                DeleteObject(hFont);
            }
            else
            {
                TCHAR startText[] = _T("点击 游戏->开始 开始游戏");
                SetTextColor(g_hdcBuffer, RGB(128, 128, 128));
                SetBkMode(g_hdcBuffer, TRANSPARENT);

                HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                          DEFAULT_PITCH | FF_DONTCARE, _T("MS Shell Dlg"));
                HFONT hOldFont = (HFONT)SelectObject(g_hdcBuffer, hFont);

                TextOut(g_hdcBuffer, BOARD_OFFSET, BOARD_OFFSET + BOARD_SIZE * CELL_SIZE + 10,
                        startText, (int)_tcslen(startText));

                SelectObject(g_hdcBuffer, hOldFont);
                DeleteObject(hFont);
            }

            BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                   g_hdcBuffer, 0, 0, SRCCOPY);

            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        DestroyBuffer();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDM_MODE_HUMAN, IDM_MODE_AI,
            (g_gameMode == MODE_TWO_PLAYER) ? IDM_MODE_HUMAN : IDM_MODE_AI);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDOK:
                if (IsDlgButtonChecked(hDlg, IDM_MODE_HUMAN))
                {
                    g_gameMode = MODE_TWO_PLAYER;
                }
                else
                {
                    g_gameMode = MODE_AI;
                }
                EndDialog(hDlg, IDOK);
                return (INT_PTR)TRUE;

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int EvaluatePosition()
{
    int score = 0;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            ChessPiece piece = GetPiece(row, col);
            if (piece.type != EMPTY)
            {
                int value = EvaluatePiece(piece.type);
                if (piece.color == BLACK)
                    score += value;
                else
                    score -= value;
            }
        }
    }

    return score;
}

int EvaluatePiece(ChessPieceType type)
{
    switch (type)
    {
    case PAWN:   return 100;
    case KNIGHT: return 320;
    case BISHOP: return 330;
    case ROOK:   return 500;
    case QUEEN:  return 900;
    case KING:   return 20000;
    default:     return 0;
    }
}

bool AITurn()
{
    std::vector<std::pair<Position, Position>> allMoves;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            ChessPiece piece = GetPiece(row, col);
            if (piece.color == BLACK && piece.type != EMPTY)
            {
                std::vector<Position> moves;
                GetValidMoves(row, col, moves);
                for (size_t i = 0; i < moves.size(); i++)
                {
                    allMoves.push_back(std::make_pair(Position(row, col), moves[i]));
                }
            }
        }
    }

    if (allMoves.empty())
        return false;

    int bestScore = -100000;
    std::pair<Position, Position> bestMove = allMoves[0];

    for (size_t i = 0; i < allMoves.size(); i++)
    {
        Position from = allMoves[i].first;
        Position to = allMoves[i].second;

        ChessPiece originalFrom = GetPiece(from.row, from.col);
        ChessPiece originalTo = GetPiece(to.row, to.col);
        Position originalWhiteKing = g_whiteKingPos;
        Position originalBlackKing = g_blackKingPos;

        bool fromHasMoved = originalFrom.hasMoved;

        ChessPiece movingPiece = originalFrom;
        movingPiece.hasMoved = true;

        if (CanPromote(to.row, movingPiece.type, movingPiece.color))
        {
            movingPiece.type = QUEEN;
        }

        SetPiece(to.row, to.col, movingPiece);
        SetPiece(from.row, from.col, ChessPiece());

        int score = EvaluatePosition();

        if (IsKingInCheck(WHITE))
        {
            score += 50;
        }

        SetPiece(from.row, from.col, originalFrom);
        g_board[from.row][from.col].hasMoved = fromHasMoved;
        SetPiece(to.row, to.col, originalTo);
        g_whiteKingPos = originalWhiteKing;
        g_blackKingPos = originalBlackKing;

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = allMoves[i];
        }
        else if (score == bestScore)
        {
            if (rand() % 2 == 0)
            {
                bestMove = allMoves[i];
            }
        }
    }

    bool capturedKing = MakeMove(bestMove.first.row, bestMove.first.col, bestMove.second.row, bestMove.second.col);

    return capturedKing;
}
