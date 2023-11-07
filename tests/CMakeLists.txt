
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

if (WIN32)
	add_custom_command(TARGET test_executables POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "../bin/Release/gtest.dll" "./Release/"
		COMMAND ${CMAKE_COMMAND} -E copy "../bin/Release/gtest_main.dll" "./Release/"
	)
endif()
