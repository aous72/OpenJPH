cmake_minimum_required(VERSION 3.10.0)

############################################################
# Parse version file
# credit: https://stackoverflow.com/a/47084079

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/core/common/ojph_version.h" VERFILE)
if (NOT VERFILE)
  message(FATAL_ERROR "Failed to parse ojph_version.h!")
endif()

string(REGEX MATCH "OPENJPH_VERSION_MAJOR ([0-9])*" _ ${VERFILE})
set(OPENJPH_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "OPENJPH_VERSION_MINOR ([0-9])*" _ ${VERFILE})
set(OPENJPH_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "OPENJPH_VERSION_PATCH ([0-9])*" _ ${VERFILE})
set(OPENJPH_VERSION_PATCH ${CMAKE_MATCH_1})

set(OPENJPH_VERSION "${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}.${OPENJPH_VERSION_PATCH}")
############################################################

project (openjph DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)

option(OJPH_DISABLE_INTEL_SIMD "Disables the use of SIMD instructions and associated files" OFF)
option(BUILD_SHARED_LIBS "Shared Libraries" ON)

set(CMAKE_CXX_STANDARD 11)
if (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /D \"_CRT_SECURE_NO_WARNINGS\"")
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fexceptions -Wall -Wextra -Wconversion -Wunused-parameter")
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

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/pkg-config.pc.cmake"
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.pc"
)

if (OJPH_DISABLE_INTEL_SIMD)
    add_library(openjph ${SOURCES})
else()
    add_library(openjph ${SOURCES} ${SSE_SOURCES} ${SSE2_SOURCES} ${AVX_SOURCES} ${AVX2_SOURCES})
endif()

target_include_directories(openjph PUBLIC src/core/common)

set_target_properties(openjph
  PROPERTIES
    OUTPUT_NAME "openjph"
)

if (OPENJPH_VERSION AND CMAKE_SYSTEM_NAME MATCHES "Linux")
  set_target_properties(openjph
    PROPERTIES
      SOVERSION "${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}")
endif()

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

INSTALL(TARGETS ojph_expand
        DESTINATION bin)

INSTALL(TARGETS ojph_compress
	DESTINATION bin)

INSTALL(TARGETS openjph LIBRARY
        DESTINATION lib)

INSTALL (DIRECTORY src/core/common/
  DESTINATION include/openjph
  FILES_MATCHING
  PATTERN "*.h")

INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.pc"
        DESTINATION lib/pkgconfig)

