/*
 * input.c
 *
 *  Created on: 27.05.2017
 *      Author: andre
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
//#include <intuition/intuition.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/blit.h>
#include <hardware/cia.h>
#include <graphics/display.h>
#include <exec/interrupts.h>

#include "input.h"
#include "uart.h"

extern volatile struct Custom custom;
extern volatile struct CIA ciaa, ciab;


volatile int keyPress = -1;

void keyboardCheck()
{
	//Check zuerst, ob ein Interrupt eingetragen wurde.
	UBYTE cia_interrupt=ciaa.ciaicr;
	if (cia_interrupt & CIAICRF_SP)
	{
		uint8_t keycode = ~ciaa.ciasdr;
		uint8_t key = (keycode >> 1) | (keycode << 7); /* ROR 1 */

		if (!(key & 0x80))
			keyPress=key;
		else
			keyPress=-1;

		ciaa.ciacra = CIACRAF_SPMODE | CIACRAF_START ; /* start the handshake */

		uint8_t waitFor = ciaa.ciatalo + 70;
		//uart_printf("Keycode: %x %x %x %x\n",key,keycode, cia_interrupt, keyPress);

		while (waitFor != ciaa.ciatalo)
		{
			//uart_printf("Warte: %x %x\n",ciaa.ciatahi,ciaa.ciatalo);
		}

		//uart_printf("Warte: %x\n",keycode);
		ciaa.ciacra = 0;
	}
}

uint16_t mouseDataLast;
volatile int16_t mouseYDiff=0;
volatile int16_t mouseXDiff=0;

void mouseInit()
{
	mouseDataLast = custom.joy0dat;

	//uart_printf("mouseDataLast %x\n",mouseDataLast);
}

void mouseCheck()
{
	uint16_t mouseData = custom.joy0dat;

	/*
	 * mouseY ist ein unsigned byte counter.
	 * 0x10 zu 0x15 wären 5 Schritte, weil 0x15 - 0x10 = 5
	 * 0xfe zu 0x03 sind aber auch 5 Schritte, weil es zum Overlap kommt.
	 * 0x03 - 0xfe sind aber -0x251
	 *
	 */

	mouseYDiff = ((mouseData>>8) - (mouseDataLast>>8));
	mouseXDiff = ((mouseData & 0xff) - (mouseDataLast & 0xff));

	//uart_printf("mouseData %x %x %d %d\n",mouseData, mouseDataLast, mouseXDiff, mouseYDiff);

	if (mouseYDiff >= 128)
		mouseYDiff -= 256;
	else if (mouseYDiff <= -128)
		mouseYDiff += 256;

	if (mouseXDiff >= 128)
		mouseXDiff -= 256;
	else if (mouseXDiff <= -128)
		mouseXDiff += 256;

	mouseDataLast = mouseData;
}

