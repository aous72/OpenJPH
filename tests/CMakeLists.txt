
# create the mse_pae executable
include(mse_pae.cmake)

# Fetch googletest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Fetch test files
FetchContent_Declare(
  jp2k_test_codestreams
  URL https://github.com/aous72/jp2k_test_codestreams/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(jp2k_test_codestreams)

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