
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

#define KEYCODE_1 0x01
#define KEYCODE_2 0x02
#define KEYCODE_3 0x03
#define KEYCODE_4 0x04

#define KEYCODE_Q 0x10
#define KEYCODE_W 0x11
#define KEYCODE_E 0x12
#define KEYCODE_R 0x13
#define KEYCODE_T 0x14
//#define KEYCODE_Z 0x15

#define KEYCODE_I 0x17

#define KEYCODE_A 0x20
#define KEYCODE_S 0x21
#define KEYCODE_D 0x22
#define KEYCODE_G 0x24
#define KEYCODE_H 0x25
#define KEYCODE_J 0x26
#define KEYCODE_K 0x27
#define KEYCODE_L 0x28


#define KEYCODE_NUM2	0x1E
#define KEYCODE_NUM4	0x2D
#define KEYCODE_NUM5	0x2E
#define KEYCODE_NUM6	0x2F
#define KEYCODE_NUM8	0x3E

#define POTINP_DATLY_MASK (1<<10)



extern uint16_t *tilemapChip;
uint8_t *chipMemMod=NULL;
uint8_t *chipTestSoundEffect=NULL;
uint16_t *testBobChip=NULL;
extern uint16_t *bitmap;

extern struct Custom custom;
extern volatile struct CIA ciaa, ciab;

extern uint8_t mapData[LEVELMAP_HEIGHT*LEVELMAP_WIDTH];
extern unsigned char leveldata[];

extern int scrollY;
extern int scrollX;

extern int debugVerticalScrollAmount;
extern int debugHorizontalScrollAmount;


void startDebugger() //stub function
{

}


extern volatile char mt_Enable asm ("mt_Enable");


static inline void mt_install_cia(void)
{
	//PTreplay installiert Interrupt Vektor für CIA
	asm (	"clr.b d0\n"
				"suba.l a0,a0\n"
				"lea %[custom],a6\n"
				"jsr mt_install_cia\n"
		     : //no outputs
		     : [custom] "irm" (custom)
		     : "a0","a6","d0");
}

static inline void mt_init(void *mod)
{
	//PTreplay initialisiert
	asm (	"move.b #0,d0\n"
			"move.l #0,a1\n"
			"lea %[custom],a6\n"
			"move.l %[module],a0\n"
			"jsr mt_init"
	     : //no outputs
	     : [custom] "irm" (custom),
		   [module] "irm" (mod)
	     : "a0","a6","d0","a1","a2");
}

static inline void mt_mastervol(uint16_t masterVolume)
{
	//PTreplay Lautstärke auf 20 von 0 bis 64
	asm (	"move.w %[masterVolume],d0\n"
			"jsr mt_mastervol\n"
		 : //no outputs
		 : [masterVolume] "irm" (masterVolume)
		 : "d0");
}


static inline void mt_soundfx(void *samplePtr, uint16_t sampleLen, uint16_t samplePeriod, uint16_t sampleVolume)
{
	//PTreplay initialisiert
	asm (	"move.w %[sampleLen],d0\n"
			"move.w %[samplePeriod],d1\n"
			"move.w %[sampleVolume],d2\n"
			"lea %[custom],a6\n"
			"move.l %[samplePtr],a0\n"
			"jsr mt_soundfx"
	     : //no outputs
	     : [custom] "irm" (custom),
		   [samplePtr] "irm" (samplePtr),
		   [samplePeriod] "irm" (samplePeriod),
		   [sampleVolume] "irm" (sampleVolume),
		   [sampleLen] "irm" (sampleLen)

	     : "a0","a6","d0","d2","d1");
}


volatile uint16_t vBlankCounter=0;
uint16_t vBlankCounterLast;

void waitVBlank()
{
	while (vBlankCounterLast == vBlankCounter);
	vBlankCounterLast=vBlankCounter;
}

//Based on https://github.com/keirf/Amiga-Stuff/blob/master/systest/keyboard.c

int keyPress = -1;
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
int16_t mouseYDiff=0;
int16_t mouseXDiff=0;
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

#define SC_END		0
#define SC_UP		1
#define SC_RIGHT	2
#define SC_DOWN		3
#define SC_LEFT		4
#define SC_UP16		5
#define SC_RIGHT16	6
#define SC_DOWN16	7
#define SC_LEFT16	8

unsigned char camMoveScript[9000];

//DOWN16 DOWN16 DOWN16 DOWN16 LEFT16 UP   1
//777781 fail on run 69631

unsigned char *camMoveScriptPtr = camMoveScript;
unsigned char camMoveScriptStepsTaken=0;

void executeCamMoveScriptPtr()
{
	int i;
	switch (*camMoveScriptPtr)
	{
	case SC_LEFT:		scrollLeft(); break;
	case SC_RIGHT:		scrollRight(); break;
	case SC_UP:			scrollUp(); break;
	case SC_DOWN:		scrollDown(); break;

	case SC_LEFT16:		for (i=0; i< 16;i++) scrollLeft(); break;
	case SC_RIGHT16:	for (i=0; i< 16;i++) scrollRight(); break;
	case SC_UP16:		for (i=0; i< 16;i++) scrollUp(); break;
	case SC_DOWN16:		for (i=0; i< 16;i++) scrollDown(); break;

	}

	if (*camMoveScriptPtr != SC_END)
		camMoveScriptPtr++;
}

void executeCamMoveScript()
{

	camMoveScriptPtr = camMoveScript;
	while (*camMoveScriptPtr != SC_END)
	{
		executeCamMoveScriptPtr();
	}
}

void initCamMoveScript()
{
	int i=0;
	for(i=0;i<sizeof(camMoveScript);i++)
	{
		camMoveScript[i]=SC_END;
	}
}

//Implementiere Tiefensuche
void incrementCamMoveScript()
{
	camMoveScriptPtr = camMoveScript;

	for(;;)
	{
		(*camMoveScriptPtr)++;
		if (*camMoveScriptPtr > SC_LEFT16)
		{
			*camMoveScriptPtr=1;
			camMoveScriptPtr++;
		}
		else
			return;
	}
}

void printCamMoveScriptWord()
{
	camMoveScriptPtr = camMoveScript;
	while (*camMoveScriptPtr != SC_END)
	{
		switch (*camMoveScriptPtr)
		{
		case SC_LEFT:		uart_printf("LEFT "); break;
		case SC_RIGHT:		uart_printf("RIGHT "); break;
		case SC_UP:			uart_printf("UP "); break;
		case SC_DOWN:		uart_printf("DOWN "); break;
		case SC_LEFT16:		uart_printf("LEFT16 "); break;
		case SC_RIGHT16:	uart_printf("RIGHT16 "); break;
		case SC_UP16:		uart_printf("UP16 "); break;
		case SC_DOWN16:		uart_printf("DOWN16 "); break;
		}
		camMoveScriptPtr++;
	}
	//uart_printf("\n");
}


void printCamMoveScriptNumber()
{
	camMoveScriptPtr = camMoveScript;
	while (*camMoveScriptPtr != SC_END)
	{
		uart_printf("%d",*camMoveScriptPtr);
		camMoveScriptPtr++;
	}
	//uart_printf("\n");
}

void ScrollUnitTest()
{
	uart_printf("ScrollUnitTest!\n");
	uint32_t i;

	initCamMoveScript();
	for(i=0; i < 16777216ULL;i++)
	{
		//printCamMoveScript();
		scrollX=128*2;
		scrollY=128;

		renderFullScreen();

		executeCamMoveScript();
		int ret = verifyVisibleWindow();

		if (ret)
		{
			//printCamMoveScriptNumber();
			printCamMoveScriptWord();
			uart_printf("  %d\n",ret);
		}

		if (ret)
			break;
		incrementCamMoveScript();


		if ((i&0x3ff)==0)
		{
			printCamMoveScriptNumber();
			uart_printf(" at %d\n",i);
		}
	}
	printCamMoveScriptNumber();
	uart_printf(" fail on run %d\n",i);
}

//Notizen

//LEFT DOWN RIGHT UP LEFT UP   1
//432141 fail on run 2160

//UP UP LEFT DOWN DOWN   1
//11433 fail on run 1029

//LEFT DOWN LEFT DOWN LEFT DOWN LEFT DOWN LEFT LEFT LEFT DOWN LEFT LEFT
//DOWN LEFT LEFT LEFT DOWN LEFT DOWN LEFT DOWN LEFT DOWN LEFT DOWN LEFT DOWN LEFT DOWN LEFT DOWN DOWN DOWN verifyTile failed 1

void overtake()
{
	int i,x,y;
	
	tilemapChip=AllocMem(sizeof(tilemap), MEMF_CHIP);
	assert(tilemapChip);
	chipMemMod = AllocMem(94848, MEMF_CHIP);
	assert(chipMemMod);
	chipTestSoundEffect = AllocMem(26262+2, MEMF_CHIP);
	assert(testSoundEffect);
	testBobChip=AllocMem(12288, MEMF_CHIP);
	assert(testBobChip);

	initVideo();
	

	for (i=0;i<FRAMEBUFFER_SIZE/2;i++)
	{
		bitmap[i]=0x5555;
	}

#if 1
	for (i=0;i<sizeof(mapData);i++)
	{
		mapData[i]=2;
	}
#endif

#if 0
	for (y=0;y<LEVELMAP_HEIGHT;y++)
	{
		for (x=0;x<LEVELMAP_WIDTH;x++)
		{
			mapData[y*LEVELMAP_WIDTH + x] = (y & 1) ? 0 : 2;
		}
	}

	for (i=0;i<LEVELMAP_HEIGHT;i++)
	{
		mapData[0*LEVELMAP_WIDTH + i]=(i & 1) ? 0 : 2;
		mapData[1*LEVELMAP_WIDTH + i]=(i & 2) ? 0 : 2;
		mapData[2*LEVELMAP_WIDTH + i]=(i & 4) ? 0 : 2;
		mapData[3*LEVELMAP_WIDTH + i]=(i & 8) ? 0 : 2;
		mapData[4*LEVELMAP_WIDTH + i]=(i & 16) ? 0 : 2;
		mapData[5*LEVELMAP_WIDTH + i]=(i & 32) ? 0 : 2;
		mapData[6*LEVELMAP_WIDTH + i]=(i & 64) ? 0 : 2;
	}

	for (i=0;i<LEVELMAP_HEIGHT;i++)
	{
		mapData[i*LEVELMAP_WIDTH + 0]=(i & 1) ? 0 : 2;
		mapData[i*LEVELMAP_WIDTH + 1]=(i & 2) ? 0 : 2;
		mapData[i*LEVELMAP_WIDTH + 2]=(i & 4) ? 0 : 2;
		mapData[i*LEVELMAP_WIDTH + 3]=(i & 8) ? 0 : 2;
		mapData[i*LEVELMAP_WIDTH + 4]=(i & 16) ? 0 : 2;
		mapData[i*LEVELMAP_WIDTH + 5]=(i & 32) ? 0 : 2;
		mapData[i*LEVELMAP_WIDTH + 6]=(i & 64) ? 0 : 2;
	}

#endif
#if 1
	for (i=0;i<sizeof(mapData);i++)
	{
		mapData[i]=leveldata[i]-1;
	}
#endif

	/*
	for (i=0;i<30;i++)
	{
		uart_printf("mapdata %p %d\n",&mapData[i*LEVELMAP_WIDTH + 0], i);
	}
	*/
/*
	mapData[0]=0;
	mapData[1]=0;
	mapData[2]=0;

	mapData[97]=0;
	mapData[98]=0;
	mapData[99]=0;

	mapData[100]=1;
	mapData[101]=1;
	mapData[102]=1;

	mapData[200]=3;
	*/


	volatile uint16_t *src=tilemap;
	volatile uint16_t *dest=bitmap;
	for (y=0;y<SCREEN_HEIGHT * SCREEN_DEPTH;y++)
	{
		for (x=0;x<SCREEN_WIDTH;x+=16)
		{
			*dest=*src;
			dest++;
			src++;
		}
		dest+=5;
	}



	//einmal alle 8 Farben durch in der ersten Zeile. 4 Pixel pro Farbe
	bitmap[0 + 0*FRAMEBUFFER_WIDTH/16] = 0x0F0F;
	bitmap[0 + 1*FRAMEBUFFER_WIDTH/16] = 0x00FF;
	bitmap[0 + 2*FRAMEBUFFER_WIDTH/16] = 0x0000;
	bitmap[0 + 3*FRAMEBUFFER_WIDTH/16] = 0x0000;
	bitmap[0 + 4*FRAMEBUFFER_WIDTH/16] = 0x0000;

	bitmap[0 + 5*FRAMEBUFFER_WIDTH/16] = 0x0F0F;
	bitmap[0 + 6*FRAMEBUFFER_WIDTH/16] = 0x00FF;
	bitmap[0 + 7*FRAMEBUFFER_WIDTH/16] = 0xFFFF;
	bitmap[0 + 8*FRAMEBUFFER_WIDTH/16] = 0x0000;
	bitmap[0 + 9*FRAMEBUFFER_WIDTH/16] = 0x0000;

	//in der letzten Zeile auch noch mal ein paar Werte
	bitmap[0 + ((FRAMEBUFFER_HEIGHT-1)*5+0)*FRAMEBUFFER_WIDTH/16] = 0x0F0F;
	bitmap[0 + ((FRAMEBUFFER_HEIGHT-1)*5+1)*FRAMEBUFFER_WIDTH/16] = 0x00FF;
	bitmap[0 + ((FRAMEBUFFER_HEIGHT-1)*5+2)*FRAMEBUFFER_WIDTH/16] = 0x0000;
	bitmap[0 + ((FRAMEBUFFER_HEIGHT-1)*5+3)*FRAMEBUFFER_WIDTH/16] = 0x0000;
	bitmap[0 + ((FRAMEBUFFER_HEIGHT-1)*5+4)*FRAMEBUFFER_WIDTH/16] = 0x0000;

	i=0;


	custom.intena = 0x3fff; //disable all interrupts
	*(uint32_t*)0x6c = (uint32_t)isr_verticalBlank; //set level 3 interrupt handler
	custom.intena = INTF_SETCLR | INTF_INTEN | INTF_COPER ; //set INTB_VERTB

	//Komplettes Protracker-Modul in Chip-Ram
	for (i=0 ; i<sizeof(protrackerModule_alien); i++)
	{
		chipMemMod[i] = protrackerModule_alien[i];
	}

	for (i=0 ; i<sizeof(testSoundEffect); i++)
	{
		chipTestSoundEffect[i] = testSoundEffect[i];
	}

	for (i=0 ; i<sizeof(tilemap)/2; i++)
	{
		tilemapChip[i] = tilemap[i];
	}

	for (i=0;i<sizeof(testbob)/2;i++)
		testBobChip[i] = testbob[i];

	
#if 0
	mt_install_cia();
	mt_init(chipMemMod);
	mt_mastervol(40);
	
	mt_Enable=1; //PTreplay darf abspielen
#endif

	vBlankCounterLast=vBlankCounter;

	//Startposition
	scrollX=128*2;
	scrollY=128;
	//scrollX=0;
	//scrollY=0;

	renderFullScreen();
	camMoveScriptPtr=camMoveScript;
	constructCopperList();

	//blitTestBlob();

	mouseDataLast = custom.joy0dat;

	int16_t toScrollY=0;
	int16_t toScrollX=0;

	/*
	for (i=0; i < 16*5;i++)
	{
		waitVBlank();
		scrollDown();
		waitVBlank();
	}

	for (i=0; i < 16*5;i++)
	{
		scrollRight();
		waitVBlank();
	}

	for (i=0; i < 16*5;i++)
	{
		scrollUp();
		waitVBlank();
	}
	*/

	//ScrollUnitTest();

	i=0;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_LEFT;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_DOWN;
	camMoveScript[i++]=SC_END;


	int mouseCursorX=0;
	int mouseCursorY=0;

	while(1)
	{
		//bitmap[vBlankCounter>>3]=0xFFFF;

		mouseCheck();

#if 0
		toScrollX+= mouseXDiff;
		toScrollY+= mouseYDiff;
#else
		mouseCursorX+=mouseXDiff;
		mouseCursorY+=mouseYDiff;
		if (mouseCursorX > (SCREEN_WIDTH-1))
			mouseCursorX = SCREEN_WIDTH-1;
		if (mouseCursorY > (SCREEN_HEIGHT-1))
			mouseCursorY = SCREEN_HEIGHT-1;
		if (mouseCursorX < 0)
			mouseCursorX = 0;
		if (mouseCursorY < 0)
			mouseCursorY = 0;

#endif

		/*
		uart_printf("ciacra %x\n",ciaa.ciacra);
		UBYTE cia_interrupt=ciaa.ciaicr;
		if (cia_interrupt)
		{

			uart_printf("Key:%x %p %x\n",cia_interrupt,&ciaa.ciaicr,ciaa.ciasdr);
		}
		*/
		//uart_printf("ciapra %x %x\n",ciaa.ciapra, custom.potinp);

		if (!(ciaa.ciapra & CIAF_GAMEPORT0))
		{
			//renderFullScreen();
			blitTestBlob_mapCoordinate(testBobChip, mouseCursorX, mouseCursorY, 128, 128);
		}

		if (!(custom.potinp & POTINP_DATLY_MASK))
		{
			toScrollY = -((SCREEN_HEIGHT/2) - mouseCursorY) / 10;
			toScrollX = -((SCREEN_WIDTH/2) - mouseCursorX) / 10;
		}

		keyboardCheck();

		if (keyPress != -1)
		{
			switch (keyPress)
			{
			case KEYCODE_NUM8:
				debugVerticalScrollAmount=-2;
				uart_printf("debugVerticalScrollAmount=-1;\n");
				keyPress=-1;
				break;
			case KEYCODE_NUM2:
				debugVerticalScrollAmount=2;
				uart_printf("debugVerticalScrollAmount=2;\n");
				keyPress=-1;
				break;
			case KEYCODE_NUM4:
				debugHorizontalScrollAmount=-2;
				uart_printf("debugHorizontalScrollAmount=-1;\n");
				keyPress=-1;
				break;
			case KEYCODE_NUM6:
				debugHorizontalScrollAmount=2;
				uart_printf("debugHorizontalScrollAmount=2;\n");
				keyPress=-1;
				break;
			case KEYCODE_NUM5:
				debugHorizontalScrollAmount=0;
				debugVerticalScrollAmount=0;
				uart_printf("debugScrollAmount disabled\n");

				mt_soundfx(chipTestSoundEffect, sizeof(testSoundEffect)/2, 230, 64);
				keyPress=-1;
				break;

			case KEYCODE_S:
				scrollDown();
				keyPress=-1;
				break;
			case KEYCODE_W:
				scrollUp();
				keyPress=-1;
				break;
			case KEYCODE_A:
				scrollLeft();
				keyPress=-1;
				break;
			case KEYCODE_D:
				scrollRight();
				keyPress=-1;
				break;
			case KEYCODE_G:
				//ScrollUnitTest();
				executeCamMoveScriptPtr();
				keyPress=-1;
				break;
			case KEYCODE_T:
				scrollX=128*2;
				scrollY=128*2;

				renderFullScreen();
				camMoveScriptPtr=camMoveScript;
				break;
			case KEYCODE_H:
				//renderFullScreen();
				//alignOnTileBoundary();
				if (verifyVisibleWindow())
					uart_printf("Verify failed!\n");
				else
					uart_printf("Verify success!\n");
				//camMoveScriptPtr=camMoveScript;
				keyPress=-1;
				break;

#if 0
			case KEYCODE_K:
				scrollDown();
				break;
			case KEYCODE_I:
				scrollUp();
				break;
			case KEYCODE_J:
				scrollLeft();
				break;
			case KEYCODE_L:
				scrollRight();
				break;
#else
			case KEYCODE_K:
				for (i=0;i<16;i++)
					scrollDown();
				keyPress=-1;
				break;
			case KEYCODE_I:
				for (i=0;i<16;i++)
					scrollUp();
				keyPress=-1;
				break;
			case KEYCODE_J:
				for (i=0;i<16;i++)
					scrollLeft();
				keyPress=-1;
				break;
			case KEYCODE_L:
				for (i=0;i<16;i++)
					scrollRight();
				keyPress=-1;
				break;
#endif

			case KEYCODE_Q:
				scrollDown();
				scrollRight();
				break;
			case KEYCODE_E:
				scrollLeft();
				scrollUp();
				break;
			}
		}

#if 1

		for (i=0; i < 40;i++)
		{
#if 0
			if (verifyVisibleWindow())
			{
				printCamMoveScriptWord();
			}
			else
#endif
			{
				if (toScrollY > 0)
				{
#if 0
					camMoveScriptPtr[0]=SC_DOWN;
					camMoveScriptPtr[1]=SC_END;
					camMoveScriptPtr++;
#endif
					scrollDown();
					toScrollY--;
				}

				if (toScrollY < 0)
				{
#if 0
					camMoveScriptPtr[0]=SC_UP;
					camMoveScriptPtr[1]=SC_END;
					camMoveScriptPtr++;
#endif
					scrollUp();
					toScrollY++;
				}

				if (toScrollX > 0)
				{
#if 0
					camMoveScriptPtr[0]=SC_RIGHT;
					camMoveScriptPtr[1]=SC_END;
					camMoveScriptPtr++;
#endif
					scrollRight();
					toScrollX--;
				}

				if (toScrollX < 0)
				{
#if 0
					camMoveScriptPtr[0]=SC_LEFT;
					camMoveScriptPtr[1]=SC_END;
					camMoveScriptPtr++;
#endif
					scrollLeft();
					toScrollX++;
				}
			}

		}
#else
		if (toScrollY > 2)
		{
			scrollDown();
			toScrollY=0;
		}

		if (toScrollY < -2)
		{
			scrollUp();
			toScrollY=0;
		}

#endif
		waitVBlank();

		setSpriteStruct(mouseCursorX, mouseCursorY, 16);
		constructCopperList();

	}


}


void maustest()
{
	int16_t mouseYLast = (custom.joy0dat>>8)&0xff;

	while(1)
	{
		//bitmap[vBlankCounter>>3]=0xFFFF;
		int16_t mouseY=(custom.joy0dat>>8)&0xff;

		if (mouseY != mouseYLast)
		{
			/*
			 * mouseY ist ein unsigned byte counter.
			 * 0x10 zu 0x15 wären 5 Schritte, weil 0x15 - 0x10 = 5
			 * 0xfe zu 0x03 sind aber auch 5 Schritte, weil es zum Overlap kommt.
			 * 0x03 - 0xfe sind aber -0x251
			 */

			int16_t mouseYDiff = (mouseY - mouseYLast);

			if (mouseYDiff >= 128)
				mouseYDiff -= 256;
			else if (mouseYDiff <= -128)
				mouseYDiff += 256;


			uart_printf("%d %d %d\n",mouseY,mouseYLast,mouseYDiff);

			mouseYLast = mouseY;

		}
	}
}


int main()
{
	//maustest();
	uart_printf("Howdy %d!\n", 42);

	printf("Testbob: %p\n",testbob);
	//printf("CACR = 0x%x\n",Supervisor(supervisor_getCACR));

	overtake();

	return 0;
	
}





