/*
 * game_helpers.h
 *
 *  Created on: Jun 2, 2024
 *      Author: benyo
 */

#ifndef EEC172_FINAL_PROJECT_GAME_HELPERS_H_
#define EEC172_FINAL_PROJECT_GAME_HELPERS_H_

#include <stdint.h>

extern const uint32_t title_data[95][8];
extern const int title_palette[4];

int getBrightnessScaledColor(int color, int step, int max_step);

void getRGBComponents(int color, int *r_dest, int *g_dest, int *b_dest);

int combineRGBComponents(int r, int g, int b);

int ProcessReadCommand(char *pcInpString, unsigned char* RdDataBuf);

void DisplayBuffer(unsigned char *pucDataBuf, unsigned char ucLen);

void myDrawChar(int x, int y, unsigned char c,
			    unsigned int color, unsigned char size);

int interpolateNumber(int start, int end, int step, int max_steps);

#endif /* EEC172_FINAL_PROJECT_GAME_HELPERS_H_ */
