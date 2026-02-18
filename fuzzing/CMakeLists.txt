if(DEFINED ENV{BUILD_UID})
  link_libraries($ENV{LIB_FUZZING_ENGINE})
else()
  add_compile_options(-fsanitize=fuzzer,address)
  add_link_options(-fsanitize=fuzzer,address)
endif()

add_executable(j2c_expand_fuzz_target fuzz_targets/j2c_expand_fuzz_target.cpp)
target_link_libraries(j2c_expand_fuzz_target PRIVATE openjph)

