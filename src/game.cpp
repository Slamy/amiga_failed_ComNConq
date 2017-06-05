
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
#include "Tank.h"
#include "AStar.h"


//volatile struct Custom custom;
//volatile struct CIA ciaa, ciab;

int __nocommandline = 1;
int __oslibversion = 34;

extern volatile struct Custom custom;
extern volatile struct CIA ciaa, ciab;

extern unsigned char leveldata[];

void gameloop();


namespace Game
{

UnitPool unitpool;


void gameloop()
{
	int16_t toScrollY=0;
	int16_t toScrollX=0;

	int16_t mouseCursorX=0;
	int16_t mouseCursorY=0;

	AStar::init();

	Unit* selectedUnit = NULL;

	new (unitpool) Harvester(3,3);
	new (unitpool) Harvester(4,3);
	new (unitpool) Harvester(6,3);

	new (unitpool) Tank(6,7);
	new (unitpool) Harvester(6,8);
	new (unitpool) Tank(16,20);

	//testUnit.walkTo(8,8);

	bool mouseLeftClick = false;
	bool mouseLeftClickLast = false;
	bool mouseRightClick = false;
	bool mouseRightClickLast = false;
	int16_t mouseRightClickCnt = 0;

	int16_t rightPressMouseCursorX=0;
	int16_t rightPressMouseCursorY=0;

	uart_printf("Available Chip Mem %d \n",AvailMem(MEMF_CHIP));
	uart_printf("Available Fast Mem %d \n",AvailMem(MEMF_FAST));

	for(;;)
	{
		keyboardCheck();

		switch (keyPress)
		{
		case KEYCODE_H:
			if (selectedUnit)
			{
				if (selectedUnit->specialAction())
				{
					mt_soundfx(assets->sfx_ackno, sizeof(assets->sfx_ackno)/2, 322, 64);
				}
			}

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

		int mouseTileX = (mouseCursorX + scrollX)/16;
		int mouseTileY = (mouseCursorY + scrollY)/16;

		mouseLeftClick = (!(ciaa.ciapra & CIAF_GAMEPORT0));
		mouseRightClick = (!(custom.potinp & POTINP_DATLY_MASK));

		if (mouseLeftClick && !mouseLeftClickLast)
		{
			uart_printf("Left Click %d %d %d %d\n", mouseCursorX, mouseCursorX,mouseTileX,mouseTileY);

			Unit *unitUnderCursor = Unit::unitAt(mouseTileX,mouseTileY);
			if (unitUnderCursor)
			{
				selectedUnit = unitUnderCursor;
				mt_soundfx(assets->sfx_await1, sizeof(assets->sfx_await1)/2, 322, 64);
			}
			else if (selectedUnit)
			{
				if (selectedUnit->walkTo(mouseTileX,mouseTileY))
				{
					mt_soundfx(assets->sfx_ackno, sizeof(assets->sfx_ackno)/2, 322, 64);
				}
			}

			//alterTile(tileX, mouseTileY, 0);
		}

		if (mouseRightClick && !mouseRightClickLast) //Right Press Event
		{
			rightPressMouseCursorX = mouseCursorX;
			rightPressMouseCursorY = mouseCursorY;
		}

		if (!mouseRightClick && mouseRightClickLast) //Right Release Event
		{
			if ((abs(rightPressMouseCursorX - mouseCursorX) < 5) && (abs(rightPressMouseCursorY - mouseCursorY) < 5))
			{
				//uart_printf("Deselect!\n");
				selectedUnit = NULL;
			}

		}

		if (vBlankReached())
		{
			if (mouseRightClick)
				mouseRightClickCnt++;
			else
				mouseRightClickCnt=0;

			if (mouseRightClickCnt > 3)
			{

				//toScrollY = -((SCREEN_HEIGHT/2) - mouseCursorY) / 10;
				//toScrollX = -((SCREEN_WIDTH/2) - mouseCursorX) / 10;

				toScrollX = (mouseCursorX - rightPressMouseCursorX) / 8;
				toScrollY = (mouseCursorY - rightPressMouseCursorY) / 8;
				//Scroll-Mauszeiger
				mouseSpritePtr = assets->sprite_mouseCursor2;
				setSpriteStruct(mouseSpritePtr,mouseCursorX-16/2, mouseCursorY-18/2, 18);
			}
			else if (Unit::unitAt(mouseTileX,mouseTileY))
			{
				//auswählender Mauszeiger
				mouseSpritePtr = assets->sprite_mouseCursor3;
				setSpriteStruct(mouseSpritePtr,mouseCursorX-16/2, mouseCursorY-16/2, 16);
			}
			else
			{
				//normaler Mauszeiger
				if (selectedUnit)
				{
					mouseSpritePtr = assets->sprite_mouseCursor4;
					setSpriteStruct(mouseSpritePtr,mouseCursorX-16/2, mouseCursorY-16/2, 16);
				}
				else
				{
					mouseSpritePtr = assets->sprite_mouseCursor1;
					setSpriteStruct(mouseSpritePtr,mouseCursorX, mouseCursorY, 16);
				}


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

			unitpool.simulate();

			blitAlteredTiles();

			unitpool.blit();
		}

		mouseLeftClickLast = mouseLeftClick;
		mouseRightClickLast = mouseRightClick;

	}
}


}


int main()
{
	uint16_t i;

	//maustest();
	printf("Howdy %d!\n", 42);
	uart_printf("Howdy %d!\n", 42);

	uart_printf("Available Chip Mem %d \n",AvailMem(MEMF_CHIP));
	uart_printf("Available Fast Mem %d \n",AvailMem(MEMF_FAST));

	uart_printf("sizeof Game::Unit %d \n",sizeof(Game::Unit));
	uart_printf("sizeof Astar %d \n",sizeof(AStar));
	uart_printf("sizeof AStarPath %d \n",sizeof(AStarPath));
	uart_printf("sizeof Harvester %d \n",sizeof(Harvester));

	/*
	char *buf = (char*)&units[0];
	Harvester *u = new (buf) Harvester;
	Harvester &testUnit = *u;

	testUnit.init();
	*/

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
	mt_mastervol(32);

	mt_Enable=1; //PTreplay darf abspielen
#endif

	mouseInit();

	//Color Palette
	custom.color[0] = 0x0000;
	custom.color[1] = 0x0121;
	custom.color[2] = 0x0421;
	custom.color[3] = 0x0322;
	custom.color[4] = 0x0710;
	custom.color[5] = 0x0641;
	custom.color[6] = 0x0353;
	custom.color[7] = 0x0ace;
	custom.color[8] = 0x0362;
	custom.color[9] = 0x0684;
	custom.color[10] = 0x0000;
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
	custom.color[21] = 0x0a00;
	custom.color[22] = 0x0e00;
	custom.color[23] = 0x09bd;
	custom.color[24] = 0x0dcb;
	custom.color[25] = 0x0ddd;
	custom.color[26] = 0x0ed4;
	custom.color[27] = 0x0eee;
	custom.color[28] = 0x0fea;
	custom.color[29] = 0x0000;
	custom.color[30] = 0x0000;
	custom.color[31] = 0x0000;



	Game::gameloop();

	return 0;
}


