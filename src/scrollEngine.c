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


/*
 * Das Cornertile sitzt in den Ecken und bewegt sich nur bei Übergängen X|Y&0xf >= 0 oder <= 0xf.
 */
struct CornerTile
{
	uint16_t *dest;
	uint8_t *tile;
};

/*
 * Das Movingtile startet bei einem CornerTile und bewegt sich beim Scrolling in eine Richtung.
 * Es gibt ein MovingTile für horizontal und eines für vertikal.
 * Es wird immer von oben nach unten oder von links nach rechts gearbeitet.
 * Die Steps definieren den AdressenIncrement in Richtung der zu scrollenden Position.
 */
struct MovingTile
{
	uint16_t *dest;
	uint8_t *tile;

	int destStep;
	int tileStep;

	char isRight_isDown;
	char isActive;
};

struct MovingTile horizontalScrollBlitTile;
struct MovingTile verticalScrollBlitTile;

struct CornerTile topLeft;
struct CornerTile topRight;
struct CornerTile bottomLeft;
struct CornerTile bottomRight;

int scrollYLine=0;
int scrollY=0;
int scrollX=0;

//Pointer die beim Scrollen immer mitgeführt werden müssen
uint16_t *firstFetchWord;
uint16_t *lastFetchWord;


#define WRAPPED_TILE_MOVEPTR_DOWN(arg) \
	arg.dest += FRAMEBUFFER_LINE_PITCH/2*16; \
	if (arg.dest >= lastFetchWord) \
		arg.dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT; \
	arg.tile+=LEVELMAP_WIDTH; \

#define WRAPPED_TILE_MOVEPTR_UP(arg) \
	arg.dest -= FRAMEBUFFER_LINE_PITCH/2*16; \
	if (arg.dest < firstFetchWord) \
		arg.dest += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT; \
	arg.tile -= LEVELMAP_WIDTH; \

#define TILE_MOVEPTR_RIGHT(arg) \
	arg.dest++; \
	arg.tile++; \

#define TILE_MOVEPTR_LEFT(arg) \
	arg.dest--; \
	arg.tile--; \

uint16_t *bitmap=NULL;
//uint16_t __attribute__((__chip__)) bitmap[10];
uint16_t *copperlist=NULL;
uint16_t *spriteStruct=NULL;
uint16_t *spriteNullStruct=NULL;
uint16_t *tilemapChip=NULL;


const int displayWindowVStart=44;
const int displayWindowVStop=300;
const int displayWindowHStart=128; //eigentlich 129.... aber Amiga Bugs in Hardware...?


uint8_t mapData[LEVELMAP_WIDTH*LEVELMAP_HEIGHT];
uint8_t *topLeftMapTile;

//#define DEBUG_VERTICAL_SCROLL	0
//#define DEBUG_HORIZONTAL_SCROLL	0
//#define DEBUG_PRINT
#define DEBUG_RUNTIME
#define DEBUG_COPPER_WRAP

#ifdef DEBUG_VERTICAL_SCROLL
int debugVerticalScrollAmount	= DEBUG_VERTICAL_SCROLL;
#else
int debugVerticalScrollAmount	= 0;
#endif

#ifdef DEBUG_HORIZONTAL_SCROLL
int debugHorizontalScrollAmount	= DEBUG_HORIZONTAL_SCROLL;
#else
int debugHorizontalScrollAmount	= 0;
#endif



void constructCopperList()
{
	int i;
	i=0;

	custom.bplcon1 = scrollXDelay | (scrollXDelay<<4);

#ifdef DEBUG_VERTICAL_SCROLL
	int backup_scrollYLine = scrollYLine;

	scrollYLine+=(16*debugVerticalScrollAmount);
	if (scrollYLine >= FRAMEBUFFER_HEIGHT)
		scrollYLine -= FRAMEBUFFER_HEIGHT;
	if (scrollYLine < 0)
		scrollYLine += FRAMEBUFFER_HEIGHT;
#endif
#ifdef DEBUG_HORIZONTAL_SCROLL
	int backup_scrollXWord = scrollXWord;

	scrollXWord+=debugHorizontalScrollAmount;
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

#ifdef DEBUG_COPPER_WRAP
		copperlist[i++]=COLOR00; //Löse Interrupt aus
		copperlist[i++]=0x0fff;
#endif

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

#ifdef DEBUG_COPPER_WRAP
	copperlist[i++]=COLOR00; //Löse Interrupt aus
	copperlist[i++]=0;
#endif

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


void setSpriteStruct(int x, int y, int h)
{
	uint16_t spriteX = displayWindowHStart + x;
	uint16_t spriteYStart = displayWindowVStart + y;
	uint16_t spriteYEnd = spriteYStart + h;

	spriteStruct[0] = (spriteYStart<<8) | (spriteX>>1);
	spriteStruct[1] = (spriteYEnd<<8) | (((spriteYStart>>8)&1)<<2) | (((spriteYEnd>>8)&1)<<1) | (spriteX&1);

}

void initVideo()
{
	int i,j;
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
	//uart_printf("CACR = 0x%x\n",Supervisor(supervisor_getCACR));

	//Supervisor(supervisor_disableDataCache);
	//uart_printf("CACR = 0x%x\n",Supervisor(supervisor_getCACR));

	/*
	asm (	"movec cacr,d0\n"
			"bset #0,d0\n" //i cache on
			"bset #4,d0\n" //i burst on
			"bclr #8,d0\n" //d cache off
			"bclr #12,d0\n" //d burst off
			"movec d0,cacr\n"
		 : //no outputs
		 : //no inputs
		 : "d0");
	*/


	uint16_t spriteX = displayWindowHStart;
	uint16_t spriteYStart = displayWindowVStart + 1;
	uint16_t spriteYEnd = spriteYStart + 16;

	//spriteX = 0x90;

	i=0;
	spriteStruct[i++] = (spriteYStart<<8) | (spriteX>>1);
	spriteStruct[i++] = (spriteYEnd<<8) | (((spriteYStart>>8)&1)<<2) | (((spriteYEnd>>8)&1)<<1) | (spriteX&1);

	for (j=0;j<sizeof(mouseCursor)/2;j++)
		spriteStruct[i++] = mouseCursor[j];



#if 0
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
#endif

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

	custom.color[0] = 0x0000;
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


void checkPtrBoundary(uint16_t *dest)
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
}

#if 0
struct blitterRegSet
{
	uint16_t bltcon0;
	uint16_t bltcon1;

	uint16_t bltapt;
	uint16_t bltbpt;
	uint16_t bltcpt;
	uint16_t bltdpt;

	uint16_t bltamod;
	uint16_t bltbmod;
	uint16_t bltcmod;
	uint16_t bltdmod;

	uint16_t bltafwm;
	uint16_t bltalwm;

	uint16_t bltsize;
} blitterPrepareRegs;

static inline void setBlitterRegSet()
{
	custom.bltcon0 = blitterPrepareRegs.bltcon0;
	custom.bltcon1 = blitterPrepareRegs.bltcon0;

	custom.bltapt = blitterPrepareRegs.bltcon0;
	custom.bltbpt = blitterPrepareRegs.bltcon0;
	custom.bltcpt = blitterPrepareRegs.bltcon0;
	custom.bltdpt = blitterPrepareRegs.bltcon0;

	custom.bltamod = blitterPrepareRegs.bltcon0;
	custom.bltbmod = blitterPrepareRegs.bltcon0;
	custom.bltcmod = blitterPrepareRegs.bltcon0;
	custom.bltdmod = blitterPrepareRegs.bltcon0;
}
#endif


void blitTestBlob_mapCoordinate(uint16_t* src, int x, int y, int width, int height)
{
	uint16_t *dest = topLeft.dest + x/16 + y*FRAMEBUFFER_LINE_PITCH/2 + 1 + 16*FRAMEBUFFER_LINE_PITCH/2;
	int shift = x & 0xf;
	blitMaskedBob(dest, shift, src, width, height);
}

/*
 * Zeichnet ACBM Bob mit angefügter Maske in den Framebuffer an diese Adresse dest.
 * Der Copper-Wrap wird berücksichtigt.
 * Kein Clipping!
 */
void blitMaskedBob(uint16_t *dest, int shift, uint16_t* src, int width, int height )
{
	int i=0;

	//Schon mal dest korrigieren, falls das Ziel bereits über das Ende hinaus schießt
	if (dest >= lastFetchWord)
		dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;


	//uint16_t lastWordMask = 0xffff;
	int lastWordMask = 0xffff;
	int firstWordMask = 0xffff;
	int destMod = (FRAMEBUFFER_WIDTH - 128)/8 + FRAMEBUFFER_PLANE_PITCH*(SCREEN_DEPTH-1);
	int16_t srcMod = 0;

	int srcWidth = width/16;
	int destWidth = srcWidth;
	//int height = 128;

	if (shift)
	{
		srcMod = -2;
		destMod = (FRAMEBUFFER_WIDTH - 128 - 16)/8 + FRAMEBUFFER_PLANE_PITCH*(SCREEN_DEPTH-1);
		destWidth++;
		//lastWordMask = 0xffff << shift;
		lastWordMask = 0;
		//firstWordMask=0;
	}

	uint16_t* firstAdrAfterBlit = dest + (FRAMEBUFFER_LINE_PITCH/2) * height;

	while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

	/*
	 * A ist die Maske
	 * B ist die BOB Grafik
	 * C ist der Framebuffer als Quelle
	 * D ist auch der Framebuffer aber als Ziel
	 *
	 * AB + NAC -> Also wenn Maske 1 ist, dann nimm die Bob Grafik. Und wenn 1, dann den Framebuffer
	 * Übersetzt, da wenn AB, dann ist C egal.
	 * Umgekehrt, wenn NAC, dann ist B egal
	 * ABNC + ABC + NABC + NANBC
	 */


	//einmal alles setzen, was ohnehin gleich ist...
	custom.bltcon0 = BC0F_SRCA | BC0F_SRCB | BC0F_SRCC | ABC | ABNC | NABC | NANBC | BC0F_DEST | (shift<<ASHIFTSHIFT);
	custom.bltcon1 = (shift << BSHIFTSHIFT);
	custom.bltamod = srcMod;
	custom.bltbmod = srcMod;
	custom.bltcmod = destMod;
	custom.bltdmod = destMod;
	custom.bltafwm = firstWordMask;
	custom.bltalwm = lastWordMask;

	//Löse das Copper-Wrap Problem, in dem 2 mal geblittet wird
	if (firstAdrAfterBlit > lastFetchWord)
	{

		//Wir müssen teilen...
		int firstHeight = (lastFetchWord - dest) / (FRAMEBUFFER_LINE_PITCH/2) + 1;
		int secondHeight = height - firstHeight;

		//uart_printf("Das erste Stück ist %d hoch, dann %d!\n",firstHeight,secondHeight);

		checkPtrBoundary(dest);
		checkPtrBoundary(dest + (firstHeight-1) * (FRAMEBUFFER_LINE_PITCH/2));

		uint16_t firstHalf_blitsize = (firstHeight << HSIZEBITS) | destWidth;
		uint16_t secondHalf_blitsize = (secondHeight << HSIZEBITS) | destWidth;

		uint16_t *firstHalf_bltapt = src + height*srcWidth*SCREEN_DEPTH; //Maske erste Hälfte
		uint16_t *secondHalf_bltapt = firstHalf_bltapt + firstHeight * srcWidth; //Maske 2. Hälfte

		uint16_t *firstHalf_bltcpt = dest;
		uint16_t *secondHalf_bltcpt = firstHalf_bltcpt + firstHeight*(FRAMEBUFFER_LINE_PITCH/2) - FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

		custom.bltbpt = src; //Einmal setzen reicht! Danach führt der Blitter selbst nach!

#if 1
		for (i=0;i<SCREEN_DEPTH;i++)
		{
			//Erst die obere Hälfte
			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

			custom.bltapt = firstHalf_bltapt;
			custom.bltcpt = firstHalf_bltcpt;
			custom.bltdpt = firstHalf_bltcpt;
			custom.bltsize = firstHalf_blitsize; //starts blitter.

			//Zielpointer
			firstHalf_bltcpt += (FRAMEBUFFER_PLANE_PITCH/2);

#if 1
			//Dann die untere Hälfte nach dem Copper-Wrap

			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

			custom.bltapt = secondHalf_bltapt;
			custom.bltcpt = secondHalf_bltcpt;
			custom.bltdpt = secondHalf_bltcpt;
			custom.bltsize = secondHalf_blitsize; //starts blitter.

			//Zielpointer
			secondHalf_bltcpt += (FRAMEBUFFER_PLANE_PITCH/2);
#endif

		}
#endif


	}
	else
	{
		//Zeichne ganz normal als ganzes Stück

		uint16_t *bltapt = src + height*srcWidth*SCREEN_DEPTH;
		custom.bltbpt = src; //Einmal setzen reicht! Danach führt der Blitter selbst nach!

		uint16_t *bltcpt = dest;
		uint16_t blitsize = (height << HSIZEBITS) | destWidth;

		for (i=0;i<SCREEN_DEPTH;i++)
		{
			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

			custom.bltapt = bltapt; //Muss jedes Mal neu gesetzt werden, weil der Blitter selbst nachführt.
			custom.bltcpt = bltcpt;
			custom.bltdpt = bltcpt;
			custom.bltsize = blitsize; //starts blitter.

			bltcpt += (FRAMEBUFFER_PLANE_PITCH/2);
		}
	}
}

static inline void blitTile(uint8_t tileid, uint16_t *dest)
{
	uint16_t *src = tilemapChip + SCREEN_DEPTH * 16 * tileid;
#if 1
	while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

	//uart_printf("b %p\n",dest);
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
}

static inline int verifyTile(uint8_t tileid, uint16_t *dest)
{
	uint16_t *src = tilemapChip + SCREEN_DEPTH * 16 * tileid;
	int i;
	for (i=0; i<5*16; i++)
	{
		if (*dest != *src)
		{
			uart_printf("verifyTile failed %d\n",i);
			return 1;
		}
		src++;
		dest+=FRAMEBUFFER_WIDTH/16;
	}
	return 0;
}


void processMovingTile(struct MovingTile *mt)
{
#ifdef DEBUG_PRINT
	if (mt == &horizontalScrollBlitTile)
		uart_printf("processMovingTile horizontalMove\n");
	else if (mt == &verticalScrollBlitTile)
		uart_printf("processMovingTile verticalMove\n");
#endif
	blitTile(*mt->tile, mt->dest);

	mt->dest += mt->destStep;
	mt->tile += mt->tileStep;

	if (mt->dest >= lastFetchWord)
		mt->dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;
	else if (mt->dest < firstFetchWord)
		mt->dest += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

	//mt->number--;
}


//Das vertikale Scrolling erstreckt sich über die gesamte Zeile.
void scrollUp()
{
	int i;
#ifdef DEBUG_PRINT
	uart_printf("%s %d %d\n",__func__,scrollY, scrollYLine);
#endif

	if (scrollY == 0)
		return;

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0x0FFF;
#endif
	scrollY--;

	scrollYLine--;
	if (scrollYLine < 0)
		scrollYLine = FRAMEBUFFER_HEIGHT-1;

	if ((scrollY & 0xf)==0xf)
	{
		topLeftMapTile -= LEVELMAP_WIDTH;

		if (horizontalScrollBlitTile.dest == bottomLeft.dest || horizontalScrollBlitTile.dest == bottomRight.dest)
		{
			horizontalScrollBlitTile.isActive=0;
#ifdef DEBUG_PRINT
			uart_printf("horizontalScrollBlitTile abgeschaltet\n");
#endif
		}

		WRAPPED_TILE_MOVEPTR_UP(topLeft);
		WRAPPED_TILE_MOVEPTR_UP(topRight);
		WRAPPED_TILE_MOVEPTR_UP(bottomRight);
		WRAPPED_TILE_MOVEPTR_UP(bottomLeft);

		verticalScrollBlitTile.isActive=0;

#ifdef DEBUG_PRINT
		uart_printf("scrollUp verticalScrollBlitTile init\n");
#endif
		verticalScrollBlitTile.dest = topLeft.dest;
		verticalScrollBlitTile.tile = topLeft.tile;

		verticalScrollBlitTile.destStep = 1;
		verticalScrollBlitTile.tileStep = 1;

		//verticalScrollBlitTile.number = FRAMEBUFFER_WIDTH/16;
		verticalScrollBlitTile.isRight_isDown=0;
		verticalScrollBlitTile.isActive = 1;

#ifdef DEBUG_PRINT
		uart_printf("scrollUp WRAPPED_TILE_MOVEPTR_UP\n");
#endif


	}

	if (verticalScrollBlitTile.isRight_isDown==0)
	{
		for (i=0;i<2;i++)
		{
			if (verticalScrollBlitTile.isActive)
			{
				if (verticalScrollBlitTile.dest == topRight.dest)
				{
					verticalScrollBlitTile.isActive=0;
#ifdef DEBUG_PRINT
					uart_printf("verticalScrollBlitTile abgeschaltet\n");
#endif
				}

				processMovingTile(&verticalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0;
#endif
}

void scrollDown()
{
	int i;
#ifdef DEBUG_PRINT
	uart_printf("%s %d %d %d\n",__func__,scrollY, scrollYLine,scrollY&0xf);
#endif

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0x0FFF;
#endif

	scrollY++;
	scrollYLine++;
	if (scrollYLine >= FRAMEBUFFER_HEIGHT)
		scrollYLine = 0;

	if ((scrollY & 0xf)==1)
	{
#ifdef DEBUG_PRINT
		uart_printf("scrollDown Init\n");
#endif
		verticalScrollBlitTile.dest = bottomLeft.dest;
		verticalScrollBlitTile.tile = bottomLeft.tile;

		verticalScrollBlitTile.destStep = 1;
		verticalScrollBlitTile.tileStep = 1;

		//verticalScrollBlitTile.number = FRAMEBUFFER_WIDTH/16;
		verticalScrollBlitTile.isRight_isDown=1;
		verticalScrollBlitTile.isActive = 1;
	}

	if ((scrollY & 0xf)==0)
	{
		topLeftMapTile += LEVELMAP_WIDTH;

#ifdef DEBUG_PRINT
		uart_printf("scrollDown WRAPPED_TILE_MOVEPTR_DOWN\n");
#endif

		WRAPPED_TILE_MOVEPTR_DOWN(topLeft);
		WRAPPED_TILE_MOVEPTR_DOWN(topRight);
		WRAPPED_TILE_MOVEPTR_DOWN(bottomRight);
		WRAPPED_TILE_MOVEPTR_DOWN(bottomLeft);

		verticalScrollBlitTile.isActive=0;

		if (horizontalScrollBlitTile.dest == topLeft.dest || horizontalScrollBlitTile.dest == topRight.dest)
		{
			//horizontalScrollBlitTile.isActive=0;
			processMovingTile(&horizontalScrollBlitTile);
#ifdef DEBUG_PRINT
			uart_printf("horizontalScrollBlitTile abgeschaltet\n");
#endif
		}
	}

	if (verticalScrollBlitTile.isRight_isDown==1)
	{
		for (i=0;i<2;i++)
		{
			if (verticalScrollBlitTile.isActive)
			{
				if (verticalScrollBlitTile.dest == bottomRight.dest)
				{
					verticalScrollBlitTile.isActive=0;
#ifdef DEBUG_PRINT
					uart_printf("verticalScrollBlitTile abgeschaltet\n");
#endif
				}

				processMovingTile(&verticalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0;
#endif
}


void scrollLeft()
{
	int i;
#ifdef DEBUG_PRINT
	uart_printf("%s %d %d %d\n",__func__,scrollX, scrollXDelay, scrollX&0xf);
#endif

	if (scrollX == 0)
		return;

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0x0FFF;
#endif

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
#ifdef DEBUG_PRINT
		uart_printf("scrollLeft horizontalScrollBlitTile init\n");
#endif

		topLeftMapTile--;
		lastFetchWord--;
		firstFetchWord--;

		if (verticalScrollBlitTile.dest == topRight.dest || verticalScrollBlitTile.dest == bottomRight.dest)
		{
			verticalScrollBlitTile.isActive=0;
#ifdef DEBUG_PRINT
			uart_printf("BARF BARF if (verticalScrollBlitTile.dest == topRight.dest || verticalScrollBlitTile.dest == bottomRight.dest)\n");
			//uart_printf("verticalScrollBlitTile decrement auf %d\n",verticalScrollBlitTile.number);
#endif
		}

		TILE_MOVEPTR_LEFT(topLeft);
		TILE_MOVEPTR_LEFT(topRight);
		TILE_MOVEPTR_LEFT(bottomRight);
		TILE_MOVEPTR_LEFT(bottomLeft);

		horizontalScrollBlitTile.isActive=0;

#if 0
		if (verticalScrollBlitTile.dest == topRight.dest || verticalScrollBlitTile.dest == bottomRight.dest)
		{
			verticalScrollBlitTile.isActive=0;
#ifdef DEBUG_PRINT
			uart_printf("if (verticalScrollBlitTile.dest == topRight.dest || verticalScrollBlitTile.dest == bottomRight.dest)\n");
			//uart_printf("verticalScrollBlitTile decrement auf %d\n",verticalScrollBlitTile.number);
#endif
		}
#endif

		horizontalScrollBlitTile.dest = topLeft.dest;
		horizontalScrollBlitTile.tile = topLeft.tile;

		horizontalScrollBlitTile.destStep = FRAMEBUFFER_LINE_PITCH/2*16;
		horizontalScrollBlitTile.tileStep = LEVELMAP_WIDTH;
		horizontalScrollBlitTile.isActive = 1;
		//horizontalScrollBlitTile.number = FRAMEBUFFER_HEIGHT/16;
		horizontalScrollBlitTile.isRight_isDown=0;
	}


	if (horizontalScrollBlitTile.isRight_isDown==0)
	{
		for (i=0;i<2;i++)
		{
			if (horizontalScrollBlitTile.isActive)
			{
				if (horizontalScrollBlitTile.dest == bottomLeft.dest)
				{
					horizontalScrollBlitTile.isActive=0;
#ifdef DEBUG_PRINT
					uart_printf("horizontalScrollBlitTile abgeschaltet\n");
#endif
				}
				processMovingTile(&horizontalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0;
#endif

}



void scrollRight()
{
	int i;

#ifdef DEBUG_PRINT
	uart_printf("%s %d %d\n",__func__,scrollX, scrollXDelay);
#endif

	if (scrollX == HORIZONTAL_SCROLL_WORDS*16)
		return;

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0x0FFF;
#endif

	scrollX++;

	//Umsetzung von scrollXDelay nach Word scrolling.
	scrollXDelay--;
	if (scrollXDelay == -1)
	{
		scrollXDelay = 15;
		scrollXWord++;
	}

	if ((scrollX & 0xf)==1)
	{
		//Stelle den horizontalen Scroller auf die rechte Seite ein.
#ifdef DEBUG_PRINT
		uart_printf("scrollRight horizontalScrollBlitTile init\n");
#endif
		horizontalScrollBlitTile.dest = topRight.dest;
		horizontalScrollBlitTile.tile = topRight.tile;

		horizontalScrollBlitTile.destStep = FRAMEBUFFER_LINE_PITCH/2*16;
		horizontalScrollBlitTile.tileStep = LEVELMAP_WIDTH;

		//horizontalScrollBlitTile.number = FRAMEBUFFER_HEIGHT/16;
		horizontalScrollBlitTile.isActive = 1;
		horizontalScrollBlitTile.isRight_isDown=1;
	}

	if ((scrollX & 0xf)==0)
	{
#ifdef DEBUG_PRINT
		uart_printf("scrollRight TILE_MOVEPTR_RIGHT\n");
#endif
		topLeftMapTile++;
		lastFetchWord++;
		firstFetchWord++;

		TILE_MOVEPTR_RIGHT(topLeft);
		TILE_MOVEPTR_RIGHT(topRight);
		TILE_MOVEPTR_RIGHT(bottomRight);
		TILE_MOVEPTR_RIGHT(bottomLeft);

		horizontalScrollBlitTile.isActive=0;

		if (verticalScrollBlitTile.dest == topLeft.dest || verticalScrollBlitTile.dest == bottomLeft.dest)
		{
			//verticalScrollBlitTile.isActive=0;
			//verticalScrollBlitTile.dest++;
			//verticalScrollBlitTile.tile++;
			processMovingTile(&verticalScrollBlitTile);

#ifdef DEBUG_PRINT
			uart_printf("if (verticalScrollBlitTile.dest == topLeft.dest || verticalScrollBlitTile.dest == bottomLeft.dest)\n");
			//uart_printf("verticalScrollBlitTile decrement auf %d\n",verticalScrollBlitTile.number);
#endif
		}
	}

	if (horizontalScrollBlitTile.isRight_isDown==1)
	{
		for (i=0;i<2;i++)
		{
			if (horizontalScrollBlitTile.isActive)
			{
				if (horizontalScrollBlitTile.dest == bottomRight.dest)
				{
					horizontalScrollBlitTile.isActive=0;
#ifdef DEBUG_PRINT
					uart_printf("horizontalScrollBlitTile abgeschaltet\n");
#endif
				}
				processMovingTile(&horizontalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_RUNTIME
	custom.color[0] = 0;
#endif
}

/*
if ()
	tileIdPtr-=LEVELMAP_WIDTH*(FRAMEBUFFER_HEIGHT/16);
*/

//uart_printf("tileIdPtr %p %d %d %d\n",tileIdPtr, blitTileX, tileIdPtr[0] , tileIdPtr[1]);

void alignOnTileBoundary()
{
	if ((scrollX&0xf)>=8)
	{
		while ((scrollX&0xf)!=0)
			scrollRight();
	}
	else
	{
		while ((scrollX&0xf)!=0)
			scrollLeft();
	}

	if ((scrollY&0xf)>=8)
	{
		while ((scrollY&0xf)!=0)
			scrollDown();
	}
	else
	{
		while ((scrollY&0xf)!=0)
			scrollUp();
	}
}


int verifyVisibleWindow()
{
	int x,y;
	//uart_printf("verifyVisibleWindow %d\n", scrollYLine);
	uint16_t *dest=firstFetchWord + ((scrollYLine&~0xf)-16) * FRAMEBUFFER_LINE_PITCH/2 + 1;
	uint8_t *src = topLeftMapTile-LEVELMAP_WIDTH-1;

	dest = topLeft.dest + (FRAMEBUFFER_LINE_PITCH/2)*16 + 1;
	src = topLeft.tile + LEVELMAP_WIDTH + 1;

	if (dest < firstFetchWord)
		dest += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;
	if (dest >= lastFetchWord)
		dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

	for (y=0; y<SCREEN_HEIGHT/16 + 1; y++) //2 extra Tiles für oben und unten
	{
		uint16_t *lineStartDest = dest;
		uint8_t *lineStartSrc = src;

		for (x=0; x<SCREEN_WIDTH/16 + 1; x++) //2 extra Tiles für links und rechts
		{
			uint8_t tileId = *src;
			//uint8_t tileId = 0;

			if (verifyTile(tileId,dest))
			{
				uart_printf("Verify failed %d %d\n",x,y);
				return 1;
			}


			dest++; //ein Wort weiter nach rechts
			src++;
		}

		dest = lineStartDest + (FRAMEBUFFER_LINE_PITCH/2)*16 ;
		src = lineStartSrc + LEVELMAP_WIDTH;

		if (dest >= lastFetchWord)
			dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;
	}


	return 0;
}

void renderFullScreen()
{
	int x=0;
	int y=0;

	/*
	 * Wir initialisieren die Scroll-Routine mit gegebenen Scroll-Werten.
	 * Die oberste Tile-Zeile ist nicht zu sehen. Ebenso die linkeste Spalte.
	 * Das Display-Window beginnt oben links mit dem 2. Tile von links und dem 2. Tile von oben. Das erscheint irgendwie sinnvoll als Anfangszustand.
	 */
	scrollX&=~0xf;
	scrollY&=~0xf;

	topLeftMapTile = mapData + scrollX/16 + (scrollY/16)*LEVELMAP_WIDTH;

	scrollXWord = (scrollX/16);

	firstFetchWord = bitmap + scrollXWord - 1;
	lastFetchWord = firstFetchWord + FRAMEBUFFER_WIDTH/16*FRAMEBUFFER_HEIGHT*SCREEN_DEPTH;


	//topLeftWord = bitmap + scrollX/16 + FRAMEBUFFER_LINE_PITCH/2*16;
	//topTileWord = bitmap + scrollX/16 + FRAMEBUFFER_LINE_PITCH/2*16;

	topLeft.dest = firstFetchWord + (FRAMEBUFFER_LINE_PITCH/2)*16 + 1;
	topLeft.tile = topLeftMapTile - LEVELMAP_WIDTH*1 - 1;

	topRight.dest = topLeft.dest + (FRAMEBUFFER_WIDTH/16) - 2;
	topRight.tile = topLeft.tile + (FRAMEBUFFER_WIDTH/16) - 2;

	bottomLeft.dest = topLeft.dest + (FRAMEBUFFER_LINE_PITCH/2)*16 * ((FRAMEBUFFER_HEIGHT/16)-2); //16 lassen wir wegen Übersicht
	bottomLeft.tile = topLeft.tile + LEVELMAP_WIDTH*((FRAMEBUFFER_HEIGHT/16)-2);

	bottomRight.dest = bottomLeft.dest + (FRAMEBUFFER_WIDTH/16) - 2;
	bottomRight.tile = bottomLeft.tile + (FRAMEBUFFER_WIDTH/16) - 2;

	horizontalScrollBlitTile.isActive=0;
	horizontalScrollBlitTile.dest=NULL;
	horizontalScrollBlitTile.tile=NULL;
	horizontalScrollBlitTile.destStep=0;
	horizontalScrollBlitTile.isRight_isDown=0;
	horizontalScrollBlitTile.tileStep=0;

	verticalScrollBlitTile.isActive=0;
	verticalScrollBlitTile.dest=NULL;
	verticalScrollBlitTile.tile=NULL;
	verticalScrollBlitTile.destStep=0;
	verticalScrollBlitTile.isRight_isDown=0;
	verticalScrollBlitTile.tileStep=0;

	//uart_printf("lastFetchWord %x\n",lastFetchWord);
	//uart_printf("hiddenBlitColumnBottom %x\n",hiddenBlitColumnBottom);
	//hiddenBlitColumn = bitmap + 18; //for testing, weil man es sieht

	//firstFetchWord = topLeftWord - EXTRA_FETCH_WORDS;

	//Da wir ein Word früher anfangen zu fetchen...

	scrollXDelay=0;
	scrollYLine=32;


	custom.bplcon1=0;

	uint16_t *dest=firstFetchWord + (FRAMEBUFFER_LINE_PITCH/2)*16 + 1;
	uint8_t *src = topLeftMapTile-LEVELMAP_WIDTH-1;

	//src = mapData-1;
#if 1

	for (y=0; y<SCREEN_HEIGHT/16 + 2; y++) //2 extra Tiles für oben und unten
	//for (y=0; y<3;y++)
	{
		uint16_t *lineStartDest = dest;
		uint8_t *lineStartSrc = src;

		//src = &mapData[99];
		for (x=0; x<SCREEN_WIDTH/16 + 2; x++) //2 extra Tiles für links und rechts
		{
			uint8_t tileId = *src;
			//uint8_t tileId = 0;

			blitTile(tileId,dest);


			dest++; //ein Wort weiter nach rechts
			src++;
		}

		//Wir sind nun am Rand angelangt. Das Wort nun per LINE Modulo auf den Anfang der nächsten Zeile und dann mit TODO

		dest = lineStartDest + (FRAMEBUFFER_LINE_PITCH/2)*16 ;
		src = lineStartSrc + LEVELMAP_WIDTH;
	}

#endif
}


