#include "ir.h"
#include "lab4.h"
#include "game.h"
#include "arduino/oled_test.h"
#include "i2c_if.h"

#define SPI_IF_BIT_RATE        3000000

#define BACKGROUND_COLOR       0x000000

static void BoardInit(void);
void main() {
//    long lRetVal = -1;
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
    drawTitleScreen();
//    while(1) {};
    // drawGameboard();
    gameLoop();
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
