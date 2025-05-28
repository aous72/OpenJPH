############################
# This is to compile MSE_PAE

# Configure project and output directory
project (mse_pae DESCRIPTION "A program to find MSE and peak absolute error between two images" LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(../src/apps/common)
include_directories(../src/core/common)

# Configure source files
set(SOURCES mse_pae.cpp "../src/apps/others/ojph_img_io.cpp" "../src/core/others/ojph_message.cpp" "../src/core/others/ojph_file.cpp" "../src/core/others/ojph_mem.cpp" "../src/core/others/ojph_arch.cpp")
set(OJPH_IMG_IO_SSE41 "../src/apps/others/ojph_img_io_sse41.cpp")
set(OJPH_IMG_IO_AVX2 "../src/apps/others/ojph_img_io_avx2.cpp")

# if SIMD are not disabled
if (NOT OJPH_DISABLE_SIMD)
  if (("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_X86_64") 
    OR ("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_I386")
    OR MULTI_GEN_X86_64)

    if (NOT OJPH_DISABLE_SSE4)
      list(APPEND SOURCES ${OJPH_IMG_IO_SSE41})
    endif()
    if (NOT OJPH_DISABLE_AVX2)
      list(APPEND SOURCES ${OJPH_IMG_IO_AVX2})
    endif()

    # Set compilation flags
    if (MSVC)
      set_source_files_properties(../src/apps/others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    else()
      set_source_files_properties(../src/apps/others/ojph_img_io_sse41.cpp PROPERTIES COMPILE_FLAGS -msse4.1)
      set_source_files_properties(../src/apps/others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
    endif()
  endif()

  if (("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_ARM") OR MULTI_GEN_ARM64)

  endif()

endif()

# Add executable
add_executable(mse_pae ${SOURCES})

# Add tiff library if it is available
if( USE_TIFF )
  target_link_libraries (mse_pae ${TIFF_LIBRARIES})
endif()
