cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_SYSTEM_NAME Generic)

project (openjphwasm DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../html)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../html)

add_subdirectory("../.." openjph EXCLUDE_FROM_ALL)
add_executable(libopenjph "src/ojph_wrapper.cpp")
set_target_properties(libopenjph PROPERTIES SUFFIX ".js" LINK_FLAGS "-O3 -s WASM=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s ENVIRONMENT=web -s EXPORTED_FUNCTIONS=[_free,_malloc] -s EXPORTED_RUNTIME_METHODS=[ccall,cwrap,writeArrayToMemory] -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1")
target_link_libraries(libopenjph PRIVATE openjph)

add_executable(libopenjph_simd "src/ojph_wrapper.cpp" )
target_compile_options(libopenjph_simd PRIVATE -DOJPH_ENABLE_WASM_SIMD -msimd128)
set_target_properties(libopenjph_simd PROPERTIES SUFFIX ".js" LINK_FLAGS "-O3 -s WASM=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s ENVIRONMENT=web -s EXPORTED_FUNCTIONS=[_free,_malloc] -s EXPORTED_RUNTIME_METHODS=[ccall,cwrap,writeArrayToMemory] -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1")
target_link_libraries(libopenjph_simd PRIVATE openjphsimd)

