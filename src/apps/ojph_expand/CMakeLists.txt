## building ojph_expand
#######################

include_directories(../common)

file(GLOB OJPH_EXPAND    "ojph_expand.cpp")
file(GLOB OJPH_IMG_IO    "../others/ojph_img_io*.cpp")
file(GLOB OJPH_IMG_IO_H  "../common/ojph_img_io.h")

if(NOT OJPH_DISABLE_INTEL_SIMD)
  list(REMOVE_ITEM OJPH_IMG_IO ${OJPH_IMG_IO_SSE41})
  list(REMOVE_ITEM OJPH_IMG_IO ${OJPH_IMG_IO_AVX2})
endif()

source_group("main"        FILES ${OJPH_EXPAND})
source_group("others"      FILES ${OJPH_IMG_IO})
source_group("common"      FILES ${OJPH_IMG_IO_H})

if(OJPH_BUILD_EXECUTABLES)
  add_executable(ojph_expand ${OJPH_EXPAND} ${OJPH_IMG_IO} ${OJPH_IMG_IO_H})
endif()

if (MSVC)
  set_source_files_properties(../others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
  set_source_files_properties(../others/ojph_img_io_sse41.cpp PROPERTIES COMPILE_FLAGS -msse4.1)
  set_source_files_properties(../others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

if(OJPH_BUILD_EXECUTABLES)
  if( USE_TIFF )
    target_link_libraries(ojph_expand PUBLIC openjph ${TIFF_LIBRARIES})
  else()
    target_link_libraries(ojph_expand PUBLIC openjph)
  endif()
endif()

install(TARGETS ojph_expand DESTINATION bin)
