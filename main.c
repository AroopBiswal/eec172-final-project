//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution. 
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
//
//*****************************************************************************


//*****************************************************************************
//
// Application Name     -   SSL Demo
// Application Overview -   This is a sample application demonstrating the
//                          use of secure sockets on a CC3200 device.The
//                          application connects to an AP and
//                          tries to establish a secure connection to the
//                          Google server.
// Application Details  -
// docs\examples\CC32xx_SSL_Demo_Application.pdf
// or
// http://processors.wiki.ti.com/index.php/CC32xx_SSL_Demo_Application
//
//*****************************************************************************


//*****************************************************************************
//
//! \addtogroup ssl
//! @{
//
//*****************************************************************************

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "rom.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"
#include "hw_nvic.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"

//Common interface includes
#include "pinmux.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"

#include "gpio.h"
#include "systick.h"

// arduino
#include "spi.h"


// Custom includes
#include "utils/network_utils.h"

#define SPI_IF_BIT_RATE      1000000
#define CONSOLE              UARTA0_BASE
#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define MAX_STRING_LENGTH    80
#define SYSCLKFREQ           80000000ULL
#define SYSTICK_RELOAD_VAL   32000000UL
#define BIT_AT(i, n)         i & (1 << n);

#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

#define US_TO_TICKS(us)      ((SYSCLKFREQ / 1000000ULL) * (us))
#define IR_GPIO_PIN          0x40
#define IR_GPIO_PORT         GPIOA1_BASE


// bits patterns corresponding to signals from remote
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

#define DELETE  10
#define BOARD_UART      UARTA1_BASE



//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                22    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                14    /* Time - hours */
#define MINUTE              1    /* Time - minutes */
#define SECOND              0     /* Time - seconds */


#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
//#define SERVER_NAME           "a26ypaoxj1nj7v-ats.iot.us-west-2.amazonaws.com" // CHANGE ME
#define SERVER_NAME           "a62ofxyy56n2n-ats.iot.us-east-2.amazonaws.com"
#define GOOGLE_DST_PORT       8443


//#define POSTHEADER "POST /things/CC3200_Thing/shadow HTTP/1.1\r\n"             // CHANGE ME
#define POSTHEADER "POST /things/CC3200Board/shadow HTTP/1.1\r\n"             // CHANGE ME
//#define HOSTHEADER "Host: a26ypaoxj1nj7v-ats.iot.us-west-2.amazonaws.com\r\n"  // CHANGE ME
#define HOSTHEADER "Host: a62ofxyy56n2n-ats.iot.us-east-2.amazonaws.com\r\n"  // CHANGE ME
#define GETHEADER "GET /things/CC3200Board/shadow HTTP/1.1\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"message\" :\""                                           \
                        "Hello phone, "                                     \
                        ":)!"                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"

#define DATAHEADER "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"message\" :" \
                     " {\"default\":\"default message\", "   \
                     "\"email\":\""

#define DATATAIL    "\"}\r\n"                                                \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

static int set_time();
static void BoardInit(void);
static int http_post(int);
static int http_get(int);

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

volatile int time_since_write = 0;
volatile char sending_buf[100];
volatile int cursor = 0;
volatile int systick_expired = 0;
volatile uint64_t systick_delta_us = 0;
volatile uint32_t bits;
volatile int bit_count;

long socket_id;

static void SysTickHandler(void) {
    systick_expired += 1;
    time_since_write += 1;
}

static inline void SysTickReset(void) {
    HWREG(NVIC_ST_CURRENT) = 1;
    systick_expired = 0;
}


static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

static void IRIntHandler(void) {
    // clear int flag
    unsigned long status;
    status = MAP_GPIOIntStatus(IR_GPIO_PORT, true);
    MAP_GPIOIntClear(IR_GPIO_PORT, status);

    volatile uint64_t delta = SYSTICK_RELOAD_VAL - SysTickValueGet();;

    bit_count = bit_count + 1;
    if (bit_count == 1) return; // ignore the first bit
    if (bit_count > 34) return; // ignore long signals

    if (systick_expired) { // ignore signals that are too old
        SysTickReset();
        return;
    }

    if (delta > 16930000)  // this number is very finicky :(
        bits |= 1;
    bits <<= 1;

//    Report("delay: %lld status: %d \r\n", delta, status);
    SysTickReset();
}

static void newButtonPress(int button_pressed) {

    // these are constants that represent the number of characters associated with each button on the remote
    static int button_offsets[] = {0,0,3,6,9,12,15,19,22,26}; // how many characters after 'A' does each button start
    static int chars_per_button[] = {0,3,3,3,3,3,4,3,4}; // how many characters are associated with each button
    static char previous_button = 100;
    static int next_pos = 0;
    static int repeat_count = 0;
    char c = ' ';

    if (time_since_write > 5) previous_button = 100; // threshold delay
    time_since_write = 0;
    if (button_pressed == 0) { // space button
        cursor = cursor + 1;
        next_pos = cursor;
        c = ' ';
    }
    else if (button_pressed == DELETE) { // vol- button removes a key
        c = ' ';
        next_pos = cursor - 1;
        previous_button = 100;
    }

    else if (button_pressed == previous_button) {
        repeat_count = (repeat_count + 1) % chars_per_button[button_pressed];
        c = 'A' + button_offsets[button_pressed] + repeat_count;
        next_pos = cursor;
    } else {
        repeat_count = 0;
        if (sending_buf[cursor] != ' ')
            cursor = cursor + 1;
        next_pos = cursor;
        c = 'A' + button_offsets[button_pressed] + repeat_count;
    }
    Report("%d, %d, %d, %c, %d \r\n", previous_button, button_pressed, repeat_count, c, cursor);

    if (cursor < 0) cursor = 0;
    sending_buf[cursor] = c;
    cursor = next_pos;
    previous_button = button_pressed;
}

static void clearSendBuf() {
    int i;
    for (i = 0; i < 100; i++) {
        sending_buf[i] = ' ';
    }
    cursor = 0;
}

static void decodeBits() {
    if (bits & (0x40000000)) bits >>= 1; // if they are offset for some reason, unoffset them
    else bits >>= 5;

    switch ((uint16_t) bits) {
        case ONE:
            Report("ONE\r\n");
            break;
        case TWO:
            Report("TWO\r\n");
            newButtonPress(1);
            break;
        case THREE:
            Report("THREE\r\n");
            newButtonPress(2);
            break;
        case FOUR:
            Report("FOUR\r\n");
            newButtonPress(3);
            break;
        case FIVE:
            Report("FIVE\r\n");
            newButtonPress(4);
            break;
        case SIX:
            Report("SIX\r\n");
            newButtonPress(5);
            break;
        case SEVEN:
            Report("SEVEN\r\n");
            newButtonPress(6);
            break;
        case EIGHT:
            Report("EIGHT\r\n");
            newButtonPress(7);
            break;
        case NINE:
            Report("NINE\r\n");
            newButtonPress(8);
            break;
        case ZERO:
            Report("ZERO\r\n");
            newButtonPress(0);
            break;
        case MUTE:
            Report("MUTE\r\n");
            http_post(socket_id);
            break;
        case VOL:
            Report("vol\r\n");
            newButtonPress(DELETE);
            break;
    }
    Report("\r\n");
    bits = 0;
    bit_count = 0;
}

void main() {
    long lRetVal = -1;
    BoardInit();

    PinMuxConfig();

    InitTerm();
    ClearTerm();
    UART_PRINT("My terminal works!\n\r");
    systick_expired = 1;
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);
    MAP_SysTickIntRegister(SysTickHandler);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();
    bits = 0;

    MAP_GPIOIntRegister(IR_GPIO_PORT, IRIntHandler);

    MAP_GPIOIntTypeSet(IR_GPIO_PORT, IR_GPIO_PIN, GPIO_RISING_EDGE);
    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
    uint64_t status = MAP_GPIOIntStatus(IR_GPIO_PORT, false);
    MAP_GPIOIntClear(IR_GPIO_PORT, status);

    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
    int i;

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
    const char* test_string = "12345";
    strcpy(sending_buf, test_string);
    cursor = strlen(test_string);
    http_post(socket_id);
    UtilsDelay(1000000);
    http_get(socket_id);
//    sl_Stop(SL_STOP_TIMEOUT);
    while(1)
    {
        if (systick_expired) {
            if (bit_count > 25) {
                // print the signal that was read
                uint32_t bits_copy = bits;
                for (i = 0; i < 32; i++) {
                    if ((i) % 4 == 0) Report(" ");
                    if (bits_copy & (0x80000000)) Report("1"); // check if highest bit == 1
                    else Report("0");
                    bits_copy <<= 1;
                }
                decodeBits();
                Report("\r\n");
                for (i = 0; i <= cursor; i++) {
                    Report("%c", sending_buf[i]);
                }
                Report("\r\n");

            }
        }
    }
}

static int http_post(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    if (sending_buf[cursor] != ' ') {
        sending_buf[++cursor] = '\0';
    } else sending_buf[cursor] = '\0';

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int data_length = strlen(DATAHEADER) + strlen(sending_buf) + strlen(DATATAIL);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", data_length);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, DATAHEADER);
    pcBufHeaders += strlen(DATAHEADER);
    strcpy(pcBufHeaders, sending_buf);
    pcBufHeaders += strlen(sending_buf);
    strcpy(pcBufHeaders, DATATAIL);
    pcBufHeaders += strlen(DATATAIL);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);

    clearSendBuf();
    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}

static int http_get(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(DATA1);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, DATA1);
    pcBufHeaders += strlen(DATA1);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("GET failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }
    // clear string after sending
}

