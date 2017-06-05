/*
 * Tank.h
 *
 *  Created on: 05.06.2017
 *      Author: andre
 */

#ifndef SRC_TANK_H_
#define SRC_TANK_H_

#include <Unit.h>

class Tank: public Game::Unit
{
public:
	virtual void simulate();
	virtual void blit();

	Tank(int16_t tileX, int16_t tileY);
	virtual ~Tank();
};

#endif /* SRC_TANK_H_ */
