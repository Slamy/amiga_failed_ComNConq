cmake_minimum_required (VERSION 2.8.11)

add_custom_command (OUTPUT tilemap.bin
	COMMAND ../tools/gifToPlanar gifToPlanar ${CMAKE_CURRENT_SOURCE_DIR}/tilemap.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt tilemap.bin
	DEPENDS gifToPlanar tilemap.gif palette.txt
	)

add_custom_target (tilemap DEPENDS tilemap.bin)


add_custom_command (OUTPUT leveldata.c
	COMMAND ../tools/tiledParser ${CMAKE_CURRENT_SOURCE_DIR}/level.tmx leveldata.c
	DEPENDS tiledParser ${CMAKE_CURRENT_SOURCE_DIR}/level.tmx
	)
	
add_custom_target (leveldata DEPENDS leveldata.c)
	