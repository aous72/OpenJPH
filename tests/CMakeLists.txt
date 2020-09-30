set(ENV{CTEST_OUTPUT_ON_FAILURE} TRUE)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

add_test(NAME no_argument_message COMMAND ojph_compress)
set_tests_properties(no_argument_message PROPERTIES PASS_REGULAR_EXPRESSION "The following arguments are necessary")
