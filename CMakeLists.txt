cmake_minimum_required (VERSION 2.6)
project (GT.M)
SET(CMAKE_C_FLAGS "-std=c99  -Wall -pedantic ")


include_directories("sr_unix")
include_directories("sr_port")
include_directories("sr_port_cm")
include_directories("sr_i386")

file(GLOB mumps_SRC
    "./sr_port/*.h" 

    "./sr_port_cm/*.h"
    "./sr_port/*.c"
)

add_executable(mumps ${mumps_SRC})