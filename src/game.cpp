/*
 * game.cpp
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */


extern "C"
{
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

	#include "scrollEngine.h"

	#include <stdio.h>
	#include <stdint.h>
	#include <string.h>

	#include <assert.h>

	#include "asmstuff.h"
	#include "copper.h"
	#include "uart.h"
	#include "assets.h"

	#include "input.h"
	#include "ptplayerWrapper.h"
}

#include "unit.h"
#include "astar.h"


//volatile struct Custom custom;
//volatile struct CIA ciaa, ciab;


extern volatile struct Custom custom;
extern volatile struct CIA ciaa, ciab;

extern unsigned char leveldata[];

void gameloop();

unit testUnit;


int main()
{
	uint16_t i;

	//maustest();
	printf("Howdy %d!\n", 42);
	uart_printf("Howdy %d!\n", 42);

	if (readAssets())
		return 1;

	for (i=0;i<sizeof(mapData);i++)
	{
		mapData[i]=leveldata[i]-1;
	}



	initVideo();


	scrollX=0;
	scrollY=0;

	renderFullScreen();
	constructCopperList();

#if 1
	mt_install_cia();
	mt_init(assets->protrackerModule_alien);
	mt_mastervol(40);

	mt_Enable=1; //PTreplay darf abspielen
#endif

	mouseInit();
	gameloop();

	return 0;
}


void gameloop()
{
	int16_t toScrollY=0;
	int16_t toScrollX=0;

	int16_t mouseCursorX=0;
	int16_t mouseCursorY=0;

	testUnit.init();

	testUnit.walkTo(8,8);

	bool mouseLeftClick = false;
	bool mouseLeftClickLast = false;

	for(;;)
	{
		mouseCheck();

		mouseCursorX += mouseXDiff;
		mouseCursorY += mouseYDiff;

		if (mouseCursorX > (SCREEN_WIDTH-1))
			mouseCursorX = SCREEN_WIDTH-1;

		if (mouseCursorY > (SCREEN_HEIGHT-1))
			mouseCursorY = SCREEN_HEIGHT-1;

		if (mouseCursorX < 0)
			mouseCursorX = 0;

		if (mouseCursorY < 0)
			mouseCursorY = 0;

		waitVBlank();

		mouseLeftClick=(!(ciaa.ciapra & CIAF_GAMEPORT0));

		if (mouseLeftClick && !mouseLeftClickLast)
		{
			int tileX = (mouseCursorX - scrollX)/16;
			int tileY = (mouseCursorY - scrollY)/16;

			//uart_printf("Left Click %d %d %d %d\n", mouseCursorX, mouseCursorX,tileX,tileY);
			testUnit.walkTo(tileX,tileY);
		}

		mouseLeftClickLast = mouseLeftClick;
		if (!(custom.potinp & POTINP_DATLY_MASK))
		{
			toScrollY = -((SCREEN_HEIGHT/2) - mouseCursorY) / 10;
			toScrollX = -((SCREEN_WIDTH/2) - mouseCursorX) / 10;
			mouseSpritePtr = assets->sprite_mouseCursor2;
		}
		else
			mouseSpritePtr = assets->sprite_mouseCursor1;

		setSpriteStruct(mouseSpritePtr,mouseCursorX, mouseCursorY, 16);

		constructCopperList();

		restoreBackground();

		int i;
		for (i = 0; i < 40; i++)
		{
			if (toScrollY > 0)
			{
				scrollDown();
				toScrollY--;
			}

			if (toScrollY < 0)
			{
				scrollUp();
				toScrollY++;
			}

			if (toScrollX > 0)
			{
				scrollRight();
				toScrollX--;
			}

			if (toScrollX < 0)
			{
				scrollLeft();
				toScrollX++;
			}
		}

		/*
		static int animCnt=0;
		uint16_t* bobPtr=NULL;

		switch (animCnt/16)
		{
		case 0: bobPtr = &assets->bobunit0[0*288/2]; break;
		case 1: bobPtr = &assets->bobunit0[1*288/2]; break;
		case 2: bobPtr = &assets->bobunit0[2*288/2]; break;
		}

		animCnt++;
		if (animCnt>=3*16)
			animCnt=0;

		if (bobPtr)
			blitMaskedBob_mapCoordinate(bobPtr, 64, 64, 16, 24);
		*/
		testUnit.simulate();
		testUnit.blit();


	}
}

