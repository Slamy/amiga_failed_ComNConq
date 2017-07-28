/*
 * unit.h
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */

#ifndef SRC_UNIT_H_
#define SRC_UNIT_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "AStar.h"
extern "C"
{
#include "uart.h"
}
namespace Game
{

#define DEAD_ID 0xFF

class Unit
{
public:
	Unit();
	virtual ~Unit();

	virtual void simulate();
	virtual void blit() = 0;

	virtual bool walkTo(int16_t endX, int16_t endY);
	virtual bool specialAction(){return false;};

	unsigned char id;

protected:

	uint16_t posX, posY;
	uint16_t tileX, tileY;
	uint8_t animCnt;

	uint16_t* animationTable[10];
	AStarPath *path;
	uint16_t wayPointX, wayPointY;
	bool moving;

	uint16_t targetX, targetY;


	static AStar sharedAstar;


	enum
	{
		N,S,E,W,
		NE,NW,SW,SE,
	} facingDirection;
};

#define UNIT_POOL_SIZE 40
class PaddedUnit : public Unit
{
public:
	//PaddedUnit(){uart_puts((char*)"Constructor PaddedUnits\n");};
	virtual void blit(){};
	virtual bool specialAction(){return false;};

	uint16_t padding[10];
};


#define UNIT_POOL_SIZE 40
class UnitPool
{
public:

	Game::PaddedUnit units[UNIT_POOL_SIZE];
	Unit* unitAt(int16_t tileX, int16_t tileY);
	int lastAllocatedUnitId;
	uint8_t presenceMap[LEVELMAP_HEIGHT * LEVELMAP_WIDTH];
	
	UnitPool()
	{
		memset(presenceMap,DEAD_ID,sizeof(presenceMap));
		lastAllocatedUnitId=0;

	}

	void* allocate()
	{
		//for(auto &unit : units)
		int i;
		for (i=0; i < UNIT_POOL_SIZE; i++)
		{
			if (units[i].id == DEAD_ID)
			{
				lastAllocatedUnitId=i;
				return &units[i];
			}
		}
		return NULL;
	}

	void simulate()
	{
		for(auto &unit : units)
		{
			if (unit.id != DEAD_ID)
				unit.simulate();
		}
	}

	void blit()
	{
		for(auto &unit : units)
		{
			if (unit.id != DEAD_ID)
				unit.blit();
		}
	}

};


extern UnitPool unitpool;

}


void *operator new (size_t size, Game::UnitPool& pool);

#endif /* SRC_UNIT_H_ */
