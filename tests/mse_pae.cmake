############################
# This is to compile MSE_PAE

# Configure project and output directory
project (mse_pae DESCRIPTION "A program to find MSE and peak absolute error between two images" LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})


# Configure source files
set(mse_pae mse_pae.cpp "../src/apps/others/ojph_img_io.cpp" "../src/core/others/ojph_message.cpp" "../src/core/others/ojph_file.cpp" "../src/core/others/ojph_mem.cpp" "../src/core/others/ojph_arch.cpp")
set(OJPH_IMG_IO_SSE41 "../src/apps/others/ojph_img_io_sse41.cpp")
set(OJPH_IMG_IO_AVX2 "../src/apps/others/ojph_img_io_avx2.cpp")

# if SIMD are not disabled
if(NOT OJPH_DISABLE_INTEL_SIMD)
  list(APPEND mse_pae ${OJPH_IMG_IO_SSE41})
  list(APPEND mse_pae ${OJPH_IMG_IO_AVX2})
endif()

# Set compilation flags
if (MSVC)
  set_source_files_properties(../src/apps/others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
  set_source_files_properties(../src/apps/others/ojph_img_io_sse41.cpp PROPERTIES COMPILE_FLAGS -msse4.1)
  set_source_files_properties(../src/apps/others/ojph_img_io_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

# Add executable
add_executable(mse_pae ${mse_pae})

# Add tiff library if it is available
IF( USE_TIFF )
  target_link_libraries (mse_pae ${TIFF_LIBRARIES})
ELSE()
  target_link_libraries (mse_pae)
ENDIF()