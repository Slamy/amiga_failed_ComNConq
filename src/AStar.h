/*
 * astar.h
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */

#ifndef SRC_ASTAR_H_
#define SRC_ASTAR_H_

#include <stdint.h>
#include <stdlib.h>

extern "C"
{
#include "scrollEngine.h"
}


struct node
{

	uint8_t costHeuristic;
	uint8_t costYet;
	uint8_t costSum;
	uint8_t fromX;
	uint8_t fromY;
	uint8_t posX;
	uint8_t posY;
	//bool closed;
};

struct waypoint
{
	uint8_t posX;
	uint8_t posY;
};

#define MAX_WAYPOINT_ANZ 200

class AStarPath
{
public:
	struct waypoint waypoints[MAX_WAYPOINT_ANZ];
	int waypointsAnz;
};

#define OPEN_NODE_ANZ 180
#define CLOSED_NODE_ANZ 1400

#define PATHES_ANZ 8

class AStar
{
public:
	AStar();
	virtual ~AStar();

	static void init();
	bool findWay(int16_t startX, int16_t startY, int16_t endX, int16_t endY, AStarPath **path);
	bool findWayToTileType(int16_t startX, int16_t startY, char targetTileIdMin, char targetTileIdMax, AStarPath **path);
	void visualize();

private:
	int openNodesAnz;
	int closedNodesAnz;

	struct node openNodes[OPEN_NODE_ANZ];
	struct node closedNodes[CLOSED_NODE_ANZ];
	struct node* nodeMap[LEVELMAP_HEIGHT*LEVELMAP_WIDTH];

	AStarPath pathes[PATHES_ANZ];

	AStarPath *getFreePath()
	{
		for(auto &path : pathes)
		{
			if (path.waypointsAnz==0)
				return &path;
		}
		return NULL;
	}

};

#endif /* SRC_ASTAR_H_ */
