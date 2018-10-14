 
cmake_minimum_required (VERSION 2.8.11)


#project (SlamySTM32Floppy C)

set(CROSS_COMPILE m68k-amigaos-)

SET(CMAKE_C_COMPILER   m68k-amigaos-gcc)
SET(CMAKE_CXX_COMPILER   m68k-amigaos-g++)

#SET(OPTIMIZE_FLAGS " -fdefer-pop -fmerge-constants -fthread-jumps -floop-optimize -fif-conversion -fif-conversion2 -fdelayed-branch -fguess-branch-probability -fcprop-registers ")
#SET(OPTIMIZE_FLAGS " ${OPTIMIZE_FLAGS} -fomit-frame-pointer ")
SET(OPTIMIZE_FLAGS " -O1 -fomit-frame-pointer -fno-exceptions -ffunction-sections ")
#SET(OPTIMIZE_FLAGS " -O1 -fno-exceptions ")
#SET(OPTIMIZE_FLAGS " -O2 -fno-exceptions ")
#SET(OPTIMIZE_FLAGS " -O1 ")
#SET(OPTIMIZE_FLAGS " -fomit-frame-pointer -fauto-inc-dec -fbranch-count-reg -fcombine-stack-adjustments -fcompare-elim -fcprop-registers -fdce -fdefer-pop -fdelayed-branch -fdse -fforward-propagate -fguess-branch-probability -fif-conversion2 -fif-conversion -finline-functions-called-once -fipa-pure-const -fipa-profile -fipa-reference -fmerge-constants -fmove-loop-invariants -freorder-blocks -fshrink-wrap -fsplit-wide-types -fssa-backprop -fssa-phiopt -ftree-bit-ccp -ftree-ccp -ftree-ch -ftree-coalesce-vars -ftree-copy-prop -ftree-dce -ftree-dominator-opts -ftree-dse -ftree-forwprop -ftree-fre -ftree-phiprop -ftree-sink -ftree-slsr -ftree-sra -ftree-pta -ftree-ter -funit-at-a-time ")

SET(CMAKE_C_FLAGS " -mcrt=nix13 -mcpu=68000 -Wall ${OPTIMIZE_FLAGS} ")
SET(CMAKE_CXX_FLAGS "-mcrt=nix13  -mcpu=68000 -Wall ${OPTIMIZE_FLAGS} ")
#SET(CMAKE_C_LINK_FLAGS " -Wl,--print-gc-sections")

#SET(CMAKE_C_LINK_FLAGS " -Wl,-Map=output.map -T ${CMAKE_CURRENT_SOURCE_DIR}/script.ld -Wl,-verbose ")
# -Wl,-Map=output.map -Wl,-verbose
SET(CMAKE_C_LINK_FLAGS " -mcrt=nix13 -m68000  -v -Wl,-Map=coutput.map -Wl,--gc-sections ")
#SET(CMAKE_CXX_LINK_FLAGS " -mcrt=nix13  -v -Wl,-verbose ")
SET(CMAKE_CXX_LINK_FLAGS " -nodefaultlibs  -mcrt=nix13 -m68000 -Wl,-Map=cxxoutput.map -Wl,--gc-sections ")


#Vermeidet -rdynamic
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_C_LINK_FLAGS> <OBJECTS>  -o <TARGET>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER>  <FLAGS> <CMAKE_CXX_LINK_FLAGS> <OBJECTS>  -o <TARGET> \
	-lnixmain -lstdc++ -lnix -lamiga -lgcc -lm -lnix13 -lgcc -lnix")

#set(CMAKE_C_LINK_EXECUTABLE "vc  <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET>")

#SET(CMAKE_C_LINKER  vlink)


add_custom_command(
	OUTPUT asmstuff.o
	COMMAND vasm -Faout -quiet -x -m68000 -spaces -showopt  -o asmstuff.o ${CMAKE_CURRENT_SOURCE_DIR}/hardware/asmstuff.s
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hardware/asmstuff.s
	
)

add_custom_command(
	OUTPUT ptplayer_noa4.o
	COMMAND vasm -Faout -quiet -x -m68000 -spaces -showopt  -o ptplayer_noa4.o ${CMAKE_CURRENT_SOURCE_DIR}/ptplayer/ptplayer_noa4.s
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ptplayer/ptplayer_noa4.s
	
)

add_executable (tilescrollTest
	tileScrollTest.c
	hardware/scrollEngine.c
	hardware/uart.c
	assets.c
	hardware/input.c
	asmstuff.o
	ptplayer_noa4.o
	ptplayer/ptplayerWrapper.c
	../assets/leveldata.c
	)

add_executable (game
	game.cpp
	
	Unit.cpp
	AStar.cpp
	Harvester.cpp
	Tank.cpp
	
	hardware/scrollEngine.c
	hardware/uart.c
	assets.c
	hardware/input.c
	asmstuff.o
	ptplayer_noa4.o
	ptplayer/ptplayerWrapper.c
	../assets/leveldata.c
	)
	
set_source_files_properties (../assets/leveldata.c PROPERTY GENERATED)
	
target_include_directories (tilescrollTest PUBLIC
	ptplayer
	../assets
	hardware
	.
	)
	
target_include_directories (game PUBLIC
	ptplayer
	../assets
	hardware
	.
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


