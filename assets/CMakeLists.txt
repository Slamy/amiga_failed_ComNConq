cmake_minimum_required (VERSION 2.8.11)

add_custom_command (OUTPUT tilemap.bin
	COMMAND ../tools/gifToPlanar gifToPlanar ${CMAKE_CURRENT_SOURCE_DIR}/tilemap.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 tilemap.bin
	DEPENDS gifToPlanar tilemap.gif palette.txt
	)
add_custom_target (tilemap DEPENDS tilemap.bin)




add_custom_command (OUTPUT mousecursor.bin
	COMMAND ../tools/gifToPlanar gifToPlanar ${CMAKE_CURRENT_SOURCE_DIR}/mousecursor.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 2 mousecursor.bin
	DEPENDS gifToPlanar mousecursor.gif palette.txt
	)
add_custom_target (mousecursor DEPENDS mousecursor.bin)



add_custom_command (OUTPUT testbob.bin
	COMMAND ../tools/gifToPlanar gifToMaskedACBM ${CMAKE_CURRENT_SOURCE_DIR}/testbob.gif ${CMAKE_CURRENT_SOURCE_DIR}/palette.txt 5 testbob.bin
	DEPENDS gifToPlanar testbob.gif palette.txt
	)
add_custom_target (testbob DEPENDS testbob.bin)




add_custom_command (OUTPUT leveldata.c
	COMMAND ../tools/tiledParser ${CMAKE_CURRENT_SOURCE_DIR}/level.tmx leveldata.c
	DEPENDS tiledParser ${CMAKE_CURRENT_SOURCE_DIR}/level.tmx
	)
	
add_custom_target (leveldata DEPENDS leveldata.c)
	