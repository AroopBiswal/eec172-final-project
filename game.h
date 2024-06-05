/*
 * game.h
 *
 *  Created on: May 29, 2024
 *      Author: benyo
 */

#ifndef EEC172_FINAL_PROJECT_GAME_H_
#define EEC172_FINAL_PROJECT_GAME_H_
#define FPS                    6

#define NUM_COLORS             4
#define PIECES_PER_SHAPE       4

#define NUM_ROWS               20
#define NUM_COLS               10
#define BLOCK_WIDTH            6
#define BOARD_WIDTH            NUM_COLS * BLOCK_WIDTH
#define BOARD_HEIGHT           NUM_ROWS * BLOCK_WIDTH
#define TOP_EDGE_PIXEL         4
#define BOTTOM_EDGE_PIXEL      TOP_EDGE_PIXEL + BOARD_HEIGHT
#define LEFT_EDGE_PIXEL        34
#define RIGHT_EDGE_PIXEL       LEFT_EDGE_PIXEL + BOARD_WIDTH;

#define NO_COLLISION           0
#define PIECE_COLLISION        1
#define WALL_COLLISION         2
#define NEXT_SHAPE_ROW         7
#define NEXT_SHAPE_COL         14
#define HELD_SHAPE_ROW         14
#define LINES_PER_LEVEL        5

#include "shapes.h"
void drawGameboard();

void drawTitleScreen();
void gameLoop();

void addCurrentShapeToBoard(int style);
void fadeToBlack();
void fadeFromBlack();
void swapPalettes(int);
void drawShapePreview(enum SHAPE shape_id, size_t init_row, size_t init_col);
void chooseNextShape();
void updateLinesClearedDisplay();
void updateScoreDisplay();

#endif /* EEC172_FINAL_PROJECT_GAME_H_ */
