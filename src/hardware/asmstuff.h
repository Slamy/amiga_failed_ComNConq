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

#endif /* ASMSTUFF_H_ */
