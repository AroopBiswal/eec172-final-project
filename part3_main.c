//// Names:
//// Benjamin Young
//// Aroop Biswal
//
////*****************************************************************************
////
//// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
////
////
////  Redistribution and use in source and binary forms, with or without
////  modification, are permitted provided that the following conditions
////  are met:
////
////    Redistributions of source code must retain the above copyright
////    notice, this list of conditions and the following disclaimer.
////
////    Redistributions in binary form must reproduce the above copyright
////    notice, this list of conditions and the following disclaimer in the
////    documentation and/or other materials provided with the
////    distribution.
////
////    Neither the name of Texas Instruments Incorporated nor the names of
////    its contributors may be used to endorse or promote products derived
////    from this software without specific prior written permission.
////
////  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
////  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
////  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
////  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
////  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
////  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
////  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
////  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
////  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
////  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
////  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////
////*****************************************************************************
//
////*****************************************************************************
////
//// Application Name     - UART Demo
//// Application Overview - The objective of this application is to showcase the
////                        use of UART. The use case includes getting input from
////                        the user and display information on the terminal. This
////                        example take a string as input and display the same
////                        when enter is received.
////
////*****************************************************************************
//
////*****************************************************************************
////
////! \addtogroup uart_demo
////! @{
////
////*****************************************************************************
//
//#include <stdbool.h>
//#include <stdint.h>
//#include <stdio.h>
//
//// Driverlib includes
//#include "rom.h"
//#include "rom_map.h"
//#include "hw_memmap.h"
//#include "hw_common_reg.h"
//#include "hw_types.h"
//#include "hw_ints.h"
//#include "uart.h"
//#include "interrupt.h"
//#include "pin_mux_config.h"
//#include "utils.h"
//#include "prcm.h"
//#include "rom_map.h"
//#include "hw_nvic.h"
//
//// Common interface include
//#include "uart_if.h"
//#include "gpio.h"
//#include "systick.h"
//
//// arduino
//#include "spi.h"
//#include "pin.h"
//#include "arduino/Adafruit_GFX.h"
//#include "arduino/Adafruit_SSD1351.h"
//#include "arduino/oled_test.h"
//
//#if defined(ccs)
//extern void (* const g_pfnVectors[])(void);
//#endif
//#if defined(ewarm)
//extern uVectorEntry __vector_table;
//#endif
//
//#define SPI_IF_BIT_RATE      1000000
//#define CONSOLE              UARTA0_BASE
//#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
//#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
//#define MAX_STRING_LENGTH    80
//#define SYSCLKFREQ           80000000ULL
//#define SYSTICK_RELOAD_VAL   32000000UL
//#define BIT_AT(i, n)         i & (1 << n);
//
//#define TICKS_TO_US(ticks) \
//    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
//    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\
//
//#define US_TO_TICKS(us)      ((SYSCLKFREQ / 1000000ULL) * (us))
//#define IR_GPIO_PIN          0x40
//#define IR_GPIO_PORT         GPIOA1_BASE
//
//
//// bits patterns corresponding to signals from remote
//#define ONE     0b1000000001111111
//#define TWO     0b0100000010111111
//#define THREE   0b1100000000111111
//#define FOUR    0b0001000011101111
//#define FIVE    0b1001000001101111
//#define SIX     0b0101000010101111
//#define SEVEN   0b1101000000101111
//#define EIGHT   0b0000100011110111
//#define NINE    0b1000100001110111
//#define ZERO    0b0000000011111111
//#define MUTE    0b0101100010100111
//#define VOL     0b0001100011100111
//
//#define DELETE  10
//#define BOARD_UART      UARTA1_BASE
//
//#define BG_COLOR WHITE
//static void
//DisplayBanner(char * AppName)
//{
//
//    Report("\n\n\n\r");
//    Report("\t\t *************************************************\n\r");
//    Report("\t\t        CC3200 %s Application       \n\r", AppName);
//    Report("\t\t *************************************************\n\r");
//    Report("\n\n\n\r");
//}
//
//
//static void
//BoardInit(void)
//{
//#ifndef USE_TIRTOS
//#if defined(ccs)
//    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
//#endif
//#if defined(ewarm)
//    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
//#endif
//#endif
//    MAP_IntMasterEnable();
//    MAP_IntEnable(FAULT_SYSTICK);
//
//    PRCMCC3200MCUInit();
//}
//
//// volatile variables
//volatile char sending_buf[100];
//volatile size_t sending_cursor_pos = 0;
//volatile char receiving_buf[100];
//volatile size_t receiving_cursor_pos = 0;
//volatile size_t receiving_screen_cursor = 0;
//volatile char dummy[100];
//
// // these keep track of signals from remote
//volatile int systick_expired = 0;
//volatile uint64_t systick_delta_us = 0;
//volatile uint32_t bits;
//volatile int bit_count;
//
//volatile bool received_string = false;
//
//volatile int time_since_write = 0;
//
//static inline void SysTickReset(void) {
//    HWREG(NVIC_ST_CURRENT) = 1;
//    systick_expired = 0;
//}
//
//static void IRIntHandler(void) {
//    // clear int flag
//    unsigned long status;
//    status = MAP_GPIOIntStatus(IR_GPIO_PORT, true);
//    MAP_GPIOIntClear(IR_GPIO_PORT, status);
//
//    volatile uint64_t delta = SYSTICK_RELOAD_VAL - SysTickValueGet();;
//
//    bit_count = bit_count + 1;
//    if (bit_count == 1) return; // ignore the first bit
//    if (bit_count > 34) return; // ignore long signals
//
//    if (systick_expired) { // ignore signals that are too old
//        SysTickReset();
//        return;
//    }
//
//    if (delta > 16930000)  // this number is very finicky :(
//        bits |= 1;
//    bits <<= 1;
//
////    Report("delay: %lld status: %d \r\n", delta, status);
//    SysTickReset();
//}
//
//static void UARTIntHandler(void) {
//    unsigned long status;
//    status = UARTIntStatus(BOARD_UART, true);
//    UARTIntClear(BOARD_UART, status);
//
////    Report("uart int received\r\n");
//
//    while(UARTCharsAvail(BOARD_UART)) {
//        if(receiving_cursor_pos > 100) {
//            receiving_cursor_pos = 0;
//        }
//
//        char c = UARTCharGet(BOARD_UART);
//        if(c == '1') {
//            // if null character, set received flag
//            Report("done\r\n");
//            received_string = true;
//            break; // we are done receiving values
//        }
////        Report("%c", c);
//        receiving_buf[receiving_cursor_pos] = c;
//        receiving_cursor_pos = receiving_cursor_pos + 1;
//    }
//}
//
//static void IR_SysTickHandler(void) {
//    systick_expired += 1;
//    time_since_write += 1;
//}
//
//
//
//updateSendBuf(bool top, char c, int color) {
//    volatile char* buf;
//    int init_y;
//    volatile int* pos_pointer;
//
//    if (!top) {
//        buf = sending_buf;
//        pos_pointer = &sending_cursor_pos;
//        init_y = 69;
//    }
//    else {
//        buf = dummy;
//        pos_pointer = &receiving_screen_cursor;
//        receiving_screen_cursor++;
//        init_y = 5;
//    }
//
//    int size = 2;
//    int x = size + *pos_pointer * size * 6;
//    int y = init_y;
//
//    while (x + size * 6 > 128) {
//        x -= 120;
//        y += size * 8;
//    }
//
//    if (y > (40 + init_y)) {
//        *pos_pointer = 0;
//        x = *pos_pointer * size * 6;
//        y = init_y;
//    }
//
//    buf[*pos_pointer] = c;
//    drawChar(x, y, c, color, BG_COLOR, size);
//}
//
//static void InitUART() {
//    PRCMPeripheralReset(PRCM_UARTA1);
//    UARTConfigSetExpClk(BOARD_UART, PRCMPeripheralClockGet(PRCM_UARTA1),
//                        UART_BAUD_RATE, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
//                        UART_CONFIG_PAR_NONE);
//    // UART Interrupt Setup
//    UARTIntRegister(BOARD_UART, UARTIntHandler);
//    UARTIntEnable(BOARD_UART, UART_INT_RX);
//    UARTFIFOLevelSet(BOARD_UART, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
////    uint64_t status = UARTIntStatus(BOARD_UART, false);
////    UARTIntClear(BOARD_UART, status);
//    UARTEnable(BOARD_UART);
//}
//
//static void newButtonPress(int button_pressed) {
//
//    // these are constants that represent the number of characters associated with each button on the remote
//    static int button_offsets[] = {0,0,3,6,9,12,15,19,22,26}; // how many characters after 'A' does each button start
//    static int chars_per_button[] = {0,3,3,3,3,3,4,3,4}; // how many characters are associated with each button
//    static char previous_button = 100;
//    static int next_pos = 0;
//    static int repeat_count = 0;
//    char c = ' ';
//
//    if (time_since_write > 5) previous_button = 100; // threshold delay
//    Report("%d, %d\r\n", previous_button, button_pressed);
//    time_since_write = 0;
//    if (button_pressed == 0) { // space button
//        sending_cursor_pos = sending_cursor_pos + 1;
//        next_pos = sending_cursor_pos;
//        c = ' ';
//    }
//    else if (button_pressed == DELETE) { // vol- button removes a key
//        c = ' ';
////        updateSendBuf(false, ' ', BLUE);
//        next_pos = sending_cursor_pos - 1;
//        previous_button = 100;
////        return;
//    }
//
//    else if (button_pressed == previous_button) {
//        repeat_count = (repeat_count + 1) % chars_per_button[button_pressed];
//        c = 'A' + button_offsets[button_pressed] + repeat_count;
//        next_pos = sending_cursor_pos;
//    } else {
//        repeat_count = 0;
//        sending_cursor_pos = sending_cursor_pos + 1;
//        next_pos = sending_cursor_pos;
//        c = 'A' + button_offsets[button_pressed] + repeat_count;
//    }
//
//    if (sending_cursor_pos < 0) sending_cursor_pos = 0;
//    updateSendBuf(false, c, BLUE);
//    sending_cursor_pos = next_pos;
//    previous_button = button_pressed;
//}
//
//static void clearHalf(bool top) {
//    if (top) {
//        fillRect(0, 0, 128, 63, BG_COLOR);
//    } else {
//        fillRect(0, 65, 128, 63, BG_COLOR);
//    }
//}
//
//static void sendMessage() {
//    int i;
////    if (sending_cursor_pos == 0) return;
//    Report("sending %s length %d\r\n", sending_buf, sending_cursor_pos);
//    for (i = 0; i <= sending_cursor_pos; i++) {
//        UARTCharPut(BOARD_UART, sending_buf[i]);
//    }
//    UARTCharPut(BOARD_UART, '1');
//    sending_cursor_pos = 0; // clear sent message
//    clearHalf(false); // clear sending area
//}
//
//static void decodeBits() {
//    if (bits & (0x40000000)) bits >>= 1; // if they are offset for some reason, unoffset them
//    else bits >>= 5;
////    Report("\r\n");
//
//    switch ((uint16_t) bits) {
//        case ONE:
//            Report("ONE\r\n");
////                        newButtonPress(1);
//            break;
//        case TWO:
//            Report("TWO\r\n");
//            newButtonPress(1);
//            break;
//        case THREE:
//            Report("THREE\r\n");
//            newButtonPress(2);
//            break;
//        case FOUR:
//            Report("FOUR\r\n");
//            newButtonPress(3);
//            break;
//        case FIVE:
//            Report("FIVE\r\n");
//            newButtonPress(4);
//            break;
//        case SIX:
//            Report("SIX\r\n");
//            newButtonPress(5);
//            break;
//        case SEVEN:
//            Report("SEVEN\r\n");
//            newButtonPress(6);
//            break;
//        case EIGHT:
//            Report("EIGHT\r\n");
//            newButtonPress(7);
//            break;
//        case NINE:
//            Report("NINE\r\n");
//            newButtonPress(8);
//            break;
//        case ZERO:
//            Report("ZERO\r\n");
//            newButtonPress(0);
//            break;
//        case MUTE:
//            Report("MUTE\r\n");
//            sendMessage();
//            break;
//        case VOL:
//            Report("vol\r\n");
//            newButtonPress(DELETE);
//            break;
//    }
//    Report("\r\n");
//    bits = 0;
//    bit_count = 0;
//}
//
//static void testString() {
//    newButtonPress(2);
//    newButtonPress(3);
//    newButtonPress(4);
//    newButtonPress(5);
//    UtilsDelay(100000);
//    sendMessage();
//}
//
//static void checkReceived() {
//    if (received_string) {
//        Report("received: %d \r\n", receiving_cursor_pos);
//        clearHalf(true);
//        receiving_screen_cursor = 0;
//        int i;
//        for (i = 0; i < receiving_cursor_pos; i++) {
//            Report("%c", receiving_buf[i]);
//            updateSendBuf(true, receiving_buf[i], RED);
//        }
//        Report("\r\n");
//        receiving_cursor_pos = 0;
//        received_string = false;
//    }
//}
//
//
////void main()
////{
////    BoardInit();
////    systick_expired = 1;
////    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);
////    MAP_SysTickIntRegister(IR_SysTickHandler);
////    MAP_SysTickIntEnable();
////    MAP_SysTickEnable();
////
////    bits = 0;
////    //
////    // Muxing for Enabling UART_TX and UART_RX.
////    //
////    PinMuxConfig();
////
////    InitTerm();
////    InitUART();
////    ClearTerm();
////
////    MAP_GPIOIntRegister(IR_GPIO_PORT, IRIntHandler);
////
////    MAP_GPIOIntTypeSet(IR_GPIO_PORT, IR_GPIO_PIN, GPIO_RISING_EDGE);
////    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
////    uint64_t status = MAP_GPIOIntStatus(IR_GPIO_PORT, false);
////    MAP_GPIOIntClear(IR_GPIO_PORT, status);
////
////    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
////
////    //
////    // Enable the SPI module clock
////    //
////    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
////    MAP_SPIReset(GSPI_BASE);
////
////    //
////    // Enable SPI for communication
////    //
////    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
////                    SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
////                    (SPI_SW_CTRL_CS |
////                    SPI_4PIN_MODE |
////                    SPI_TURBO_OFF |
////                    SPI_CS_ACTIVEHIGH |
////                    SPI_WL_8));
////
////    MAP_SPIEnable(GSPI_BASE);
////
////    Adafruit_Init();
////    fillScreen(BG_COLOR);
////    drawFastHLine(0, 64, 128, RED);
////
////    DisplayBanner("ir receiver");
////    int i;
////    while(1)
////    {
////        if (systick_expired) {
////            if (bit_count > 25) {
////                // print the signal that was read
////                uint32_t bits_copy = bits;
////                for (i = 0; i < 32; i++) {
////                    if ((i) % 4 == 0) Report(" ");
////                    if (bits_copy & (0x80000000)) Report("1");// check if highest bit == 1
////                    else Report("0");
////                    bits_copy <<= 1;
////                }
////                Report("\r\n");
////                decodeBits();
////            }
////            checkReceived();
////        }
////    }
////}
//
//
////*****************************************************************************
////
//// Close the Doxygen group.
////! @}
////
////*****************************************************************************
//
//
//
