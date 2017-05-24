/*
 * asmstuff.h
 *
 *  Created on: 23.04.2017
 *      Author: andre
 */

#ifndef ASMSTUFF_H_
#define ASMSTUFF_H_

#include <stdint.h>
void isr_verticalBlank();

void supervisor_disableDataCache();
void MOVECtrap();
uint32_t getSR();
uint32_t supervisor_getCACR();

extern volatile uint16_t vBlankCounter;
extern uint16_t tilemap[1280/2];
extern uint8_t protrackerModule_alien[94848];
extern uint8_t testSoundEffect[26262+2];
extern uint16_t mouseCursor[16*2];
extern uint16_t testbob[12288/2];

#endif /* ASMSTUFF_H_ */
