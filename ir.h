/*
 * ir.h
 *
 *  Created on: May 29, 2024
 *      Author: benyo
 */

#ifndef EEC172_LAB4_IR_H_
#define EEC172_LAB4_IR_H_

#include "common_includes.h"

static void IRIntHandler(void);
static void IR_SysTickHandler(void);
static void newButtonPress(int button_pressed);
static void decodeBits();
static void clearSendBuf();
static void IR_read_loop();

#define SYSCLKFREQ                 80000000ULL
#define IR_SYSTICK_RELOAD_VAL      32000000UL
#define BIT_AT(i, n)               i & (1 << n);

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

#define MAX_STRING_LENGTH    80
#define DELETE  10

#endif /* EEC172_LAB4_IR_H_ */
