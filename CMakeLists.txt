cmake_minimum_required(VERSION 3.11.0)

## project
project (openjph DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################################
# Building OpenJPH
################################################################################################

## options
option(OJPH_DISABLE_INTEL_SIMD "Disables the use of SIMD instructions and associated files" OFF)
option(OJPH_ENABLE_INTEL_AVX512 "enables the use of AVX512 SIMD instructions and associated files" ON)
option(BUILD_SHARED_LIBS "Shared Libraries" ON)
option(OJPH_ENABLE_TIFF_SUPPORT "Enables input and output support for TIFF files" ON)
option(OJPH_BUILD_TESTS "Enables building test code" OFF)
option(OJPH_BUILD_EXECUTABLES "Enables building command line executables" ON)

## Setting some of the options if EMSCRIPTEN is the compiler
if(EMSCRIPTEN)
  set(OJPH_DISABLE_INTEL_SIMD ON)
  set(BUILD_SHARED_LIBS OFF)
  set(OJPH_ENABLE_TIFF_SUPPORT OFF)
endif()

# This is related to how the timestamp is set for URL downloaded files.
# Set DOWNLOAD_EXTRACT_TIMESTAMP
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
  if(POLICY CMP0065)
    cmake_policy(SET CMP0135 NEW)
  endif()
endif()

set(CMAKE_CXX_FLAGS_ASAN
  "-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
  CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds."
  FORCE)

## build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message( STATUS "To use AddressSanitizer, use \"cmake .. -DCMAKE_BUILD_TYPE=asan\"" )
endif()
message(STATUS "Building ${CMAKE_BUILD_TYPE}")

## Library name
include(ojph_libname.cmake)

## C++ version and flags
set(CMAKE_CXX_STANDARD 14)
if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /D \"_CRT_SECURE_NO_WARNINGS\"")
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -Wall -Wextra -Wconversion -Wunused-parameter")
endif()

## The option OJPH_DISABLE_INTEL_SIMD and OJPH_ENABLE_INTEL_AVX512
if (OJPH_DISABLE_INTEL_SIMD)
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_DISABLE_INTEL_SIMD\"")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_DISABLE_INTEL_SIMD")
	endif()
elseif (OJPH_ENABLE_INTEL_AVX512)
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_ENABLE_INTEL_AVX512\"")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_ENABLE_INTEL_AVX512")
  endif()
endif()

## The option BUILD_SHARED_LIBS
if (BUILD_SHARED_LIBS AND MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_BUILD_SHARED_LIBRARY\"")
endif()

## Build library and applications
add_subdirectory(src/core)
if (OJPH_BUILD_EXECUTABLES)
  add_subdirectory(src/apps)
endif()

################################################################################################
# Install
################################################################################################

include(GNUInstallDirs)
install(TARGETS openjph LIBRARY 
  DESTINATION ${CMAKE_INSTALL_LIBDIR})

install (DIRECTORY src/core/common/
  DESTINATION include/openjph
  FILES_MATCHING
  PATTERN "*.h")

install(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

################################################################################################
# Testing (OJPH_BUILD_TESTS)
################################################################################################

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND OJPH_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

