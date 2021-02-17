########################
## Build test exeuctable
########################
project (psnr_pae DESCRIPTION "A program to find PSNR and peak absolute error between two images" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(psnr_pae psnr_pae.cpp)

########################
## Test environment
########################
set(ENV{CTEST_OUTPUT_ON_FAILURE} TRUE)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
set(kakadu_name KDU805_Demo_Apps_for_Linux-x86-64_200602)
set(images_folder ..)

file(DOWNLOAD https://kakadusoftware.com/wp-content/uploads/${kakadu_name}.zip ./t.zip)
execute_process(COMMAND unzip -jo ./t.zip ${kakadu_name}/kdu_expand ${kakadu_name}/kdu_compress ${kakadu_name}/libkdu_v80R.so)
file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/Malamute.ppm ./im1.ppm)
file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/Rainier.ppm ./im2.ppm)
file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/monarch.ppm ./im3.ppm)
file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/lena_444.yuv ./lena_444.yuv)
file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/foreman_422.yuv ./foreman_422.yuv)
file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/foreman_420.yuv ./foreman_420.yuv)

add_test(NAME no_argument_ojph_compress COMMAND ojph_compress)
set_tests_properties(no_argument_ojph_compress PROPERTIES WILL_FAIL TRUE)

add_test(NAME simple_64x64 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh "-i ${images_folder}/im1.ppm -o test.jph" "-i test.jph -o test.ppm")
add_test(NAME simple_32x32 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh "-i ${images_folder}/im1.ppm -o test.jph Cblk=\{32,32\}" "-i test.jph -o test.ppm")
