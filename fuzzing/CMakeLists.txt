if(DEFINED ENV{LIB_FUZZING_ENGINE})
  link_libraries($ENV{LIB_FUZZING_ENGINE})
else()
  add_compile_definitions(OJPH_FUZZ_TARGET_MAIN)
endif()

add_executable(ojph_expand_fuzz_target fuzz_targets/ojph_expand_fuzz_target.cpp)
target_link_libraries(ojph_expand_fuzz_target PRIVATE openjph)
