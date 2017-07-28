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

#include "asmstuff.h"
#include "copper.h"
#include "uart.h"

#include "scrollEngine.h"
#include "assets.h"


//#define DEBUG_VERTICAL_SCROLL	0
//#define DEBUG_HORIZONTAL_SCROLL	0
//#define DEBUG_PRINT
#define DEBUG_SCROLL_RUNTIME
#define DEBUG_BOB_RUNTIME
#define DEBUG_COPPER_WRAP
//#define DEBUG_DISABLE_BLIT


extern volatile struct Custom custom;


volatile uint16_t vBlankCounter=0;
uint16_t vBlankCounterLast;


int scrollXDelay=0;
int scrollXWord=0;


/*
 * Das Cornertile sitzt in den Ecken und bewegt sich nur bei Übergängen X|Y&0xf >= 0 oder <= 0xf.
 */
struct CornerTile
{
	uint16_t *dest;
	uint8_t *tile;
	int8_t x;
	int8_t y;
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

	int8_t x;
	int8_t y;
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

#define BACKGROUND_RESCUE_BUFFER_SIZE 5000


uint16_t* backgroundRescueBuffer;
uint16_t* backgroundRescueBufferWritePtr;


#define WRAPPED_TILE_MOVEPTR_DOWN(arg) \
	arg.dest += FRAMEBUFFER_LINE_PITCH/2*16; \
	if (arg.dest >= lastFetchWord) \
		arg.dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT; \
	arg.tile+=LEVELMAP_WIDTH; \
	arg.y++; \

#define WRAPPED_TILE_MOVEPTR_UP(arg) \
	arg.dest -= FRAMEBUFFER_LINE_PITCH/2*16; \
	if (arg.dest < firstFetchWord) \
		arg.dest += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT; \
	arg.tile -= LEVELMAP_WIDTH; \
	arg.y--; \

#define TILE_MOVEPTR_RIGHT(arg) \
	arg.dest++; \
	arg.tile++; \
	arg.x++; \

#define TILE_MOVEPTR_LEFT(arg) \
	arg.dest--; \
	arg.tile--; \
	arg.x--; \

volatile uint16_t *bitmap=NULL;
//uint16_t __attribute__((section(".data_chip"))) bitmap[FRAMEBUFFER_SIZE/2];
//uint16_t bitmap[FRAMEBUFFER_SIZE/2];

volatile uint16_t *copperlist=NULL;
uint16_t *spriteNullStruct=NULL;

uint16_t *mouseSpritePtr = NULL;

const uint16_t displayWindowVStart=44;
const uint16_t displayWindowVStop=300;
const uint16_t displayWindowHStart=128; //eigentlich 129.... aber Amiga Bugs in Hardware...?

uint8_t mapData[LEVELMAP_WIDTH*LEVELMAP_HEIGHT];


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
	COPPER_WRITE_32(SPR0PTH, mouseSpritePtr);
	COPPER_WRITE_32(SPR1PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR2PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR3PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR4PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR5PTH, spriteNullStruct);
	COPPER_WRITE_32(SPR6PTH, spriteNullStruct);
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
	COPPER_WAIT_VERTICAL((uint8_t)(displayWindowVStop+1))

#ifdef DEBUG_COPPER_WRAP
	copperlist[i++]=COLOR00;
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


void initVideo()
{
	uart_printf("Available Chip Mem %d \n",AvailMem(MEMF_CHIP));
	uart_printf("AllocMem bitmap %d\n",FRAMEBUFFER_SIZE);

	bitmap=AllocMem(FRAMEBUFFER_SIZE, MEMF_CHIP );
	uart_assert(bitmap);
	copperlist=AllocMem(300, MEMF_CHIP);
	uart_assert(copperlist);
	spriteNullStruct=AllocMem(32, MEMF_CHIP);
	uart_assert(spriteNullStruct);
	uart_printf("AllocMem backgroundRescueBuffer %d\n",BACKGROUND_RESCUE_BUFFER_SIZE*2);
	backgroundRescueBuffer = AllocMem(BACKGROUND_RESCUE_BUFFER_SIZE*2, MEMF_CHIP);
	uart_assert(backgroundRescueBuffer);
	backgroundRescueBufferWritePtr = backgroundRescueBuffer;


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

	spriteNullStruct[0]=0;
	spriteNullStruct[1]=0;
	spriteNullStruct[2]=0;
	spriteNullStruct[3]=0;

	mouseSpritePtr = spriteNullStruct;

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


	//topLeftWord = bitmap;
	//firstFetchWord = topLeftWord-EXTRA_FETCH_WORDS;

	constructCopperList();

	custom.cop1lc = (uint32_t)copperlist;
	custom.copjmp1 = 0;

	custom.dmacon = DMAF_SETCLR | DMAF_COPPER | DMAF_RASTER | DMAF_MASTER | DMAF_BLITTER | DMAF_SPRITE;


	custom.intena = 0x3fff; //disable all interrupts
	*(volatile uint32_t*)0x6c = (uint32_t)isr_verticalBlank; //set level 3 interrupt handler
	custom.intena = INTF_SETCLR | INTF_INTEN | INTF_COPER ; //set INTB_VERTB

	vBlankCounterLast=vBlankCounter;

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

void blitMaskedBob_mapCoordinate(const uint16_t* src, int x, int y, int width, int height)
{

	if (x < scrollX-32)
		return;
	if (y < scrollY-32)
		return;

	if (x > scrollX + SCREEN_WIDTH)
		return;
	if (y > scrollY + SCREEN_HEIGHT)
		return;

	uart_printf("Not clipped\n");

	uint16_t *dest = topLeft.dest +
				x/16 - scrollX/16 +
				(y-(scrollY&~0xf))*FRAMEBUFFER_LINE_PITCH/2 +
				1 + 16*FRAMEBUFFER_LINE_PITCH/2;
	int shift = x & 0xf;

	blitMaskedACBMBob(dest, shift, src, width, height);
}

void blitMaskedBob_screenCoordinate(const uint16_t* src, int x, int y, int width, int height)
{
	//FIXME Falsche Formel
	uint16_t *dest = topLeft.dest + x/16 + y*FRAMEBUFFER_LINE_PITCH/2 + 1 + 16*FRAMEBUFFER_LINE_PITCH/2;
	int shift = x & 0xf;

	blitMaskedACBMBob(dest, shift, src, width, height);
}


void restoreBackground()
{


#if 1
	uint16_t* backgroundRescueBufferReadPtr = backgroundRescueBufferWritePtr;
	while (backgroundRescueBufferReadPtr != backgroundRescueBuffer)
	{
#ifdef DEBUG_BOB_RUNTIME
		custom.color[0] = 0x0700;
#endif
		backgroundRescueBufferReadPtr-=4;

		uint16_t* dest = *((uint16_t**)backgroundRescueBufferReadPtr);
		uint16_t height = backgroundRescueBufferReadPtr[2];
		uint16_t destWidth = backgroundRescueBufferReadPtr[3];

		uint16_t blitsize = ((height) << HSIZEBITS) | destWidth;
		uint16_t packageSize = height*destWidth*2;

		backgroundRescueBufferReadPtr -= packageSize;

		while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

		//uart_printf("backgroundRescue angewendet %p %d %d %d %d\n", dest, height, destWidth,(FRAMEBUFFER_WIDTH)/8 - destWidth, blitsize);

		custom.bltcon0 = BC0F_SRCA | A_TO_D | BC0F_DEST;
		custom.bltapt = backgroundRescueBufferReadPtr;
		custom.bltdpt = dest;
		custom.bltamod = 0;
		custom.bltdmod = (FRAMEBUFFER_WIDTH)/8 - destWidth*2;
		custom.bltafwm = 0xffff;
		custom.bltalwm = 0xffff;
		custom.bltsize = blitsize; //starts blitter.


	}

#endif

	backgroundRescueBufferWritePtr = backgroundRescueBuffer;

#ifdef DEBUG_BOB_RUNTIME
	custom.color[0] = 0x0;
#endif
}


/*
 * Zeichnet ACBM Bob mit angefügter Maske in den Framebuffer an diese Adresse dest.
 * Der Copper-Wrap wird berücksichtigt.
 * Kein Clipping!
 */
void blitMaskedACBMBob(uint16_t *dest, int shift, const uint16_t* src, int width, int height)
{
	int i=0;

	//Schon mal dest korrigieren, falls das Ziel bereits über das Ende hinaus schießt
	if (dest >= lastFetchWord)
		dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

#ifdef DEBUG_BOB_RUNTIME
	custom.color[0] = 0x0077;
#endif

	uint16_t lastWordMask = 0xffff;
	uint16_t firstWordMask = 0xffff;
	int destMod = (FRAMEBUFFER_WIDTH - width)/8 + FRAMEBUFFER_PLANE_PITCH*(SCREEN_DEPTH-1);
	int16_t srcMod = 0;

	uint16_t srcWidth = width/16;
	uint16_t destWidth = srcWidth;
	//int height = 128;

	if (shift)
	{
#ifdef DEBUG_BOB_RUNTIME
	custom.color[0] = 0x0F77;
#endif
		srcMod = -2;
		destMod = (FRAMEBUFFER_WIDTH - width - 16)/8 + FRAMEBUFFER_PLANE_PITCH*(SCREEN_DEPTH-1);
		destWidth++;
		//lastWordMask = 0xffff << shift;
		lastWordMask = 0;
		//firstWordMask=0;
	}

	uint16_t* lastLineOffBlit = dest + (FRAMEBUFFER_LINE_PITCH/2) * (height-1);

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


	//Löse das Copper-Wrap Problem, in dem 2 mal geblittet wird
	if (lastLineOffBlit > lastFetchWord)
	{

		//Wir müssen teilen...
		int firstHeight = (lastFetchWord - dest) / (FRAMEBUFFER_LINE_PITCH/2) + 1;
		int secondHeight = height - firstHeight;

		//uart_printf("Das erste Stück ist %d hoch, dann %d!\n",firstHeight,secondHeight);

		checkPtrBoundary(dest);
		checkPtrBoundary(dest + (firstHeight-1) * (FRAMEBUFFER_LINE_PITCH/2));

		uint16_t firstHalf_blitsize = (firstHeight*SCREEN_DEPTH << HSIZEBITS) | destWidth;
		uint16_t secondHalf_blitsize = (secondHeight*SCREEN_DEPTH << HSIZEBITS) | destWidth;

		const uint16_t *firstHalf_bltapt = src + height*srcWidth*SCREEN_DEPTH; //Maske erste Hälfte
		const uint16_t *secondHalf_bltapt = firstHalf_bltapt + firstHeight * srcWidth; //Maske 2. Hälfte

		uint16_t *firstHalf_bltcpt = dest;
		uint16_t *secondHalf_bltcpt = firstHalf_bltcpt + firstHeight*(FRAMEBUFFER_LINE_PITCH/2) - FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;


		//Sichere beide Teile

		uint16_t packageSize = SCREEN_DEPTH * firstHeight * destWidth * 2;

		//uart_printf("backgroundRescue erstellt %p %d %d %d %d \n", dest, SCREEN_DEPTH*height, destWidth,(FRAMEBUFFER_WIDTH)/8 - destWidth, blitsize);
		if (backgroundRescueBufferWritePtr + packageSize >= &backgroundRescueBuffer[BACKGROUND_RESCUE_BUFFER_SIZE])
		{
			uart_printf("backgroundRescueBuffer overflow!\n");
			for(;;);
		}

		while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter
		custom.bltcon0 = BC0F_SRCA | A_TO_D | BC0F_DEST;
		custom.bltapt = firstHalf_bltcpt;
		custom.bltdpt = backgroundRescueBufferWritePtr;
		custom.bltamod = (FRAMEBUFFER_WIDTH)/8 - destWidth*2;
		custom.bltdmod = 0;
		custom.bltafwm = 0xffff;
		custom.bltalwm = 0xffff;
#ifndef DEBUG_DISABLE_BLIT
		custom.bltsize = firstHalf_blitsize; //starts blitter.
#endif
		backgroundRescueBufferWritePtr += packageSize;

		*((uint16_t**)backgroundRescueBufferWritePtr) = firstHalf_bltcpt; //Langwort für den Pointer
		backgroundRescueBufferWritePtr[2] = SCREEN_DEPTH*firstHeight;
		backgroundRescueBufferWritePtr[3] = destWidth;
		backgroundRescueBufferWritePtr+=4;

		packageSize = SCREEN_DEPTH * secondHeight * destWidth * 2;

		//uart_printf("backgroundRescue erstellt %p %d %d %d %d \n", dest, SCREEN_DEPTH*height, destWidth,(FRAMEBUFFER_WIDTH)/8 - destWidth, blitsize);
		if (backgroundRescueBufferWritePtr + packageSize >= &backgroundRescueBuffer[BACKGROUND_RESCUE_BUFFER_SIZE])
		{
			uart_printf("backgroundRescueBuffer overflow!\n");
			for(;;);
		}

		while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter
		custom.bltcon0 = BC0F_SRCA | A_TO_D | BC0F_DEST;
		custom.bltapt = secondHalf_bltcpt;
		custom.bltdpt = backgroundRescueBufferWritePtr;
		custom.bltamod = (FRAMEBUFFER_WIDTH)/8 - destWidth*2;
		custom.bltdmod = 0;
		custom.bltafwm = 0xffff;
		custom.bltalwm = 0xffff;
#ifndef DEBUG_DISABLE_BLIT
		custom.bltsize = secondHalf_blitsize; //starts blitter.
#endif

		backgroundRescueBufferWritePtr += packageSize;

		*((uint16_t**)backgroundRescueBufferWritePtr) = secondHalf_bltcpt; //Langwort für den Pointer
		backgroundRescueBufferWritePtr[2] = SCREEN_DEPTH*secondHeight;
		backgroundRescueBufferWritePtr[3] = destWidth;
		backgroundRescueBufferWritePtr+=4;


#ifdef DEBUG_BOB_RUNTIME
	custom.color[0] = 0x00FF;
#endif
		/* Zeichne nun beide Teile.
		 * Da wir hier von ACBM nach ILBM blitten, machen wir das ganze SCREEN_DEPTH mal.
		 * Erst schon mal alle Register setzen, die sich ohnehin nicht ändern.
		 */

		firstHalf_blitsize = (firstHeight << HSIZEBITS) | destWidth;
		secondHalf_blitsize = (secondHeight << HSIZEBITS) | destWidth;

		while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter
		//einmal alles setzen, was ohnehin gleich ist...
		custom.bltcon0 = BC0F_SRCA | BC0F_SRCB | BC0F_SRCC | ABC | ABNC | NABC | NANBC | BC0F_DEST | (shift<<ASHIFTSHIFT);
		custom.bltcon1 = (shift << BSHIFTSHIFT);
		custom.bltamod = srcMod;
		custom.bltbmod = srcMod;
		custom.bltcmod = destMod;
		custom.bltdmod = destMod;
		custom.bltafwm = firstWordMask;
		custom.bltalwm = lastWordMask;

		custom.bltbpt = (void*)src; //Einmal setzen reicht! Danach führt der Blitter selbst nach!

#if 1
		for (i=0;i<SCREEN_DEPTH;i++)
		{
			//Erst die obere Hälfte
			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

			custom.bltapt = (void*)firstHalf_bltapt;
			custom.bltcpt = firstHalf_bltcpt;
			custom.bltdpt = firstHalf_bltcpt;
#ifndef DEBUG_DISABLE_BLIT
			custom.bltsize = firstHalf_blitsize; //starts blitter.
#endif
			//Zielpointer
			firstHalf_bltcpt += (FRAMEBUFFER_PLANE_PITCH/2);

#if 1
			//Dann die untere Hälfte nach dem Copper-Wrap

			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

			custom.bltapt = (void*)secondHalf_bltapt;
			custom.bltcpt = secondHalf_bltcpt;
			custom.bltdpt = secondHalf_bltcpt;
#ifndef DEBUG_DISABLE_BLIT
			custom.bltsize = secondHalf_blitsize; //starts blitter.
#endif
			//Zielpointer
			secondHalf_bltcpt += (FRAMEBUFFER_PLANE_PITCH/2);
#endif

		}
#endif


	}
	else
	{

		//erstmal den Framebuffer sichern. Wir kopieren vom Framebuffer in the Rescue Buffer

		uint16_t blitsize = ((SCREEN_DEPTH*height) << HSIZEBITS) | destWidth;
		uint16_t packageSize = SCREEN_DEPTH * height * destWidth * 2;

		//uart_printf("backgroundRescue erstellt %p %d %d %d %d \n", dest, SCREEN_DEPTH*height, destWidth,(FRAMEBUFFER_WIDTH)/8 - destWidth, blitsize);
		if (backgroundRescueBufferWritePtr + packageSize >= &backgroundRescueBuffer[BACKGROUND_RESCUE_BUFFER_SIZE])
		{
			uart_printf("backgroundRescueBuffer overflow!\n");
			for(;;);
		}

		while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter
		custom.bltcon0 = BC0F_SRCA | A_TO_D | BC0F_DEST;
		custom.bltapt = dest;
		custom.bltdpt = backgroundRescueBufferWritePtr;
		custom.bltamod = (FRAMEBUFFER_WIDTH)/8 - destWidth*2;
		custom.bltdmod = 0;
		custom.bltafwm = 0xffff;
		custom.bltalwm = 0xffff;
#ifndef DEBUG_DISABLE_BLIT
		custom.bltsize = blitsize; //starts blitter.
#endif

		backgroundRescueBufferWritePtr += packageSize;

		*((uint16_t**)backgroundRescueBufferWritePtr) = dest; //Langwort für den Pointer
		backgroundRescueBufferWritePtr[2] = SCREEN_DEPTH*height;
		backgroundRescueBufferWritePtr[3] = destWidth;
		backgroundRescueBufferWritePtr+=4;

#ifdef DEBUG_BOB_RUNTIME
	custom.color[0] = 0x00FF;
#endif
		/* Zeichne ganz normal als ganzes Stück.
		 * Da wir hier von ACBM nach ILBM blitten, machen wir das ganze SCREEN_DEPTH mal.
		 * Erst schon mal alle Register setzen, die sich ohnehin nicht ändern.
		 */
		while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter
		custom.bltcon0 = BC0F_SRCA | BC0F_SRCB | BC0F_SRCC | ABC | ABNC | NABC | NANBC | BC0F_DEST | (shift<<ASHIFTSHIFT);
		custom.bltcon1 = (shift << BSHIFTSHIFT);
		custom.bltamod = srcMod;
		custom.bltbmod = srcMod;
		custom.bltcmod = destMod;
		custom.bltdmod = destMod;
		custom.bltafwm = firstWordMask;
		custom.bltalwm = lastWordMask;

		const uint16_t *maskPtr = src + height*srcWidth*SCREEN_DEPTH;
		custom.bltbpt = (void*)src; //Einmal setzen reicht! Danach führt der Blitter selbst nach!

		uint16_t *framebufferPtr = dest;
		blitsize = (height << HSIZEBITS) | destWidth;

		for (i=0;i<SCREEN_DEPTH;i++)
		{
			while (custom.dmaconr & DMAF_BLTDONE); //warte auf blitter

			custom.bltapt = (void*)maskPtr; //Muss jedes Mal neu gesetzt werden, weil der Blitter selbst nachführt.
			custom.bltcpt = framebufferPtr;
			custom.bltdpt = framebufferPtr;
#ifndef DEBUG_DISABLE_BLIT
			custom.bltsize = blitsize; //starts blitter.
#endif
			framebufferPtr += (FRAMEBUFFER_PLANE_PITCH/2);
		}
	}

#ifdef DEBUG_BOB_RUNTIME
	custom.color[0] = 0x0;
#endif

	uart_printf("finished blit\n");

}




struct
{
	uint16_t *dest;
	uint8_t tileid;
	int8_t x;
	int8_t y;
} alteredTileBlits[10];
uint16_t alteredTileBlitsAnz=0;


void alterTile(uint16_t x, uint16_t y, uint8_t tileid)
{
	//first set inside the map data
	mapData[x + y * LEVELMAP_WIDTH] = tileid;

	int tileScrollX = scrollX/16;
	int tileScrollY = scrollY/16;

	//is this tile in the current window that is kept stable for scrolling ?
	if (x > tileScrollX -1 && x < tileScrollX + SCREEN_WIDTH/16 + 1 &&
			y > tileScrollY -1 && y < tileScrollY + SCREEN_HEIGHT/16 + 1) //FIXME formula might be wrong
	{
		uint16_t *dest = topLeft.dest + FRAMEBUFFER_LINE_PITCH/2*16 * (y - tileScrollY + 1) + (x - tileScrollX + 1);
		if (dest >= lastFetchWord)
			dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

		alteredTileBlits[alteredTileBlitsAnz].dest = dest;
		alteredTileBlits[alteredTileBlitsAnz].tileid = tileid;
		alteredTileBlits[alteredTileBlitsAnz].x = x;
		alteredTileBlits[alteredTileBlitsAnz].y = y;
		alteredTileBlitsAnz++;
		//blitTile(tileid, dest);
	}
}

void blitAlteredTiles()
{
	uint16_t i;
	for (i=0; i< alteredTileBlitsAnz;i++)
	{
		blitTile(alteredTileBlits[i].tileid,
				alteredTileBlits[i].dest,
				alteredTileBlits[i].x,
				alteredTileBlits[i].y
				);
	}
	alteredTileBlitsAnz=0;
}

static inline int verifyTile(uint8_t tileid, volatile uint16_t *dest)
{
	uint16_t *src = assets->tilemap + SCREEN_DEPTH * 16 * tileid;
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


void processHorizontalMovingTile(struct MovingTile *mt)
{
#ifdef DEBUG_PRINT
	if (mt == &horizontalScrollBlitTile)
		uart_printf("processMovingTile horizontalMove\n");
	else if (mt == &verticalScrollBlitTile)
		uart_printf("processMovingTile verticalMove\n");
#endif
	blitTile(*mt->tile, mt->dest, mt->x, mt->y);

	mt->dest += mt->destStep;
	mt->tile += mt->tileStep;
	mt->y++;

	if (mt->dest >= lastFetchWord)
		mt->dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;
	else if (mt->dest < firstFetchWord)
		mt->dest += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

	//mt->number--;
}

void processVerticalMovingTile(struct MovingTile *mt)
{
#ifdef DEBUG_PRINT
	if (mt == &horizontalScrollBlitTile)
		uart_printf("processMovingTile horizontalMove\n");
	else if (mt == &verticalScrollBlitTile)
		uart_printf("processMovingTile verticalMove\n");
#endif
	blitTile(*mt->tile, mt->dest, mt->x, mt->y);

	mt->dest += mt->destStep;
	mt->tile += mt->tileStep;
	mt->x++;

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
	uart_printf("Derp\n");

	uart_printf("%s %d %d\n",__func__,scrollY, scrollYLine);
	uart_printf("Derp\n");
#endif

	if (scrollY == 0)
		return;

#ifdef DEBUG_SCROLL_RUNTIME
	custom.color[0] = 0x0070;
#endif
	scrollY--;

	scrollYLine--;
	if (scrollYLine < 0)
		scrollYLine = FRAMEBUFFER_HEIGHT-1;

	if ((scrollY & 0xf)==0xf)
	{

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
		verticalScrollBlitTile.x	= topLeft.x;
		verticalScrollBlitTile.y 	= topLeft.y;

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

				processVerticalMovingTile(&verticalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_SCROLL_RUNTIME
	custom.color[0] = 0;
#endif
}

void scrollDown()
{
	int i;
#ifdef DEBUG_PRINT
	uart_printf("%s %d %d %d\n",__func__,scrollY, scrollYLine,scrollY&0xf);
#endif

#ifdef DEBUG_SCROLL_RUNTIME
	custom.color[0] = 0x0070;
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
		verticalScrollBlitTile.x	= bottomLeft.x;
		verticalScrollBlitTile.y 	= bottomLeft.y;

		verticalScrollBlitTile.destStep = 1;
		verticalScrollBlitTile.tileStep = 1;

		//verticalScrollBlitTile.number = FRAMEBUFFER_WIDTH/16;
		verticalScrollBlitTile.isRight_isDown=1;
		verticalScrollBlitTile.isActive = 1;
	}

	if ((scrollY & 0xf)==0)
	{

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
			processHorizontalMovingTile(&horizontalScrollBlitTile);
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

				processVerticalMovingTile(&verticalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_SCROLL_RUNTIME
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

#ifdef DEBUG_SCROLL_RUNTIME
	custom.color[0] = 0x0070;
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
		horizontalScrollBlitTile.x = topLeft.x;
		horizontalScrollBlitTile.y = topLeft.y;

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
				processHorizontalMovingTile(&horizontalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_SCROLL_RUNTIME
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

#ifdef DEBUG_SCROLL_RUNTIME
	custom.color[0] = 0x0070;
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
		horizontalScrollBlitTile.x = topRight.x;
		horizontalScrollBlitTile.y = topRight.y;

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
			processVerticalMovingTile(&verticalScrollBlitTile);

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
				processHorizontalMovingTile(&horizontalScrollBlitTile);
			}
		}
	}

#ifdef DEBUG_SCROLL_RUNTIME
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
	volatile uint16_t *dest=firstFetchWord + ((scrollYLine&~0xf)-16) * FRAMEBUFFER_LINE_PITCH/2 + 1;
	uint8_t *src;

	dest = topLeft.dest + (FRAMEBUFFER_LINE_PITCH/2)*16 + 1;
	src = topLeft.tile + LEVELMAP_WIDTH + 1;

	if (dest < firstFetchWord)
		dest += FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;
	if (dest >= lastFetchWord)
		dest -= FRAMEBUFFER_LINE_PITCH/2*FRAMEBUFFER_HEIGHT;

	for (y=0; y<SCREEN_HEIGHT/16 + 1; y++) //2 extra Tiles für oben und unten
	{
		volatile uint16_t *lineStartDest = dest;
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

	scrollXWord = (scrollX/16);

	firstFetchWord = (uint16_t*)bitmap + scrollXWord - 1;
	lastFetchWord = firstFetchWord + FRAMEBUFFER_WIDTH/16*FRAMEBUFFER_HEIGHT*SCREEN_DEPTH;


	//topLeftWord = bitmap + scrollX/16 + FRAMEBUFFER_LINE_PITCH/2*16;
	//topTileWord = bitmap + scrollX/16 + FRAMEBUFFER_LINE_PITCH/2*16;

	topLeft.dest = firstFetchWord + (FRAMEBUFFER_LINE_PITCH/2)*16 + 1;
	topLeft.tile = mapData + scrollX/16 + (scrollY/16)*LEVELMAP_WIDTH  - LEVELMAP_WIDTH*1 - 1;
	topLeft.x = (scrollX/16) - 1;
	topLeft.y = (scrollY/16) - 1;

	topRight.dest = topLeft.dest + (FRAMEBUFFER_WIDTH/16) - 2;
	topRight.tile = topLeft.tile + (FRAMEBUFFER_WIDTH/16) - 2;
	topRight.x = topLeft.x + (FRAMEBUFFER_WIDTH/16) - 2;
	topRight.y = topLeft.y;

	bottomLeft.dest = topLeft.dest + (FRAMEBUFFER_LINE_PITCH/2)*16 * ((FRAMEBUFFER_HEIGHT/16)-2); //16 lassen wir wegen Übersicht
	bottomLeft.tile = topLeft.tile + LEVELMAP_WIDTH*((FRAMEBUFFER_HEIGHT/16)-2);
	bottomLeft.x = topLeft.x;
	bottomLeft.y = topLeft.y + (FRAMEBUFFER_HEIGHT/16) - 2;;

	bottomRight.dest = bottomLeft.dest + (FRAMEBUFFER_WIDTH/16) - 2;
	bottomRight.tile = bottomLeft.tile + (FRAMEBUFFER_WIDTH/16) - 2;
	bottomRight.x = bottomLeft.x + (FRAMEBUFFER_WIDTH/16) - 2;;
	bottomRight.y = bottomLeft.y;

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
	uint8_t *src = topLeft.tile;

	//src = mapData-1;
#if 1
	uint8_t tileX = topLeft.x;
	uint8_t tileY = topLeft.y;

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

			blitTile(tileId,dest,tileX,tileY);

			dest++; //ein Wort weiter nach rechts
			src++;
			tileX++;
		}

		//Wir sind nun am Rand angelangt. Das Wort nun per LINE Modulo auf den Anfang der nächsten Tile-Zeile gelegt

		dest = lineStartDest + (FRAMEBUFFER_LINE_PITCH/2)*16 ;
		src = lineStartSrc + LEVELMAP_WIDTH;
		tileY++;
		tileX = topLeft.x;
	}

#endif
}


void setSpriteStruct(uint16_t *spriteStruct, int x, int y, int h)
{
	uint16_t spriteX = displayWindowHStart + x;
	uint16_t spriteYStart = displayWindowVStart + y;
	uint16_t spriteYEnd = spriteYStart + h;

	((volatile uint16_t *)spriteStruct)[0] = (spriteYStart<<8) | (spriteX>>1);
	((volatile uint16_t *)spriteStruct)[1] = (spriteYEnd<<8) | (((spriteYStart>>8)&1)<<2) | (((spriteYEnd>>8)&1)<<1) | (spriteX&1);
}


void waitVBlank()
{
	while (vBlankCounterLast == vBlankCounter);
	vBlankCounterLast=vBlankCounter;
}

char vBlankReached()
{
	if ( vBlankCounterLast != vBlankCounter)
	{
		vBlankCounterLast=vBlankCounter;
		return 1;
	}
	else
		return 0;
}

