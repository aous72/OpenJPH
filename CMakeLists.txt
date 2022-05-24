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
option(OJPH_BUILD_TEST "Enables building test code" OFF)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Builing ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)
if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /D \"_CRT_SECURE_NO_WARNINGS\"")
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -Wall -Wextra -Wconversion -Wunused-parameter")
endif()

if (OJPH_DISABLE_INTEL_SIMD)
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_DISABLE_INTEL_SIMD\"")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_DISABLE_INTEL_SIMD")
	endif()
endif()

if (BUILD_SHARED_LIBS AND MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_BUILD_SHARED_LIBRARY\"")
endif()

if (OJPH_CODE_COVERAGE AND NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

include_directories(src/core/common)
include_directories(src/apps/common)

file(GLOB CODESTREAM       "src/core/codestream/*.cpp" "src/core/codestream/*.h")
file(GLOB CODESTREAM_SSE   "src/core/codestream/*_sse.cpp")
file(GLOB CODESTREAM_SSE2  "src/core/codestream/*_sse2.cpp")
file(GLOB CODESTREAM_AVX   "src/core/codestream/*_avx.cpp")
file(GLOB CODESTREAM_AVX2  "src/core/codestream/*_avx2.cpp")
file(GLOB CODESTREAM_WASM  "src/core/codestream/*_wasm.cpp")
file(GLOB CODING           "src/core/coding/*.cpp" "src/core/coding/*.h")
file(GLOB CODING_SSSE3     "src/core/coding/*_ssse3.cpp")
file(GLOB CODING_WASM      "src/core/coding/*_wasm.cpp")
file(GLOB COMMON           "src/core/common/*.h")
file(GLOB OTHERS           "src/core/others/*.cpp")
file(GLOB TRANSFORM        "src/core/transform/*.cpp" "src/core/transform/*.h")
file(GLOB TRANSFORM_SSE    "src/core/transform/*_sse.cpp")
file(GLOB TRANSFORM_SSE2   "src/core/transform/*_sse2.cpp")
file(GLOB TRANSFORM_AVX    "src/core/transform/*_avx.cpp")
file(GLOB TRANSFORM_AVX2   "src/core/transform/*_avx2.cpp")
file(GLOB TRANSFORM_WASM   "src/core/transform/*_wasm.cpp")

list(REMOVE_ITEM CODESTREAM ${CODESTREAM_SSE} ${CODESTREAM_SSE2} ${CODESTREAM_AVX} ${CODESTREAM_AVX2} ${CODESTREAM_WASM})
list(REMOVE_ITEM CODING ${CODING_SSSE3} ${CODING_WASM})
list(REMOVE_ITEM TRANSFORM ${TRANSFORM_SSE} ${TRANSFORM_SSE2} ${TRANSFORM_AVX} ${TRANSFORM_AVX2} ${TRANSFORM_WASM})
list(APPEND SOURCES ${CODESTREAM} ${CODING} ${COMMON} ${OTHERS} ${TRANSFORM})

source_group("codestream"        FILES ${CODESTREAM})
source_group("coding"            FILES ${CODING})
source_group("common"            FILES ${COMMON})
source_group("others"            FILES ${OTHERS})
source_group("transform"         FILES ${TRANSFORM})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/pkg-config.pc.cmake"
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.pc"
)

if(EMSCRIPTEN)
  set(OJPH_DISABLE_INTEL_SIMD ON)
  set(BUILD_SHARED_LIBS OFF)
  set(OJPH_ENABLE_TIFF_SUPPORT OFF)
  add_compile_options(-std=c++11 -O3 -fexceptions -DOJPH_DISABLE_INTEL_SIMD)
  add_library(openjph ${SOURCES})
  add_library(openjphsimd ${SOURCES} ${CODESTREAM_WASM} ${CODING_WASM} ${TRANSFORM_WASM})
  target_include_directories(openjph PUBLIC src/core/common)
  target_include_directories(openjphsimd PUBLIC src/core/common)
  target_compile_options(openjphsimd PRIVATE -DOJPH_ENABLE_WASM_SIMD -msimd128)
  source_group("codestream" FILES ${CODESTREAM_WASM})
  source_group("coding" FILES ${CODING_WASM})
  source_group("transform" FILES ${TRANSFORM_WASM})
elseif(NOT OJPH_DISABLE_INTEL_SIMD)
  add_library(openjph ${SOURCES} ${CODESTREAM_SSE} ${CODESTREAM_SSE2} ${CODESTREAM_AVX} ${CODESTREAM_AVX2} ${CODING_SSSE3} ${TRANSFORM_SSE} ${TRANSFORM_SSE2} ${TRANSFORM_AVX} ${TRANSFORM_AVX2})
  source_group("codestream" FILES ${CODESTREAM_SSE} ${CODESTREAM_SSE2} ${CODESTREAM_AVX} ${CODESTREAM_AVX2})
  source_group("coding" FILES ${CODING_SSSE3})
  source_group("transform" FILES ${TRANSFORM_SSE} ${TRANSFORM_SSE2} ${TRANSFORM_AVX} ${TRANSFORM_AVX2})
else()
  add_library(openjph ${SOURCES})
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
  set_source_files_properties(src/core/codestream/ojph_codestream_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
  set_source_files_properties(src/core/codestream/ojph_codestream_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
	set_source_files_properties(src/core/transform/ojph_colour_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
	set_source_files_properties(src/core/transform/ojph_colour_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
	set_source_files_properties(src/core/transform/ojph_transform_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
	set_source_files_properties(src/core/transform/ojph_transform_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
  set_source_files_properties(src/core/codestream/ojph_codestream_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
  set_source_files_properties(src/core/codestream/ojph_codestream_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
  set_source_files_properties(src/core/coding/ojph_block_decoder_ssse3.cpp PROPERTIES COMPILE_FLAGS -mssse3)
	set_source_files_properties(src/core/transform/ojph_colour_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
	set_source_files_properties(src/core/transform/ojph_colour_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
	set_source_files_properties(src/core/transform/ojph_transform_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
	set_source_files_properties(src/core/transform/ojph_transform_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

############################################################
if( OJPH_ENABLE_TIFF_SUPPORT )

  if( WIN32 )

    set(TIFF_INCLUDE_DIR "C:\\Program Files\\tiff\\include" CACHE PATH "the directory containing the TIFF headers")
    set(TIFF_LIBRARY_DEBUG   "C:\\Program Files\\tiff\\lib\\tiffd.lib" CACHE FILEPATH "the path to the TIFF library for debug configurations")
    set(TIFF_LIBRARY_RELEASE "C:\\Program Files\\tiff\\lib\\tiff.lib"  CACHE FILEPATH "the path to the TIFF library for release configurations")
    set(TIFFXX_LIBRARY_DEBUG  "C:\\Program Files\\tiff\\lib\\tiffxxd.lib" CACHE FILEPATH "the path to the TIFFXX  library for debug configurations")
    set(TIFFXX_LIBRARY_RELEASE "C:\\Program Files\\tiff\\lib\\tiffxx.lib" CACHE FILEPATH "the path to the TIFFXX  library for release configurations")

    message( STATUS "WIN32 detected: Setting CMakeCache TIFF values as follows, use CMake-gui Advanced to modify them" )
    message( STATUS "   TIFF_INCLUDE_DIR : \"${TIFF_INCLUDE_DIR}\"  " )
    message( STATUS "   TIFF_LIBRARY_DEBUG : \"${TIFF_LIBRARY_DEBUG}\"  " )
    message( STATUS "   TIFF_LIBRARY_RELEASE : \"${TIFF_LIBRARY_RELEASE}\"  " )
    message( STATUS "   TIFFXX_LIBRARY_DEBUG : \"${TIFFXX_LIBRARY_DEBUG}\"  " )
    message( STATUS "   TIFFXX_LIBRARY_RELEASE : \"${TIFFXX_LIBRARY_RELEASE}\"  " )

  endif( WIN32 )

  FIND_PACKAGE( TIFF )

  if( TIFF_FOUND )
    set(USE_TIFF TRUE CACHE BOOL "Add TIFF support")
    include_directories( ${TIFF_INCLUDE_DIR} ) 
    if (MSVC)
		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_ENABLE_TIFF_SUPPORT\"")
	  else()
		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_ENABLE_TIFF_SUPPORT")
	  endif()
    #include_directories(${CMAKE_BINARY_DIR}/libtiff) # for tiffconf.h on windows
  endif( TIFF_FOUND )

endif() 
############################################################

set(OJPH_EXPAND src/apps/ojph_expand/ojph_expand.cpp src/apps/others/ojph_img_io.cpp)
set(OJPH_COMPRESS src/apps/ojph_compress/ojph_compress.cpp src/apps/others/ojph_img_io.cpp)
set(OJPH_IMG_IO_SSE41 src/apps/others/ojph_img_io_sse41.cpp)
set(OJPH_IMG_IO_AVX2 src/apps/others/ojph_img_io_avx2.cpp)

if(NOT OJPH_DISABLE_INTEL_SIMD)
  list(APPEND OJPH_EXPAND ${OJPH_IMG_IO_SSE41})
  list(APPEND OJPH_EXPAND ${OJPH_IMG_IO_AVX2})
  list(APPEND OJPH_COMPRESS ${OJPH_IMG_IO_SSE41})
  list(APPEND OJPH_COMPRESS ${OJPH_IMG_IO_AVX2})
endif()

add_executable(ojph_expand ${OJPH_EXPAND})
add_executable(ojph_compress ${OJPH_COMPRESS})

if (MSVC)
  set_source_files_properties(src/apps/others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
set_source_files_properties(src/apps/others/ojph_img_io_sse41.cpp PROPERTIES COMPILE_FLAGS -msse4.1)
set_source_files_properties(src/apps/others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

if( USE_TIFF )
  target_link_libraries(ojph_expand openjph ${TIFF_LIBRARIES})
  target_link_libraries(ojph_compress openjph ${TIFF_LIBRARIES})
else()
  target_link_libraries(ojph_expand openjph)
  target_link_libraries(ojph_compress openjph)
endif()


################################################################################################
# Install
################################################################################################

install(TARGETS ojph_expand
  DESTINATION bin)

install(TARGETS ojph_compress
	DESTINATION bin)

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
# Testing (OJPH_BUILD_TEST)
################################################################################################

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND OJPH_BUILD_TEST)
    include(CTest)
    add_subdirectory(tests)
endif()

