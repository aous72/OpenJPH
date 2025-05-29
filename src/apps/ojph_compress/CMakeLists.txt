## building ojph_compress
#########################

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
  if (OJPH_ENABLE_WASM_SIMD)
    list(APPEND SOURCES ${OJPH_IMG_IO_SSE4})
    source_group("others" FILES ${OJPH_IMG_IO_SSE4})
    set_source_files_properties(${OJPH_IMG_IO_SSE4} PROPERTIES COMPILE_FLAGS -msse4.1)
  endif()
else()
  if (NOT OJPH_DISABLE_SIMD)
    if (("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_X86_64") 
      OR ("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_I386")
      OR MULTI_GEN_X86_64)

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
    endif()

    if (("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_ARM") OR MULTI_GEN_ARM64)

    endif()

  endif()

endif()

add_executable(ojph_compress ${SOURCES})
target_include_directories(ojph_compress PRIVATE ../common)
target_link_libraries(ojph_compress PRIVATE openjph $<TARGET_NAME_IF_EXISTS:TIFF::TIFF>)

install(TARGETS ojph_compress)
