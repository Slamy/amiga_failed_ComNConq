
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

#include "Unit.h"
#include "Harvester.h"
#include "AStar.h"


//volatile struct Custom custom;
//volatile struct CIA ciaa, ciab;

int __nocommandline = 1;
int __oslibversion = 34;

extern volatile struct Custom custom;
extern volatile struct CIA ciaa, ciab;

extern unsigned char leveldata[];

void gameloop();

Harvester testUnit;
AStar testAStar;

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

	//Color Palette
	custom.color[0] = 0x0000;
	custom.color[1] = 0x0121;
	custom.color[2] = 0x0421;
	custom.color[3] = 0x0322;
	custom.color[4] = 0x0710;
	custom.color[5] = 0x0a00;
	custom.color[6] = 0x0641;
	custom.color[7] = 0x0353;
	custom.color[8] = 0x0e21;
	custom.color[9] = 0x0362;
	custom.color[10] = 0x0684;
	custom.color[11] = 0x0c65;
	custom.color[12] = 0x0682;
	custom.color[13] = 0x0888;
	custom.color[14] = 0x0e71;
	custom.color[15] = 0x069b;
	custom.color[16] = 0x0991;
	custom.color[17] = 0x00a0;
	custom.color[18] = 0x00e0;
	custom.color[19] = 0x0fff;
	custom.color[20] = 0x0aaa;
	custom.color[21] = 0x09bd;
	custom.color[22] = 0x0ace;
	custom.color[23] = 0x0dcb;
	custom.color[24] = 0x0ddd;
	custom.color[25] = 0x0ed4;
	custom.color[26] = 0x0eee;
	custom.color[27] = 0x0fea;
	custom.color[28] = 0x0000;
	custom.color[29] = 0x0000;
	custom.color[30] = 0x0000;
	custom.color[31] = 0x0000;


	gameloop();

	return 0;
}


void gameloop()
{
	int16_t toScrollY=0;
	int16_t toScrollX=0;

	int16_t mouseCursorX=0;
	int16_t mouseCursorY=0;

	AStar::init();

	testUnit.init();

	//testUnit.walkTo(8,8);

	bool mouseLeftClick = false;
	bool mouseLeftClickLast = false;

	for(;;)
	{
		keyboardCheck();

		switch (keyPress)
		{
		case KEYCODE_H:
			testUnit.harvest(testAStar);
			keyPress=-1;
		}

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

		mouseLeftClick=(!(ciaa.ciapra & CIAF_GAMEPORT0));

		if (mouseLeftClick && !mouseLeftClickLast)
		{
			int tileX = (mouseCursorX + scrollX)/16;
			int tileY = (mouseCursorY + scrollY)/16;

			uart_printf("Left Click %d %d %d %d\n", mouseCursorX, mouseCursorX,tileX,tileY);
			testUnit.walkTo(tileX,tileY,testAStar);
			//alterTile(tileX, tileY, 0);
		}


		if (vBlankReached())
		{
			if (!(custom.potinp & POTINP_DATLY_MASK))
			{
				toScrollY = -((SCREEN_HEIGHT/2) - mouseCursorY) / 10;
				toScrollX = -((SCREEN_WIDTH/2) - mouseCursorX) / 10;
				mouseSpritePtr = assets->sprite_mouseCursor2;
				setSpriteStruct(mouseSpritePtr,mouseCursorX, mouseCursorY, 18);
			}
			else
			{
				mouseSpritePtr = assets->sprite_mouseCursor1;
				setSpriteStruct(mouseSpritePtr,mouseCursorX, mouseCursorY, 16);
			}



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

			testUnit.simulate(testAStar);
			blitAlteredTiles();

			testUnit.blit();
		}

		mouseLeftClickLast = mouseLeftClick;


	}
}


