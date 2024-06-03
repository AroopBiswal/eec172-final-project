/*
 * game_helpers.c
 *
 *  Created on: Jun 2, 2024
 *      Author: benyo
 */

#include "game_helpers.h"
#include "common_includes.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "hw_i2c.h"
#include "i2c.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"

// Common interface includes
#include "uart_if.h"
#include "pin_mux_config.h"
#include "i2c_if.h"
#include "i2c_demo.h"


int getBrightnessScaledColor(int color, int step, int max_step) {
    int r, g, b;
    getRGBComponents(color, &r, &g, &b);
    float scalar = (float) (max_step - step) / (float) max_step;
    return combineRGBComponents((int) r * scalar, (int) g * scalar, (int) b * scalar);
}

void getRGBComponents(int color, int *r_dest, int *g_dest, int *b_dest) {
    // from a 16 bit color, split it into its rgb components
    // r - high 5 bits
    // g - middle 6 bits. green gets higher precision because the human eye is most sensitive to green
    // b - low 5 bits.
    *r_dest = (color >> 11) % 32;
    *g_dest = (color >> 5) % 64;
    *b_dest = (color) % 32;
}


int combineRGBComponents(int r, int g, int b) {
    // combine RGB components into a 16 bit color
    return (r << 11) + (g << 5) + b;
}

void DisplayBuffer(unsigned char *pucDataBuf, unsigned char ucLen) {
    unsigned char ucBufIndx = 0;
    UART_PRINT("Read contents");
    UART_PRINT("\n\r");
    while(ucBufIndx < ucLen)
    {
        UART_PRINT(" 0x%x, ", pucDataBuf[ucBufIndx]);
        ucBufIndx++;
        if((ucBufIndx % 8) == 0)
        {
            UART_PRINT("\n\r");
        }
    }
    UART_PRINT("\n\r");
}


int ProcessReadCommand(char *pcInpString, unsigned char* RdDataBuf) {
    unsigned char ucDevAddr, ucRegOffset, ucRdLen;
    unsigned char aucRdDataBuf[256];
    char *pcErrPtr;

    //
    // Get the device address
    //
    pcInpString = strtok(pcInpString, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucDevAddr = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
    //
    // Get the register offset address
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucRegOffset = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);

    //
    // Get the length of data to be read
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucRdLen = (unsigned char)strtoul(pcInpString, &pcErrPtr, 10);
    //RETERR_IF_TRUE(ucLen > sizeof(aucDataBuf));

    //
    // Write the register address to be read from.
    // Stop bit implicitly assumed to be 0.
    //
    // Report("  starting write...");
    RET_IF_ERR(I2C_IF_Write(ucDevAddr,&ucRegOffset,1,0));

    //
    // Read the specified length of data
    //
    // Report("   starting read...");
    RET_IF_ERR(I2C_IF_Read(ucDevAddr, RdDataBuf, ucRdLen));

//    UART_PRINT("I2C Read From address complete\n\r");

    //
    // Display the buffer over UART on successful readreg
    //
    // DisplayBuffer(RdDataBuf, ucRdLen);
    return SUCCESS;
}

void myDrawChar(int x, int y, unsigned char c,
			    unsigned int color, unsigned char size) {

  unsigned char line;	
  char i;						
  char j;						
						
  if((x >= WIDTH)            || // Clip right
     (y >= HEIGHT)           || // Clip bottom
     ((x + 6 * size - 1) < 0) || // Clip left
     ((y + 8 * size - 1) < 0))   // Clip top
    return;

  for (i=0; i<6; i++ ) {
    if (i == 5) 
      line = 0x0;
    else 
      line = font[(c*5)+i];
    for (j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, color);
        else {  // big size
          fillRect(x+(i*size), y+(j*size), size, size, color);
        } 
      }
//      } else if (bg != color) {
        // if (size == 1) // default size
        //   drawPixel(x+i, y+j, bg);
        // else {  // big size
        //   fillRect(x+i*size, y+j*size, size, size, bg);
        // }
//      }
      line >>= 1;
    }
  }
}
