/*
 * scrollEngine.c
 *
 *  Created on: 29.04.2017
 *      Author: andre
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/blit.h>
#include <graphics/display.h>
#include <exec/interrupts.h>


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <assert.h>

#include "asmstuff.h"
#include "copper.h"
#include "uart.h"

#include "scrollEngine.h"


extern struct Custom custom;


int scrollXDelay=0;
int scrollXWord=0;

int scrollYLine=0;
int scrollY=0;
int scrollX=0;


struct ScreenRowTile
{
	uint16_t *dest;
	union
	{
		uint8_t *scrollDownTile;
		uint8_t *tile1;
	};

	union
	{
		uint8_t *scrollUpTile;
		uint8_t *tile2;
	};
};

struct ScreenColumnTile
{
	uint16_t *dest;
	union
	{
		uint8_t *scrollLeftTile;
		uint8_t *tile1;
	};

	union
	{
		uint8_t *scrollRightTile;
		uint8_t *tile2;
	};
};


//Pointer die beim Scrollen immer mitgeführt werden müssen
uint16_t *firstFetchWord;
uint16_t *lastFetchWord;

struct ScreenRowTile hiddenBlitRowLeft;
struct ScreenRowTile hiddenBlitRowRight;
struct ScreenRowTile hiddenBlitRowCurrent;

struct ScreenColumnTile hiddenBlitColumnTop;
struct ScreenColumnTile hiddenBlitColumnBottom;
struct ScreenColumnTile hiddenBlitColumnCurrent;

#define WRAPPED_TILE_MOVEPTR_DOWN(tile) \
	tile.dest += FRAMEBUFFER_LINE_PITCH/2*16; \
	if (tile.dest >= lastFetchWord) \
		tile.dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT; \
	tile.tile1+=LEVELMAP_WIDTH; \
	tile.tile2+=LEVELMAP_WIDTH; \

#define WRAPPED_TILE_MOVEPTR_UP(tile) \
	tile.dest -= FRAMEBUFFER_LINE_PITCH/2*16; \
	if (tile.dest < firstFetchWord) \
		tile.dest += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT; \
	tile.tile1 -= LEVELMAP_WIDTH; \
	tile.tile2 -= LEVELMAP_WIDTH; \

#define TILE_MOVEPTR_RIGHT(tile) \
	tile.dest++; \
	tile.tile1++; \
	tile.tile2++; \

#define TILE_MOVEPTR_LEFT(tile) \
	tile.dest--; \
	tile.tile1--; \
	tile.tile2--; \



uint16_t *bitmap=NULL;
uint16_t *copperlist=NULL;
uint16_t *spriteStruct=NULL;
uint16_t *spriteNullStruct=NULL;
uint16_t *tilemapChip=NULL;

const int displayWindowVStart=44;
const int displayWindowVStop=300;
const int displayWindowHStart=128; //eigentlich 129.... aber Amiga Bugs in Hardware...?


uint8_t mapData[LEVELMAP_WIDTH*LEVELMAP_HEIGHT];
uint8_t *topLeftMapTile;

#define DEBUG_VERTICAL_SCROLL	2
#define DEBUG_HORIZONTAL_SCROLL	2

#define DEBUG_PRINT

void constructCopperList()
{
	int i;
	i=0;

	custom.bplcon1 = scrollXDelay | (scrollXDelay<<4);

#ifdef DEBUG_VERTICAL_SCROLL
	int backup_scrollYLine = scrollYLine;

	scrollYLine+=(16*DEBUG_VERTICAL_SCROLL);
	if (scrollYLine >= FRAMEBUFFER_HEIGHT)
		scrollYLine -= FRAMEBUFFER_HEIGHT;

#endif
#ifdef DEBUG_HORIZONTAL_SCROLL
	int backup_scrollXWord = scrollXWord;

	scrollXWord+=DEBUG_HORIZONTAL_SCROLL;
#endif

	//Sprites
	COPPER_WRITE_32(SPR0PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR1PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR2PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR3PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR4PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR5PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR6PTH, spriteStruct);
	COPPER_WRITE_32(SPR7PTH, spriteNullStruct);

	COPPER_WAIT_VERTICAL(displayWindowVStart-4)


	//Am Anfang des Screens mit vertikalem Scrolling offset
	COPPER_WRITE_32(BPL1PTH, &bitmap[scrollXWord+(scrollYLine*5 + 0)*FRAMEBUFFER_WIDTH/16]);
	COPPER_WRITE_32(BPL2PTH, &bitmap[scrollXWord+(scrollYLine*5 + 1)*FRAMEBUFFER_WIDTH/16]);
	COPPER_WRITE_32(BPL3PTH, &bitmap[scrollXWord+(scrollYLine*5 + 2)*FRAMEBUFFER_WIDTH/16]);
	COPPER_WRITE_32(BPL4PTH, &bitmap[scrollXWord+(scrollYLine*5 + 3)*FRAMEBUFFER_WIDTH/16]);
	COPPER_WRITE_32(BPL5PTH, &bitmap[scrollXWord+(scrollYLine*5 + 4)*FRAMEBUFFER_WIDTH/16]);

	//copperlist[i++]=0x2C01;
	//copperlist[i++]=0xFFFE;


	if (scrollYLine > FRAMEBUFFER_HEIGHT-SCREEN_HEIGHT)
	//if (0)
	{
		//Bei der Scroll-Position....

		if (displayWindowVStart+(FRAMEBUFFER_HEIGHT-scrollYLine) > 255)
		{
			//Warte auf PAL Copper Wrap.... wasn kack...
			copperlist[i++]=0xffdf;
			copperlist[i++]=0xfffe;
		}

		COPPER_WAIT_VERTICAL(displayWindowVStart+(FRAMEBUFFER_HEIGHT-scrollYLine));


		//An den Anfang des Buffers setzen, zusammen mit horizontalem Scrolling
		COPPER_WRITE_32(BPL1PTH, &bitmap[scrollXWord+0*FRAMEBUFFER_WIDTH/16]);
		COPPER_WRITE_32(BPL2PTH, &bitmap[scrollXWord+1*FRAMEBUFFER_WIDTH/16]);
		COPPER_WRITE_32(BPL3PTH, &bitmap[scrollXWord+2*FRAMEBUFFER_WIDTH/16]);
		COPPER_WRITE_32(BPL4PTH, &bitmap[scrollXWord+3*FRAMEBUFFER_WIDTH/16]);
		COPPER_WRITE_32(BPL5PTH, &bitmap[scrollXWord+4*FRAMEBUFFER_WIDTH/16]);

		if (displayWindowVStart+(FRAMEBUFFER_HEIGHT-scrollYLine) <= 255)
		{
			//Warte auf PAL Copper Wrap.... wasn kack...
			copperlist[i++]=0xffdf;
			copperlist[i++]=0xfffe;
		}
	}
	else
	{
		//Warte auf PAL Copper Wrap.... wasn kack...
		copperlist[i++]=0xffdf;
		copperlist[i++]=0xfffe;


	}
	COPPER_WAIT_VERTICAL(displayWindowVStop+1)


	copperlist[i++]=INTREQ; //Löse Interrupt aus
	copperlist[i++]=0x8010;

	//Ende der Liste
	copperlist[i++]=0xffff;
	copperlist[i++]=0xfffe;

#ifdef DEBUG_VERTICAL_SCROLL
	scrollYLine = backup_scrollYLine;
#endif
#ifdef DEBUG_HORIZONTAL_SCROLL
	scrollXWord=backup_scrollXWord;
#endif
}


void initVideo()
{
	int i;
	bitmap=AllocMem(FRAMEBUFFER_SIZE, MEMF_CHIP | MEMF_CLEAR);
	assert(bitmap);
	copperlist=AllocMem(300, MEMF_CHIP);
	assert(copperlist);
	spriteStruct=AllocMem(200, MEMF_CHIP);
	assert(spriteStruct);
	spriteNullStruct=AllocMem(32, MEMF_CHIP);
	assert(spriteNullStruct);

	custom.dmacon=DMAF_ALL;

	Disable(); //OS abschalten

	uint16_t spriteX = displayWindowHStart;
	uint16_t spriteYStart = displayWindowVStart + 1;
	uint16_t spriteYEnd = spriteYStart + 2;

	//spriteX = 0x90;

	i=0;
	spriteStruct[i++] = (spriteYStart<<8) | (spriteX>>1);
	spriteStruct[i++] = (spriteYEnd<<8) | (((spriteYStart>>8)&1)<<2) | (((spriteYEnd>>8)&1)<<1) | (spriteX&1);
	spriteStruct[i++]=0x0f0f;	spriteStruct[i++]=0x00ff;
	spriteStruct[i++]=0xf0f0;	spriteStruct[i++]=0xff00;

	spriteX = displayWindowHStart +1;
	spriteYStart = displayWindowVStart + 4;
	spriteYEnd = spriteYStart + 2;

	spriteStruct[i++] = (spriteYStart<<8) | (spriteX>>1);
	spriteStruct[i++] = (spriteYEnd<<8) | (((spriteYStart>>8)&1)<<2) | (((spriteYEnd>>8)&1)<<1) | (spriteX&1);
	spriteStruct[i++]=0x0f0f;	spriteStruct[i++]=0x00ff;
	spriteStruct[i++]=0xf0f0;	spriteStruct[i++]=0xff00;

	spriteX = displayWindowHStart + SCREEN_WIDTH - 16 -1;
	spriteYStart = displayWindowVStart + 7;
	spriteYEnd = spriteYStart + 2;

	spriteStruct[i++] = (spriteYStart<<8) | (spriteX>>1);
	spriteStruct[i++] = (spriteYEnd<<8) | (((spriteYStart>>8)&1)<<2) | (((spriteYEnd>>8)&1)<<1) | (spriteX&1);
	spriteStruct[i++]=0x0f0f;	spriteStruct[i++]=0x00ff;
	spriteStruct[i++]=0xf0f0;	spriteStruct[i++]=0xff00;

	spriteX = displayWindowHStart + SCREEN_WIDTH - 16;
	spriteYStart = displayWindowVStart + 10;
	spriteYEnd = spriteYStart + 2;

	spriteStruct[i++] = (spriteYStart<<8) | (spriteX>>1);
	spriteStruct[i++] = (spriteYEnd<<8) | (((spriteYStart>>8)&1)<<2) | (((spriteYEnd>>8)&1)<<1) | (spriteX&1);
	spriteStruct[i++]=0x0f0f;	spriteStruct[i++]=0x00ff;
	spriteStruct[i++]=0xf0f0;	spriteStruct[i++]=0xff00;


	spriteStruct[i++]=0;		spriteStruct[i++]=0;


	spriteNullStruct[0]=0;
	spriteNullStruct[1]=0;
	spriteNullStruct[2]=0;
	spriteNullStruct[3]=0;

	custom.bplcon0=COLORON | (SCREEN_DEPTH<<PLNCNTSHFT); //Two bitplanes, enable composite color
	custom.bplcon1=0x0000; //horizontal scroll = 0


	custom.bpl1mod=FRAMEBUFFER_LINE_MODULO-EXTRA_FETCH_WORDS*2; //modulo 0 for odd bitplanes
	custom.bpl2mod=FRAMEBUFFER_LINE_MODULO-EXTRA_FETCH_WORDS*2; //modulo 0 for even bitplanes


	//siehe http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node002C.html
	//Normaler Display Fetch wird verwendet.
	//custom.ddfstrt=0x38;	//Linke Kante für Display Fetch
	custom.ddfstrt=0x30;	//Linke Kante für Display Fetch
	custom.ddfstop=208;		//Rechte Kante für Display Fetch

	//siehe http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node002E.html

	//Definiere NTSC Display Window 320 x 200
	//custom.diwstrt=0x2C81; //Display Window Start bei Vertikal 44, Horizontal 129
	//custom.diwstop=0xF4C1; //Ende bei Vertikal 244, Horizontal 449

	//Definiere PAL Display Window 320 x 256
	custom.diwstrt=0x2C81; //Display Window Start bei Vertikal 44, Horizontal 129
	custom.diwstop=0x2CC1; //Ende bei Vertikal 300, Horizontal 449


	//Farbpalette

	custom.color[0] = 0x0511;
	custom.color[1] = 0x0421;
	custom.color[2] = 0x0322;
	custom.color[3] = 0x0710;
	custom.color[4] = 0x0a00;
	custom.color[5] = 0x0233;
	custom.color[6] = 0x0135;
	custom.color[7] = 0x0242;
	custom.color[8] = 0x0342;
	custom.color[9] = 0x0641;
	custom.color[10] = 0x0353;
	custom.color[11] = 0x0355;
	custom.color[12] = 0x0e21;
	custom.color[13] = 0x0363;
	custom.color[14] = 0x0654;
	custom.color[15] = 0x0563;
	custom.color[16] = 0x0362;
	custom.color[17] = 0x0468;
	custom.color[18] = 0x0471;
	custom.color[19] = 0x0575;
	custom.color[20] = 0x0577;
	custom.color[21] = 0x0684;
	custom.color[22] = 0x0c65;
	custom.color[23] = 0x0682;
	custom.color[24] = 0x0e71;
	custom.color[25] = 0x069b;
	custom.color[26] = 0x0799;
	custom.color[27] = 0x0991;
	custom.color[28] = 0x0998;
	custom.color[29] = 0x0b96;
	custom.color[30] = 0x0d90;
	custom.color[31] = 0x0dcb;



	//topLeftWord = bitmap;
	//firstFetchWord = topLeftWord-EXTRA_FETCH_WORDS;

	constructCopperList();

	custom.cop1lc = (uint32_t)copperlist;
	custom.copjmp1 = 0;

	custom.dmacon=DMAF_SETCLR | DMAF_COPPER | DMAF_RASTER | DMAF_MASTER | DMAF_BLITTER | DMAF_SPRITE | DMAF_BLITHOG;

	//renderFullScreen();

	constructCopperList();

	//memset(bitmap,0x55,FRAMEBUFFER_SIZE);
}

static inline void blitTile(uint8_t tileid, uint16_t *dest)
{
	uint16_t *src = tilemapChip + SCREEN_DEPTH * 16 * tileid;

	//uart_printf("b %p\n",dest);
	custom.bltcon0 = BC0F_SRCA | A_TO_D | BC0F_DEST;
	custom.bltapt = src;
	custom.bltdpt = dest;
	custom.bltamod = 0;
	custom.bltdmod = (FRAMEBUFFER_WIDTH - 16)/8;
	custom.bltafwm = 0xffff;
	custom.bltalwm = 0xffff;
	custom.bltsize = ((5*16) << HSIZEBITS) | 1; //starts blitter. 16 x 16 Pixel

	while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter
}

uint8_t scrollRightBlitFinished=0;
uint8_t scrollLeftBlitFinished=0;

void scrollUp()
{
	int i;
#ifdef DEBUG_PRINT
	uart_printf("%s %d %d\n",__func__,scrollY, scrollYLine);
#endif

#if 1
	if (scrollY == 0)
		return;

	custom.color[0] = 0x0FFF;

	scrollY--;

	scrollYLine--;
	if (scrollYLine < 0)
		scrollYLine = FRAMEBUFFER_HEIGHT-1;

	if ((scrollY & 0xf)==0xf)
	{
#ifdef DEBUG_PRINT
		uart_printf("(scrollY & 0xf)==0xf\n");
#endif

		topLeftMapTile -= LEVELMAP_WIDTH;

		WRAPPED_TILE_MOVEPTR_UP(hiddenBlitRowLeft)

		hiddenBlitRowRight.dest = hiddenBlitRowLeft.dest + 21;
		hiddenBlitRowRight.scrollDownTile = hiddenBlitRowLeft.scrollDownTile + 21;
		hiddenBlitRowRight.scrollUpTile = hiddenBlitRowLeft.scrollUpTile + 21;

		hiddenBlitRowCurrent.dest = hiddenBlitRowRight.dest + 1;
		hiddenBlitRowCurrent.scrollDownTile = hiddenBlitRowRight.scrollDownTile + 1;
		hiddenBlitRowCurrent.scrollUpTile = hiddenBlitRowRight.scrollUpTile + 1;


		//blitTile(*hiddenBlitColumnTop.scrollRightTile, hiddenBlitColumnTop.dest);

		WRAPPED_TILE_MOVEPTR_UP(hiddenBlitColumnTop);
		WRAPPED_TILE_MOVEPTR_UP(hiddenBlitColumnBottom);
		WRAPPED_TILE_MOVEPTR_UP(hiddenBlitColumnCurrent);

		if (!scrollRightBlitFinished)
			blitTile(*hiddenBlitColumnCurrent.scrollRightTile, hiddenBlitColumnCurrent.dest);

	}

	//Bei jedem Scroll-Vorgang um einen Pixel ist es bei 22 Tiles in der Breite sinnvoll, direkt 2 Tiles zu zeichnen.

	if ((scrollY & 0xf) < 0xf)
	{
		for (i=0;i<2;i++) //2 Tiles gleichzeitig
		{
			if (hiddenBlitRowCurrent.dest > hiddenBlitRowLeft.dest)
			{
				hiddenBlitRowCurrent.dest--;
				hiddenBlitRowCurrent.scrollDownTile--;
				hiddenBlitRowCurrent.scrollUpTile--;


#ifdef DEBUG_PRINT
				uart_printf("Blit tile : %d %d\n",(hiddenBlitRowCurrent.scrollUpTile-mapData)/LEVELMAP_WIDTH,(hiddenBlitRowCurrent.scrollUpTile-mapData)%LEVELMAP_WIDTH);
#endif
				blitTile(*hiddenBlitRowCurrent.scrollUpTile, hiddenBlitRowCurrent.dest);
			}
		}

	}
	else
	{
#ifdef DEBUG_PRINT
		uart_printf("Scroll without blit\n");
#endif
	}

#endif

	custom.color[0] = 0x0511;
}

void scrollDown()
{
	int i;
#ifdef DEBUG_PRINT
	uart_printf("%s %d %d\n",__func__,scrollY, scrollYLine);
#endif

	custom.color[0] = 0x0FFF;

	scrollY++;
	scrollYLine++;
	if (scrollYLine >= FRAMEBUFFER_HEIGHT)
		scrollYLine = 0;

	if ((scrollY & 0xf)==0)
	{
#ifdef DEBUG_PRINT
		uart_printf("if ((scrollY & 0xf)==0)\n");
#endif
		//Verschieben des Level-Daten Pointers
		topLeftMapTile += LEVELMAP_WIDTH;

		//Verschieben der hiddenBlitRow, um eine Zeile nach unten
		WRAPPED_TILE_MOVEPTR_DOWN(hiddenBlitRowLeft);

		hiddenBlitRowRight.dest = hiddenBlitRowLeft.dest + 21;
		hiddenBlitRowRight.scrollDownTile = hiddenBlitRowLeft.scrollDownTile + 21;
		hiddenBlitRowRight.scrollUpTile = hiddenBlitRowLeft.scrollUpTile + 21;

		hiddenBlitRowCurrent = hiddenBlitRowLeft;

		//Verschieben der BlitColumn ebenfalls um eine Zeile nach unten. Mit blitten!


		//Modifikation der Blitting Pointer for rechts und links


#ifdef DEBUG_PRINT
		uart_printf("Verschiebe auch hiddenBlitColumnCurrentDest nach unten\n");
#endif

		if (!scrollRightBlitFinished)
		{
			blitTile(*hiddenBlitColumnCurrent.scrollRightTile, hiddenBlitColumnCurrent.dest);
			blitTile(*hiddenBlitColumnBottom.scrollRightTile, hiddenBlitColumnBottom.dest);
		}
		else
		{
			/*
			uint16_t* temp = hiddenBlitColumnBottom.dest - FRAMEBUFFER_LINE_PITCH/2*16;

			if (temp < firstFetchWord)
					temp += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

			blitTile(7, temp);
			*/
		}

		WRAPPED_TILE_MOVEPTR_DOWN(hiddenBlitColumnCurrent);
		WRAPPED_TILE_MOVEPTR_DOWN(hiddenBlitColumnTop);
		WRAPPED_TILE_MOVEPTR_DOWN(hiddenBlitColumnBottom);

		if (scrollRightBlitFinished)
		{
			blitTile(*hiddenBlitColumnBottom.scrollRightTile, hiddenBlitColumnBottom.dest);
		}

		/*
		uint16_t *tilePtr = tilemapChip + SCREEN_DEPTH * 16 * (*hiddenBlitColumnTop.scrollRightTile);
		blitTile(tilePtr, hiddenBlitColumnTop.dest);

		tilePtr = tilemapChip + SCREEN_DEPTH * 16 * (*hiddenBlitColumnBottom.scrollRightTile);
		blitTile(tilePtr, hiddenBlitColumnBottom.dest);
		*/

	}

	//Bei jedem Scroll-Vorgang um einen Pixel ist es bei 22 Tiles in der Breite sinnvoll, direkt 2 Tiles zu zeichnen.

	if ((scrollY & 0xf) > 0) //Warte einen Pixel. Das macht das rechnen einfacher...
	{
		for (i=0;i<2;i++) //2 Tiles gleichzeitig
		{
			if (hiddenBlitRowCurrent.dest <= hiddenBlitRowRight.dest)
			{

#ifdef DEBUG_PRINT
				uart_printf("Blit tile : %d %d\n",(hiddenBlitRowCurrent.scrollDownTile-mapData)/LEVELMAP_WIDTH,(hiddenBlitRowCurrent.scrollDownTile-mapData)%LEVELMAP_WIDTH);
#endif
				blitTile(*hiddenBlitRowCurrent.scrollDownTile, hiddenBlitRowCurrent.dest);

				hiddenBlitRowCurrent.dest++;
				hiddenBlitRowCurrent.scrollDownTile++;
				hiddenBlitRowCurrent.scrollUpTile++;
			}
		}
	}
	else
	{
#ifdef DEBUG_PRINT
		uart_printf("Scroll without blit\n");
#endif
	}

	custom.color[0] = 0x0511;
}


void scrollLeft()
{
	int i;
#ifdef DEBUG_PRINT
	uart_printf("%s %d %d\n",__func__,scrollX, scrollXDelay);
#endif

	if (scrollX == 0)
		return;

	custom.color[0] = 0x0FFF;


	scrollX--;

	//Umsetzung von scrollXDelay nach Word scrolling.
	scrollXDelay++;
	if (scrollXDelay == 16)
	{
		scrollXDelay = 0;
		scrollXWord--;
	}


	if ((scrollX & 0xf)==0xf)
	{
		topLeftMapTile--;
		lastFetchWord--;
		firstFetchWord--;

		TILE_MOVEPTR_LEFT(hiddenBlitColumnTop);
		TILE_MOVEPTR_LEFT(hiddenBlitColumnBottom);
		TILE_MOVEPTR_LEFT(hiddenBlitColumnCurrent);


		hiddenBlitColumnCurrent.dest = hiddenBlitColumnBottom.dest + FRAMEBUFFER_LINE_PITCH/2*16;
		hiddenBlitColumnCurrent.scrollRightTile = hiddenBlitColumnBottom.scrollRightTile + LEVELMAP_WIDTH;
		hiddenBlitColumnCurrent.scrollLeftTile = hiddenBlitColumnBottom.scrollLeftTile + LEVELMAP_WIDTH;


		/*
		if (hiddenBlitRowCurrent.dest > hiddenBlitRowLeft.dest)
			blitTile(*hiddenBlitRowCurrent.scrollDownTile, hiddenBlitRowCurrent.dest);
		*/

		TILE_MOVEPTR_LEFT(hiddenBlitRowLeft);
		TILE_MOVEPTR_LEFT(hiddenBlitRowRight);
		TILE_MOVEPTR_LEFT(hiddenBlitRowCurrent);

		//blitTile(*hiddenBlitRowRight.scrollUpTile, hiddenBlitRowRight.dest);

		scrollRightBlitFinished=0;
		scrollLeftBlitFinished=0;
	}


	if ((scrollY & 0xf) < 0xf)
	{
		for (i=0;i<2;i++) //2 Tiles gleichzeitig
		{
			if (!scrollLeftBlitFinished)
			{
#ifdef DEBUG_PRINT
				//uart_printf("hiddenBlitColumnCurrentDest %x\n",hiddenBlitColumnCurrent.dest);
				//uart_printf("hiddenBlitColumnBottom %x\n",hiddenBlitColumnBottom.dest);
#endif

				if (hiddenBlitColumnCurrent.dest == hiddenBlitColumnTop.dest)
				{
					scrollLeftBlitFinished=1;
#ifdef DEBUG_PRINT
					uart_printf("scrollRightBlitFinished=1;\n");
#endif
				}


#ifdef DEBUG_PRINT
				uart_printf("Blit tile : %d %d\n",(hiddenBlitColumnCurrent.scrollLeftTile-mapData)/LEVELMAP_WIDTH,(hiddenBlitColumnCurrent.scrollLeftTile-mapData)%LEVELMAP_WIDTH);
#endif
				WRAPPED_TILE_MOVEPTR_UP(hiddenBlitColumnCurrent);

				blitTile(*hiddenBlitColumnCurrent.scrollLeftTile, hiddenBlitColumnCurrent.dest - FRAMEBUFFER_PLANE_PITCH/2);

				scrollRightBlitFinished=0;

			}
		}

	}
	else
	{
#ifdef DEBUG_PRINT
		uart_printf("Scroll without blit\n");
#endif
	}


	custom.color[0] = 0x0511;
}



void scrollRight()
{
	int i;
#if 1
	if (scrollX == HORIZONTAL_SCROLL_WORDS*16)
		return;

	custom.color[0] = 0x0FFF;

	scrollX++;

	//Umsetzung von scrollXDelay nach Word scrolling.
	scrollXDelay--;
	if (scrollXDelay == -1)
	{
		scrollXDelay = 15;
		scrollXWord++;
	}

	if ((scrollX & 0xf) == 0)
	{
#ifdef DEBUG_PRINT
		uart_printf("if ((scrollX & 0xf) == 0)\n");
#endif

		topLeftMapTile++;
		lastFetchWord++;
		firstFetchWord++;

		TILE_MOVEPTR_RIGHT(hiddenBlitColumnTop);
		TILE_MOVEPTR_RIGHT(hiddenBlitColumnBottom);
		TILE_MOVEPTR_RIGHT(hiddenBlitColumnCurrent);

		hiddenBlitColumnCurrent = hiddenBlitColumnTop;


		if (hiddenBlitRowCurrent.dest > hiddenBlitRowLeft.dest)
			blitTile(*hiddenBlitRowCurrent.scrollDownTile, hiddenBlitRowCurrent.dest);


		TILE_MOVEPTR_RIGHT(hiddenBlitRowLeft);
		TILE_MOVEPTR_RIGHT(hiddenBlitRowRight);
		TILE_MOVEPTR_RIGHT(hiddenBlitRowCurrent);

		if (hiddenBlitRowCurrent.dest <= hiddenBlitRowRight.dest)
			blitTile(*hiddenBlitRowRight.scrollUpTile, hiddenBlitRowRight.dest);


		scrollRightBlitFinished=0;
	}

	//uart_printf("scrollXDelay %d\n",scrollXDelay);

	if ((scrollX & 0xf) > 0)
	{
		for (i=0;i<2;i++) //2 Tiles gleichzeitig
		{
			if (!scrollRightBlitFinished)
			{
#ifdef DEBUG_PRINT
				//uart_printf("hiddenBlitColumnCurrentDest %x\n",hiddenBlitColumnCurrent.dest);
				//uart_printf("hiddenBlitColumnBottom %x\n",hiddenBlitColumnBottom.dest);
#endif

				if (hiddenBlitColumnCurrent.dest == hiddenBlitColumnBottom.dest)
				{
					scrollRightBlitFinished=1;
#ifdef DEBUG_PRINT
					uart_printf("scrollRightBlitFinished=1;\n");
#endif
				}


#ifdef DEBUG_PRINT
				uart_printf("Blit tile : %d %d\n",(hiddenBlitColumnCurrent.scrollRightTile-mapData)/LEVELMAP_WIDTH,(hiddenBlitColumnCurrent.scrollRightTile-mapData)%LEVELMAP_WIDTH);
#endif
				blitTile(*hiddenBlitColumnCurrent.scrollRightTile, hiddenBlitColumnCurrent.dest);

				WRAPPED_TILE_MOVEPTR_DOWN(hiddenBlitColumnCurrent);

				scrollLeftBlitFinished=0;
			}
		}
	}
	else
	{
#ifdef DEBUG_PRINT
		uart_printf("Scroll without blit\n");
#endif
	}
#endif
	custom.color[0] = 0x0511;
}

/*
if ()
	tileIdPtr-=LEVELMAP_WIDTH*(FRAMEBUFFER_HEIGHT/16);
*/

//uart_printf("tileIdPtr %p %d %d %d\n",tileIdPtr, blitTileX, tileIdPtr[0] , tileIdPtr[1]);


void renderFullScreen()
{
	int x=0;
	int y=0;

	/*
	 * Wir initialisieren die Scroll-Routine mit gegebenen Scroll-Werten.
	 * Die oberste Tile-Zeile ist nicht zu sehen. Ebenso die linkeste Spalte.
	 * Das Display-Window beginnt oben links mit dem 2. Tile von links und dem 2. Tile von oben. Das erscheint irgendwie sinnvoll als Anfangszustand.
	 */
	scrollX&=~15;
	scrollY&=~15;

	topLeftMapTile = mapData;

	//topLeftWord = bitmap + scrollX/16 + FRAMEBUFFER_LINE_PITCH/2*16;
	//topTileWord = bitmap + scrollX/16 + FRAMEBUFFER_LINE_PITCH/2*16;

	hiddenBlitRowLeft.dest = bitmap + (FRAMEBUFFER_LINE_PITCH/2)*16*18;
	hiddenBlitRowLeft.scrollDownTile = topLeftMapTile + LEVELMAP_WIDTH*17 - 1;
	hiddenBlitRowLeft.scrollUpTile = topLeftMapTile - LEVELMAP_WIDTH*2 - 1;

	hiddenBlitRowRight.dest = hiddenBlitRowLeft.dest + 21;
	hiddenBlitRowRight.scrollDownTile = topLeftMapTile + LEVELMAP_WIDTH*17 + 20;
	hiddenBlitRowRight.scrollUpTile = topLeftMapTile - LEVELMAP_WIDTH*2 + 20;
	uart_printf("%p %p\n",topLeftMapTile + LEVELMAP_WIDTH*17 + 20, hiddenBlitRowLeft.scrollDownTile + 21);
	uart_printf("%p %p\n",topLeftMapTile - LEVELMAP_WIDTH*2 + 20, hiddenBlitRowLeft.scrollUpTile + 21);

	hiddenBlitRowCurrent = hiddenBlitRowLeft;



	hiddenBlitColumnTop.dest = bitmap + 22;
	hiddenBlitColumnTop.scrollRightTile = topLeftMapTile -LEVELMAP_WIDTH + 21;
	hiddenBlitColumnTop.scrollLeftTile = topLeftMapTile -LEVELMAP_WIDTH - 2;

	hiddenBlitColumnBottom.dest = hiddenBlitColumnTop.dest + (FRAMEBUFFER_LINE_PITCH/2)*16 *17;
	hiddenBlitColumnBottom.scrollRightTile = topLeftMapTile + 16*LEVELMAP_WIDTH + 21;
	hiddenBlitColumnBottom.scrollLeftTile = topLeftMapTile + 16*LEVELMAP_WIDTH - 2;

	hiddenBlitColumnCurrent = hiddenBlitColumnTop;


	firstFetchWord = bitmap + scrollX/16;
	lastFetchWord = firstFetchWord + FRAMEBUFFER_WIDTH/16*FRAMEBUFFER_HEIGHT*SCREEN_DEPTH;

	uart_printf("lastFetchWord %x\n",lastFetchWord);
	uart_printf("hiddenBlitColumnBottom %x\n",hiddenBlitColumnBottom);
	//hiddenBlitColumn = bitmap + 18; //for testing, weil man es sieht

	//firstFetchWord = topLeftWord - EXTRA_FETCH_WORDS;



	//Da wir ein Word früher anfangen zu fetchen...



	scrollXDelay=0;
	scrollYLine=16;
	scrollXWord=0;

	custom.bplcon1=0;

	uint8_t *dest=(uint8_t*)bitmap;
	uint8_t *src = topLeftMapTile-LEVELMAP_WIDTH-1;

	//src = mapData-1;
#if 1

	for (y=0; y<SCREEN_HEIGHT/16 + 2; y++) //2 extra Tiles für oben und unten
	//for (y=0; y<3;y++)
	{
		//src = &mapData[99];
		for (x=0; x<SCREEN_WIDTH/16 + 2; x++) //2 extra Tiles für links und rechts
		{
			uint8_t tileId = *src;
			//uint8_t tileId = 0;

			uint16_t *tilePtr = tilemapChip + SCREEN_DEPTH * 16 * tileId;

			//vom tile ins bild
			custom.bltcon0 = BC0F_SRCA | A_TO_D | BC0F_DEST;
			custom.bltapt = (void*)tilePtr;
			custom.bltdpt = (void*)dest;
			custom.bltamod = 0;
			custom.bltdmod = (FRAMEBUFFER_WIDTH - 16)/8;
			custom.bltafwm = 0xffff;
			custom.bltalwm = 0xffff;
			custom.bltsize = ((5*16) << HSIZEBITS) | 1; //starts blitter. 16 x 16 Pixel

			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter
			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter. am besten 2 mal?

			dest+=2; //ein Wort weiter nach rechts
			src++;
		}

		//Wir sind nun am Rand angelangt. Das Wort nun per LINE Modulo auf den Anfang der nächsten Zeile und dann mit TODO

		dest+=2 + 4 * FRAMEBUFFER_PLANE_PITCH + 15 * FRAMEBUFFER_LINE_PITCH;
		src += 100 - (SCREEN_WIDTH/16 + 2);
	}
#endif
}


