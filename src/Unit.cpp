
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

#include "AStar.h"
#include "Unit.h"

namespace Game
{
/*
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
*/

AStar Unit::sharedAstar;
Unit* Unit::presenceMap[LEVELMAP_HEIGHT * LEVELMAP_WIDTH];

Unit::Unit()
{
	uart_printf("Unit Constructor\n");
	animCnt=0;
	posX=0;
	posY=0;
	tileX=0;
	tileY=0;
	alive=false;
	wayPointX=0;
	wayPointY=0;
	moving=false;
	targetX=0;
	targetY=0;
	facingDirection=N;
}

Unit::~Unit()
{
	alive=false;
	moving=false;
}

//uint8_t sineIndex=0;

void Unit::simulate()
{
	//posX = 50+sineTable[sineIndex];
	//posY = 50+sineTable[(sineIndex + sizeof(sineTable)/4)&0xff];
	//sineIndex++;

	if (moving)
	{
		moving=false;

		if (posX < wayPointX)
		{
			posX+=1;
			moving=true;
		}
		else if (posX > wayPointX)
		{
			posX-=1;
			moving=true;
		}

		if (posY < wayPointY)
		{
			posY+=1;
			moving=true;
		}
		else if (posY > wayPointY)
		{
			posY-=1;
			moving=true;
		}

	}

	if (!moving && path.waypointsAnz)
	{
		uint16_t newTileX = path.waypoints[path.waypointsAnz-1].posX;
		uint16_t newTileY = path.waypoints[path.waypointsAnz-1].posY;

		//Können wir uns überhaupt auf das neue Tile bewegen?
		if (presenceMap[newTileX + newTileY * LEVELMAP_WIDTH] == NULL || presenceMap[newTileX + newTileY * LEVELMAP_WIDTH] == this)
		{
			moving=true;

			//FIXME Mit LUT... oder ?
			if (newTileX > tileX)
			{
				if (newTileY > tileY)
					facingDirection = SE;
				else if (newTileY < tileY)
					facingDirection = NE;
				else
					facingDirection = E;
			}
			else if (newTileX < tileX)
			{
				if (newTileY > tileY)
					facingDirection = SW;
				else if (newTileY < tileY)
					facingDirection = NW;
				else
					facingDirection = W;
			}
			else
			{
				if (newTileY > tileY)
					facingDirection = S;
				else
					facingDirection = N;
			}

			//Da wo ich vorher war, bin ich nicht mehr.
			presenceMap[tileX + tileY * LEVELMAP_WIDTH] = NULL;

			//Belege den nun nächsten Tile als meine Position, obwohl ich noch nicht da bin.
			tileX = newTileX;
			tileY = newTileY;
			presenceMap[tileX + tileY * LEVELMAP_WIDTH] = this;


			wayPointX = tileX * 16;
			wayPointY = tileY * 16;

			path.waypointsAnz--;
		}
		else
		{
			path.waypointsAnz = 0;
			uart_printf("Plötzlich was im Weg... versuche neue Route\n");
			if (sharedAstar.findWay(tileX,tileY,targetX,targetY,path))
			{
				uart_printf("Glück gehabt.\n");
			}
			else
			{
				uart_printf("Kein neuer Weg gefunden\n");
			}
		}
	}
}

bool Unit::walkTo(int16_t endX, int16_t endY)
{
	if (sharedAstar.findWay(tileX,tileY,endX,endY,path))
	{
		targetX = endX;
		targetY = endY;
		return true;
	}

	return false;
}


Unit* Unit::unitAt(int16_t tileX, int16_t tileY)
{

	return presenceMap[tileX + tileY* LEVELMAP_WIDTH];
}


}



void *operator new (size_t size, Game::UnitPool& pool)
{
	uart_assert(size <= sizeof(Game::PaddedUnit));
    return pool.allocate();
}


