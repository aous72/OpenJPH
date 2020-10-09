set(ENV{CTEST_OUTPUT_ON_FAILURE} TRUE)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

add_test(NAME no_argument_ojph_compress COMMAND ojph_compress)
set_tests_properties(no_argument_ojph_compress PROPERTIES WILL_FAIL TRUE)
