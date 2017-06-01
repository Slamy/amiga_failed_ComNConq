/*
 * ptplayerWrapper.h
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */

#ifndef SRC_PTPLAYERWRAPPER_H_
#define SRC_PTPLAYERWRAPPER_H_

extern volatile char mt_Enable asm ("mt_Enable");


extern volatile struct Custom custom;


void mt_install_cia(void);
void mt_init(void *mod);
void mt_mastervol(uint16_t masterVolume);
void mt_soundfx(void *samplePtr, uint16_t sampleLen, uint16_t samplePeriod, uint16_t sampleVolume);

#endif /* SRC_PTPLAYERWRAPPER_H_ */
