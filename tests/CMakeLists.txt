
# Fetch googletest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  EXCLUDE_FROM_ALL
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
option(INSTALL_GTEST "Enable installation of googletest." OFF)
FetchContent_MakeAvailable(googletest)

# Fetch test files
FetchContent_Declare(
  jp2k_test_codestreams
  URL               https://github.com/aous72/jp2k_test_codestreams/archive/refs/heads/main.zip
  SOURCE_DIR        jp2k_test_codestreams/
)
FetchContent_MakeAvailable(jp2k_test_codestreams)

# create the mse_pae executable
include(mse_pae.cmake)

# create the compare_files executable
add_executable(
  compare_files
  compare_files.cpp
)

# configure test executables
add_executable(
  test_executables
  test_executables.cpp
)

target_link_libraries(
  test_executables
  GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET test_executables)

if (MSVC)
  add_custom_command(TARGET test_executables POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "../bin/\$(Configuration)/gtest.dll" "./"
    COMMAND ${CMAKE_COMMAND} -E copy "../bin/\$(Configuration)/gtest_main.dll" "./"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ojph_compress>" "./"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ojph_expand>" "./"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:openjph>" "./"
  )
  add_custom_command(TARGET compare_files POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "./\$(Configuration)/compare_files.exe" "./"
  )
  add_custom_command(TARGET mse_pae POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "./\$(Configuration)/mse_pae.exe" "./"
  )
  if (OJPH_ENABLE_TIFF_SUPPORT)
      file(COPY "${TIFF_INCLUDE_DIR}\\..\\bin\\tiff.dll" DESTINATION "./")
      file(COPY "${TIFF_INCLUDE_DIR}\\..\\bin\\tiffxx.dll" DESTINATION "./")
      file(COPY "${TIFF_INCLUDE_DIR}\\..\\bin\\tiffd.dll" DESTINATION "./")
      file(COPY "${TIFF_INCLUDE_DIR}\\..\\bin\\tiffxxd.dll" DESTINATION "./")
  endif()
else()
  add_custom_command(TARGET test_executables POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ojph_expand>" "./"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ojph_compress>" "./"
  )
  if(EMSCRIPTEN)
    add_custom_command(TARGET test_executables POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:ojph_expand>/ojph_expand.wasm" "./"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:ojph_compress>/ojph_compress.wasm" "./"
    )
  endif(EMSCRIPTEN)
  if(CYGWIN OR MINGW)
    add_custom_command(TARGET test_executables POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "../bin/${CMAKE_SHARED_LIBRARY_PREFIX}gtest.dll" "./"
      COMMAND ${CMAKE_COMMAND} -E copy "../bin/${CMAKE_SHARED_LIBRARY_PREFIX}gtest_main.dll" "./"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:openjph>" "./"
    )
  endif()
endif(MSVC)
