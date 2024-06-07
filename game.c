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

#define BACKGROUND      DARK_GRAY
#define SWITCH_1_BASE   GPIOA1_BASE
#define SWITCH_1_PIN    0x20
#define SWITCH_2_BASE   GPIOA2_BASE
#define SWITCH_2_PIN    0x40
#define BUTTON_COOLDOWN 10
#define ALL_WHITE       6
#define ALL_BLACK       7
#define LOCK_TIME       10

bool game_over = false;
// the following values track a shape based on it's "center", generally top left of center in a shape
// these keep track of the x, y position in *gridspace*
size_t current_col;
size_t current_row;
size_t current_rotation;
enum BLOCK_STYLE current_style;

int palettes[8][4] = {
    {BLACK, WHITE, RED, ORANGE},
    {BLACK, WHITE, SKY_BLUE, GRAY},
    {BLACK, WHITE, BLUE, PINK}, 
    {BLACK, WHITE, GREEN, LIME},
    {BLACK, WHITE, ORANGE, BLUE},
    {BLACK, WHITE, GREEN, MAGENTA},
    {WHITE, WHITE, WHITE, WHITE},
    {BLACK, BLACK, BLACK, BLACK},
};

int rainbow[8] = {RED, ORANGE, YELLOW, GREEN, SKY_BLUE, BLUE, MAGENTA};

uint16_t active_palette[4] = {BLACK, WHITE, RED, ORANGE};
uint8_t active_palette_index = 0;

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
int8_t current_shape[4][2];
enum SHAPE current_shape_id;
int8_t previous_preview[4][2];
enum SHAPE next_shape;
enum SHAPE held_shape;
bool held_used;
bool currently_on_ground;

volatile int frames_elapsed = 0; // how many ticks need to be processed. one tick is added each time the systick triggers
volatile int rotation_cooldown = 0;
int8_t down_counter_reset = 15; // this is how many frames will happen between the piece moving down
int8_t lock_counter = 10; // this is how many frames you have to move a piece before its locked in
volatile float down_counter; // once this reaches 0, the current piece will move downward
float difficulty = 0.75;
int level;
volatile int rotations; 

volatile int queue_transition;
volatile int queue_hold_swap;

int total_lines_cleared;
int score;

// 0 - title screen
// 1 - gameloop
// 2 - end screen
int game_state;
// -1 = left
// 0 = none
// 1 = right
int menu_input; 
int menu_select;

void GameTickHandler() {
    if (paused) return; // don't update anything while paused
    rand(); // update rand
    // update countdowns
    if (rotation_cooldown > 0) rotation_cooldown -= 1;
    if (currently_on_ground) lock_counter -= 1;
    frames_elapsed += 1;
}

void switch1Handler() {
    uint64_t status = MAP_GPIOIntStatus(SWITCH_1_BASE, false);
    MAP_GPIOIntClear(SWITCH_1_BASE, status);
    // Report("switch 2 %d!\r\n", status);
    if (GPIOPinRead(SWITCH_1_BASE, SWITCH_1_PIN) != SWITCH_1_PIN) return;
    if (game_state == 0) {
        game_state = 1;
    }
    queue_hold_swap = 1;
}

void switch2Handler() {
    uint64_t status = MAP_GPIOIntStatus(SWITCH_2_BASE, false);
    MAP_GPIOIntClear(SWITCH_2_BASE, status);
    // Report("switch 2 %d!\r\n", status);
    if (game_state == 0) {
        game_state = 1;
    }
    if (rotation_cooldown == 0) {
        rotations = 1;
        rotation_cooldown = BUTTON_COOLDOWN;
    }
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
        if (x < 0 || x >= NUM_COLS) collision_type = WALL_COLLISION;
        // check for collision with floor
        if (y < 0 || y >= NUM_ROWS) collision_type = PIECE_COLLISION;
        // check for collision with other pieces
        if (gameboard[y][x] != EMPTY) collision_type = PIECE_COLLISION;
    }
    // put the current shape back
    addCurrentShapeToBoard(current_style);
    return collision_type;
}


void changeThemes(int new_palette) {
    paused = true;
    active_palette_index = new_palette;
    fadeToBlack(2);
    swapPalettes(new_palette);
    fadeFromBlack(2);
    paused = false;
}

void swapPalettes(int new_palette) {
    int i;
    for (i = 0; i < NUM_COLORS; i++) 
        active_palette[i] = palettes[new_palette][i];
}

void fadeToBlack(int speed) {
    int NUM_STEPS = 4;
    int i,j;

    for (i = 0; i <= NUM_STEPS; i++) {
        for (j = 0; j < NUM_COLORS; j++) {
            active_palette[j] = getBrightnessScaledColor(active_palette[j], i, NUM_STEPS);
        }
        drawGameboard();
        drawShapePreview(next_shape, NEXT_SHAPE_ROW, NEXT_SHAPE_COL);
        drawShapePreview(held_shape, HELD_SHAPE_ROW, NEXT_SHAPE_COL);
        UtilsDelay(5000000 / speed);
    }
    UtilsDelay(5000000 / speed);
}

void fadeFromBlack(int speed) {
    int NUM_STEPS = 4;
    int i,j;

    for (i = NUM_STEPS; i >= 0; i--) {
        for (j = 0; j < NUM_COLORS; j++) {
            active_palette[j] = getBrightnessScaledColor(palettes[active_palette_index][j], i, NUM_STEPS);
        }
        drawGameboard();
        drawShapePreview(next_shape, NEXT_SHAPE_ROW, NEXT_SHAPE_COL);
        drawShapePreview(held_shape, HELD_SHAPE_ROW, NEXT_SHAPE_COL);
        UtilsDelay(5000000 / speed);
    }
    UtilsDelay(5000000 / speed);
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
            previous_gameboard[row][col] = gameboard[row][col];
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
    float x_accel = (int) -((int8_t) data[3]) / 64.0;
    float y_accel = (int) ((int8_t) data[1]) / 64.0;

    if (game_state == 0) {
        if (x_accel > 0.5) menu_input = 1;
        else if (x_accel < -0.5) menu_input = -1;
    }
    y_accel += 0.3; // bias accelerometer to make holding it slightly angled more neutral
    if (y_accel < 0) 
        y_accel = -(y_accel * y_accel) * 2.5;
    else 
        y_accel = (y_accel * y_accel) * 0.7;
    if (y_accel > 0.9) // clamp holding
        y_accel = 0.9;


    down_counter += y_accel;
    movement_counter += x_accel * 60;
    movement_counter *= 0.8; // apply friction
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
    size_t i,j,row;
    int shape_copy[4][2];
    int old_rotation = current_rotation;

    if (new_rotations == 0) return;
    // if (current_shape_id == SQUARE) return; // squares should not be able to rotate
    // clear current shape from board
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        // row = current_row + current_shape[i][0];
        // col = current_col + current_shape[i][1];
        // make a copy in case we need to undo rotation
        shape_copy[i][0] = current_shape[i][0];
        shape_copy[i][1] = current_shape[i][1];
        // gameboard[row][col] = 0;   
    }
    addCurrentShapeToBoard(EMPTY);

    while (new_rotations > 0) {
        if (current_rotation == 0 || current_rotation == 2)
            flip_x = !flip_x;
        if (current_rotation == 1 || current_rotation == 3)
            flip_y = !flip_y;
        current_rotation++;
        current_rotation = current_rotation % 4;
        // Report("rotation: %d\r\n", current_rotation);
        new_rotations--;
    }

    // update current shape
    bool collision = false;
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        bool done = false;
        for (j = 0; j < 8; j++) {
            if (current_shape[i][0] == ROTATION_MAP[j][0][0] &&
                current_shape[i][1] == ROTATION_MAP[j][0][1]) {
                    int new_row_offset = ROTATION_MAP[j][1][0];
                    int new_col_offset = ROTATION_MAP[j][1][1];
                    int new_row = new_row_offset + current_row;
                    int new_col = new_col_offset + current_col;
                    if (gameboard[new_row][new_col] != EMPTY
                      || new_row >= NUM_ROWS
                      || new_col >= NUM_COLS
                      || new_col < 0)
                    {
                        done = true;
                        collision = true; 
                        break;
                    }
                    current_shape[i][0] = new_row_offset;
                    current_shape[i][1] = new_col_offset;
                    done = true;
                    break;
                }
        }
        if (!done) {
            if (flip_x) current_shape[i][1] = -shape_copy[i][0];
            else current_shape[i][1] = shape_copy[i][0];
            if (flip_y) current_shape[i][0] = -shape_copy[i][1];
            else current_shape[i][0]  = shape_copy[i][1];
        }
    }
    
    // if there is a collision, undo rotation :(
    if (collision || checkCollision(current_row, current_col) != NO_COLLISION) {
        addCurrentShapeToBoard(EMPTY);
        current_rotation = old_rotation;
        for (i = 0; i < PIECES_PER_SHAPE; i++) {
            // bring back old shape
            current_shape[i][0] = shape_copy[i][0];
            current_shape[i][1] = shape_copy[i][1];
        }
    }
    rotations  = 0;
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

void newCurrentShape(enum SHAPE new_shape) {
    current_shape_id = new_shape;
    current_col = 5;
    current_row = 1;
    current_style = default_shape_styles[current_shape_id];
    int i;
    
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        current_shape[i][0] = SHAPES[current_shape_id][i][0];
        current_shape[i][1] = SHAPES[current_shape_id][i][1];
    }
    addCurrentShapeToBoard(current_style);
}

void activateNextShape() {
    newCurrentShape(next_shape);
    chooseNextShape();
}

void drawDropPreview() {
    int i, row, prev_row, prev_col, next_row, next_col;
    
    // find the lowest place the current shape could go
    for (row = current_row; row < NUM_ROWS; row++) 
        if (checkCollision(row, current_col)) break;
    
    // go one up from the lowest place to draw preview
    row -= 1;
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        // give names to long array accesses
        prev_row = previous_preview[i][0];
        prev_col = previous_preview[i][1];
        next_row = current_shape[i][0] + row;
        next_col = current_shape[i][1] + current_col;
        
        // clear preview from last time
        if (prev_row != next_row 
         || prev_col != next_col)
            drawBlock(prev_row, prev_col, gameboard[prev_row][prev_col]);
        // update previous so we can clear it next time
        previous_preview[i][0] = next_row;
        previous_preview[i][1] = next_col;
        // never draw over any actual tiles
        if (gameboard[next_row][next_col] == EMPTY)
            drawBlock(next_row, next_col, DASHED_OUTLINE);
    }
}

void drawShapePreview(enum SHAPE shape_id, size_t init_row, size_t init_col) {
    // clear the previous nextshape
    int i, row, col;

    for (row = -1; row < 3; row++) 
        for (col = -1; col < 1; col++)
            drawBlock(init_row + row, init_col + col, EMPTY);

    // draw nextshape in the relevant box
    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        row = init_row + SHAPES[shape_id][i][0];
        col = init_col + SHAPES[shape_id][i][1];
        drawBlock(row, col, default_shape_styles[shape_id]);
    }
}

void swapHeldAndCurrentShape() {
    if (held_used) return;
    // clear current shape
    addCurrentShapeToBoard(EMPTY);
    if (held_shape == NONE_SHAPE) {
        held_shape = current_shape_id;
        activateNextShape();
    } else {
        enum SHAPE temp = current_shape_id;
        current_shape_id = held_shape;
        held_shape = temp;
        newCurrentShape(current_shape_id);
    }
    held_used = true;
    drawShapePreview(held_shape, HELD_SHAPE_ROW, NEXT_SHAPE_COL);
}

void chooseNextShape() {
    // choose number from 0-6 using rand
    next_shape = rand() % NUM_SHAPE_TYPES;
    drawShapePreview(next_shape, NEXT_SHAPE_ROW, NEXT_SHAPE_COL);
}

void initializeGame() {
    // clear background
    fillScreen(DARK_GRAY);    
    // draw game board border
    drawRect(LEFT_EDGE_PIXEL-3, TOP_EDGE_PIXEL-3, BOARD_WIDTH+6, BOARD_HEIGHT+6, GRAY);
    drawRect(LEFT_EDGE_PIXEL-2, TOP_EDGE_PIXEL-2, BOARD_WIDTH+4, BOARD_HEIGHT+4, WHITE);
    
    // draw border around next shape area
    int border = 3;
    int x = ((NEXT_SHAPE_COL - 1) * BLOCK_WIDTH) + LEFT_EDGE_PIXEL - border;
    int next_y = ((NEXT_SHAPE_ROW - 1) * BLOCK_WIDTH) + TOP_EDGE_PIXEL - border;
    int width = 2 * BLOCK_WIDTH + border * 2;
    int height = 4 * BLOCK_WIDTH + border * 2;
    drawRect(x-1, next_y -1, width+2, height+2, GRAY);
    drawRect(x, next_y, width, height, WHITE);

    // draw border around held shape area
    x = ((NEXT_SHAPE_COL - 1) * BLOCK_WIDTH) + LEFT_EDGE_PIXEL - border;
    int y = ((HELD_SHAPE_ROW - 1) * BLOCK_WIDTH) + TOP_EDGE_PIXEL - border;
    drawRect(x-1, y-1, width+2, height+2, GRAY);
    drawRect(x, y, width, height, WHITE);
    
    // clear board
    int i, row, col; 
    for (row = 0; row < NUM_ROWS; row++) 
        for (col = 0; col < NUM_COLS; col++)  {
            gameboard[row][col] = 0;
            previous_gameboard[row][col] = 0;
        }

    total_lines_cleared = 0;
    score = 0;
    // draw lines area
    char *lines = "LINES";
    char *score = "SCORE";
    char *next = "NEXT ";
    char *held = "HELD ";
    for (i = 0; i < strlen(lines); i++) {
        myDrawChar(1 + 6 * i, 80, lines[i], CYAN, 1);
        myDrawChar(1 + 6 * i, 30, score[i], ORANGE, 1);
        myDrawChar(x - 7 + 6 * i, next_y - 10, next[i], BLUE, 1);
        myDrawChar(x - 7 + 6 * i, y - 10, held[i], GREEN, 1);
    }
    
    total_lines_cleared = 0;
    char* display = "000000";
    snprintf(display, 7, "%06d", total_lines_cleared);
    for (i = 0; i < strlen(display); i++) {
        drawChar(1 + 5 * i, 90, display[i], CYAN, BACKGROUND, 1);
    }

    updateLinesClearedDisplay(0);
    updateScoreDisplay(0, 1);

    for (i = 0; i < PIECES_PER_SHAPE; i++) {
        previous_preview[i][0] = 0;
        previous_preview[i][1] = 0;
    }

    // initialize game state
    level = 1;
    difficulty = 0.75;

    movement_counter = 0;
    down_counter = down_counter_reset;
    active_palette_index = rand() % 5;
    swapPalettes(active_palette_index);
    held_shape = NONE_SHAPE;
    held_used = false;
    queue_transition = -1;
    queue_hold_swap = -1;
    chooseNextShape();
    activateNextShape();
    drawGameboard();
    game_over = false;
    paused = false;
}

void flashChanges(int num_flashes) {
    // flash now empty rows
    int i;
    for (i = 0; i < num_flashes; i++) {
        swapPalettes(ALL_WHITE); // draw removed tiles white
        drawNewGameboard(false);
        UtilsDelay(600000); 
        swapPalettes(ALL_BLACK); // draw removed tiles black
        drawNewGameboard(false);
        UtilsDelay(600000);
    }
    swapPalettes(active_palette_index);
}

void updateLinesClearedDisplay(int new_lines) {
    int i, j;
    paused = true;
    for (j = new_lines; j > 0; j--) {
        total_lines_cleared++;
        char* display = "000000";
        snprintf(display, 7, "%06d", total_lines_cleared);
        for (i = 0; i < strlen(display); i++) {
            drawChar(1 + 5 * i, 90, display[i], CYAN, BACKGROUND, 1);
        }
        UtilsDelay(500000);
    }
    paused = false;
}

void updateScoreDisplay(int new_score, int animation_steps) {
    paused = true;
    int i, j;
    // const int STEPS = 20;
    char* display = "000000";
    for (j = 0; j < animation_steps; j++) {
        snprintf(display, 7, "%06d", interpolateNumber(score, score + new_score,j,animation_steps));
        for (i = 0; i < strlen(display); i++) {
            drawChar(1 + 5 * i, 40, display[i], ORANGE, BACKGROUND, 1);
        }
        UtilsDelay(500000);
    }
    score += new_score;
    snprintf(display, 7, "%06d", score);
    for (i = 0; i < strlen(display); i++) 
        drawChar(1 + 5 * i, 40, display[i], WHITE, BACKGROUND, 1);
    UtilsDelay(1000000);
    for (i = 0; i < strlen(display); i++) 
        drawChar(1 + 5 * i, 40, display[i], ORANGE, BACKGROUND, 1);
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
        for (col = 0; col < NUM_COLS; col++) {
            if (gameboard[row][col] == EMPTY) {
                full = false;
                first_empty_index = -1;
                // Report("empty found row: %d col %d\r\n", row, col);
                break;
            }
        }

        if (full) {
            for (col = 0; col < NUM_COLS; col++) 
                gameboard[row][col] = EMPTY;
            // if (first_empty_index == -1)
            //     first_empty_index = row;
            // else {
            //     for (i = first_empty_index; i > row; i--) {
            //         row_shift_amount[i] = row_shift_amount[i] - 1;
            //     }
            // }
            lines_cleared++;
        }
        row_shift_amount[row] = row - lines_cleared; 
    }

    // no rows were removed
    if (lines_cleared == 0) return 0;
    // dont process game ticks during animation
    paused = true;
    flashChanges(3);
    // move rows downward to fill gaps
    // remove current shape while moving
    addCurrentShapeToBoard(EMPTY);
    int copy_row;
    for (row = NUM_ROWS; row >= 0; row--) {
        copy_row = row_shift_amount[row];
        if (copy_row < 0) copy_row = 0;
        // Report("new_row: %d    ", copy_row);
        if (row % 5 == 0) Report("\r\n");
        // check if the row to be copied is empty
        bool done = false;
        while (copy_row > 0 && !done) {
            for (col = 0; col < NUM_COLS; col++) 
                if (gameboard[copy_row][col] != EMPTY) {    
                    done = true;
                    break;
                };
            if (!done) copy_row--;
        }
        Report("row: %d copy_row %d", row, copy_row);
        for (col = 0; col < NUM_COLS; col++) {
            gameboard[row][col] = gameboard[copy_row][col];
        }
    }
    addCurrentShapeToBoard(current_style);
    drawGameboard();
    paused = false;
    return lines_cleared;
}

void incrementLevel() {
    paused = true;
    int new_palette = (active_palette_index + 1) % 6;
    changeThemes(new_palette);
    level++;
    difficulty += 0.5;
    paused = false;
}

void registerGameInterrupts() {
    // systick is used to time the game updates
    // MAP_SysTickPeriodSet(2000000 / FPS);
    MAP_SysTickPeriodSet(10000000 / FPS);
    MAP_SysTickIntRegister(GameTickHandler);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();
    // switch 1 handler
    MAP_GPIOIntRegister(SWITCH_1_BASE, switch1Handler);
    MAP_GPIOIntTypeSet(SWITCH_1_BASE, SWITCH_1_PIN, GPIO_RISING_EDGE);
    MAP_GPIOIntEnable(SWITCH_1_BASE, SWITCH_1_PIN);
    uint64_t status = MAP_GPIOIntStatus(SWITCH_1_BASE, false);
    MAP_GPIOIntClear(SWITCH_1_BASE, status);
    // switch 2 handler 
    MAP_GPIOIntRegister(SWITCH_2_BASE, switch2Handler);
    MAP_GPIOIntTypeSet(SWITCH_2_BASE, SWITCH_2_PIN, GPIO_RISING_EDGE);
    MAP_GPIOIntEnable(SWITCH_2_BASE, SWITCH_2_PIN);
    status = MAP_GPIOIntStatus(SWITCH_2_BASE, false);
    MAP_GPIOIntClear(SWITCH_2_BASE, status);
}

void fillScreenWithBlocks() {
    int row, col;
    for (row = 0; row < NUM_ROWS; row++) {
        for (col = 0; col < NUM_COLS; col++) {
            gameboard[row][col] = 2;
            drawNewGameboard(true);
            UtilsDelay(50000);
        }
    }
}

void drawTitleScreen() {
    int i, j;
    int width = 128;
    int height = 95;
    game_state = 0;

    writeCommand(SSD1351_CMD_SETCOLUMN); // the next data bits will be x and width
    writeData(0);
    writeData(width-1);
    writeCommand(SSD1351_CMD_SETROW); // the next data bits will be y and height
    writeData(0);
    writeData(height-1);
    writeCommand(SSD1351_CMD_WRITERAM);



//    int fillcolor, palette_index, block_index, block_offset;
//    for (j = 0; j < height; j++) {
//        for (i = 0; i < width; i++) {
//            block_index = i / 16;
//            block_offset = i % 16;
//            palette_index = (title_data[j][block_index] >> (block_offset*2)) % 4;
//            fillcolor = title_palette[palette_index];
//            writeData(fillcolor >> 8); // sending the color one byte at a time, high byte first
//            writeData(fillcolor);
//        }
//    }
//    fillRect(10 - 2, 100 - 2, 128 - 20 + 4, 20 + 4, GRAY);
//    fillRect(10, 100, 108, 20, BLACK);
//    char* easy = "EASY";
//    char* med =  "MED ";
//    char* hard = "HARD";
//    // drawRect(x, y + 5, 20, 15, WHITE);
////    int i;
//    int y = 100;
//    for (i = 0; i < strlen(easy); i++) {
//        myDrawChar(22 + i * 6, y+5, easy[i], GREEN, 1);
//        myDrawChar(56 + i * 6, y+5, med[i], ORANGE, 1);
//        myDrawChar(90 + i * 6, y+5, hard[i], RED, 1);
//    }

}

void drawMenuOptions() {
    int x = 10;
    int width = 128 - 20;
    int y = 100;
    int height = 20;
    
    // fillRect(x, y, width, height, BLACK);
    if (menu_select == -1) drawRect(20 - 2, y + 2, 30, 15, BLACK);
    if (menu_select == 0) drawRect(54 - 2, y + 2, 30, 15, BLACK);
    if (menu_select == 1) drawRect(88 - 2, y + 2, 30, 15, BLACK);

    menu_select += menu_input;
    if (menu_select < -1) menu_select = -1;
    if (menu_select > 1) menu_select = 1;
    menu_input = 0;
    char* string;
    int color;
    int i;
    switch (menu_select) {
        case -1:
            x = 20;
            string = "EASY";
            color = GREEN;
            break;
        case 0:
            x = 54;
            string = "MED ";
            color = ORANGE;
            break;
        case 1:
            x = 88;
            string = "HARD";
            color = RED;
            break;
    };
    drawRect(x-2, y + 2, 30, 15, WHITE);
    for (i = 0; i < strlen(string); i++) 
        myDrawChar(x + 2 + i * 6, y+5, string[i], color, 1);
}

int gameLoop() { 
    registerGameInterrupts();
    drawTitleScreen();
    while (game_state == 0) {
        if (frames_elapsed > 15) {
            frames_elapsed = 0;
            updateAccelerometer();
            drawMenuOptions();
        }
    };
    initializeGame();
    switch (menu_select) {
        case -1:
            difficulty = 0.75;
            level = 1;
            break;
        case 0:
            difficulty = 1.25;
            level = 2;
            break;
        case 1:
            difficulty = 1.75;
            level = 3;
            break;
    }

    int i;
    // Report("game starting \r\n");
    paused = false;
    
    while (!game_over) {
        // check if any ticks have passed
        // Report("... running ... ");
        while (frames_elapsed > 0) {
            frames_elapsed--; // consume one frame 
            down_counter -= difficulty; // one step closer to dropping
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
                    drawDropPreview();
                } else {
                    // there was a collision, so stop the shape
                    // no need to freeze the shape, that only happens while moving down
                    movement_counter = 0;
                }
            }
        }

        while (down_counter <= 0) {
            down_counter += down_counter_reset;
            int new_row = current_row + 1;
            if (checkCollision(new_row, current_col) == PIECE_COLLISION) {
                // start lock countdown
                if (!currently_on_ground) {
                    currently_on_ground = true;
                    lock_counter = LOCK_TIME;
                } else if (lock_counter <= 0) {
                    currently_on_ground = false;
                    // collision with pieces or floor! do not move piece, stop it where it is
                    // and start new shape  
                    down_counter = down_counter_reset;
                    // check if piece is touching the top, if so, game over :(
                    for (i = 0; i < NUM_COLS; i++)
                        if (gameboard[0][i] != EMPTY)  {
                            game_over = true;
                        }
                    
                    if (!game_over)
                        paused = true; // these next parts take a while, so don't 
                                       // start moving the new piece so fast
                        addCurrentShapeToBoard(EMPTY);
                        flashChanges(1);
                        addCurrentShapeToBoard(current_style);
                        activateNextShape();
                        held_used = false;
                        int lines_cleared = checkAndClearLines();
                        updateLinesClearedDisplay(lines_cleared);

                        if (lines_cleared == 1) updateScoreDisplay(level * 40, 15);
                        if (lines_cleared == 2) updateScoreDisplay(level * 100, 15);
                        if (lines_cleared == 3) updateScoreDisplay(level * 300, 25);
                        if (lines_cleared == 4) updateScoreDisplay(level * 1200, 30);
                        if ((total_lines_cleared) > (level * LINES_PER_LEVEL))
                            incrementLevel();
                        paused = false;
                    }
            }
            else { 
                moveCurrentShape(new_row, current_col);
            }
        }

        if (queue_transition != -1) {
            changeThemes(queue_transition);
            queue_transition = -1;
        }

        if (queue_hold_swap == 1) {
            swapHeldAndCurrentShape();
            queue_hold_swap = 0;
        }

        rotateCurrentShape(rotations);
        // drawGameboard();
        drawNewGameboard(true);
        drawDropPreview();
    }
    fillScreenWithBlocks();
    fadeToBlack(1);
    // gameLoop();
    // game over 
    paused = true;
    // verticalScroll(120);    
    // char *gameover_string = "game over :(";
    // for (i = 0; i < strlen(gameover_string); i++) {
    //     myDrawChar(10 + 9 * i, 60, gameover_string[i], RED, 2);
//        drawChar(x, y, c, color, bg, size)
    // }
    return score;
}
