cmake_minimum_required(VERSION 3.10.0)

project (openjph VERSION 0.1.0 DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)
# set(CMAKE_CXX_STANDARD 11)
if (MSVC)
	set(CMAKE_CXX_FLAGS "/EHsc")
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -g")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

include_directories(src/core/common)
file(GLOB SOURCES "src/core/*/*.cpp")
add_library(openjph SHARED ${SOURCES})

add_executable(ojph_expand src/apps/ojph_expand/ojph_expand.cpp src/apps/others/ojph_img_io.cpp)
target_link_libraries(ojph_expand openjph)
include_directories(src/apps/common)

add_executable(ojph_compress src/apps/ojph_compress/ojph_compress.cpp src/apps/others/ojph_img_io.cpp)
target_link_libraries (ojph_compress openjph)
include_directories(src/apps/common)

