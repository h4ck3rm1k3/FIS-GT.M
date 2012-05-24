cmake_minimum_required (VERSION 2.6)
project (GT.M)

if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
  set(ARCHITECTURE_IS_64_BITS ON)
endif()

if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 4)
  set(ARCHITECTURE_IS_32_BITS ON)
endif()



SET(CMAKE_C_FLAGS "-std=c99  -Wall -pedantic  -g3 --save-temps ")
SET(CMAKE_CXX_FLAGS "-std=c++0x  -Wall -pedantic  -g3 --save-temps ")


include_directories("sr_unix")
include_directories("sr_port")
include_directories("sr_port_cm")
include_directories("sr_i386")

include(CTest)
ENABLE_TESTING()

file(GLOB mumps_SRC
    "./sr_port/*.h" 
    "./sr_port_cm/*.h"
    "./sr_port/*.c"
    "./sr_i386/*.c"
    "./sr_linux/*.c"
    "./sr_unix/*.c"
    "./sr_unix_cm/*.c"
    "./sr_unix_gnp/*.c"
    "./sr_unix_nsb/*.c"


)

#    "./sr_i386/*.s"

SET_SOURCE_FILES_PROPERTIES(${mumps_SRC} PROPERTIES LANGUAGE CXX)

add_executable(mumps ${mumps_SRC})
target_link_libraries(mumps dl m)