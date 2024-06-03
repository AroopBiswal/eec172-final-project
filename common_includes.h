/*
 * common_includes.h
 *
 *  Created on: May 29, 2024
 *      Author: benyo
 */

#ifndef EEC172_LAB4_COMMON_INCLUDES_H_
#define EEC172_LAB4_COMMON_INCLUDES_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
#include "pin_mux_config.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"
#include "gpio.h"
#include "systick.h"

#include "arduino/Adafruit_SSD1351.h"
#include "arduino/Adafruit_GFX.h"
#include "arduino/oled_test.h"
#include "arduino/glcdfont.h"


// arduino
#include "spi.h"

// Custom includes
#include "utils/network_utils.h"


#endif /* EEC172_LAB4_COMMON_INCLUDES_H_ */
