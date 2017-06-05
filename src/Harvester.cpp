/*
 * Harvester.cpp
 *
 *  Created on: 29.05.2017
 *      Author: andre
 */

#include "Harvester.h"

extern "C"
{
#include "uart.h"
#include "assets.h"
}

#define HARVESTER_CAPACITY 8

Harvester::Harvester(int16_t tileX, int16_t tileY)
{

	harvestedOre = 0;
	state = STATE_IDLE;
	alive=true;
	this->tileX = tileX;
	this->tileY = tileY;

	posX = tileX * 16;
	posY = tileY * 16;

	animationTable[0]=&assets->bobunit0[0*288/2];
	animationTable[1]=&assets->bobunit0[1*288/2];
	animationTable[2]=&assets->bobunit0[2*288/2];
	animationTable[3]=&assets->bobunit0[3*288/2];

	uart_printf("Harvester Constructor %d %d\n",tileX,tileY);

	presenceMap[tileX + tileY * LEVELMAP_WIDTH] = this;
}

Harvester::~Harvester()
{
	// TODO Auto-generated destructor stub
}

void Harvester::harvest()
{
	state = STATE_HARVEST_IDLE;
}

bool Harvester::specialAction()
{
	harvest();
	return true;
}

void Harvester::simulate()
{
	Unit::simulate();

	animCnt++;
	if (animCnt >= 4 * 8)
		animCnt = 0;

	switch (state)
	{
	case STATE_HARVEST_IDLE:
		if (harvestedOre < HARVESTER_CAPACITY && sharedAstar.findWayToTileType(tileX, tileY, 25,26, path) == true)
		{
			state = STATE_ON_WAY_TO_ORE;
			//uart_printf("state=STATE_ON_WAY_TO_ORE;\n");
		}
		else if (harvestedOre >= HARVESTER_CAPACITY && sharedAstar.findWayToTileType(tileX, tileY, 27,27, path) == true)
		{
			state = STATE_ON_WAY_TO_RAFFINERY;
			//uart_printf("state=STATE_ON_WAY_TO_RAFFINERY;\n");
		}
		else
		{
			state = STATE_IDLE;
		}

		break;
	case STATE_ON_WAY_TO_ORE:
	{

		uint8_t standingOnTileId = mapData[tileX + tileY * LEVELMAP_WIDTH];
		if (!moving && standingOnTileId >= 25 && standingOnTileId <= 26 )
		{
			//Ja, wir stehen und ja, wir sind auf Erz.
			if (standingOnTileId == 26)
				harvestedOre+=1;
			else
				harvestedOre+=2;


			alterTile(posX/16,posY/16,1);
			//mapData[tileX + tileY * LEVELMAP_WIDTH] = 1;
			state = STATE_HARVEST_IDLE;
		}
		else if (!moving)
		{
			//Wir bewegen uns also nicht mehr und es ist kein Erz da???? Dann suchen wir weiter!
			state = STATE_HARVEST_IDLE;
		}

		break;
	}
	case STATE_ON_WAY_TO_RAFFINERY:
		if (!moving && mapData[tileX + tileY * LEVELMAP_WIDTH] == 27)
		{
			state = STATE_HARVEST_IDLE;
			harvestedOre = 0;
		}
		else if (!moving)
		{
			//Wir bewegen uns also nicht mehr und es ist keine Raffinerie da???? Dann suchen wir weiter!
			state = STATE_HARVEST_IDLE;
		}

		break;

	default:
		break;
	}
}

void Harvester::blit()
{
	//uart_printf("%d %p\n",animCnt,animationTable[animCnt / 32]);
	blitMaskedBob_mapCoordinate(animationTable[animCnt / 8], posX, posY - 14, 16, 24);
}

//Harvester benötigt Custom WalkTo, wegen Harvesting Funktion
bool Harvester::walkTo(int16_t endX, int16_t endY)
{
	state = STATE_IDLE;

	if (sharedAstar.findWay(tileX,tileY,endX,endY,path))
	{
		targetX = endX;
		targetY = endY;

		uint8_t targetTileId = mapData[targetX + targetY * LEVELMAP_WIDTH];
		if (targetTileId >= 25 && targetTileId <= 26 )
		{
			state = STATE_HARVEST_IDLE;
		}

		return true;
	}

	return false;
}
