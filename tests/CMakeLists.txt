########################
## Build test exeuctable
########################
project (psnr_pae DESCRIPTION "A program to find PSNR and peak absolute error between two images" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(psnr_pae psnr_pae.cpp "../src/apps/others/ojph_img_io.cpp" "../src/core/others/ojph_message.cpp" "../src/core/others/ojph_file.cpp" "../src/core/others/ojph_mem.cpp" )
IF( USE_TIFF )
  target_link_libraries (psnr_pae ${TIFF_LIBRARIES})
ELSE()
  target_link_libraries (psnr_pae)
ENDIF()

########################
## Test environment
########################
set(ENV{CTEST_OUTPUT_ON_FAILURE} TRUE)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
if (WIN64)
    set(kakadu_name KDU805_Demo_Apps_for_Win64_200602.msi_)
    file(DOWNLOAD https://kakadusoftware.com/wp-content/uploads/${kakadu_name}.zip ./t.zip)
    execute_process(COMMAND unzip -jo ./t.zip ${kakadu_name}/kdu_expand.exe ${kakadu_name}/kdu_compress.exe ${kakadu_name}/kdu_v80R.dll)
endif()
if (UNIX AND NOT APPLE)
    set(kakadu_name KDU805_Demo_Apps_for_Linux-x86-64_200602)
    file(DOWNLOAD https://kakadusoftware.com/wp-content/uploads/${kakadu_name}.zip ./t.zip)
    execute_process(COMMAND unzip -jo ./t.zip ${kakadu_name}/kdu_expand ${kakadu_name}/kdu_compress ${kakadu_name}/libkdu_v80R.so)
endif()
if (APPLE)
message(STATUS "Kakadu tools cannot be extracted automatically on MacOS, because I cannot extract Kakadu from a .dmg file")
message(STATUS "Please make sure that you have kdu_compress, kdu_expand and the respective library in the build folder before running the test.")
#    set(kakadu_name KDU805_Demo_Apps_for_MacOS_200602.dmg_)
#    file(DOWNLOAD https://kakadusoftware.com/wp-content/uploads/${kakadu_name}.zip ./t.zip)
#    execute_process(COMMAND unzip -jo ./t.zip)
#    execute_process(COMMAND unzip -jo ./t.zip ${kakadu_name}/kdu_expand ${kakadu_name}/kdu_compress ${kakadu_name}/libkdu_v80R.so)
endif()


set(images_folder ..)
if (NOT EXISTS "./im1.ppm")
    file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/Malamute.ppm ./im1.ppm)
endif()
if (NOT EXISTS "./im2.ppm")
    file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/Rainier.ppm ./im2.ppm)
endif()
if (NOT EXISTS "./im3.ppm")
    file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/monarch.ppm ./im3.ppm)
endif()
if (NOT EXISTS "./im3.pgm")
    file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/monarch.pgm ./im3.pgm)
endif()
if (NOT EXISTS "./lena_444.yuv")
    file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/lena_444.yuv ./lena_444.yuv)
    execute_process(COMMAND dd if=lena_444.yuv of=lena_444y.rawl bs=1 count=262144)
    execute_process(COMMAND dd if=lena_444.yuv of=lena_444u.rawl bs=1 count=262144 skip=262144)
    execute_process(COMMAND dd if=lena_444.yuv of=lena_444v.rawl bs=1 count=262144 skip=524288)
endif()
if (NOT EXISTS "./foreman_422.yuv")
    file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/foreman_422.yuv ./foreman_422.yuv)
    execute_process(COMMAND dd if=foreman_422.yuv of=foreman_422y.rawl bs=1 count=101376)
    execute_process(COMMAND dd if=foreman_422.yuv of=foreman_422u.rawl bs=1 count=50688 skip=101376)
    execute_process(COMMAND dd if=foreman_422.yuv of=foreman_422v.rawl bs=1 count=50688 skip=152064)
endif()
if (NOT EXISTS "./foreman_420.yuv")
    file(DOWNLOAD https://github.com/aous72/aous72.github.io/raw/master/test_images/foreman_420.yuv ./foreman_420.yuv)
    execute_process(COMMAND dd if=foreman_420.yuv of=foreman_420y.rawl bs=1 count=101376)
    execute_process(COMMAND dd if=foreman_420.yuv of=foreman_420u.rawl bs=1 count=25344 skip=101376)
    execute_process(COMMAND dd if=foreman_420.yuv of=foreman_420v.rawl bs=1 count=25344 skip=126720)
endif()

add_test(NAME no_argument_ojph_compress COMMAND ojph_compress)
set_tests_properties(no_argument_ojph_compress PROPERTIES WILL_FAIL TRUE)

#############################################################
# Decoding
#############################################################

add_test(NAME simple_dec_irv97_64x64 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_32x32 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{32,32\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_16x16 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{16,16\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_4x4 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{4,4\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_1024x4 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{1024,4\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_4x1024 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{4,1024\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_512x8 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{512,8\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_8x512 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{8,512\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_256x16 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{256,16\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_16x256 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{16,256\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_128x32 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{128,32\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_32x128 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.01 Cblk=\{32,128\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")

add_test(NAME simple_dec_rev53_64x64 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -rdec "-i ${images_folder}/im1.ppm -o test.jph -precise Creversible=yes"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_rev53_32x32 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -rdec "-i ${images_folder}/im1.ppm -o test.jph -precise Creversible=yes Cblk=\{32,32\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_rev53_4x4 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -rdec "-i ${images_folder}/im1.ppm -o test.jph -precise Creversible=yes Cblk=\{4,4\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_rev53_1024x4 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -rdec "-i ${images_folder}/im1.ppm -o test.jph -precise Creversible=yes Cblk=\{1024,4\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_rev53_4x1024 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -rdec "-i ${images_folder}/im1.ppm -o test.jph -precise Creversible=yes Cblk=\{4,1024\}" "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")

add_test(NAME simple_dec_irv97_64x64_yuv COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom_yuv.sh -dec "-i ${images_folder}/foreman_420y.rawl,${images_folder}/foreman_420u.rawl,${images_folder}/foreman_420v.rawl -o test.jph -precise Qstep=0.1 Sdims=\{288,352\},\{144,176\},\{144,176\} Ssampling=\{1,1\},\{2,2\},\{2,2\} Nprecision=\{8\} Nsigned=\{no\}"  "-i test.jph -o test1y.rawl,test1u.rawl,test1v.rawl -precise" "-i test.jph -o test2.yuv" "${images_folder}/foreman_420.yuv:352x288x8x420" "test1.yuv:352x288x8x420" "test2.yuv:352x288x8x420")
add_test(NAME simple_dec_rev53_64x64_yuv COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom_yuv.sh -rdec "-i ${images_folder}/foreman_420y.rawl,${images_folder}/foreman_420u.rawl,${images_folder}/foreman_420v.rawl -o test.jph -precise Creversible=yes Qstep=0.1 Sdims=\{288,352\},\{144,176\},\{144,176\} Ssampling=\{1,1\},\{2,2\},\{2,2\} Nprecision=\{8\} Nsigned=\{no\}"  "-i test.jph -o test1y.rawl,test1u.rawl,test1v.rawl -precise" "-i test.jph -o test2.yuv" "${images_folder}/foreman_420.yuv:352x288x8x420" "test1.yuv:352x288x8x420" "test2.yuv:352x288x8x420")
add_test(NAME simple_dec_irv97_64x64_tiles_yuv COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom_yuv.sh -dec "-i ${images_folder}/foreman_420y.rawl,${images_folder}/foreman_420u.rawl,${images_folder}/foreman_420v.rawl -o test.jph -precise Qstep=0.1 Sdims=\{288,352\},\{144,176\},\{144,176\} Ssampling=\{1,1\},\{2,2\},\{2,2\} Nprecision=\{8\} Nsigned=\{no\} Stiles=\{33,257\}"  "-i test.jph -o test1y.rawl,test1u.rawl,test1v.rawl -precise" "-i test.jph -o test2.yuv" "${images_folder}/foreman_420.yuv:352x288x8x420" "test1.yuv:352x288x8x420" "test2.yuv:352x288x8x420")
add_test(NAME simple_dec_rev53_64x64_tiles_yuv COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom_yuv.sh -rdec "-i ${images_folder}/foreman_420y.rawl,${images_folder}/foreman_420u.rawl,${images_folder}/foreman_420v.rawl -o test.jph -precise Creversible=yes Qstep=0.1 Sdims=\{288,352\},\{144,176\},\{144,176\} Ssampling=\{1,1\},\{2,2\},\{2,2\} Nprecision=\{8\} Nsigned=\{no\} Stiles=\{33,257\}"  "-i test.jph -o test1y.rawl,test1u.rawl,test1v.rawl -precise" "-i test.jph -o test2.yuv" "${images_folder}/foreman_420.yuv:352x288x8x420" "test1.yuv:352x288x8x420" "test2.yuv:352x288x8x420")

add_test(NAME simple_dec_irv97_64x64_tiles_LRCP COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=LRCP Cprecincts=\{2,256\} Sorigin=\{374,1717\} Stile_origin=\{374,1717\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_RLCP COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=RLCP Cprecincts=\{2,256\} Sorigin=\{374,1717\} Stile_origin=\{374,1717\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_RPCL COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=RPCL Cprecincts=\{2,256\} Sorigin=\{374,1717\} Stile_origin=\{374,1717\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_PCRL COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=PCRL Cprecincts=\{2,256\} Sorigin=\{374,1717\} Stile_origin=\{374,1717\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_CPRL COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=CPRL Cprecincts=\{2,256\} Sorigin=\{374,1717\} Stile_origin=\{374,1717\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")

add_test(NAME simple_dec_irv97_64x64_tiles_LRCP33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=LRCP Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,257\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_RLCP33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=RLCP Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,257\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_RPCL33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=RPCL Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,257\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_PCRL33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=PCRL Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,257\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_CPRL33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=CPRL Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,257\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")

add_test(NAME simple_dec_irv97_64x64_tiles_LRCP33x33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=LRCP Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,33\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_RLCP33x33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=RLCP Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,33\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_RPCL33x33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=RPCL Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,33\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_PCRL33x33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=PCRL Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,33\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_dec_irv97_64x64_tiles_CPRL33x33 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im1.ppm -o test.jph -precise Qstep=0.1 Clevels=5 Corder=CPRL Sorigin=\{5,33\} Stile_origin=\{5,10\} Stiles=\{33,33\}"  "-i test.jph -o test1.ppm -precise" "-i test.jph -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")

add_test(NAME simple_dec_rev53_64x64_gray_tiles COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -rdec "-i ${images_folder}/im3.pgm -o test.jph -precise Qstep=0.1 Creversible=yes Clevels=5 Stiles=\{33,257\}"  "-i test.jph -o test1.pgm -precise" "-i test.jph -o test2.pgm" "${images_folder}/im3.pgm" "test1.pgm" "test2.pgm")
add_test(NAME simple_dec_irv97_64x64_gray_tiles COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -dec "-i ${images_folder}/im3.pgm -o test.jph -precise Qstep=0.1 Clevels=5 Stiles=\{33,257\}"  "-i test.jph -o test1.pgm -precise" "-i test.jph -o test2.pgm" "${images_folder}/im3.pgm" "test1.pgm" "test2.pgm")



#############################################################
# Encoding
#############################################################

add_test(NAME simple_enc_irv97_64x64  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.1"                         "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_32x32  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{32,32\}"  "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_16x16  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{16,16\}"  "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_4x4    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{4,4\}"    "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_1024x4 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{4,1024\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_4x1024 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{1024,4\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_512x8  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{8,512\}"  "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_8x512  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{512,8\}"  "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_256x16 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{16,256\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_16x256 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{256,16\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_128x32 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{32,128\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_irv97_32x128 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -enc "-i ${images_folder}/im1.ppm -o test.j2c -qstep 0.01 -block_size \{128,32\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")

add_test(NAME simple_enc_rev53_64x64  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -renc "-i ${images_folder}/im1.ppm -o test.j2c -reversible true"                        "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_rev53_32x32  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -renc "-i ${images_folder}/im1.ppm -o test.j2c -reversible true -block_size \{32,32\}"  "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_rev53_4x4    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -renc "-i ${images_folder}/im1.ppm -o test.j2c -reversible true -block_size \{4,4\}"    "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_rev53_1024x4 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -renc "-i ${images_folder}/im1.ppm -o test.j2c -reversible true -block_size \{4,1024\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")
add_test(NAME simple_enc_rev53_4x1024 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom.sh -renc "-i ${images_folder}/im1.ppm -o test.j2c -reversible true -block_size \{1024,4\}" "-i test.j2c -o test1.ppm -precise" "-i test.j2c -o test2.ppm" "${images_folder}/im1.ppm" "test1.ppm" "test2.ppm")

add_test(NAME simple_enc_irv97_64x64_yuv COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom_yuv.sh -enc  "-i ${images_folder}/foreman_420.yuv -o test.j2c -qstep 0.1 -dims \{352,288\} -num_comps 3 -downsamp \{1,1\},\{2,2\},\{2,2\} -bit_depth 8,8,8 -signed false,false,false"                   "-i test.j2c -o test1y.rawl,test1u.rawl,test1v.rawl -precise" "-i test.j2c -o test2.yuv" "${images_folder}/foreman_420.yuv:352x288x8x420" "test1.yuv:352x288x8x420" "test2.yuv:352x288x8x420")
add_test(NAME simple_enc_rev53_64x64_yuv COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/com_decom_yuv.sh -renc "-i ${images_folder}/foreman_420.yuv -o test.j2c -reversible true -qstep 0.1 -dims \{352,288\} -num_comps 3 -downsamp \{1,1\},\{2,2\},\{2,2\} -bit_depth 8,8,8 -signed false,false,false"  "-i test.j2c -o test1y.rawl,test1u.rawl,test1v.rawl -precise" "-i test.j2c -o test2.yuv" "${images_folder}/foreman_420.yuv:352x288x8x420" "test1.yuv:352x288x8x420" "test2.yuv:352x288x8x420")

