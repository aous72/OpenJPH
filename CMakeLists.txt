cmake_minimum_required(VERSION 3.11.0)

## Library name/version
include(ojph_libname.cmake)

## project
project (openjph VERSION ${OPENJPH_VERSION} DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)
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

## C++ version and flags
# C++14 is needed for gtest, otherwise, C++11 is sufficient for the library
set(CMAKE_CXX_STANDARD 14)
if (MSVC)
  add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(
  -fexceptions 
  -Wall 
  -Wextra 
  -Wconversion 
  -Wunused-parameter
  )
endif()

## The option OJPH_DISABLE_INTEL_SIMD and OJPH_ENABLE_INTEL_AVX512
if (OJPH_DISABLE_INTEL_SIMD)
  add_compile_options(-DOJPH_DISABLE_INTEL_SIMD)
elseif (OJPH_ENABLE_INTEL_AVX512)
  add_compile_options(-DOJPH_ENABLE_INTEL_AVX512)
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
install(TARGETS openjph
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY src/core/common/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openjph
  FILES_MATCHING
  PATTERN "*.h")

install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set(PKG_CONFIG_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set(PKG_CONFIG_LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
set(PKG_CONFIG_LIBS "-L\${libdir} -lopenjph")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/pkg-config.pc.cmake"
  "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
  @ONLY
)

################################################################################################
# Testing (OJPH_BUILD_TESTS)
################################################################################################

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND OJPH_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

