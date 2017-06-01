/*
 * Harvester.h
 *
 *  Created on: 29.05.2017
 *      Author: andre
 */

#ifndef SRC_HARVESTER_H_
#define SRC_HARVESTER_H_

#include "Unit.h"

class Harvester: public Game::Unit
{
public:
	void simulate(AStar &astar);
	void harvest(AStar &astar);
	void blit();
	void init();

	Harvester();
	virtual ~Harvester();
private:
	enum
	{
		STATE_IDLE,
		STATE_HARVEST_IDLE,
		STATE_ON_WAY_TO_ORE,
		STATE_ON_WAY_TO_RAFFINERY,
	} state;

	int harvestedOre;

};

#endif /* SRC_HARVESTER_H_ */
