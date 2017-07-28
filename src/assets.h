#ifndef SRC_ASSETS_H_
#define SRC_ASSETS_H_

#include <stdint.h>

struct assets
{
	uint16_t tilemap[5120 / 2];
	uint8_t protrackerModule_alien[94848];
	uint16_t testSoundEffect[26262 / 2 + 2];
	uint16_t sfx_ackno[8864 / 2 + 2];
	uint16_t sfx_await1[9600 / 2 + 2];
	uint16_t sprite_mouseCursor1[64 / 2 + 4];
	uint16_t sprite_mouseCursor2[72 / 2 + 4];
	uint16_t sprite_mouseCursor3[64 / 2 + 4];
	uint16_t sprite_mouseCursor4[64 / 2 + 4];
	uint16_t testbob[12288 / 2];
	uint16_t bobunit0[4 * 288 / 2];
	uint16_t tank0_0[144 / 2];
	uint16_t tank0_1[144 / 2];
	uint16_t tank0_2[96 / 2];
	uint16_t tank0_3[96 / 2];
	uint16_t tank0_4[156 / 2];
	uint16_t tank0_5[156 / 2];
	uint16_t tank0_6[144 / 2];
	uint16_t tank0_7[144 / 2];
	uint16_t buildYesTile[192 / 2];
	uint16_t buildNoTile[192 / 2];
};

extern struct assets *assets;

int readAssets();

#endif
