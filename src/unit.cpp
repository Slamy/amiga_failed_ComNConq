/*
 * unit.cpp
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
}

#include "astar.h"
#include "unit.h"

const unsigned char sineTable[]={
	40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,54,
	55,56,57,58,59,60,61,61,
	62,63,64,65,65,66,67,68,
	68,69,70,70,71,72,72,73,
	73,74,74,75,75,76,76,77,
	77,77,78,78,78,79,79,79,
	79,79,80,80,80,80,80,80,
	80,80,80,80,80,80,80,79,
	79,79,79,78,78,78,78,77,
	77,76,76,76,75,75,74,74,
	73,73,72,71,71,70,69,69,
	68,67,67,66,65,64,64,63,
	62,61,60,59,58,58,57,56,
	55,54,53,52,51,50,49,48,
	47,46,45,44,43,42,41,40,
	40,39,38,37,36,35,34,33,
	32,31,30,29,28,27,26,25,
	24,23,22,22,21,20,19,18,
	17,16,16,15,14,13,13,12,
	11,11,10,9,9,8,7,7,
	6,6,5,5,4,4,4,3,
	3,2,2,2,2,1,1,1,
	1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,
	1,1,1,2,2,2,3,3,
	3,4,4,5,5,6,6,7,
	7,8,8,9,10,10,11,12,
	12,13,14,15,15,16,17,18,
	19,19,20,21,22,23,24,25,
	26,26,27,28,29,30,31,32,
	33,34,35,36,37,38,39,40,
};

unit::unit()
{
	// TODO Auto-generated constructor stub
	animCnt=0;
	posX=16;
	posY=16;
	waypointAnz=0;
}

unit::~unit()
{
	// TODO Auto-generated destructor stub
}

void unit::init()
{
	animationTable[0]=&assets->bobunit0[0*288/2];
	animationTable[1]=&assets->bobunit0[1*288/2];
	animationTable[2]=&assets->bobunit0[2*288/2];
}

uint8_t sineIndex=0;

astar testAStar;

void unit::simulate()
{
	animCnt++;
	if (animCnt>=3*16)
		animCnt=0;

	//posX = 50+sineTable[sineIndex];
	//posY = 50+sineTable[(sineIndex + sizeof(sineTable)/4)&0xff];
	//sineIndex++;

	if (waypointAnz)
	{
		//bool reachedGoal = false;
		if (posX < waypoints[waypointAnz-1].posX)
			posX++;
		else if (posX > waypoints[waypointAnz-1].posX)
			posX--;
		else if (posY < waypoints[waypointAnz-1].posY)
			posY++;
		else if (posY > waypoints[waypointAnz-1].posY)
			posY--;
		else
		{
			waypointAnz--;
		}
	}
}

void unit::walkTo(int endX, int endY)
{
	testAStar.findWay(posX/16,posY/16,endX,endY,waypoints,&waypointAnz);
}

void unit::blit()
{
	blitMaskedBob_mapCoordinate(animationTable[animCnt/16], posX, posY, 16, 24);
}


