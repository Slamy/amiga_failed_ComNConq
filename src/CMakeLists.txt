 
cmake_minimum_required (VERSION 2.8.11)


#project (SlamySTM32Floppy C)

set(CROSS_COMPILE m68k-amigaos-)

SET(CMAKE_C_COMPILER   m68k-amigaos-gcc)
SET(CMAKE_CXX_COMPILER   m68k-amigaos-g++)

SET(CMAKE_C_FLAGS " -mcpu=68020 -g -Wall -noixemul -O2 ")
SET(CMAKE_CXX_FLAGS " -mcpu=68020 -g -Wall -noixemul -O2 ")

#SET(CMAKE_C_LINK_FLAGS " -Wl,--print-gc-sections")

#SET(CMAKE_C_LINK_FLAGS " -Wl,-Map=output.map -T ${CMAKE_CURRENT_SOURCE_DIR}/script.ld -Wl,-verbose ")
#SET(CMAKE_C_LINK_FLAGS " -Wl,-Map=output.map -Wl,-verbose ")

#Vermeidet -rdynamic
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER>  <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET>")

#set(CMAKE_C_LINK_EXECUTABLE "vc  <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET>")

#SET(CMAKE_C_LINKER  vlink)

enable_language(ASM-VASM)
SET(CMAKE_ASM-VASM_FLAGS " -Faout -quiet -x -m68020 -spaces -showopt ")


add_executable (tilescrollTest
	tileScrollTest.c
	scrollEngine.c
	uart.c
	assets.c
	input.c
	asmstuff.s
	../ptplayer/ptplayer_noa4.o
	../assets/leveldata.c
	)

add_executable (game
	game.cpp
	unit.cpp
	astar.cpp
	scrollEngine.c
	uart.c
	assets.c
	input.c
	asmstuff.s
	../ptplayer/ptplayer_noa4.o
	../assets/leveldata.c
	)
	
set_source_files_properties (../assets/leveldata.c PROPERTY GENERATED)
	
target_include_directories (tilescrollTest PUBLIC
	../ptplayer
	../assets
	)

add_dependencies (game assets)
add_dependencies (game leveldata)

add_dependencies (tilescrollTest assets)
add_dependencies (tilescrollTest leveldata)

add_custom_target(copyToEmulator
	COMMAND cp game ~/Amiga/Festplatte/g
	COMMAND cp ../assets/assets.bin ~/Amiga/Festplatte/
	COMMAND cp tilescrollTest ~/Amiga/Festplatte/m
	DEPENDS tilescrollTest assets game
	)


