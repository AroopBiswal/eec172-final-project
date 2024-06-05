#include "ir.h"
#include "lab4.h"
#include "game.h"
#include "arduino/oled_test.h"
#include "i2c_if.h"

#define SPI_IF_BIT_RATE        3000000

#define BACKGROUND_COLOR       0x000000

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
    MAP_SysTickIntRegister(IR_SysTickHandler);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    MAP_GPIOIntRegister(IR_GPIO_PORT, IRIntHandler);
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


void main() {
    long lRetVal = -1;
    BoardInit();

    PinMuxConfig();

    I2C_IF_Open(I2C_MASTER_MODE_FST);

    InitTerm();
    ClearTerm();
//    MAP_SysTickPeriodSet(IR_SYSTICK_RELOAD_VAL);
//    MAP_SysTickIntRegister(IR_SysTickHandler);
//    MAP_SysTickIntEnable();
//    MAP_SysTickEnable();
//
    // MAP_GPIOIntRegister(IR_GPIO_PORT, IRIntHandler);

    // MAP_GPIOIntTypeSet(IR_GPIO_PORT, IR_GPIO_PIN, GPIO_RISING_EDGE);
    // MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
    // uint64_t status = MAP_GPIOIntStatus(IR_GPIO_PORT, false);
    // MAP_GPIOIntClear(IR_GPIO_PORT, status);
//
//    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);

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
    fillScreen(BLACK);
//    initializeGame();
    drawGameboard();
    game_loop();
//    while (1) {
//        UtilsDelay(10000000);
//        initializeGame();
//        chooseNextShape();
//    }
//    // initialize global default app configuration
//    g_app_config.host = SERVER_NAME;
//    g_app_config.port = GOOGLE_DST_PORT;
//
//    //Connect the CC3200 to the local access point
//    lRetVal = connectToAccessPoint();
//    //Set time so that encryption can be used
////    lRetVal = set_time();
//    if(lRetVal < 0) {
//        UART_PRINT("Unable to set time in the device");
//        LOOP_FOREVER();
//    }
//    //Connect to the website with TLS encryption
//    long socket_id = tls_connect();
//    if(socket_id < 0) {
//        ERR_PRINT(socket_id);
//    }
////    sl_Stop(SL_STOP_TIMEOUT);
    while(1) {};
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
