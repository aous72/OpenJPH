
# Fetch googletest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Fetch test files
FetchContent_Declare(
  jp2k_test_codestreams
  URL               https://github.com/aous72/jp2k_test_codestreams/archive/refs/heads/main.zip
  SOURCE_DIR        jp2k_test_codestreams/
  CONFIGURE_COMMAND ""
)
FetchContent_Populate(jp2k_test_codestreams)

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
  if (MSVC AND OJPH_ENABLE_TIFF_SUPPORT)
    if (CMAKE_BUILD_TYPE MATCHES "Release")
      file(COPY "${TIFF_PATH}\\bin\\tiff.dll" DESTINATION "./")
      file(COPY "${TIFF_PATH}\\bin\\tiffxx.dll" DESTINATION "./")
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
      file(COPY "${TIFF_PATH}\\bin\\tiffd.dll" DESTINATION "./")
      file(COPY "${TIFF_PATH}\\bin\\tiffxxd.dll" DESTINATION "./")
    endif()
  endif() 
elseif(MSYS)
  add_custom_command(TARGET test_executables POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "../bin/msys-gtest.dll" "./"
    COMMAND ${CMAKE_COMMAND} -E copy "../bin/msys-gtest_main.dll" "./"
  )
endif()
