## building ojph_compress
#########################

include_directories(../common)
include_directories(../../core/common)

file(GLOB OJPH_COMPRESS       "ojph_compress.cpp")
file(GLOB OJPH_IMG_IO         "../others/ojph_img_io.cpp")
file(GLOB OJPH_IMG_IO_SSE4    "../others/ojph_img_io_sse41.cpp")
file(GLOB OJPH_IMG_IO_AVX2    "../others/ojph_img_io_avx2.cpp")
file(GLOB OJPH_IMG_IO_H       "../common/ojph_img_io.h")

list(APPEND SOURCES ${OJPH_COMPRESS} ${OJPH_IMG_IO} ${OJPH_IMG_IO_H})

source_group("main"        FILES ${OJPH_COMPRESS})
source_group("others"      FILES ${OJPH_IMG_IO})
source_group("common"      FILES ${OJPH_IMG_IO_H})

if(EMSCRIPTEN)
  add_compile_options(-std=c++11 -O3 -fexceptions)
  add_link_options(-sWASM=1 -sASSERTIONS=1 -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1 -sENVIRONMENT=node -sEXIT_RUNTIME=1 -sEXCEPTION_CATCHING_ALLOWED=['fake'])
  add_executable(ojph_compress ${SOURCES})
  add_executable(ojph_compress_simd ${SOURCES} ${OJPH_IMG_IO_SSE4})
  target_compile_options(ojph_compress_simd PRIVATE -DOJPH_ENABLE_WASM_SIMD -msimd128 -msse4.1)
  source_group("others" FILES ${OJPH_IMG_IO_SSE4})

  target_link_libraries(ojph_compress PRIVATE openjph)
  install(TARGETS ojph_compress DESTINATION bin)
  target_link_libraries(ojph_compress_simd PRIVATE openjphsimd)
  install(TARGETS ojph_compress_simd DESTINATION bin)
else()
  if (NOT OJPH_DISABLE_SIMD)
    if (("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_X86_64") OR ("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_I386"))
      if (NOT OJPH_DISABLE_SSE4)
        list(APPEND SOURCES ${OJPH_IMG_IO_SSE4})
        source_group("others" FILES ${OJPH_IMG_IO_SSE4})
      endif()
      if (NOT OJPH_DISABLE_AVX2)
        list(APPEND SOURCES ${OJPH_IMG_IO_AVX2})
        source_group("others" FILES ${OJPH_IMG_IO_AVX2})
      endif()

      # Set compilation flags
      if (MSVC)
        set_source_files_properties(${OJPH_IMG_IO_AVX2} PROPERTIES COMPILE_FLAGS "/arch:AVX2")
      else()
        set_source_files_properties(${OJPH_IMG_IO_SSE4} PROPERTIES COMPILE_FLAGS -msse4.1)
        set_source_files_properties(${OJPH_IMG_IO_AVX2} PROPERTIES COMPILE_FLAGS -mavx2)
      endif()
    elseif ("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_ARM")
    
    endif()

  endif()

  add_executable(ojph_compress ${SOURCES})

  if( USE_TIFF )
    target_link_libraries(ojph_compress PUBLIC openjph ${TIFF_LIBRARIES})
  else()
    target_link_libraries(ojph_compress PUBLIC openjph)
  endif()

  install(TARGETS ojph_compress DESTINATION bin)
endif()
