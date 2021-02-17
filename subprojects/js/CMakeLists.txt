cmake_minimum_required(VERSION 3.10.0)

option(BUILD_TEST "Build test file" OFF)

if (BUILD_TEST)
else()
    set(CMAKE_SYSTEM_NAME Generic)
endif()

project (openjph DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -fexceptions -DOJPH_DISABLE_INTEL_SIMD")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../html)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../html)

file(GLOB SOURCES "src/ojph_wrapper.cpp" "../../src/core/coding/*.cpp" "../../src/core/others/*.cpp" "../../src/core/codestream/*.cpp" "../../src/core/transform/ojph_colour.cpp" "../../src/core/transform/ojph_transform.cpp")
add_executable(libopenjph ${SOURCES})
include_directories(../../src/core/common)
if (BUILD_TEST)
    set_source_files_properties(src/ojph_wrapper.cpp PROPERTIES COMPILE_FLAGS -DBUILD_TEST)
else()
endif()

if (BUILD_TEST)
else()
    set_target_properties(libopenjph PROPERTIES SUFFIX ".js" LINK_FLAGS "-s WASM=1 -s ENVIRONMENT=web -s EXPORTED_FUNCTIONS=[_free,_malloc] -s EXTRA_EXPORTED_RUNTIME_METHODS=[ccall,cwrap] -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1")
endif()

