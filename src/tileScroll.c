
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


#define KEYCODE_W 0x11
#define KEYCODE_A 0x20
#define KEYCODE_S 0x21
#define KEYCODE_D 0x22
#define KEYCODE_G 0x24
#define KEYCODE_H 0x25


extern uint16_t *tilemapChip;
uint8_t *chipMemMod=NULL;
extern uint16_t *bitmap;

extern struct Custom custom;
extern volatile struct CIA ciaa, ciab;

extern uint8_t mapData[LEVELMAP_HEIGHT*LEVELMAP_WIDTH];
extern unsigned char leveldata[];

extern int scrollY;
extern int scrollX;

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

static inline void mt_mastervol(void)
{
	//PTreplay Lautstärke auf 20 von 0 bis 64
	asm (	"move.w #60,d0\n"
			"jsr mt_mastervol\n"
		 : //no outputs
		 : //no inputs
		 : "d0");
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

#define SC_UP 1
#define SC_RIGHT 2
#define SC_DOWN 3
#define SC_LEFT 4


unsigned char camMoveScript[]={
	4+32,	SC_DOWN,
	32+4,	SC_RIGHT,
	32, 	SC_UP,
	0, 		0
};

unsigned char *camMoveScriptPtr = camMoveScript;
unsigned char camMoveScriptStepsTaken=0;

void overtake()
{
	int i,x,y;
	
	tilemapChip=AllocMem(1000, MEMF_CHIP);
	assert(tilemapChip);
	chipMemMod = AllocMem(94848, MEMF_CHIP);
	assert(chipMemMod);

	initVideo();
	

	for (i=0;i<FRAMEBUFFER_SIZE/2;i++)
	{
		bitmap[i]=0x5555;
	}

	for (i=0;i<sizeof(mapData);i++)
	{
		mapData[i]=2;
	}


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

	for (i=0;i<sizeof(mapData);i++)
	{
		mapData[i]=leveldata[i]-1;
	}


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

	for (i=0 ; i<sizeof(tilemap)/2; i++)
	{
		tilemapChip[i] = tilemap[i];
	}
	
#if 0
	mt_install_cia();
	mt_init(chipMemMod);
	mt_mastervol();
	
	mt_Enable=1; //PTreplay darf abspielen
#endif

	vBlankCounterLast=vBlankCounter;

	renderFullScreen();
	constructCopperList();

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

	while(1)
	{
		//bitmap[vBlankCounter>>3]=0xFFFF;

		mouseCheck();

#if 0
		toScrollX+= mouseXDiff;
		toScrollY+= mouseYDiff;
#endif

		/*
		uart_printf("ciacra %x\n",ciaa.ciacra);
		UBYTE cia_interrupt=ciaa.ciaicr;
		if (cia_interrupt)
		{

			uart_printf("Key:%x %p %x\n",cia_interrupt,&ciaa.ciaicr,ciaa.ciasdr);
		}
		*/
		keyboardCheck();

		if (keyPress != -1)
		{
			switch (keyPress)
			{
			case KEYCODE_S:
				scrollDown();
				break;
			case KEYCODE_W:
				scrollUp();
				break;
			case KEYCODE_A:
				scrollLeft();
				break;
			case KEYCODE_D:
				scrollRight();
				break;
			case KEYCODE_G:
				if (camMoveScriptPtr[0]!=0)
				{
					switch (camMoveScriptPtr[1])
					{
					case SC_LEFT: scrollLeft(); break;
					case SC_RIGHT: scrollRight(); break;
					case SC_UP: scrollUp(); break;
					case SC_DOWN: scrollDown(); break;
					}
					camMoveScriptStepsTaken++;
					if (camMoveScriptStepsTaken==camMoveScriptPtr[0])
					{
						camMoveScriptStepsTaken=0;
						camMoveScriptPtr+=2;
					}
				}
				break;
			case KEYCODE_H:
				scrollX=0;
				scrollY=0;
				renderFullScreen();
				camMoveScriptPtr=camMoveScript;
				break;
			}

			//keyPress=-1;
		}

#if 1

		for (i=0; i < 20;i++)
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


			printf("%d %d %d\n",mouseY,mouseYLast,mouseYDiff);

			mouseYLast = mouseY;

		}
	}
}


int main()
{
	//maustest();
	uart_printf("Howdy %d!\n", 2);

	overtake();
	return 0;
	
}





