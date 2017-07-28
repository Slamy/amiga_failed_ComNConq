/*
 * Tank.cpp
 *
 *  Created on: 05.06.2017
 *      Author: andre
 */

#include <Tank.h>

extern "C"
{
#include "uart.h"
#include "assets.h"
}

static const uint16_t* graphicBOB[]=
{
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
};

static const int16_t graphicHeight[]=
{
		12,12,
		8,8,
		13,13,
		12,12,
};

Tank::Tank(int16_t tileX, int16_t tileY)
{
	id = Game::unitpool.lastAllocatedUnitId;
	this->tileX = tileX;
	this->tileY = tileY;

	posX = tileX * 16;
	posY = tileY * 16;

	graphicBOB[0] = assets->tank0_0;
	graphicBOB[1] = assets->tank0_1;
	graphicBOB[2] = assets->tank0_2;
	graphicBOB[3] = assets->tank0_3;
	graphicBOB[4] = assets->tank0_4;
	graphicBOB[5] = assets->tank0_5;
	graphicBOB[6] = assets->tank0_6;
	graphicBOB[7] = assets->tank0_7;

	uart_printf("Tank Constructor %d %d\n",tileX,tileY);

	Game::unitpool.presenceMap[tileX + tileY * LEVELMAP_WIDTH] = id;

}

Tank::~Tank()
{
	// TODO Auto-generated destructor stub
}


void Tank::simulate()
{
	Unit::simulate();
}

void Tank::blit()
{
	uart_printf("Tank blit %d %p %p\n",facingDirection,graphicBOB[facingDirection],assets->tank0_0);
	blitMaskedBob_mapCoordinate(graphicBOB[facingDirection], posX, posY + 16/2 - graphicHeight[facingDirection]/2, 16, graphicHeight[facingDirection]);
	//blitMaskedBob_mapCoordinate(assets->tank0_0, posX, posY - graphicHeight[facingDirection]/2, 16, graphicHeight[facingDirection]);

}

