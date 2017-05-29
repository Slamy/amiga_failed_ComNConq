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

#define MAX_WAYPOINT_ANZ 200

class AStarPath
{
public:
	struct waypoint waypoints[MAX_WAYPOINT_ANZ];
	int waypointsAnz;
};

#define OPEN_NODE_ANZ 500
#define CLOSED_NODE_ANZ 1400

class AStar
{
public:
	AStar();
	virtual ~AStar();

	static void init();
	bool findWay(int startX, int startY, int endX, int endY, AStarPath &path);
	bool findWayToTileType(int startX, int startY, char tileId, AStarPath &path);
	void visualize();

private:
	int openNodesAnz;
	int closedNodesAnz;

	struct node openNodes[OPEN_NODE_ANZ];
	struct node closedNodes[CLOSED_NODE_ANZ];
	struct node* nodeMap[LEVELMAP_HEIGHT*LEVELMAP_WIDTH];

	static bool tilePassable[30];
};

#endif /* SRC_ASTAR_H_ */
