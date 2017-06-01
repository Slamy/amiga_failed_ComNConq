
#ifndef SRC_ASSETS_H_
#define SRC_ASSETS_H_

#include <stdint.h>

struct assets
{
	uint16_t tilemap[5120/2];
	uint8_t protrackerModule_alien[94848];
	uint8_t testSoundEffect[26262];
	uint16_t sprite_mouseCursor1[64/2+4];
	uint16_t sprite_mouseCursor2[72/2+4];
	uint16_t testbob[12288/2];
	uint16_t bobunit0[4*288/2];
};

extern struct assets *assets;

int readAssets();

#endif
