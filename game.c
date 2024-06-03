/*
 * game.c
 *
 *  Created on: May 29, 2024
 *      Author: benyo
 */

#include "common_includes.h"
#include "game.h"
#include "game_helpers.h"
#include "shapes.h"
#include "arduino/Adafruit_SSD1351.h"
#include "arduino/Adafruit_GFX.h"
#include "arduino/oled_test.h"

#define SWITCH_1_BASE   GPIOA1_BASE
#define SWITCH_1_PIN    0x20
#define SWITCH_2_BASE   GPIOA2_BASE
#define SWITCH_2_PIN    0x40


bool game_over = false;
// the following values track a shape based on it's "center", generally top left of center in a shape
// these keep track of the x, y position in *gridspace*
int current_col;
int current_row;
int current_rotation;
enum BLOCK_STYLE current_style;

int palettes[5][4] = {
    {BLACK, WHITE, RED, ORANGE},
    {BLACK, WHITE, SKY_BLUE, GRAY},
    {BLACK, WHITE, BLUE, PINK}, 
    {WHITE, WHITE, WHITE, WHITE},
    {BLACK, BLACK, BLACK, BLACK},
};

int active_palette[4] = {BLACK, WHITE, RED, ORANGE};
int active_palette_index = 0;

int gameboard[NUM_ROWS][NUM_COLS];
int previous_gameboard[NUM_ROWS][NUM_COLS];

// this is a cumulative tracker of the accelerometer input.
// positive = to the right, negative = to the left
int movement_counter;
// what value of movement_counter triggers a movement
const int MOVEMENT_THRESHOLD = 100; 
int lines_cleared = 0; //
int score = 0;

bool paused = false; // if this is true, the systick handler will not update
// array containing the offsets from the pivot point for the current piece
int current_shape[4][2];
enum SHAPE next_shape;


volatile int frames_elapsed = 0; // how many ticks need to be processed. one tick is added each time the systick triggers
int difficulty = 10; // this is how many frames will happen between the piece moving down
volatile int down_counter; // once this reaches 0, the current piece will move downward
volatile int rotations; 

void GameTickHandler() {
    if (!paused)
        frames_elapsed += 1;
}

void switch1Handler() {
    uint64_t status = MAP_GPIOIntStatus(SWITCH_1_BASE, false);
    MAP_GPIOIntClear(SWITCH_1_BASE, status);
    // Report("switch 1 %d!\r\n", status);
    rotations++;
}

void switch2Handler() {
    uint64_t status = MAP_GPIOIntStatus(SWITCH_2_BASE, false);
    MAP_GPIOIntClear(SWITCH_2_BASE, status);
    // Report("switch 2 %d!\r\n", status);
    rotations++;
}

int checkCollision(int new_row, int new_col) {
    // check if moving a piece to this new position causes a collision
    // return 0 if no collision
    // return 1 if collision with pieces or floor
    // return 2 if collision with wall
    int i,x,y;
    int collision_type = 0;
    // first, remove current shape so that it doesnt collide with itself
    addCurrentShapeToBoard(EMPTY);
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        y = new_row + current_shape[i][0];
        x = new_col + current_shape[i][1];
        // check for collision with wall
        if (x < 0 || x > NUM_COLS) collision_type = WALL_COLLISION;
        // check for collision with floor
        if (y < 0 || y >= NUM_ROWS) collision_type = PIECE_COLLISION;
        // check for collision with other pieces
        if (gameboard[y][x] != EMPTY) collision_type = PIECE_COLLISION;
    }
    // put the current shape back
    addCurrentShapeToBoard(current_style);
    return collision_type;
}


void swapPalettes(int new_palette) {
    int i;
    for (i = 0; i < NUM_COLORS; i++) 
        active_palette[i] = palettes[new_palette][i];
}


// void verticalScroll(int scroll) {
//     int i;
//     for (i = 0; i < scroll; i = i + (scroll / 16)) {
//         writeCommand(SSD1351_CMD_DISPLAYOFFSET);
//         writeData(scroll);
//         UtilsDelay(10000000);
//     }
// }


void fadeToBlack() {
    int NUM_STEPS = 4;
    int i,j;

    for (i = 0; i <= NUM_STEPS; i++) {
        for (j = 0; j < NUM_COLORS; j++) {
            active_palette[j] = getBrightnessScaledColor(active_palette[j], i, NUM_STEPS);
        }
        drawGameboard();
        UtilsDelay(5000000);
    }
    UtilsDelay(10000000);
    // swapPalettes(active_palette_index);
    // drawGameboard();
}

void fadeFromBlack() {
    int NUM_STEPS = 4;
    int i,j;

    for (i = NUM_STEPS; i >= 0; i--) {
        for (j = 0; j < NUM_COLORS; j++) {
            active_palette[j] = getBrightnessScaledColor(active_palette[j], i, NUM_STEPS);
        }
        drawGameboard();
        UtilsDelay(5000000);
    }
    UtilsDelay(10000000);
}

void drawBlock(int row, int col, int block_style) {
    int x_start = LEFT_EDGE_PIXEL + col * BLOCK_WIDTH;
    int y_start = TOP_EDGE_PIXEL + row * BLOCK_WIDTH;

    // set the rect that we want to fill
    // Filling a rect rather than drawing one pixel
    // at a time saves a lot of SPI commands because we only need to
    // set the position once
    writeCommand(SSD1351_CMD_SETCOLUMN); // the next data bits will be x and width
    writeData(x_start);
    writeData(x_start+BLOCK_WIDTH-1);
    writeCommand(SSD1351_CMD_SETROW); // the next data bits will be y and height
    writeData(y_start);
    writeData(y_start+BLOCK_WIDTH-1);
    // fill!
    writeCommand(SSD1351_CMD_WRITERAM);

    int fillcolor, palette_index, i;
    for (i=0; i < BLOCK_WIDTH * BLOCK_WIDTH; i++) {
      palette_index = block_styles[block_style][i / BLOCK_WIDTH][i % BLOCK_WIDTH];
      fillcolor = active_palette[palette_index];
      writeData(fillcolor >> 8); // sending the color one byte at a time, high byte first
      writeData(fillcolor);
    }
}

void drawNewGameboard(bool update_previous_board) {
    // this function draws only the difference between the last drawn board
    // and the newest board. This limits the amount of (slow) drawing calls
    // The parameter controls whether the previous state should be updated
    // afterwards, this could be false if we want to draw the updated board
    // multiple times 
    int row, col;
    for (row = 0; row < NUM_ROWS; row++) 
        for (col = 0; col < NUM_COLS; col++)
            // check if this position has changed
            if (previous_gameboard[row][col] != gameboard[row][col]) {
                drawBlock(row, col, gameboard[row][col]);
                if (update_previous_board)
                    previous_gameboard[row][col] = gameboard[row][col];
            }
}

void drawGameboard() {
    int row, col;
    for (row = 0; row < NUM_ROWS; row++) 
        for (col = 0; col < NUM_COLS; col++) {
            drawBlock(row, col, gameboard[row][col]);
        }
}

void updateAccelerometer() {
    // Report("reading accel...\r\n");
    char* cmd_val = "0x18 0x2 4"; // this is the command we want to send to i2c, we can't lose it
    char* cmd_buffer = " 0x18 0x2 4"; 
    unsigned char data[10];
    
    strcpy(cmd_buffer, cmd_val); // copy cmd string to buffer because the cmd is mutated during read
    ProcessReadCommand(cmd_buffer, data); // get acceleration values

    // convert accel values into float roughly from 0-1.0
    float x_accel = (int) ((int8_t) data[1]) / 64.0;
    float y_accel = (int) ((int8_t) data[3]) / 64.0;

    if (y_accel > 0.6) {
        down_counter += 1;
    }

    if (y_accel < -0.6) {
        down_counter -= 1;
    }
    if (y_accel < -0.8) {
        down_counter -= 2;
    }
    movement_counter += x_accel * 40;
    // Report("x_accel %f y_accel %f \n\r", x_accel,y_accel);
}

void addCurrentShapeToBoard(int style) {
    int i, row, col;
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        row = current_row + current_shape[i][0];
        col = current_col + current_shape[i][1];
        gameboard[row][col] = style;
    }
}

void rotateCurrentShape(int new_rotations) {
    bool flip_x = false;
    bool flip_y = false;
    int i,row,col;
    int shape_copy[4][2];
    int old_rotation = current_rotation;

    if (new_rotations == 0) return;
    
    // clear current shape from board
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        row = current_row + current_shape[i][0];
        col = current_col + current_shape[i][1];
        // make a copy in case we need to undo rotation
        shape_copy[i][0] = current_shape[i][0];
        shape_copy[i][1] = current_shape[i][1];
        gameboard[row][col] = 0;   
    }

    while (new_rotations > 0) {
        Report(current_rotation);
        if (current_rotation == 0 || current_rotation == 2)
            flip_x = !flip_x;
        if (current_rotation == 1 || current_rotation == 3)
            flip_y = !flip_y;
        current_rotation++;
        current_rotation = current_rotation % 4;
        new_rotations--;
    }

    // update current shape
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        if (flip_x) current_shape[i][0] = -current_shape[i][0];
        if (flip_y) current_shape[i][1] = -current_shape[i][1];
    }
    
    // if there is a collision, undo rotation :(
    if (checkCollision(current_row, current_col)) {
        current_rotation = old_rotation;
        for (i = 0; i < PIECES_PER_SHAPE; i++) {
            // bring back old shape
            current_shape[i][0] = shape_copy[i][0];
            current_shape[i][1] = shape_copy[i][1];
        }
    }
    rotations    = 0;

    addCurrentShapeToBoard(current_style);
}

void moveCurrentShape(int new_row, int new_col) {
    // clear old shape
    addCurrentShapeToBoard(EMPTY);
    // clamp values to be within bounds
    if (new_row >= NUM_ROWS) new_row = NUM_ROWS - 1;
    if (new_col >= NUM_COLS) new_col = NUM_COLS - 1;
    if (new_row < 0)         new_row = 0;
    if (new_col < 0)         new_col = 0;
    current_row = new_row;
    current_col = new_col;
    // draw new shape
    addCurrentShapeToBoard(current_style);
}

void newCurrentShape() {
    int new_shape = next_shape;
    chooseNextShape();
    current_col = 5;
    current_row = 1;
    current_style = default_shape_styles[new_shape];
    int i;
    
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        current_shape[i][0] = SHAPES[new_shape][i][0];
        current_shape[i][1] = SHAPES[new_shape][i][1];
    }
    addCurrentShapeToBoard(current_style);
}

void drawNextShapePreview() {
    // clear the previous nextshape
    int i, row, col;

    for (row = -1; row < 3; row++) 
        for (col = -1; col < 1; col++)
            drawBlock(NEXT_SHAPE_ROW + row, NEXT_SHAPE_COL + col, EMPTY);

    // draw nextshape in the relevant box
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        row = NEXT_SHAPE_ROW + SHAPES[next_shape][i][0];
        col = NEXT_SHAPE_COL + SHAPES[next_shape][i][1];
        drawBlock(row, col, default_shape_styles[next_shape]);
    }

}

void chooseNextShape() {
    int i, row, col;
    // choose number from 0-6 using rand
    next_shape = rand() % NUM_SHAPE_TYPES;
    drawNextShapePreview();
}

void initializeGame() {
    // draw game board border
    drawRect(LEFT_EDGE_PIXEL-2, TOP_EDGE_PIXEL-2, BOARD_WIDTH+4, BOARD_HEIGHT+4, WHITE);
    
    // draw border around next shape area
    int border = 3;
    int x = ((NEXT_SHAPE_COL - 1) * BLOCK_WIDTH) + LEFT_EDGE_PIXEL - border;
    int y = ((NEXT_SHAPE_ROW - 1) * BLOCK_WIDTH) + TOP_EDGE_PIXEL - border;
    int width = 2 * BLOCK_WIDTH + border * 2;
    int height = 4 * BLOCK_WIDTH + border * 2;
    drawRect(x, y, width, height, WHITE);
    
    // clear board
    int row, col; 
    for (row = 0; row < NUM_ROWS; row++) 
        for (col = 0; col < NUM_COLS; col++) 
            gameboard[row][col] = 0;
    
    // initialize game state
    movement_counter = 0;
    down_counter = difficulty;
    chooseNextShape();
    newCurrentShape();
    drawNewGameboard(true);
    game_over = false;
    paused = false;
}

int checkAndClearLines() {
    // this function searches the game board for full lines
    // if a full line is found, it is removed with a flashing animation
    // and the above pieces are shifted downward
    int i, row, col;
    int first_empty_index = -1;
    int row_shift_amount[NUM_ROWS];
    bool full;
    int lines_cleared = 0;
    // find full rows and store them in array
    for (row = NUM_ROWS; row >= 0; row--) {
        // check if row has any empty spaces
        full = true;
        for (col = 0; col < NUM_COLS; col++)
            if (gameboard[row][col] == EMPTY) {
                full = false;
                first_empty_index = -1;
                break;
            }
        if (full) {
            lines_cleared++;
            for (col = 0; col < NUM_COLS; col++) 
                gameboard[row][col] = EMPTY;
            if (first_empty_index == -1)
                first_empty_index = row;
            else {
                for (i = first_empty_index; i < row; i++) {
                    row_shift_amount[i] = row_shift_amount[i] - 1;
                }
            }
        }
        row_shift_amount[row] = row - lines_cleared; 
        return lines_cleared;
    }

    // no rows were removed
    if (lines_cleared == 0) return;
    // dont process game ticks during animation
    paused = true;
    // flash now empty rows
    for (i = 0; i < 3; i++) {
        swapPalettes(4); // draw removed tiles white
        drawNewGameboard(false);
        UtilsDelay(500000); 
        swapPalettes(5); // draw removed tiles black
        drawNewGameboard(false);
        UtilsDelay(500000);
    }
    swapPalettes(active_palette_index);
    
    // move rows downward to fill gaps
    int copy_row;
    for (row = NUM_ROWS; row >= 0; row--) {
        for (col = 0; col < NUM_COLS; col++) {
            copy_row = row_shift_amount[row];
            if (copy_row < 0) copy_row = 0;
            gameboard[row][col] = gameboard[copy_row][col];
        }
    }
    drawNewGameboard(true);
    paused = false;
}

void game_loop() { 
    initializeGame();

    // MAP_SysTickPeriodSet(2000000 / FPS);
    MAP_SysTickPeriodSet(10000000 / FPS);
    MAP_SysTickIntRegister(GameTickHandler);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    MAP_GPIOIntRegister(SWITCH_1_BASE, switch1Handler);
    MAP_GPIOIntTypeSet(SWITCH_1_BASE, SWITCH_1_PIN, GPIO_RISING_EDGE);
    MAP_GPIOIntEnable(SWITCH_1_BASE, SWITCH_1_PIN);
    uint64_t status = MAP_GPIOIntStatus(SWITCH_1_BASE, false);
    MAP_GPIOIntClear(SWITCH_1_BASE, status);

    MAP_GPIOIntRegister(SWITCH_2_BASE, switch2Handler);
    MAP_GPIOIntTypeSet(SWITCH_2_BASE, SWITCH_2_PIN, GPIO_RISING_EDGE);
    MAP_GPIOIntEnable(SWITCH_2_BASE, SWITCH_2_PIN);
    status = MAP_GPIOIntStatus(SWITCH_2_BASE, false);
    MAP_GPIOIntClear(SWITCH_2_BASE, status);

    int i;
    Report("game starting \r\n");
    paused = false;
    
    while (!game_over) {
        // check if any ticks have passed
        // Report("... running ... ");
        while (frames_elapsed > 0) {
            frames_elapsed--; // consume one frame 
            down_counter--; // one step closer to dropping
            updateAccelerometer(); // read values from accelerometer
            
            int direction; // 1 = right, -1 = left
            while (movement_counter >= MOVEMENT_THRESHOLD 
                || movement_counter <= -MOVEMENT_THRESHOLD) {
                // Report("counter: %d \r\n", movement_counter);
                
                if (movement_counter < 0) direction = -1; 
                else direction = 1;
                
                int new_col = current_col + direction;
                movement_counter += direction * -MOVEMENT_THRESHOLD; // bring counter towards 0
                // check for collisions before moving
                int collision_type = checkCollision(current_row, new_col);
                if (collision_type == NO_COLLISION) {
                    // no collision, so move the shape
                    moveCurrentShape(current_row, new_col);
                    drawNewGameboard(true);
                } else {
                    // there was a collision, so stop the shape
                    // no need to freeze the shape, that only happens while moving down
                    movement_counter = 0;
                }
            }
        }

        while (down_counter <= 0) {
            down_counter += difficulty;
            int new_row = current_row + 1;
            if (checkCollision(new_row, current_col) == PIECE_COLLISION) {
                // collision with pieces or floor! do not move piece, stop it where it is
                // and start new shape  
                // check if piece is touching the top, if so, game over :(
                down_counter = 10;
                for (i = 0; i < NUM_COLS; i++)
                    if (gameboard[0][i] != EMPTY)  {
                        game_over = true;
                    }
                if (!game_over)
                    newCurrentShape();
                    checkAndClearLines();
            } else { 
                moveCurrentShape(new_row, current_col);
            }
        }

        rotateCurrentShape(rotations);
        drawNewGameboard(true);
    }

    game_loop();
    // game over 
    paused = true;
    // verticalScroll(120);    
    fadeToBlack();
    char *gameover_string = "game over :(";
    for (i = 0; i < strlen(gameover_string); i++) {
        drawChar(10 + 9 * i, 60, gameover_string[i], CYAN, BLACK, 2);
    }
}




