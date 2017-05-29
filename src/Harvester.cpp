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
}

Harvester::Harvester()
{
	// TODO Auto-generated constructor stub
	harvestedOre = 0;

}

Harvester::~Harvester()
{
	// TODO Auto-generated destructor stub
}

void Harvester::harvest(AStar &astar)
{
	state = STATE_HARVEST_IDLE;
}

void Harvester::simulate(AStar &astar)
{
	Unit::simulate();

	animCnt++;
	if (animCnt >= 3 * 16)
		animCnt = 0;

	switch (state)
	{
	case STATE_HARVEST_IDLE:
		if (harvestedOre < 4 && astar.findWayToTileType(posX / 16, posY / 16, 25, path) == true)
		{
			state = STATE_ON_WAY_TO_ORE;
			uart_printf("state=STATE_ON_WAY_TO_ORE;\n");
		}
		else if (harvestedOre >= 4 && astar.findWayToTileType(posX / 16, posY / 16, 27, path) == true)
		{
			state = STATE_ON_WAY_TO_RAFFINERY;
			uart_printf("state=STATE_ON_WAY_TO_RAFFINERY;\n");
		}
		else
		{
			state = STATE_IDLE;
		}

		break;
	case STATE_ON_WAY_TO_ORE:
		if (!moving && mapData[posX / 16 + posY / 16 * LEVELMAP_WIDTH] == 25)
		{
			//Ja, wir stehen und ja, wir sind auf Erz.
			harvestedOre++;
			mapData[posX / 16 + posY / 16 * LEVELMAP_WIDTH] = 1;
			state = STATE_HARVEST_IDLE;
		}

		break;
	case STATE_ON_WAY_TO_RAFFINERY:
		if (!moving && mapData[posX / 16 + posY / 16 * LEVELMAP_WIDTH] == 27)
		{
			state = STATE_HARVEST_IDLE;
			harvestedOre = 0;
		}

		break;

	default:
		break;
	}
}

void Harvester::blit()
{
	blitMaskedBob_mapCoordinate(animationTable[animCnt / 16], posX, posY - 14, 16, 24);
}

