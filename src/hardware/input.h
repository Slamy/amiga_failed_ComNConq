/*
 * input.h
 *
 *  Created on: 27.05.2017
 *      Author: andre
 */

#ifndef SRC_INPUT_H_
#define SRC_INPUT_H_


#include <stdint.h>


volatile extern int16_t mouseYDiff;
volatile extern int16_t mouseXDiff;

volatile extern int keyPress;


#define KEYCODE_1 0x01
#define KEYCODE_2 0x02
#define KEYCODE_3 0x03
#define KEYCODE_4 0x04

#define KEYCODE_Q 0x10
#define KEYCODE_W 0x11
#define KEYCODE_E 0x12
#define KEYCODE_R 0x13
#define KEYCODE_T 0x14
//#define KEYCODE_Z 0x15

#define KEYCODE_I 0x17

#define KEYCODE_A 0x20
#define KEYCODE_S 0x21
#define KEYCODE_D 0x22
#define KEYCODE_G 0x24
#define KEYCODE_H 0x25
#define KEYCODE_J 0x26
#define KEYCODE_K 0x27
#define KEYCODE_L 0x28


#define KEYCODE_NUM2	0x1E
#define KEYCODE_NUM4	0x2D
#define KEYCODE_NUM5	0x2E
#define KEYCODE_NUM6	0x2F
#define KEYCODE_NUM8	0x3E

#define POTINP_DATLY_MASK (1<<10)


void mouseInit();
void mouseCheck();
void keyboardCheck();



#endif /* SRC_INPUT_H_ */
