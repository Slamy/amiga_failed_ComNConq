
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

bool tilePassable[30];

const unsigned char buildingFootprint[]=
{
	0,1,1,0,
	1,1,1,0,
	1,1,1,1,
	0,0,0,0
};

const unsigned char buildingAreaXLut[]=
{
	0,16,32,48,
	0,16,32,48,
	0,16,32,48,
	0,16,32,48,
};

const unsigned char buildingAreaYLut[]=
{
	0,0,0,0,
	16,16,16,16,
	32,32,32,32,
	48,48,48,48,
};

UnitPool unitpool;

void visualizeBuildingArea(const uint8_t *buildingFootprint, int16_t tileX, int16_t tileY)
{
	int8_t x=0,y=0;

	uint8_t *src = &mapData[tileX + LEVELMAP_WIDTH * tileY];
	int8_t i=0;

	tileX=tileX*16;
	tileY=tileY*16;

	for (y=0;y<4;y++)
	{
		for (x=0;x<4;x++)
		{
			if (*buildingFootprint)
			{
				uint16_t* graphic = tilePassable[*src] ? assets->buildYesTile : assets->buildNoTile;
				//blitMaskedBob_mapCoordinate(graphic, tileX + (x<<4), tileY + (y<<4), 16, 16);
				blitMaskedBob_mapCoordinate(graphic, tileX + buildingAreaXLut[i], tileY + buildingAreaYLut[i], 16, 16);
			}
			i++;
			src++;
			buildingFootprint++;
		}
		src+=(LEVELMAP_WIDTH-4);
	}
}

void gameloop()
{
	int16_t toScrollY=0;
	int16_t toScrollX=0;

	int16_t mouseCursorX=0;
	int16_t mouseCursorY=0;

	AStar::init();

	memset(tilePassable, 0, sizeof(tilePassable));
	tilePassable[1] = true; //Land
	tilePassable[2] = true; //Irgendwie verkohltes Land
	tilePassable[25] = true; //Erz1
	tilePassable[26] = true; //Erz2
	tilePassable[27] = true; //Raffinerie-Platz

	Unit* selectedUnit = NULL;

	new (unitpool) Harvester(3,3);
	new (unitpool) Harvester(4,3);
	new (unitpool) Harvester(6,3);

	new (unitpool) Tank(6,7);
	new (unitpool) Harvester(6,8);
	new (unitpool) Tank(16,20);

	renderFullScreen();
	constructCopperList();

	//testUnit.walkTo(8,8);

	bool mouseLeftClick = false;
	bool mouseLeftClickLast = false;
	bool mouseRightClick = false;
	bool mouseRightClickLast = false;
	int16_t mouseRightClickCnt = 0;

	int16_t rightPressMouseCursorX=0;
	int16_t rightPressMouseCursorY=0;

	uart_printf("Available Chip Mem %d before mainloop\n",AvailMem(MEMF_CHIP));
	uart_printf("Available Fast Mem %d before mainloop\n",AvailMem(MEMF_FAST));

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
					mt_soundfx(assets->sfx_ackno, sizeof(assets->sfx_ackno)/2, 322, 40);
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
			//uart_printf("Left Click %d %d %d %d\n", mouseCursorX, mouseCursorX,mouseTileX,mouseTileY);

			Unit *unitUnderCursor = unitpool.unitAt(mouseTileX,mouseTileY);
			if (unitUnderCursor)
			{
				selectedUnit = unitUnderCursor;
				mt_soundfx(assets->sfx_await1, sizeof(assets->sfx_await1)/2, 322, 40);
			}
			else if (selectedUnit)
			{
				if (selectedUnit->walkTo(mouseTileX,mouseTileY))
				{
					mt_soundfx(assets->sfx_ackno, sizeof(assets->sfx_ackno)/2, 322, 40);
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
			else if (Game::unitpool.unitAt(mouseTileX,mouseTileY))
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

			//restoreBackground();

			int i;
			for (i = 0; i < 16; i++)
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

			toScrollY=0;
			toScrollX=0;

			custom.color[0] = 0x0770;
			unitpool.simulate();
			custom.color[0] = 0x0000;
			blitAlteredTiles();

			//unitpool.blit();

			//visualizeBuildingArea(buildingFootprint,mouseTileX, mouseTileY);


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

	printf("sizeof Game::UnitPool %d \n",sizeof(Game::UnitPool));
	printf("sizeof Game::Unit %d \n",sizeof(Game::Unit));
	printf("sizeof Astar %d \n",sizeof(AStar));
	printf("sizeof AStarPath %d \n",sizeof(AStarPath));
	printf("sizeof Harvester %d \n",sizeof(Harvester));

	printf("Available Chip Mem %d \n",AvailMem(MEMF_CHIP));
	printf("Available Fast Mem %d \n",AvailMem(MEMF_FAST));

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



void blitTile(uint8_t tileid, uint16_t *dest, int8_t x, int8_t y)
{
	/*
	if (x<0)
		return;
	if (y<0)
		return;
	*/

	uint16_t *src = assets->tilemap + SCREEN_DEPTH * 16 * tileid;
	//uart_printf("b %p %p %d %d\n",dest, src,x,y);
#if 1
	while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

	custom.bltcon0 = BC0F_SRCA | A_TO_D | BC0F_DEST;
	custom.bltapt = src;
	custom.bltdpt = dest;
	custom.bltamod = 0;
	custom.bltdmod = (FRAMEBUFFER_WIDTH - 16)/8;
	custom.bltafwm = 0xffff;
	custom.bltalwm = 0xffff;
	custom.bltsize = ((5*16) << HSIZEBITS) | 1; //starts blitter. 16 x 16 Pixel

#else
	int i;

	if (dest < bitmap)
		return;

	for (i=0; i<5*16; i++)
	{
		if (dest < bitmap)
		{
			uart_printf("Illegale Speicheroperation dest < bitmap\n");
			for(;;);
		}
		if (dest >= &bitmap[FRAMEBUFFER_SIZE/2])
		{
			uart_printf("Illegale Speicheroperation dest >= &bitmap[FRAMEBUFFER_SIZE/2]\n");
			for(;;);
		}

		*dest = *src;
		src++;
		dest+=FRAMEBUFFER_WIDTH/16;
	}
#endif


	if (x>=0 && y>=0)
	{
		Game::Unit *u = Game::unitpool.unitAt(x,y);
		if (u)
		{
			uart_printf("%d %d present\n", x,y);
			u->blit();
		}
	}

}



//Sorgt dafür, dass exception demangling nicht eingelinkt wird!
//https://developer.mbed.org/forum/platform-32-ST-Nucleo-L152RE-community/topic/4802/?page=2#comment-25593
namespace __gnu_cxx {
    void __verbose_terminate_handler() {
    	uart_puts((const char*)"NOOO1!!\n");
    	for(;;);
    }
}
extern "C" void __cxa_pure_virtual(void);
extern "C" void __cxa_pure_virtual(void) {
	uart_puts((const char*)"NOOO2!!\n");
	for(;;);
}

