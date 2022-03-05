cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_SYSTEM_NAME Generic)

project (openjph DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)

add_compile_options(-std=c++11 -O3 -fexceptions -DOJPH_DISABLE_INTEL_SIMD)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../html)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../html)

file(GLOB SOURCES "src/ojph_wrapper.cpp" "../../src/core/coding/*.cpp" "../../src/core/others/*.cpp" "../../src/core/codestream/*.cpp" "../../src/core/transform/ojph_colour.cpp" "../../src/core/transform/ojph_transform.cpp")
file(GLOB WASM_SIMD_SOURCES  "../../src/core/transform/*_wasm.cpp")
include_directories(../../src/core/common)

add_executable(libopenjph ${SOURCES})
target_compile_options(libopenjph PRIVATE -DOJPH_DISABLE_WASM_SIMD)
set_target_properties(libopenjph PROPERTIES SUFFIX ".js" LINK_FLAGS "-s WASM=1 -s ENVIRONMENT=web -s EXPORTED_FUNCTIONS=[_free,_malloc] -s EXPORTED_RUNTIME_METHODS=[ccall,cwrap] -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1")

add_executable(libopenjph_simd ${SOURCES} ${WASM_SIMD_SOURCES})
target_compile_options(libopenjph_simd PRIVATE -msimd128)
set_target_properties(libopenjph_simd PROPERTIES SUFFIX ".js" LINK_FLAGS "-s WASM=1 -s ENVIRONMENT=web -s EXPORTED_FUNCTIONS=[_free,_malloc] -s EXPORTED_RUNTIME_METHODS=[ccall,cwrap] -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1")

