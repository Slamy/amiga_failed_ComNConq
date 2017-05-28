/*
 * astar.h
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */

#ifndef SRC_ASTAR_H_
#define SRC_ASTAR_H_

#include <stdint.h>
extern "C"
{
#include "scrollEngine.h"
}


struct node
{
	uint16_t posX;
	uint16_t posY;
	uint16_t costHeuristic;
	uint16_t costYet;
	uint16_t costSum;
	uint16_t fromX;
	uint16_t fromY;
	//bool closed;
};

struct waypoint
{
	uint16_t posX;
	uint16_t posY;
};


#define OPEN_NODE_ANZ 100
#define CLOSED_NODE_ANZ 200

class astar
{
public:
	astar();
	virtual ~astar();

	void init();
	void findWay(int startX, int startY, int endX, int endY, struct waypoint* waypoints, int *wayPointAnz);
	void visualize();

private:
	int openNodesAnz;
	int closedNodesAnz;

	struct node openNodes[OPEN_NODE_ANZ];
	struct node closedNodes[CLOSED_NODE_ANZ];
	struct node* nodeMap[LEVELMAP_HEIGHT*LEVELMAP_WIDTH];
};

#endif /* SRC_ASTAR_H_ */
