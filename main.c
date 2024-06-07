#include "ir.h"
#include "lab4.h"
#include "game.h"
#include "arduino/oled_test.h"
#include "i2c_if.h"

#define SPI_IF_BIT_RATE        3000000

#define BACKGROUND_COLOR       0x000000

#define NUM_BUTTONS 3

#define ONE     0b1000000001111111
#define TWO     0b0100000010111111
#define THREE   0b1100000000111111
#define FOUR    0b0001000011101111
#define FIVE    0b1001000001101111
#define SIX     0b0101000010101111
#define SEVEN   0b1101000000101111
#define EIGHT   0b0000100011110111
#define NINE    0b1000100001110111
#define ZERO    0b0000000011111111
#define MUTE    0b0101100010100111
#define VOL     0b0001100011100111

#define MAX_STRING_LENGTH    6
#define DELETE  10

#define SYSCLKFREQ                 80000000ULL
#define IR_SYSTICK_RELOAD_VAL      32000000UL
#define BIT_AT(i, n)               i & (1 << n);

#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

#define US_TO_TICKS(us)      ((SYSCLKFREQ / 1000000ULL) * (us))
#define IR_GPIO_PIN          0x40
#define IR_GPIO_PORT         GPIOA1_BASE



volatile int time_since_write_aroop = 0;
volatile char sending_buf[MAX_STRING_LENGTH];
volatile size_t cursor_aroop = 0;
volatile int systick_expired_aroop = 0;
volatile uint64_t systick_delta_us_aroop = 0;
volatile uint32_t bits;
volatile int bit_count;

int timesNavigated = 0;

static void BoardInit(void);

typedef struct leaderboard_entry {
    char name[7];
    char score[7];
} leaderboard_entry;

leaderboard_entry leaderboard[5];

void printEntry(leaderboard_entry entry) {
    UART_PRINT(entry.name);
    UART_PRINT(entry.score);
}

int currentButton = 0;


void IR_SysTickHandlerAroop(void) {
    systick_expired_aroop += 1;
    time_since_write_aroop += 1;
}

inline void SysTickResetAroop(void) {
    HWREG(NVIC_ST_CURRENT) = 1;
    systick_expired_aroop = 0;
}

void IRIntHandlerAroop(void) {
    // clear int flag
    unsigned long status;
    status = MAP_GPIOIntStatus(IR_GPIO_PORT, true);
    MAP_GPIOIntClear(IR_GPIO_PORT, status);

    volatile uint64_t delta = IR_SYSTICK_RELOAD_VAL - SysTickValueGet();;

    bit_count = bit_count + 1;
    if (bit_count == 1) return; // ignore the first bit
    if (bit_count > 34) return; // ignore long signals

    if (systick_expired_aroop) { // ignore signals that are too old
        SysTickReset();
        return;
    }

    if (delta > 16930000)  // this number is very finicky :(
        bits |= 1;
    bits <<= 1;

//    Report("delay: %lld status: %d \r\n", delta, status);
    SysTickResetAroop();
}

void parseHighscores(char *received, leaderboard_entry *entry_buf) {
//    UART_PRINT(received);
//    char* s = strtok(received, "leaderboard");
//    UART_PRINT(s);
//    s = strtok(NULL, "\"");
//    UART_PRINT(s);
    int i, j;
    char* search = "leaderboard";
    bool found = false;
    i = 0;
    while (i < strlen(received)) {
        j = 0;
        while (j < strlen(search))
            if (received[i + j] != search[j++]) break;
        if (j == strlen(search)) {
            i += j;
            found = true;
            break;
        }
        i++;
    }
    if (!found) {
        Report("not found :( \r\n");
        return;
    }
    received += i + 5;
//    int entry_index = 0;
    for (i = 0; i < 5; i++) {
        // read entry
        for(j = 0; j < 6; j++) {
            entry_buf[i].name[j] = received[j];
            entry_buf[i].score[j] = received[j + 9];
        }
        entry_buf[i].name[6] = '\0';
        entry_buf[i].score[6] = '\0';
//        printEntry(entry_buf[i]);
        received += 18;
    }
}

void drawEntry(int x, int y, leaderboard_entry entry) {
    int i;
    for (i = 0; i < 6; i++) {
        drawChar(x + i * 6, y, entry.name[i], BLUE, BLACK, 1);
        drawChar(x + i * 6 + 42, y, entry.score[i], SKY_BLUE, BLACK, 1);
    }
}

void drawLeaderboard() {
    fillScreen(DARK_GRAY);
    int x = 15;
    int y = 5;
    int width = 98;
    int height = 80;
    drawRect(x - 1, y - 1, width + 2, height + 2, GRAY);
    fillRect(x, y, width, height, BLACK);
    int i;
    for (i = 0; i < 5; i++) 
        drawEntry(20, 12 + 10 * i, leaderboard[i]);
    char *gameover_string = "game over :(";
    for (i = 0; i < strlen(gameover_string); i++)
        myDrawChar(10 + 9 * i, 100, gameover_string[i], RED, 2);
}


int strToInt(char* s) {
    int i, out;
    out = 0;
    for (i = strlen(s) - 2; i >= 0; i--) 
        out += s[i] - '0';
    return out;
}

void enableIR() {
    MAP_SysTickPeriodSet(IR_SYSTICK_RELOAD_VAL);
    MAP_SysTickIntRegister(IR_SysTickHandlerAroop);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    MAP_GPIOIntRegister(IR_GPIO_PORT, IRIntHandlerAroop);
    MAP_GPIOIntTypeSet(IR_GPIO_PORT, IR_GPIO_PIN, GPIO_RISING_EDGE);
    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
    uint64_t status = MAP_GPIOIntStatus(IR_GPIO_PORT, false);
    MAP_GPIOIntClear(IR_GPIO_PORT, status);

    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
}

void drawSendBuf() {
    int x = 21;
    int y = 70;
    int i;
    for (i = 0; i < MAX_STRING_LENGTH; i++)
        drawChar(x + i * 6, y, sending_buf[i], GREEN, BLACK, 1);
}

void uploadLeaderboard(int game_score, int socket_id) {
    // determine users position in scoreboard
//    int higher_than_index = -1;
    int i;
    char json_buf[20];
//    for (i = 0; i < 5; i++) {
//        int entry_score = strToInt(leaderboard[i].score);
//        if (game_score > entry_score) {
//            higher_than_index = i;
//            break;
//        }
//    }
//    if (higher_than_index != -1) {
////        snprintf(leaderboard[]);
//    } else {
//        // didn't beat any of the scores :(
//    }
//    UART_PRINT(sending_buf);
    for (i = 0; i < 6; i++) {
//        Report("%c", sending_buf[i]);
        json_buf[i] = sending_buf[i];
    }
    char *inbetween = "\":\"";
    for (i = 0; i < 3; i++)
        json_buf[i+6] = inbetween[i];
    char* score_buf = "000000";
    snprintf(score_buf, 6, "%06d", game_score);
    for (i = 0; i < 6; i++)
        json_buf[i+9] = score_buf[i];
    Report("\r\n json_buf:");
    for (i = 0; i < 15; i++) {
        Report("%c", json_buf[i]);
    }
    http_post(socket_id, json_buf);
}

void drawScore(int score) {
    int x = 63;
    int y = 70;
    int i;
    char* display = "000000";    
    snprintf(display, 7, "%06d", score);
    for (i = 0; i < 6; i++) {
        drawChar(x + i * 6, y, display[i], GREEN, BLACK, 1);
    }
}

void drawButton(int x, int y, int width, int height, const char* label, bool selected) {
    int borderColor = selected ? RED : WHITE;
    drawRect(x, y, width, height, borderColor);
    setCursor(x + 10, y + 5);
    setTextColor(WHITE, BLACK);
    Outstr(label);
}

void selectButton_TitleScreen() {
    switch (currentButton) {
        case 0:
            Report("Start selected\r\n");
            //start the game
            break;
        case 1:
            Report("Choose Difficulty selected\r\n");
            break;
        case 2:
            Report("Leaderboard selected\r\n");
            break;
    }
}

static void decodeBits_TitleScreen() {
    if (bits & (0x40000000)) bits >>= 1;
    else bits >>= 5;

    switch ((uint16_t) bits) {
        case ONE:
            Report("ONE\r\n");
            navigateButtons(-1); // Move up
            break;
        case TWO:
            Report("TWO\r\n");
            navigateButtons(1); // Move down
            break;
        case MUTE:
            Report("MUTE\r\n");
            selectButton_TitleScreen(); // Select the current button
            break;
    }
    Report("\r\n");
    bits = 0;
    bit_count = 0;
}



void drawTitleScreenAroop() {
    fillScreen(BLACK); // Clear the screen
    setTextColor(WHITE, BLACK);
    setTextSize(2);
    setCursor(20, 10); // Adjusted position
    Outstr("TETRIS");

    // Button dimensions
    int buttonWidth = 80;
    int buttonHeight = 20;
    int buttonSpacing = 10;
    int screenWidth = 128;

    int startButtonX = (screenWidth - buttonWidth) / 2;
    int startButtonY = 40;
    int chooseDifficultyButtonX = startButtonX;
    int chooseDifficultyButtonY = startButtonY + buttonHeight + buttonSpacing;
    int leaderboardButtonX = startButtonX;
    int leaderboardButtonY = chooseDifficultyButtonY + buttonHeight + buttonSpacing;

    // Draw buttons
    drawButton(startButtonX, startButtonY, buttonWidth, buttonHeight, "Start", currentButton == 0);
    drawButton(chooseDifficultyButtonX, chooseDifficultyButtonY, buttonWidth, buttonHeight, "Difficulty", currentButton == 1);
    drawButton(leaderboardButtonX, leaderboardButtonY, buttonWidth, buttonHeight, "Leaderboard", currentButton == 2);
}

void navigateButtons(int direction) {
    currentButton = (currentButton + direction + NUM_BUTTONS) % NUM_BUTTONS;
    timesNavigated++;
    drawTitleScreenAroop();
}

void drawEndScreen() {
    setTextColor(WHITE, BLACK);
    setTextSize(2);
    Outstr("GAME OVER");

    // Fetch Leaderboard from AWS
}

void IR_read_loop_aroop() {
   Report("IR READ LOOP STARTED");
   int i;
   clearSendBuf();
   while(1)
   {
       if (systick_expired_aroop) {
           if (bit_count > 25) {
               // print the signal that was read
               uint32_t bits_copy = bits;
               for (i = 0; i < 32; i++) {
                   if ((i) % 4 == 0) Report(" ");
                   if (bits_copy & (0x80000000)) Report("1"); // check if highest bit == 1
                   else Report("0");
                   bits_copy <<= 1;
               }
               decodeBits_TitleScreen();
               Report("\r\n");
               for (i = 0; i <= cursor; i++) {
                   Report("%c", sending_buf[i]);
               }
               Report("\r\n");
           }
       }
   }
}


void main() {
    long lRetVal = -1;
    BoardInit();

    PinMuxConfig();

    I2C_IF_Open(I2C_MASTER_MODE_FST);

    InitTerm();
    ClearTerm();

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
    MAP_SPIReset(GSPI_BASE);

    //
    // Enable SPI for communication
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                    SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                    (SPI_SW_CTRL_CS |
                    SPI_4PIN_MODE |
                    SPI_TURBO_OFF |
                    SPI_CS_ACTIVEHIGH |
                    SPI_WL_8));

    MAP_SPIEnable(GSPI_BASE);
    Adafruit_Init();
    drawTitleScreenAroop();
    Report("Title Screen Drawn");
    enableIR();
//    IR_read_loop();
    while(1 && timesNavigated<4) {
            if (systick_expired_aroop) {
                if (bit_count > 25) {
                    uint32_t bits_copy = bits;
                    int i = 0;
                    for (i = 0; i < 32; i++) {
                        if ((i) % 4 == 0) Report(" ");
                        if (bits_copy & (0x80000000)) Report("1");
                        else Report("0");
                        bits_copy <<= 1;
                    }
                    Report("\r\n");
                    navigateButtons(1);
                    decodeBits_TitleScreen();
                }
            }
        }

    fillScreen(BLACK);
    char* loading = "LOADING ... ";
    int i;
    for (i = 0; i < strlen(loading); i++)
        drawChar(20 + i * 9, 60, loading[i], BLUE, BACKGROUND_COLOR, 2);
    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    //Connect the CC3200 to the local access point
     lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    long socket_id = tls_connect();
    if(socket_id < 0) {
        ERR_PRINT(socket_id);
    }
    UtilsDelay(1000000);
    char received[1460];
    http_get(socket_id, received);
    parseHighscores(received, leaderboard);
    while (1) {
    int score = gameLoop();
//    int score = 1000;
    drawLeaderboard();
    drawScore(score);
    enableIR();
    IR_read_loop();
    uploadLeaderboard(score, socket_id);
//    while(1) {};
    }
}

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

static void BoardInit(void) {
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
