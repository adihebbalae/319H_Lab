// Game.h
// Pentris game engine — declarations shared between Game.cpp and Lab9HMain.cpp

#pragma once
#include <stdint.h>

// Board dimensions
#define COLS        15   // 15 cols × 8px = 120px board; 4-px borders each side
#define ROWS        18   // 18 rows × 8px = 144px board; 6-px bottom border (~57% thinner)
#define CELL        8
#define BOARD_X     4    // board left pixel x (4-px left border)
#define BOARD_Y     10   // board top pixel y
#define NUM_PIECES  5

// Piece color table (index = Board[r][c] - 1)
extern const uint16_t PieceColors[NUM_PIECES];

// Piece shapes: [type][rotation][cell][0=row,1=col]
extern const int8_t Pieces[NUM_PIECES][4][5][2];

// Board state: 0 = empty, 1..5 = locked piece (color index + 1)
extern uint8_t  Board[ROWS][COLS];

// Current falling piece
extern int8_t   CurRow, CurCol;  // signed: piece can be partially above board
extern uint8_t  CurType, CurRot;

// Score and level
extern uint32_t Score;
extern uint8_t  Level, Lines, GameOver;

// Session-only persistent best score (lives until power-off)
extern uint32_t HighScore;

// Next piece type (lookahead)
extern uint8_t NextType;

// Frame counters for rate-limiting
extern uint8_t  GravCounter, MoveCounter;

// ISR → main shared state (volatile)
extern volatile uint8_t  Semaphore;
extern volatile uint32_t JoyX, JoyY, SwEdge;

// Game API
void    InitGame(void);
void    DrawBoardFull(void);
void    DrawCurrentPiece(uint16_t color);
void    DrawCurrentPieceDelta(int8_t oldRow, int8_t oldCol, uint8_t oldRot);
void    DrawScoreLevel(void);
bool    IsValid(int8_t row, int8_t col, uint8_t type, uint8_t rot);
void    LockPiece(void);
uint8_t ClearLines(void);   // now flashes full rows 4x before collapsing
void    SpawnPiece(void);
void    RotatePiece(int8_t dir);
void    UpdateScore(uint8_t cleared);
void    DrawNextPiece(void);

// Utility: RGB565 -> quarter-intensity (used by blur background)
uint16_t DimColor(uint16_t c);
