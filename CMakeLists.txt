cmake_minimum_required(VERSION 3.10.0)

project (openjph DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)

################################################################################################
# Building OpenJPH
################################################################################################

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

option(OJPH_DISABLE_INTEL_SIMD "Disables the use of SIMD instructions and associated files" OFF)
option(BUILD_SHARED_LIBS "Shared Libraries" ON)
option(OJPH_ENABLE_TIFF_SUPPORT "Enables input and output support for TIFF files" ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Builing ${CMAKE_BUILD_TYPE}")

SET(CMAKE_CXX_STANDARD 11)
if (MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /D \"_CRT_SECURE_NO_WARNINGS\"")
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -Wall -Wextra -Wconversion -Wunused-parameter")
endif()

if (OJPH_DISABLE_INTEL_SIMD)
	if (MSVC)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_DISABLE_INTEL_SIMD\"")
	else()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_DISABLE_INTEL_SIMD")
	endif()
endif()

if (OJPH_CODE_COVERAGE AND NOT MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

############################################################
if( OJPH_ENABLE_TIFF_SUPPORT )

  if( WIN32 )

    set(TIFF_INCLUDE_DIR "C:\\Program Files\\tiff\\include" CACHE PATH "the directory containing the TIFF headers")
    set(TIFF_LIBRARY_DEBUG   "C:\\Program Files\\tiff\\lib\\tiffd.lib" CACHE FILEPATH "the path to the TIFF library for debug configurations")
    set(TIFF_LIBRARY_RELEASE "C:\\Program Files\\tiff\\lib\\tiff.lib"  CACHE FILEPATH "the path to the TIFF library for release configurations")
    set(TIFFXX_LIBRARY_DEBUG  "C:\\Program Files\\tiff\\lib\\tiffxxd.lib" CACHE FILEPATH "the path to the TIFFXX  library for debug configurations")
    set(TIFFXX_LIBRARY_RELEASE "C:\\Program Files\\tiff\\lib\\tiffxx.lib" CACHE FILEPATH "the path to the TIFFXX  library for release configurations")

    MESSAGE( STATUS "WIN32 detected: Setting CMakeCache TIFF values as follows, use CMake-gui Advanced to modify them" )
    MESSAGE( STATUS "   TIFF_INCLUDE_DIR : \"${TIFF_INCLUDE_DIR}\"  " )
    MESSAGE( STATUS "   TIFF_LIBRARY_DEBUG : \"${TIFF_LIBRARY_DEBUG}\"  " )
    MESSAGE( STATUS "   TIFF_LIBRARY_RELEASE : \"${TIFF_LIBRARY_RELEASE}\"  " )
    MESSAGE( STATUS "   TIFFXX_LIBRARY_DEBUG : \"${TIFFXX_LIBRARY_DEBUG}\"  " )
    MESSAGE( STATUS "   TIFFXX_LIBRARY_RELEASE : \"${TIFFXX_LIBRARY_RELEASE}\"  " )

  endif( WIN32 )

  FIND_PACKAGE( TIFF )

  if( TIFF_FOUND )
    SET(USE_TIFF TRUE CACHE BOOL "Add TIFF support")
    include_directories( ${TIFF_INCLUDE_DIR} ) 
    if (MSVC)
		  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_ENABLE_TIFF_SUPPORT\"")
	  else()
		  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_ENABLE_TIFF_SUPPORT")
	  endif()
    #include_directories(${CMAKE_BINARY_DIR}/libtiff) # for tiffconf.h on windows
  endif( TIFF_FOUND )

endif() 
############################################################

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

target_compile_definitions(openjph PUBLIC _FILE_OFFSET_BITS=64)

if (OPENJPH_VERSION)
  if (WIN32)
    set_target_properties(openjph
      PROPERTIES
        OUTPUT_NAME "openjph.${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}")
  else()
    set_target_properties(openjph
      PROPERTIES
        SOVERSION "${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}"
        VERSION "${OPENJPH_VERSION}")
  endif()
else()
  message(FATAL_ERROR "OPENJPH_VERSION is not set")
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
IF( USE_TIFF )
  target_link_libraries(ojph_expand openjph ${TIFF_LIBRARIES})
ELSE()
  target_link_libraries(ojph_expand openjph)
ENDIF()

add_executable(ojph_compress src/apps/ojph_compress/ojph_compress.cpp src/apps/others/ojph_img_io.cpp)
IF( USE_TIFF )
  target_link_libraries (ojph_compress openjph ${TIFF_LIBRARIES})
ELSE()
  target_link_libraries (ojph_compress openjph)
ENDIF()

INSTALL(TARGETS ojph_expand
        DESTINATION bin)

INSTALL(TARGETS ojph_compress
	DESTINATION bin)

include(GNUInstallDirs)
INSTALL(TARGETS openjph LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR})

INSTALL (DIRECTORY src/core/common/
  DESTINATION include/openjph
  FILES_MATCHING
  PATTERN "*.h")

INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.pc"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

################################################################################################
# Testing
################################################################################################

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()

