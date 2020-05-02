cmake_minimum_required(VERSION 3.10.0)

project (openjph DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)

option(OJPH_DISABLE_INTEL_SIMD "Disables the use of SIMD instructions and associated files" OFF)
option(BUILD_SHARED_LIBS "Shared Libraries" ON)

set(CMAKE_CXX_STANDARD 11)
if (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /D \"_CRT_SECURE_NO_WARNINGS\"")
endif()

if (OJPH_DISABLE_INTEL_SIMD)
	if (MSVC)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_DISABLE_INTEL_SIMD\"")
	else()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_DISABLE_INTEL_SIMD")
	endif()
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

include_directories(src/core/common)
include_directories(src/apps/common)
file(GLOB SOURCES "src/core/coding/*.cpp" "src/core/others/*.cpp" "src/core/codestream/*.cpp" "src/core/transform/ojph_colour.cpp" "src/core/transform/ojph_transform.cpp")
file(GLOB SSE_SOURCES "src/core/transform/*_sse.cpp")
file(GLOB SSE2_SOURCES "src/core/transform/*_sse2.cpp")
file(GLOB AVX_SOURCES "src/core/transform/*_avx.cpp")
file(GLOB AVX2_SOURCES "src/core/transform/*_avx2.cpp")

if (OJPH_DISABLE_INTEL_SIMD)
    add_library(openjph SHARED ${SOURCES})
else()
    add_library(openjph SHARED ${SOURCES} ${SSE_SOURCES} ${SSE2_SOURCES} ${AVX_SOURCES} ${AVX2_SOURCES})
endif()

target_include_directories(openjph PUBLIC src/core/common)

if (MSVC)
	set_source_files_properties(src/core/transform/ojph_colour_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
	set_source_files_properties(src/core/transform/ojph_colour_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
	set_source_files_properties(src/core/transform/ojph_transform_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
	set_source_files_properties(src/core/transform/ojph_transform_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
	set_source_files_properties(src/core/transform/ojph_colour_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
	set_source_files_properties(src/core/transform/ojph_colour_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
	set_source_files_properties(src/core/transform/ojph_transform_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
	set_source_files_properties(src/core/transform/ojph_transform_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

add_executable(ojph_expand src/apps/ojph_expand/ojph_expand.cpp src/apps/others/ojph_img_io.cpp)
target_link_libraries(ojph_expand openjph)

add_executable(ojph_compress src/apps/ojph_compress/ojph_compress.cpp src/apps/others/ojph_img_io.cpp)
target_link_libraries (ojph_compress openjph)

