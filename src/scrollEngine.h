/*
 * scrollEngine.h
 *
 *  Created on: 29.04.2017
 *      Author: andre
 */

#ifndef SRC_SCROLLENGINE_H_
#define SRC_SCROLLENGINE_H_

#include <stdint.h>

//tatsächlich sichtbares Window
#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	256
#define SCREEN_DEPTH	5

#define EXTRA_TILES_HORIZONTAL		4
#define EXTRA_TILES_VERTICAL		4
#define EXTRA_PIXELS_HORIZONTAL		(EXTRA_TILES_HORIZONTAL*16)
#define EXTRA_PIXELS_VERTICAL		(EXTRA_TILES_VERTICAL*16)

#define EXTRA_FETCH_WORDS			1

//Größe der Bitmap. Wichtig ist die Teilbarkeit durch 16.
#define FRAMEBUFFER_WIDTH (SCREEN_WIDTH + EXTRA_PIXELS_HORIZONTAL)
#define FRAMEBUFFER_HEIGHT (SCREEN_HEIGHT + EXTRA_PIXELS_VERTICAL)

#define FRAMEBUFFER_PLANE_MODULO (EXTRA_PIXELS_HORIZONTAL/8)
#define FRAMEBUFFER_LINE_MODULO (FRAMEBUFFER_PLANE_MODULO + (FRAMEBUFFER_WIDTH*(SCREEN_DEPTH-1))/8)

#define FRAMEBUFFER_LINE_PITCH (FRAMEBUFFER_WIDTH*SCREEN_DEPTH/8)
#define FRAMEBUFFER_PLANE_PITCH (FRAMEBUFFER_WIDTH/8)


#define HORIZONTAL_SCROLL_WORDS 100 //für 100*16 extra Platz!

#define FRAMEBUFFER_SIZE	(FRAMEBUFFER_WIDTH/8*FRAMEBUFFER_HEIGHT*SCREEN_DEPTH + HORIZONTAL_SCROLL_WORDS*2)

#define LEVELMAP_HEIGHT 100
#define LEVELMAP_WIDTH 100

void scrollUp();
void scrollDown();
void scrollRight();
void scrollLeft();
void initVideo();
void waitVBlank();
void renderFullScreen();
void constructCopperList();
void alignOnTileBoundary();
int verifyVisibleWindow();
void setSpriteStruct(uint16_t *spriteStruct, int x, int y, int h);
void blitMaskedBob_mapCoordinate(uint16_t* src, int x, int y, int width, int height);
void blitMaskedACBMBob(uint16_t *dest, int shift, uint16_t* src, int width, int height );
void blitMaskedBob_screenCoordinate(uint16_t* src, int x, int y, int width, int height);
void restoreBackground();

extern uint16_t *mouseSpritePtr;

extern volatile uint16_t *bitmap;
extern uint8_t mapData[LEVELMAP_HEIGHT*LEVELMAP_WIDTH];

extern int scrollY;
extern int scrollX;

extern int debugVerticalScrollAmount;
extern int debugHorizontalScrollAmount;


#endif /* SRC_SCROLLENGINE_H_ */
