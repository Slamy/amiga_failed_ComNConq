 
cmake_minimum_required (VERSION 2.8.11)


#project (SlamySTM32Floppy C)

set(CROSS_COMPILE m68k-amigaos-)

SET(CMAKE_C_COMPILER   m68k-amigaos-gcc)
SET(CMAKE_C_FLAGS " -mcpu=68020 -g -Wall -noixemul  ")

#Vermeidet -rdynamic
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET>")

enable_language(ASM-VASM)
SET(CMAKE_ASM-VASM_FLAGS " -Faout -quiet -x -m68020 -spaces -showopt ")


add_executable (tilescroll
	tileScroll.c
	scrollEngine.c
	asmstuff.s
	uart.c
	../ptplayer/ptplayer_noa4.o
	../assets/leveldata.c
	)
	
set_source_files_properties (../assets/leveldata.c PROPERTY GENERATED)
	
target_include_directories (tilescroll PUBLIC
	../ptplayer
	../assets
	)

add_dependencies (tilescroll tilemap)
add_dependencies (tilescroll leveldata)
add_dependencies (tilescroll testbob)
add_dependencies (tilescroll mousecursor)

add_custom_target(copyToEmulator
	COMMAND cp tilescroll ~/Amiga/Festplatte/m
	DEPENDS tilescroll
	)


