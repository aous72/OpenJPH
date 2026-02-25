# detect whether we are within the oss fuzz environment
if(DEFINED ENV{LIB_FUZZING_ENGINE})
  # if yes, we can link against the main function of the fuzzing engine
  link_libraries($ENV{LIB_FUZZING_ENGINE})
else()
  # if not, we need to define our main function
  add_compile_definitions(OJPH_FUZZ_TARGET_MAIN)
endif()

add_executable(ojph_expand_fuzz_target fuzz_targets/ojph_expand_fuzz_target.cpp)
target_link_libraries(ojph_expand_fuzz_target PRIVATE openjph)
