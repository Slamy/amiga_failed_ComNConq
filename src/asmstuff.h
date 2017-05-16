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

void MOVECtrap();
uint32_t getSR();

extern volatile uint16_t vBlankCounter;
extern uint16_t tilemap[640/2];
extern uint8_t protrackerModule_alien[94848];
extern uint8_t testSoundEffect[26262];

#endif /* ASMSTUFF_H_ */
