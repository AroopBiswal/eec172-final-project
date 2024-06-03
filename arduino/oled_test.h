/*
 * test.h
 *
 *  Created on: Jan 27, 2024
 *      Author: rtsang
 */

#ifndef OLED_OLED_TEST_H_
#define OLED_OLED_TEST_H_

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0 // 0000 0111 1110 0000
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
#define LIME            0xB7A3
#define PINK            0xF3DE
#define SKY_BLUE        0x3DDF
#define GRAY            0x7BEF
#define ORANGE          0xFC00
// lime 58D854 01011000 11011000 01010100
//             0101111011001010
// pink F878F8 1111101111011111
//             111111111011111
// teal 3CBCFC 0011110111111111
//             00111
// gray 7C7C7C 0111101111101111
// orange FCA044 

void testfastlines(unsigned int color1, unsigned int color2);
void testdrawrects(unsigned int color);
void testfillrects(unsigned int color1, unsigned int color2);
void testfillcircles(unsigned char radius, unsigned int color);
void testdrawcircles(unsigned char radius, unsigned int color);
void testtriangles();
void testroundrects();
void testlines(unsigned int color);
void lcdTestPattern(void);
void lcdTestPattern2(void);


#endif /* OLED_OLED_TEST_H_ */
