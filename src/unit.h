/*
 * unit.h
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */

#ifndef SRC_UNIT_H_
#define SRC_UNIT_H_

#include <stdint.h>
#include "astar.h"

class unit
{
public:
	unit();
	virtual ~unit();

	void simulate();
	void blit();
	void init();
	void walkTo(int endX, int endY);

private:
	uint16_t posX, posY;
	uint8_t animCnt;

	uint16_t* animationTable[10];
	struct waypoint waypoints[30];
	int waypointAnz;
};

#endif /* SRC_UNIT_H_ */
