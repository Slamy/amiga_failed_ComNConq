/*
 * astar.cpp
 *
 *  Created on: 28.05.2017
 *      Author: andre
 */

#include "AStar.h"
extern "C"
{
#include "uart.h"
}

#include <stdlib.h>
#include <string.h>


//#define DEBUG

AStar::AStar()
{
	openNodesAnz = 0;
	closedNodesAnz = 0;
}

AStar::~AStar()
{
	// TODO Auto-generated destructor stub
}

bool AStar::tilePassable[30];

void AStar::init()
{
	memset(tilePassable, 0, sizeof(tilePassable));
	tilePassable[1] = true; //Land
	tilePassable[2] = true; //Irgendwie verkohltes Land
	tilePassable[25] = true; //Erz1
	tilePassable[26] = true; //Erz2
	tilePassable[27] = true; //Raffinerie-Platz
}


void AStar::visualize()
{
	int x, y;
	char c;

	for (y = 0; y < 50; y++)
	{
		for (x = 0; x < 80; x++)
		{
			c = '.';

			if (nodeMap[x + y*LEVELMAP_WIDTH])
				c = 'X';
			else if (tilePassable[mapData[y * LEVELMAP_WIDTH + x]] == false)
				c = 'O';

			//c = mapData[y * LEVELMAP_WIDTH + x] + '0';

			uart_printChar(c);
		}
		uart_printChar('\n');
	}

}

static const int16_t newPosXTable[]={
		1,
		0,
		-1,
		0
};

static const int16_t newPosYTable[]={
		0,
		1,
		0,
		-1
};

bool AStar::findWay(int startX, int startY, int endX, int endY, AStarPath &path)
{
	memset(nodeMap, 0, sizeof(nodeMap));

	openNodes[0].costHeuristic = abs(startX - endX) + abs(startY - endY);
	openNodes[0].posX = startX;
	openNodes[0].posY = startY;
	openNodes[0].costYet = 0;
	openNodes[0].costSum = openNodes[0].costYet + openNodes[0].costHeuristic;

	openNodesAnz = 1;
	closedNodesAnz = 0;

	nodeMap[startY * LEVELMAP_WIDTH + startX] = &openNodes[0];
	int16_t i;
	path.waypointsAnz = 0;

	struct node* endPtr = NULL;

	for (;;)
	{
		//Wir suchen nun nach dem openNode der am nächsten am Ziel ist. Wir nehmen an, es ist der 1. und suchen nach besseren
		i=0;

		int16_t bestOpenNode = 0;
		int16_t bestOpenNodeCostSum = openNodes[0].costSum;

#ifdef DEBUG
		uart_printf("ON %d %d  %d + %d = %d\n", openNodes[i].posX, openNodes[i].posY,
				openNodes[i].costHeuristic,
				openNodes[i].costYet,
				openNodes[i].costSum
				);
#endif

		for (i = 1; i < openNodesAnz; i++)
		{
#ifdef DEBUG
			uart_printf("ON %d %d  %d + %d = %d\n", openNodes[i].posX, openNodes[i].posY,
							openNodes[i].costHeuristic,
							openNodes[i].costYet,
							openNodes[i].costSum
							);
#endif

			if (openNodes[i].costSum < bestOpenNodeCostSum)
			{
				bestOpenNode = i;
				bestOpenNodeCostSum = openNodes[i].costSum;
			}
		}

		//Wir haben nun den besten Open Node.

		//Füge diesen Knoten der Closed List hinzu
		//ASSERT(closedNodesAnz < CLOSED_NODE_ANZ);
		if (closedNodesAnz >= CLOSED_NODE_ANZ)
		{
			uart_printf("findway failed. overflow of closedNodes\n");
			visualize();
			return false;
		}

		closedNodes[closedNodesAnz] = openNodes[bestOpenNode];
		struct node* workNode = &closedNodes[closedNodesAnz];
		nodeMap[workNode->posY * LEVELMAP_WIDTH + workNode->posX] = workNode;
		closedNodesAnz++;

#ifdef DEBUG
		uart_printf("Neuer Closed Node OD:%d  %d %d\n",bestOpenNode, workNode->posX, workNode->posY);
		visualize();
#endif
		//Node vielleicht schon das Ziel?

		if (workNode->costHeuristic == 0)
		{
			//Ziel erreicht.
			endPtr = &openNodes[bestOpenNode];
			break;
		}

		//Erzeugen wir nun maximal 4 weitere open nodes in seiner Nähe

		for (i = 0; i < 4; i++)
		{
			uint16_t newX = workNode->posX + newPosXTable[i];
			uint16_t newY = workNode->posY + newPosYTable[i];

			//Teste, ob diese Position überhaupt möglich ist.

			if ( newY>= 0 && newX >= 0 && tilePassable[mapData[newY * LEVELMAP_WIDTH + newX]] == true) //FIXME Magic number
			{
				uint16_t newCostHeuristic = abs(newX - endX)
						+ abs(newY - endY);
				uint16_t newCostSum = newCostHeuristic + workNode->costYet + 1;

				//Gibt es schon einen Node auf diesem Feld?
				struct node *ptr = NULL;
				if (nodeMap[newY * LEVELMAP_WIDTH + newX] != NULL)
				{
					//Ist dieser Node besser oder genauso gut, als das was wir gerade erstellen wollen?
					if (nodeMap[newY * LEVELMAP_WIDTH + newX]->costSum <= newCostSum)
						ptr = NULL; //Wir bleiben dabei und erstellen nichts.
					else
						ptr = nodeMap[newY * LEVELMAP_WIDTH + newX]; //Er ist schlechter. Wir überschreiben ihn
				}
				else
				{
					//Wir erstellen einen neuen Knoten
					if (openNodesAnz >= OPEN_NODE_ANZ)
					{
						uart_printf("findway failed. overflow of openNodes\n");
						visualize();
						return false;
					}
					ptr = &openNodes[openNodesAnz];
					openNodesAnz++;
					nodeMap[newY * LEVELMAP_WIDTH + newX] = ptr;
#ifdef DEBUG
					uart_printf("Neuer Open Node %d %d   %d + %d = %d \n",newX, newY,
							newCostHeuristic,
							workNode->costYet + 1,
							newCostSum);
#endif
				}

				if (ptr)
				{
					ptr->posX = newX;
					ptr->posY = newY;
					ptr->costHeuristic = newCostHeuristic;
					ptr->costYet = workNode->costYet + 1;
					ptr->costSum = newCostSum;
					ptr->fromX = workNode->posX;
					ptr->fromY = workNode->posY;
				}

			}
		}

		//entferne den gewählten bestOpenNode aus den open nodes

		if (bestOpenNode != openNodesAnz - 1) // wenn nicht schon der letzte in der Liste ...
			openNodes[bestOpenNode] = openNodes[openNodesAnz - 1]; //schiebe den letzten zur aktuellen Position, um zu entfernen.
		openNodesAnz--;

	}
#ifdef DEBUG
	uart_printf("findWay complete. extrahiere route\n");
#endif

	if (endPtr)
	{
		struct waypoint* waypoints = path.waypoints;
		for(;;)
		{
#ifdef DEBUG
			uart_printf(" %d %d\n", endPtr->posX, endPtr->posY);
#endif

			if (path.waypointsAnz >= MAX_WAYPOINT_ANZ)
			{
				uart_printf("findway failed. overflow of waypoints\n");
				path.waypointsAnz = 0;
				return false;
			}

			waypoints->posX=endPtr->posX;
			waypoints->posY=endPtr->posY;
			waypoints++;
			path.waypointsAnz++;


			if (endPtr->posX == startX && endPtr->posY == startY)
				break;

			endPtr = nodeMap[endPtr->fromX + endPtr->fromY * LEVELMAP_WIDTH];
		}
	}


	return true;
}


bool AStar::findWayToTileType(int startX, int startY, char targetTileId, AStarPath &path)
{
	memset(nodeMap, 0, sizeof(nodeMap));

	openNodes[0].posX = startX;
	openNodes[0].posY = startY;
	openNodes[0].costYet = 0;

	openNodesAnz = 1;
	closedNodesAnz = 0;

	nodeMap[startY * LEVELMAP_WIDTH + startX] = &openNodes[0];
	int16_t i;

	struct node* endPtr = NULL;
	path.waypointsAnz = 0;
	for (;;)
	{
		//Wir suchen nun nach dem openNode der am wenigsten zurückgelegt hat. Wir nehmen an, es ist der 1. und suchen nach besseren
		i=0;

		int16_t bestOpenNode = 0;
		int16_t bestOpenNodeCostYet = openNodes[0].costYet;

#ifdef DEBUG
		uart_printf("ON %d %d  %d + %d = %d\n", openNodes[i].posX, openNodes[i].posY,
				openNodes[i].costHeuristic,
				openNodes[i].costYet,
				openNodes[i].costSum
				);
#endif

		for (i = 1; i < openNodesAnz; i++)
		{
#ifdef DEBUG
			uart_printf("ON %d %d  %d + %d = %d\n", openNodes[i].posX, openNodes[i].posY,
							openNodes[i].costHeuristic,
							openNodes[i].costYet,
							openNodes[i].costSum
							);
#endif

			if (openNodes[i].costYet < bestOpenNodeCostYet)
			{
				bestOpenNode = i;
				bestOpenNodeCostYet = openNodes[i].costYet;
			}
		}

		//Wir haben nun den besten Open Node.

		//Füge diesen Knoten der Closed List hinzu
		//ASSERT(closedNodesAnz < CLOSED_NODE_ANZ);
		if (closedNodesAnz >= CLOSED_NODE_ANZ)
		{
			uart_printf("findway failed. overflow of closedNodes\n");
			visualize();
			return false;
		}

		closedNodes[closedNodesAnz] = openNodes[bestOpenNode];
		struct node* workNode = &closedNodes[closedNodesAnz];
		nodeMap[workNode->posY * LEVELMAP_WIDTH + workNode->posX] = workNode;
		closedNodesAnz++;

#ifdef DEBUG
		uart_printf("Neuer Closed Node OD:%d  %d %d\n",bestOpenNode, workNode->posX, workNode->posY);
		visualize();
#endif
		//Node vielleicht schon das Ziel?

		if (mapData[workNode->posY * LEVELMAP_WIDTH + workNode->posX] == targetTileId)
		{
			//Ziel erreicht.
			endPtr = &openNodes[bestOpenNode];
			break;
		}

		//Erzeugen wir nun maximal 4 weitere open nodes in seiner Nähe

		for (i = 0; i < 4; i++)
		{
			uint16_t newX = workNode->posX + newPosXTable[i];
			uint16_t newY = workNode->posY + newPosYTable[i];

			//Teste, ob diese Position überhaupt möglich ist.

			if ( newY>= 0 && newX >= 0 && tilePassable[mapData[newY * LEVELMAP_WIDTH + newX]] == true) //FIXME Magic number
			{
				uint16_t newCostYet = workNode->costYet + 1;

				//Gibt es schon einen Node auf diesem Feld?
				struct node *ptr = NULL;
				if (nodeMap[newY * LEVELMAP_WIDTH + newX] != NULL)
				{
					//Ist dieser Node besser oder genauso gut, als das was wir gerade erstellen wollen?
					if (nodeMap[newY * LEVELMAP_WIDTH + newX]->costYet <= newCostYet)
						ptr = NULL; //Wir bleiben dabei und erstellen nichts.
					else
						ptr = nodeMap[newY * LEVELMAP_WIDTH + newX]; //Er ist schlechter. Wir überschreiben ihn
				}
				else
				{
					//Wir erstellen einen neuen Knoten
					if (openNodesAnz >= OPEN_NODE_ANZ)
					{
						uart_printf("findway failed. overflow of openNodes\n");
						visualize();
						return false;
					}
					ptr = &openNodes[openNodesAnz];
					openNodesAnz++;
					nodeMap[newY * LEVELMAP_WIDTH + newX] = ptr;
#ifdef DEBUG
					uart_printf("Neuer Open Node %d %d   %d + %d = %d \n",newX, newY,
							newCostHeuristic,
							workNode->costYet + 1,
							newCostSum);
#endif
				}

				if (ptr)
				{
					ptr->posX = newX;
					ptr->posY = newY;
					ptr->costYet = newCostYet;
					ptr->fromX = workNode->posX;
					ptr->fromY = workNode->posY;
				}

			}
		}

		//entferne den gewählten bestOpenNode aus den open nodes

		if (bestOpenNode != openNodesAnz - 1) // wenn nicht schon der letzte in der Liste ...
			openNodes[bestOpenNode] = openNodes[openNodesAnz - 1]; //schiebe den letzten zur aktuellen Position, um zu entfernen.
		openNodesAnz--;

	}
#ifdef DEBUG
	uart_printf("findWay complete. extrahiere route\n");
#endif

	if (endPtr)
	{
		struct waypoint* waypoints = path.waypoints;
		for(;;)
		{
#ifdef DEBUG
			uart_printf(" %d %d\n", endPtr->posX, endPtr->posY);
#endif

			if (path.waypointsAnz >= MAX_WAYPOINT_ANZ)
			{
				uart_printf("findway failed. overflow of waypoints\n");
				path.waypointsAnz = 0;
				return false;
			}

			waypoints->posX=endPtr->posX;
			waypoints->posY=endPtr->posY;
			waypoints++;
			path.waypointsAnz++;


			if (endPtr->posX == startX && endPtr->posY == startY)
				break;

			endPtr = nodeMap[endPtr->fromX + endPtr->fromY * LEVELMAP_WIDTH];
		}
	}


	return true;
}




