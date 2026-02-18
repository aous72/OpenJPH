if(DEFINED ENV{BUILD_UID})
  link_libraries($ENV{LIB_FUZZING_ENGINE})
else()
  add_compile_options(-fsanitize=fuzzer,address)
  add_link_options(-fsanitize=fuzzer,address)
endif()

add_executable(ojph_expand_fuzz_target fuzz_targets/ojph_expand_fuzz_target.cpp)
target_link_libraries(ojph_expand_fuzz_target PRIVATE openjph)

