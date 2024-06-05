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

int interpolateNumber(int start, int end, int step, int max_steps) {
  float diff = (float) (end - start);
  return (int) (start + (diff * (float) step / (float) max_steps));
}

const uint32_t title_data[95][8] = {
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
{0x5555557f, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0xfd555555},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfdffffff},
{0xffffff7f, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfdffffff},
{0xffffff7f, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfdffffff},
{0x3000fff, 0xf000c00, 0xf000, 0xc003f, 0x3c0030, 0xffff00c0, 0xff0003, 0xfdfc0000},
{0xf33fcf7f, 0xcf0fcc3f, 0xffcf0ff, 0xffccff3f, 0xff3cff30, 0xfff0fcc3, 0x3c3f3ff3, 0xfffcffff},
{0xf3cfcfff, 0xcf0f3c3f, 0xffcf0ff, 0xffcf3f3f, 0xff3cfcf0, 0xffc3fcc3, 0xf0cf3ff3, 0xfdfcfffc},
{0xf3f3cf7f, 0xcf0f3c3f, 0xffcf0ff, 0xffcf3f3f, 0xff3cf3f0, 0xff0ffcc3, 0xc3cf3ff3, 0xfffcfff3},
{0xf3f3cfff, 0xcf0f3c3f, 0xffcf0ff, 0xffcf3f3f, 0xff3cf3f0, 0xff3ffcc3, 0xc3f33ff3, 0xfdfcffcf},
{0xf3fccf7f, 0xcf0cfc3f, 0xffcf0ff, 0xffcfcf3f, 0xff3ccff0, 0xfc3ffcc3, 0xff33ff3, 0xfdfcffcf},
{0xf3fccfff, 0xcf0cfc3f, 0xffcf0ff, 0xffcfcf3f, 0xff3ccff0, 0xfc3ffcc3, 0x3ff33ff3, 0xfdfcff3c},
{0xf3ff0f7f, 0xcf03fc3f, 0xffcf0ff, 0xffcff33f, 0xff3c3ff0, 0xfc3ffcc3, 0x3ff33ff3, 0xfdfcfcfc},
{0xf3ff0fff, 0xcf03fc3f, 0xffcf0ff, 0xffcff33f, 0xff3c3ff0, 0xfc3ffcc3, 0xfff33ff3, 0xfdfcfcf0},
{0xf3ffcf7f, 0xcf0ffc3f, 0xffcf0ff, 0xffcffc3f, 0xff3cfff0, 0xfc3ffcc3, 0xffcf3ff3, 0xfdfcf3c3},
{0xf3ffcfff, 0xcf0ffc3f, 0xffcf0ff, 0xffcffc3f, 0xff3cfff0, 0xfc3ffcc3, 0xffcf3ff3, 0xfdfccfc3},
{0xf3ffcf7f, 0xcf3ffc3f, 0xffcf0ff, 0xffcfff3f, 0xff3cfff0, 0xfc3ffcc3, 0xffcf3ff3, 0xfffc3f0f},
{0xf3ffffff, 0xcffffc3f, 0xffcf0ff, 0xffcfffff, 0xff3ffff0, 0xff0ffcc3, 0xff3f3ff3, 0xfdfc3c3f},
{0xf3ffff7f, 0xcffffc3f, 0xffcf0ff, 0xffcfffff, 0xff3ffff0, 0xff0ffcc3, 0xff3f3ff3, 0xfdfcf0ff},
{0x53ffffff, 0x4ffffc15, 0x554f055, 0x554fffff, 0x553ffff0, 0xffc054c1, 0x54ff1553, 0xfdfcf155},
{0x53ffff7f, 0x4ffffc15, 0x554f055, 0x554fffff, 0x553ffff0, 0xfff004c1, 0x53ff1553, 0xfdffc1b5},
{0xe3ffffff, 0x8ffffc19, 0x678f067, 0x678fffff, 0x9e3ffff0, 0xffff00c1, 0x4fcf1b63, 0xfdff055b},
{0x53ffff7f, 0x4ffffc16, 0x594f059, 0x594fffff, 0x653ffff0, 0xfffff0c1, 0x4f0f1653, 0xffff1596},
{0x53ffffff, 0x4ffffc1d, 0x754f075, 0x754fffff, 0xd53ffff0, 0xffffc4c1, 0x3c0f1553, 0xfdfc5d75},
{0x93ffff7f, 0x4ffffc19, 0x664f066, 0x664fffff, 0x993ffff0, 0xffffc4c1, 0xfc4f19d3, 0xfdfc6564},
{0xd3ffff7f, 0x4ffffc15, 0x574f057, 0x574fffff, 0x5d3ffff0, 0xffff14c1, 0xf04f1d53, 0xfdf05654},
{0x93ffff7f, 0x4ffffc19, 0x664f066, 0x664fffff, 0x993ffff0, 0xffff54c1, 0xc14f1993, 0xfff075e3},
{0x3ffff7f, 0xffffc00, 0xf000, 0xfffff, 0x3ffff0, 0xfffc00c0, 0xf0003, 0xfdf0000f},
{0x3ffff7f, 0xffffc00, 0xf000, 0xf3fff, 0x3ffff0, 0xfffc00c0, 0xf0003, 0xfdf0000f},
{0x3ffff7f, 0xffffc00, 0xf000, 0xf3fff, 0x3ffff0, 0xfff000c0, 0xf0003, 0xfdf0003c},
{0x3ffff7f, 0xffffc00, 0xf000, 0xf0fff, 0x3ffff0, 0xffc000c0, 0xf0003, 0xfff000f0},
{0x3ffffff, 0xffffc00, 0xf000, 0xf03ff, 0x3ffff0, 0xff0000c0, 0xf0003, 0xfdf003f0},
{0x3ffff7f, 0xffffc00, 0xf000, 0xf00ff, 0x3ffff0, 0xff0000c0, 0xf0003, 0xfdf003c0},
{0x3ffff7f, 0xffffc00, 0xf000, 0xf003f, 0x3ffff0, 0xfc0000c0, 0xf0003, 0xfdf00f00},
{0x3ffff7f, 0xffffc00, 0xf000, 0xf000f, 0x3ffff0, 0xf00000c0, 0xf0003, 0xfff03c00},
{0x3ffffff, 0xffffc00, 0xf000, 0xf0003, 0x3ffff0, 0xc00000c0, 0xf0003, 0xfdfc0000},
{0xffffff7f, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
{0xffffff7f, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfdffffff},
{0xffffff7f, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfdffffff},
{0x5755d5ff, 0x55dd7757, 0x77777777, 0x5d5d5755, 0x75dd55d5, 0x5d55d755, 0x5d5d5755, 0xfdd755d5},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
{0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc0000000},
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
{0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555},
{0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x99999999, 0x65999999},
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
{0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555541, 0x55555555},
{0x66666665, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x6666653c, 0x66666666},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffff3c, 0xffffffff},
{0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x15555555, 0x555554ff, 0x55555555},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x3fffffff, 0xfffffcff, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x3fffffff, 0xfffffc7f, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffff1c, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffc3, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffc3, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffc3, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffc3, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffc3, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffc3, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffc3, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffff1c, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffff1c, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffff00, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x3fffffff, 0xfffffc77, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xcfffffff, 0xfffff177, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xf3ffffff, 0xffffc77d, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7cffffff, 0xffff179d, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x5f3fffff, 0xfffc5f5f, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xdfcfffff, 0xfff1df9f, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xd5f3ffff, 0xffc76f5f, 0xffffffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xf5fcffff, 0xff177f67, 0xffc3ffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfd5f3fff, 0xfc5dbff7, 0xff3cffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xff97cfff, 0xf1797fd9, 0xff3cffff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffd7f3ff, 0xc5d57fd5, 0xfcff3fff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7fe57cff, 0x17d9ffd6, 0xfcff3fff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7fd65cff, 0x1f65ffd9, 0xfc7f3fff},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7ffd7f3f, 0x6fd5ffd7, 0xff1cfffc},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffff7fff, 0x7ff65f3f, 0x7d9bffd9, 0xffc3fffc},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffff7fff, 0x9ff557cf, 0x755fffd5, 0xffc3fff1},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffff7fff, 0x5ff6e7cf, 0xf667ff5b, 0xffc3fff1},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x9ff55fcf, 0xfb5fff95, 0xffc3fff1},
{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfffd5fff, 0x5ff657cf, 0xf59ffd79, 0xffc3fff1},
{0xffffffff, 0xffffffff, 0xffffffff, 0xf7ffffff, 0xfffd5fff, 0xdffd77cf, 0xf55ffd55, 0xffc3fff1},
{0xffffffff, 0xffffffff, 0xffffffff, 0xd5ffffff, 0xfffd7fff, 0x7ff657cf, 0x795fff96, 0xffc3fff1},
{0xffffffff, 0xffffffff, 0xffffffff, 0xd57fffff, 0xdffd9fff, 0x7fe55f3f, 0x7ddff565, 0xff1cfffc},
{0xffffffff, 0xffffffff, 0xffffffff, 0x595f7fff, 0x77f557fd, 0xffd66f3f, 0x6d5ff576, 0xff1cfffc},
{0xffffffff, 0xffffffff, 0xffffffff, 0x9d5f7fff, 0xdff597fd, 0xff55dcff, 0x1f97f665, 0xff00ff7f},
{0xffffffff, 0xffffffff, 0xffffffff, 0x595f7fff, 0xdff75bfd, 0xfd667cff, 0x175bf757, 0xfc773f5f},
{0xffffffff, 0xffffffff, 0xffffffff, 0xd77f7fff, 0x57f597ff, 0xf55ff3ff, 0xc5d7f59f, 0xf177cf7f},
{0xffffffff, 0xffffffff, 0xffffffff, 0xfffd5fff, 0x57f677ff, 0xf7f, 0xf0000000, 0xc77df375},
{0xffffffff, 0xffffffff, 0xffffffff, 0x557d5fff, 0x67d5657f, 0xffff355f, 0xfcc125ff, 0x179d7c75},
{0xffffffff, 0xffffffff, 0xffffffff, 0x66fdbfff, 0x556d567f, 0x155d, 0x58000000, 0x5f5f5f19},};
const int title_palette[4] = {BLUE, SKY_BLUE, GRAY, WHITE};
