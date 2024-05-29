#include "ir.h"
#include "lab4.h"



void main() {
    long lRetVal = -1;
    BoardInit();

    PinMuxConfig();

    InitTerm();
    ClearTerm();
    MAP_SysTickPeriodSet(IR_SYSTICK_RELOAD_VAL);
    MAP_SysTickIntRegister(SysTickHandler);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    MAP_GPIOIntRegister(IR_GPIO_PORT, IRIntHandler);

    MAP_GPIOIntTypeSet(IR_GPIO_PORT, IR_GPIO_PIN, GPIO_RISING_EDGE);
    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
    uint64_t status = MAP_GPIOIntStatus(IR_GPIO_PORT, false);
    MAP_GPIOIntClear(IR_GPIO_PORT, status);

    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);

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
//    sl_Stop(SL_STOP_TIMEOUT);
}
