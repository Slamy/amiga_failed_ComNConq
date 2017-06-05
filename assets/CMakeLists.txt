cmake_minimum_required (VERSION 2.8.11)



add_custom_command (OUTPUT assets.bin
	COMMAND gifsicle --crop 0,0+64x128		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tilemap_tiled.gif
	
	COMMAND gifsicle --crop 64,0+16x16		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > sprite_mouse1.gif
	COMMAND gifsicle --crop 64,16+16x18		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > sprite_mouse2.gif
	COMMAND gifsicle --crop 64,34+16x16		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > sprite_mouse3.gif
	COMMAND gifsicle --crop 80,0+16x16		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > sprite_mouse4.gif
	
	COMMAND gifsicle --crop 151,93+128x128 	< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > testbob.gif
	
	COMMAND gifsicle --crop 0,128+16x24		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > bobunit0_0.gif
	COMMAND gifsicle --crop 16,128+16x24	< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > bobunit0_1.gif
	COMMAND gifsicle --crop 32,128+16x24	< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > bobunit0_2.gif
	COMMAND gifsicle --crop 48,128+16x24	< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > bobunit0_3.gif
	
	COMMAND gifsicle --crop 64,75+16x12		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_0.gif
	COMMAND gifsicle --crop 80,75+16x12		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_1.gif
	COMMAND gifsicle --crop 64,88+16x8		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_2.gif
	COMMAND gifsicle --crop 80,88+16x8		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_3.gif
	COMMAND gifsicle --crop 64,98+16x13		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_4.gif
	COMMAND gifsicle --crop 80,98+16x13		< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_5.gif
	COMMAND gifsicle --crop 64,115+16x12	< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_6.gif
	COMMAND gifsicle --crop 80,115+16x12	< ${CMAKE_CURRENT_SOURCE_DIR}/graphics.gif > tank0_7.gif
	
	COMMAND ../tools/gifToPlanar gifTileMap16ToILBM tilemap_tiled.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tilemap.bin
	COMMAND ../tools/gifToPlanar gifToSprite sprite_mouse1.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette_sprite01.txt sprite_mouse1.bin
	COMMAND ../tools/gifToPlanar gifToSprite sprite_mouse2.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette_sprite01.txt sprite_mouse2.bin
	COMMAND ../tools/gifToPlanar gifToSprite sprite_mouse3.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette_sprite01.txt sprite_mouse3.bin
	COMMAND ../tools/gifToPlanar gifToSprite sprite_mouse4.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette_sprite01.txt sprite_mouse4.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM testbob.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 testbob.bin
	
	COMMAND ../tools/gifToPlanar gifToMaskedACBM bobunit0_0.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 bobunit0_0.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM bobunit0_1.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 bobunit0_1.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM bobunit0_2.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 bobunit0_2.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM bobunit0_3.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 bobunit0_3.bin
	
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_0.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_0.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_1.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_1.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_2.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_2.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_3.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_3.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_4.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_4.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_5.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_5.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_6.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_6.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM tank0_7.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tank0_7.bin
	
	
	COMMAND dd if=/dev/zero of=zeroSpriteHeader bs=4 count=1
	COMMAND cat
		tilemap.bin
		${CMAKE_CURRENT_SOURCE_DIR}/alien.mod
		zeroSpriteHeader ${CMAKE_CURRENT_SOURCE_DIR}/ironchg1.raw
		zeroSpriteHeader ${CMAKE_CURRENT_SOURCE_DIR}/ackno.raw
		zeroSpriteHeader ${CMAKE_CURRENT_SOURCE_DIR}/await1.raw
		zeroSpriteHeader sprite_mouse1.bin zeroSpriteHeader
		zeroSpriteHeader sprite_mouse2.bin zeroSpriteHeader
		zeroSpriteHeader sprite_mouse3.bin zeroSpriteHeader
		zeroSpriteHeader sprite_mouse4.bin zeroSpriteHeader
		testbob.bin
		bobunit0_0.bin
		bobunit0_1.bin
		bobunit0_2.bin
		bobunit0_3.bin
		tank0_*.bin
		> assets.bin
	
	COMMAND ls -lh assets.bin
	DEPENDS gifToPlanar graphics.gif palette.txt palette_sprite01.txt
	)
add_custom_target (assets DEPENDS assets.bin)


add_custom_command (OUTPUT leveldata.c
	COMMAND ../tools/tiledParser ${CMAKE_CURRENT_SOURCE_DIR}/testmap.tmx leveldata.c
	DEPENDS tiledParser ${CMAKE_CURRENT_SOURCE_DIR}/testmap.tmx
	)
	
add_custom_target (leveldata DEPENDS leveldata.c)

