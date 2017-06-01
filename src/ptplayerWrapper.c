/*
 * ptplayerWrapper.c
 *
 *  Created on: 31.05.2017
 *      Author: andre
 */

#include <stdint.h>
#include "ptplayerWrapper.h"

void mt_install_cia(void)
{
	//PTreplay installiert Interrupt Vektor für CIA
	asm volatile (	"clr.b d0\n"
				"suba.l a0,a0\n"
				"lea %[custom],a6\n"
				"jsr mt_install_cia\n"
		     : //no outputs
		     : [custom] "m" (custom)
		     : "a0","a6","d0");
}

void mt_init(void *mod)
{
	//PTreplay initialisiert
	asm volatile (	"move.b #0,d0\n"
			"move.l #0,a1\n"
			"lea %[custom],a6\n"
			"move.l %[module],a0\n"
			"jsr mt_init"
	     : //no outputs
	     : [custom] "m" (custom),
		   [module] "m" (mod)
	     : "a0","a6","d0","a1","a2");
}

void mt_mastervol(uint16_t masterVolume)
{
	//PTreplay Lautstärke auf 20 von 0 bis 64
	asm volatile (	"move.w %[masterVolume],d0\n"
			"jsr mt_mastervol\n"
		 : //no outputs
		 : [masterVolume] "irm" (masterVolume)
		 : "d0");
}


void mt_soundfx(void *samplePtr, uint16_t sampleLen, uint16_t samplePeriod, uint16_t sampleVolume)
{
	//PTreplay initialisiert
	asm volatile (	"move.w %[sampleLen],d0\n"
			"move.w %[samplePeriod],d1\n"
			"move.w %[sampleVolume],d2\n"
			"lea %[custom],a6\n"
			"move.l %[samplePtr],a0\n"
			"jsr mt_soundfx"
	     : //no outputs
	     : [custom] "m" (custom),
		   [samplePtr] "m" (samplePtr),
		   [samplePeriod] "irm" (samplePeriod),
		   [sampleVolume] "irm" (sampleVolume),
		   [sampleLen] "irm" (sampleLen)

	     : "a0","a6","d0","d2","d1");
}
