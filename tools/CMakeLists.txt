cmake_minimum_required (VERSION 2.8.11)


SET(CMAKE_C_FLAGS "-g -Wall -O2")

add_executable (gifToPlanar
	gifToPlanar.c
	)
	
target_link_libraries (gifToPlanar LINK_PUBLIC gif)




add_executable (tiledParser
	tiledParser.c
	)
	
