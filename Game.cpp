// Game.cpp
// Pentris game engine — piece data, state variables, and helper functions

#include <stdio.h>
#include "Game.h"
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "Sound.h"
#include "LED.h"

// Random() is defined in Lab9HMain.cpp
extern uint32_t Random(uint32_t n);

// Blue border color matching TetrisBG
#define BORDER_BLUE  0x351D

// Single-cell tile sprites (defined in tetris_images.h, included from Lab9HMain.cpp)
extern const uint16_t TileT[64];
extern const uint16_t TileL[64];
extern const uint16_t TileO[64];
extern const uint16_t TileI[64];
extern const uint16_t TileZ[64];

// Indexed by piece type (same order as Pieces[] and PieceColors[])
static const uint16_t * const TileSprites[NUM_PIECES] = {
    TileT, TileL, TileO, TileI, TileZ
};

// ============================================================
// Piece data (flash/ROM)
// ============================================================

// Main colors per piece — must match the sprite palette so DrawNextPiece
// mini-tiles look consistent with the full tile sprites.
const uint16_t PieceColors[NUM_PIECES] = {
    0xDA8E,  // T = pink  (TetrisT)
    0xAA97,  // L = purple (TetrisL)
    0x351D,  // O = dark blue (TetrisO)
    0x8AE9,  // I = brown (TetrisI)
    0x7DE3,  // Z = green (TetrisZ)
};

// All shapes verified against TetrisBG cell grid (8x8 px per cell)
//
// T (3x3 box):        L (2x4 box):         O (2x3 box):
//   Rot0  Rot1          Rot0  Rot1            Rot0  Rot1
//   XXX   ..X           X.    XXXX            XX    XXX
//   .X.   XXX           X.    X...            XX    .XX
//   .X.   ..X           X.                    X.
//                       XX
//
// I (1x5 / 5x1):      Z (4x2 / 2x4):
//   Rot0  Rot1          Rot0   Rot1
//   X     XXXXX         .XXX   X.
//   X                   XX..   XX
//   X                          .X
//   X                          .X
//   X
const int8_t Pieces[NUM_PIECES][4][5][2] = {
    // Piece 0: T
    {
        {{0,0},{0,1},{0,2},{1,1},{2,1}},  // rot 0
        {{0,2},{1,0},{1,1},{1,2},{2,2}},  // rot 1 (CW 90)
        {{0,1},{1,1},{2,0},{2,1},{2,2}},  // rot 2 (180)
        {{0,0},{1,0},{1,1},{1,2},{2,0}},  // rot 3 (CCW 90)
    },
    // Piece 1: L
    {
        {{0,0},{1,0},{2,0},{3,0},{3,1}},  // rot 0
        {{0,0},{0,1},{0,2},{0,3},{1,0}},  // rot 1
        {{0,0},{0,1},{1,1},{2,1},{3,1}},  // rot 2
        {{0,3},{1,0},{1,1},{1,2},{1,3}},  // rot 3
    },
    // Piece 2: O
    {
        {{0,0},{0,1},{1,0},{1,1},{2,0}},  // rot 0
        {{0,0},{0,1},{0,2},{1,1},{1,2}},  // rot 1
        {{0,1},{1,0},{1,1},{2,0},{2,1}},  // rot 2
        {{0,0},{0,1},{1,0},{1,1},{1,2}},  // rot 3
    },
    // Piece 3: I
    {
        {{0,0},{1,0},{2,0},{3,0},{4,0}},  // rot 0 (vertical)
        {{0,0},{0,1},{0,2},{0,3},{0,4}},  // rot 1 (horizontal)
        {{0,0},{1,0},{2,0},{3,0},{4,0}},  // rot 2 = rot 0
        {{0,0},{0,1},{0,2},{0,3},{0,4}},  // rot 3 = rot 1
    },
    // Piece 4: Z
    {
        {{0,1},{0,2},{0,3},{1,0},{1,1}},  // rot 0
        {{0,0},{1,0},{1,1},{2,1},{3,1}},  // rot 1
        {{0,2},{0,3},{1,0},{1,1},{1,2}},  // rot 2
        {{0,0},{1,0},{2,0},{2,1},{3,1}},  // rot 3
    },
};

// Score awarded per simultaneous line clears (multiplied by level)
// 1 row=1000, 2=2500, 3=5000, 4=10000, 5=20000
static const uint32_t ScoreTable[6] = {0, 100, 250, 500, 2000, 4000};

// ============================================================
// Game state variables
// ============================================================

uint8_t  Board[ROWS][COLS];
int8_t   CurRow, CurCol;
uint8_t  CurType, CurRot;
uint32_t Score;
uint8_t  Level, Lines, GameOver;
uint8_t  GravCounter, MoveCounter;
uint8_t  NextType;
uint32_t HighScore = 0;   // session-only; main updates on game over

volatile uint8_t  Semaphore;
volatile uint32_t JoyX, JoyY, SwEdge;

// ============================================================
// Internal helpers
// ============================================================

// Fill a cell with a solid color (used for erase/white and flash effects).
static void DrawCell(int8_t r, int8_t c, uint16_t color) {
    int16_t px = BOARD_X + c * CELL;
    int16_t py = BOARD_Y + r * CELL;
    ST7735_FillRect(px, py, CELL, CELL, color);
}

// Draw a cell using the beveled tile sprite for the given piece type index.
static void DrawCellIdx(int8_t r, int8_t c, uint8_t typeIdx) {
    int16_t px = BOARD_X + c * CELL;
    int16_t py = BOARD_Y + r * CELL;
    ST7735_DrawBitmap(px, py + CELL - 1, TileSprites[typeIdx], CELL, CELL);
}

// Draw a right-aligned 5-digit number in white on the blue border strip
static void DrawUInt(uint32_t n, int16_t x, int16_t y) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%5lu", (unsigned long)n);
    for (int i = 0; buf[i]; i++, x += 6)
        ST7735_DrawCharS(x, y, buf[i], ST7735_WHITE, BORDER_BLUE, 1);
}

// Difficulty now tracks score directly so gravity ramps with successful play.
static uint8_t LevelForScore(uint32_t score) {
    uint8_t level = (uint8_t)(1 + (score / 1000U));
    return (level > 10U) ? 10U : level;
}

static bool PieceOccupiesCell(int8_t baseRow, int8_t baseCol, uint8_t type, uint8_t rot,
                              int8_t row, int8_t col) {
    for (int i = 0; i < 5; i++) {
        if ((baseRow + Pieces[type][rot][i][0] == row) &&
            (baseCol + Pieces[type][rot][i][1] == col)) {
            return true;
        }
    }
    return false;
}

// ============================================================
// Public API
// ============================================================

void DrawCurrentPiece(uint16_t color) {
    for (int i = 0; i < 5; i++) {
        int8_t r = CurRow + Pieces[CurType][CurRot][i][0];
        int8_t c = CurCol + Pieces[CurType][CurRot][i][1];
        if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
            if (color == ST7735_WHITE)
                DrawCell(r, c, ST7735_WHITE);
            else
                DrawCellIdx(r, c, CurType);
        }
    }
}

// Repaint the cells uncovered by the old pose, then redraw the full new pose.
// The full redraw is intentional: SD-card music uses the same SPI bus as the
// TFT, and occasional bus-side corruption was leaving overlapping cells stale
// when we only redrew the changed subset.
void DrawCurrentPieceDelta(int8_t oldRow, int8_t oldCol, uint8_t oldRot) {
    for (int i = 0; i < 5; i++) {
        int8_t r = oldRow + Pieces[CurType][oldRot][i][0];
        int8_t c = oldCol + Pieces[CurType][oldRot][i][1];
        if (r < 0 || r >= ROWS || c < 0 || c >= COLS) continue;
        if (!PieceOccupiesCell(CurRow, CurCol, CurType, CurRot, r, c)) {
            if (Board[r][c]) {
                DrawCellIdx(r, c, Board[r][c] - 1);
            } else {
                DrawCell(r, c, ST7735_WHITE);
            }
        }
    }
    DrawCurrentPiece(PieceColors[CurType]);
}

void DrawBoardFull(void) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (Board[r][c])
                DrawCellIdx(r, c, Board[r][c] - 1);
            else
                DrawCell(r, c, ST7735_WHITE);
        }
    }
}

void DrawScoreLevel(void) {
    DrawUInt(Score, 10, 2);
    DrawUInt(Level,  90, 2);
}

bool IsValid(int8_t row, int8_t col, uint8_t type, uint8_t rot) {
    for (int i = 0; i < 5; i++) {
        int8_t r = row + Pieces[type][rot][i][0];
        int8_t c = col + Pieces[type][rot][i][1];
        if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return false;
        if (Board[r][c]) return false;
    }
    return true;
}

void LockPiece(void) {
    for (int i = 0; i < 5; i++) {
        int8_t r = CurRow + Pieces[CurType][CurRot][i][0];
        int8_t c = CurCol + Pieces[CurType][CurRot][i][1];
        if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
            Board[r][c] = CurType + 1;
    }
    Score += 10;  // 10 points per block places
    Level = LevelForScore(Score);
    Sound_Shoot();
}

// Busy-wait that keeps the SD audio buffer fed — safe to stall the main loop
// for up to ~500 ms without starving SysTick.
static void FlashDelay_ms(uint32_t ms) {
    uint32_t cycles = ms / 5;
    if (cycles == 0) cycles = 1;
    for (uint32_t i = 0; i < cycles; i++) {
        Music_Task();
        Clock_Delay1ms(5);
    }
}

uint8_t ClearLines(void) {
    // Pass 1: identify which rows are full
    bool fullRow[ROWS];
    uint8_t cleared = 0;
    for (int r = 0; r < ROWS; r++) {
        fullRow[r] = true;
        for (int c = 0; c < COLS; c++) {
            if (!Board[r][c]) { fullRow[r] = false; break; }
        }
        if (fullRow[r]) cleared++;
    }

    // Pass 2: flash the full rows 4 times (white <-> board cell colors)
    // ~4 x 60 ms = 240 ms; music keeps playing because FlashDelay_ms pumps Music_Task.
    if (cleared > 0) {
        for (int blink = 0; blink < 4; blink++) {
            uint16_t color = (blink & 1) ? ST7735_WHITE : ST7735_BLACK;
            for (int r = 0; r < ROWS; r++) {
                if (!fullRow[r]) continue;
                for (int c = 0; c < COLS; c++) {
                    DrawCell(r, c, color);
                }
            }
            LED_Toggle(1<<16);   // visible heartbeat during flash
            FlashDelay_ms(60);
        }
    }

    // Pass 3: collapse — shift everything above each full row down by one
    for (int r = ROWS-1; r >= 0; r--) {
        if (fullRow[r]) {
            for (int rr = r; rr > 0; rr--) {
                for (int c = 0; c < COLS; c++) Board[rr][c] = Board[rr-1][c];
                fullRow[rr] = fullRow[rr-1];
            }
            for (int c = 0; c < COLS; c++) Board[0][c] = 0;
            fullRow[0] = false;
            r++;  // re-check the same index (now holds the shifted row)
        }
    }
    return cleared;
}

// Quarter-intensity per-channel dim of an RGB565 color — used for the
// title-screen "ghost blocks" background animation.
uint16_t DimColor(uint16_t c) {
    uint16_t r = (c >> 11) & 0x1F;
    uint16_t g = (c >>  5) & 0x3F;
    uint16_t b =  c        & 0x1F;
    r >>= 2; g >>= 2; b >>= 2;
    return (r << 11) | (g << 5) | b;
}

void DrawNextPiece(void) {
    // Preview area: x=42..61 (20px), y=0..9 (10px) — gap between score and LVL label
    const int16_t PX = 42, PY = 0, PW = 20, PH = 10;
    ST7735_FillRect(PX, PY, PW, PH, BORDER_BLUE);

    // Bounding box of NextType in rot 0
    int8_t r0 = 7, c0 = 7, r1 = -1, c1 = -1;
    for (int i = 0; i < 5; i++) {
        int8_t r = Pieces[NextType][0][i][0];
        int8_t c = Pieces[NextType][0][i][1];
        if (r < r0) r0 = r;  if (r > r1) r1 = r;
        if (c < c0) c0 = c;  if (c > c1) c1 = c;
    }
    int8_t ph = (r1-r0+1)*2, pw = (c1-c0+1)*2;  // 2px per cell
    int16_t ox = PX + (PW - pw) / 2;
    int16_t oy = PY + (PH - ph) / 2;
    for (int i = 0; i < 5; i++) {
        int8_t r = Pieces[NextType][0][i][0] - r0;
        int8_t c = Pieces[NextType][0][i][1] - c0;
        ST7735_FillRect(ox + c*2, oy + r*2, 2, 2, PieceColors[NextType]);
    }
}

void SpawnPiece(void) {
    CurType = NextType;           // use the lookahead piece
    NextType = (uint8_t)Random(NUM_PIECES);  // pick the next one now
    CurRot  = 0;
    CurRow  = 0;
    CurCol  = 7;  // near center of 15-col board
    GravCounter = 0;
    MoveCounter = 0;
    if (!IsValid(CurRow, CurCol, CurType, CurRot))
        GameOver = 1;
}

void RotatePiece(int8_t dir) {
    uint8_t newRot = (uint8_t)((CurRot + dir + 4) % 4);
    static const int8_t kicks[5] = {0, -1, 1, -2, 2};
    for (int k = 0; k < 5; k++) {
        if (IsValid(CurRow, CurCol + kicks[k], CurType, newRot)) {
            CurCol += kicks[k];
            CurRot  = newRot;
            return;
        }
    }
    // All kick positions blocked — rotation ignored
}

void UpdateScore(uint8_t cleared) {
    if (cleared > 5) cleared = 5;
    Score += ScoreTable[cleared] * Level;
    Lines += cleared;
    Level = LevelForScore(Score);
}

void InitGame(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) Board[r][c] = 0;
    Score       = 0;
    Level       = 1;
    Lines       = 0;
    GameOver    = 0;
    GravCounter = 0;
    MoveCounter = 0;
    Semaphore   = 0;
    NextType    = (uint8_t)Random(NUM_PIECES);  // seed before first SpawnPiece
    SpawnPiece();
}
