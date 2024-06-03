/*
 * ir.c
 *
 *  Created on: May 29, 2024
 *      Author: benyo
 */

#include "ir.h"

volatile int time_since_write = 0;
volatile char sending_buf[100];
volatile int cursor = 0;
volatile int systick_expired = 0;
volatile uint64_t systick_delta_us = 0;
volatile uint32_t bits;
volatile int bit_count;

long socket_id;

static void IR_SysTickHandler(void) {
    systick_expired += 1;
    time_since_write += 1;
}

static inline void SysTickReset(void) {
    HWREG(NVIC_ST_CURRENT) = 1;
    systick_expired = 0;
}

static void IRIntHandler(void) {
    // clear int flag
    unsigned long status;
    status = MAP_GPIOIntStatus(IR_GPIO_PORT, true);
    MAP_GPIOIntClear(IR_GPIO_PORT, status);

    volatile uint64_t delta = IR_SYSTICK_RELOAD_VAL - SysTickValueGet();;

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
    for (i = 0; i < MAX_STRING_LENGTH; i++) {
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

static void IR_read_loop() {
    int i;
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
