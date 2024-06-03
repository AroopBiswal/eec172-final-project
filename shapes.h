/*
 * shapes.h
 *
 *  Created on: May 29, 2024
 *      Author: benyo
 */

#ifndef EEC172_LAB4_SHAPES_H_
#define EEC172_LAB4_SHAPES_H_

#define NUM_SHAPE_TYPES     7
#define NUM_BLOCK_STYLES    5

extern int block_styles[NUM_BLOCK_STYLES][6][6];
extern int SHAPES[7][4][2];
extern int default_shape_styles[7];
extern int ROTATION_MAP[8][2][2];

enum SHAPE {
    LINE,
    L,
    L_REVERSE,
    Z,
    Z_REVERSE,
    SQUARE,
    T
};

enum BLOCK_STYLE {
    EMPTY,
    SOLID,
    HOLLOW,
    ACC_SOLID,
    DASHED_OUTLINE,
};

#endif /* EEC172_LAB4_SHAPES_H_ */
