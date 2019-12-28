cmake_minimum_required(VERSION 3.15)
project(game)

set(CMAKE_C_STANDARD 99)
add_executable(game main.c)

find_library(LIB_X11 X11 /usr/X11R6/lib)
target_link_libraries(game LINK_PUBLIC ${LIB_X11})

