/*
 * unit.h
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */

#ifndef SRC_UNIT_H_
#define SRC_UNIT_H_

#include <stdint.h>

#include "AStar.h"

namespace Game
{

class Unit
{
public:
	Unit();
	virtual ~Unit();

	void simulate();
	void blit();
	void init();
	void walkTo(int endX, int endY, AStar &astar);

protected:
	uint16_t posX, posY;
	uint8_t animCnt;

	uint16_t* animationTable[10];
	AStarPath path;
	uint16_t wayPointX, wayPointY;
	bool moving;
};

}
#endif /* SRC_UNIT_H_ */
