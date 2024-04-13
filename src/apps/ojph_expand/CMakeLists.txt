## building ojph_expand
#######################

include_directories(../common)
include_directories(../../core/common)

file(GLOB OJPH_EXPAND         "ojph_expand.cpp")
file(GLOB OJPH_IMG_IO         "../others/ojph_img_io.cpp")
file(GLOB OJPH_IMG_IO_SSE4    "../others/ojph_img_io_sse41.cpp")
file(GLOB OJPH_IMG_IO_AVX2    "../others/ojph_img_io_avx2.cpp")
file(GLOB OJPH_IMG_IO_H       "../common/ojph_img_io.h")

list(APPEND SOURCES ${OJPH_EXPAND} ${OJPH_IMG_IO} ${OJPH_IMG_IO_H})

source_group("main"        FILES ${OJPH_EXPAND})
source_group("others"      FILES ${OJPH_IMG_IO})
source_group("common"      FILES ${OJPH_IMG_IO_H})

if(EMSCRIPTEN)
  add_compile_options(-std=c++11 -O3 -fexceptions)
  add_link_options(-sWASM=1 -sASSERTIONS=1 -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1 -sENVIRONMENT=node -sEXIT_RUNTIME=1 -sEXCEPTION_CATCHING_ALLOWED=['fake'])
  add_executable(ojph_expand ${SOURCES})
  add_executable(ojph_expand_simd ${SOURCES} ${OJPH_IMG_IO_SSE4})
  target_compile_options(ojph_expand_simd PRIVATE -DOJPH_ENABLE_WASM_SIMD -msimd128 -msse4.1)
  source_group("others" FILES ${OJPH_IMG_IO_SSE4})

  target_link_libraries(ojph_expand PRIVATE openjph)
  install(TARGETS ojph_expand DESTINATION bin)
  target_link_libraries(ojph_expand_simd PRIVATE openjphsimd)
  install(TARGETS ojph_expand_simd DESTINATION bin)    
else()
  if(NOT OJPH_DISABLE_INTEL_SIMD)
    add_executable(ojph_expand ${SOURCES} ${OJPH_IMG_IO_SSE4} ${OJPH_IMG_IO_AVX2})

    if (MSVC)
      set_source_files_properties(${OJPH_IMG_IO_AVX2} PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    else()
      set_source_files_properties(${OJPH_IMG_IO_SSE4} PROPERTIES COMPILE_FLAGS -msse4.1)
      set_source_files_properties(${OJPH_IMG_IO_AVX2} PROPERTIES COMPILE_FLAGS -mavx2)
    endif()
  else()
    add_executable(ojph_expand ${SOURCES})
  endif()

  if( USE_TIFF )
    target_link_libraries(ojph_expand PUBLIC openjph ${TIFF_LIBRARIES})
  else()
    target_link_libraries(ojph_expand PUBLIC openjph)
  endif()

  install(TARGETS ojph_expand DESTINATION bin)
endif()
