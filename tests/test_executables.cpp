//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman 
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: test_executables.cpp
// Author: Aous Naman
// Date: 30 December 2022
//***************************************************************************/

#include <array>
#include <string>
#include "ojph_arch.h"
#include "gtest/gtest.h"

////////////////////////////////////////////////////////////////////////////////
// STATIC                         ojph_popen
////////////////////////////////////////////////////////////////////////////////
static inline
FILE* ojph_popen(const char* command, const char* modes)
{
#ifdef OJPH_COMPILER_MSVC
  return _popen(command, modes);
#else
  return popen(command, modes);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// STATIC                         ojph_pclose
////////////////////////////////////////////////////////////////////////////////
static inline
int ojph_pclose(FILE* stream)
{
#ifdef OJPH_COMPILER_MSVC
  return _pclose(stream);
#else
  return pclose(stream);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// STATIC                           execute
////////////////////////////////////////////////////////////////////////////////
static
int execute(const std::string& cmd, std::string& result)
{
  std::array<char, 128> buffer;
  result.clear();

  FILE* pipe = ojph_popen(cmd.c_str(), "r");
  if (!pipe)
    throw std::runtime_error("ojph_popen() failed!");

  while (!feof(pipe))
    if (fgets(buffer.data(), 128, pipe) != nullptr)
      result += buffer.data();

  int rc = ojph_pclose(pipe);
  if (rc != EXIT_SUCCESS)
    return 1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//                                 MACROS
////////////////////////////////////////////////////////////////////////////////

#ifdef OJPH_OS_WINDOWS
#define SRC_FILE_DIR ".\\jp2k_test_codestreams\\openjph\\"
#define OUT_FILE_DIR ".\\"
#define REF_FILE_DIR ".\\jp2k_test_codestreams\\openjph\\references\\"
#define MSE_PAE_PATH  ".\\mse_pae"
#define COMPARE_FILES_PATH  ".\\compare_files"
#define EXPAND_EXECUTABLE ".\\ojph_expand.exe"
#define COMPRESS_EXECUTABLE ".\\ojph_compress.exe"
#else
#define SRC_FILE_DIR "./jp2k_test_codestreams/openjph/"
#define OUT_FILE_DIR "./"
#define REF_FILE_DIR "./jp2k_test_codestreams/openjph/references/"
#define MSE_PAE_PATH  "./mse_pae"
#define COMPARE_FILES_PATH  "./compare_files"

// This is a comment to me, to help with emscripten testing.
// This is written after the completion of the tests.
// 1. Compile for the target platform (Linux), selecting from the following
//    code the version that suits you; in particular it should be the one
//    the uses node.  Ideally create two versions of test_executables, one
//    for WASM SIMD, and for WASM without SIMD -- use linux cp command to
//    create test_executables_simd and test_executables_no_simd
// 2. Compile again, without deleting what compiled; this time compile using
//    emscripten, targeting WASM.  The compilation is very finicky, do
//    'make clean && make' after every change in code.
// 3. cd to tests, and run test_executables_simd or test_executables_no_simd.

#define EXPAND_EXECUTABLE "./ojph_expand"
#define COMPRESS_EXECUTABLE "./ojph_compress"
//#define EXPAND_EXECUTABLE "20.18.0_64bit/bin/node ./ojph_expand.js"
//#define COMPRESS_EXECUTABLE "20.18.0_64bit/bin/node ./ojph_compress.js"
//#define EXPAND_EXECUTABLE "node-v18.7.0-linux-x64/bin/node ./ojph_expand_simd.js"
//#define COMPRESS_EXECUTABLE "node-v18.7.0-linux-x64/bin/node ./ojph_compress_simd.js"
//#define EXPAND_EXECUTABLE "./../../../sde/sde64 -skx -- ./ojph_expand"
//#define COMPRESS_EXECUTABLE "./../../../sde/sde64 -skx -- ./ojph_compress"
#endif
#define TOL_DOUBLE 0.01
#define TOL_INTEGER 1

////////////////////////////////////////////////////////////////////////////////
//                            run_ojph_compress
////////////////////////////////////////////////////////////////////////////////
void run_ojph_compress(const std::string& ref_filename,
  const std::string& base_filename,
  const std::string& extended_base_fname,
  const std::string& out_ext,
  const std::string& extra_options)
{
  try {
    std::string result, command;
    command = std::string(COMPRESS_EXECUTABLE)
      + " -i " + REF_FILE_DIR + ref_filename
      + " -o " + OUT_FILE_DIR + base_filename + extended_base_fname +
      "." + out_ext + " " + extra_options;
    EXPECT_EQ(execute(command, result), 0);
  }
  catch (const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
//                            run_ojph_expand
////////////////////////////////////////////////////////////////////////////////
void run_ojph_expand(const std::string& base_filename,
  const std::string& src_ext,
  const std::string& out_ext)
{
  try {
    std::string result, command;
    command = std::string(EXPAND_EXECUTABLE)
      + " -i " + SRC_FILE_DIR + base_filename + "." + src_ext
      + " -o " + OUT_FILE_DIR + base_filename + "." + out_ext;
    EXPECT_EQ(execute(command, result), 0);
  }
  catch (const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
//                            run_ojph_compress
////////////////////////////////////////////////////////////////////////////////
void run_ojph_compress_expand(const std::string& base_filename,
  const std::string& out_ext,
  const std::string& decode_ext)
{
  try {
    std::string result, command;
    command = std::string(EXPAND_EXECUTABLE)
      + " -i " + OUT_FILE_DIR + base_filename + "." + out_ext
      + " -o " + OUT_FILE_DIR + base_filename + "." + decode_ext;
    EXPECT_EQ(execute(command, result), 0);
  }
  catch (const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
//                             run_mse_pae
////////////////////////////////////////////////////////////////////////////////
void run_mse_pae(const std::string& base_filename,
  const std::string& out_ext,
  const std::string& ref_filename,
  const std::string& yuv_specs,
  int num_components, double* mse, int* pae)
{
  try {
    std::string result, command;
    command = std::string(MSE_PAE_PATH)
      + " " + OUT_FILE_DIR + base_filename + "." + out_ext + yuv_specs
      + " " + REF_FILE_DIR + ref_filename + yuv_specs;
    EXPECT_EQ(execute(command, result), 0);

    size_t pos = 0;
    for (int c = 0; c < num_components; ++c) {
      if (pos < result.length()) {
        double valf = atof(result.c_str() + pos);
        EXPECT_NEAR((valf - mse[c]) / (valf + TOL_DOUBLE), 0.0, TOL_DOUBLE);
      }
      else {
        FAIL() << "mse_pae result string does not have enough entries.";
      }
      pos = result.find(" ", pos);
      if (pos != std::string::npos)
        ++pos;
      if (pos < result.length()) {
        int vali = atoi(result.c_str() + pos);
        EXPECT_NEAR(vali, pae[c], TOL_INTEGER);
      }
      else {
        FAIL() << "mse_pae result string does not have enough entries.";
      }
      pos = result.find("\n", pos);
      if (pos != std::string::npos)
        ++pos;
    }
  }
  catch (const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
//                             compare_files
////////////////////////////////////////////////////////////////////////////////
void compare_files(const std::string& base_filename,
  const std::string& extended_base_fname,
  const std::string& ext)
{
  try {
    std::string result, command;
    command = std::string(COMPARE_FILES_PATH)
      + " " + OUT_FILE_DIR + base_filename + extended_base_fname + "." + ext
      + " " + SRC_FILE_DIR + base_filename + "." + ext;
    EXPECT_EQ(execute(command, result), 0);
  }
  catch (const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
//                                  tests
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Test ojph_compress on its own
TEST(TestExecutables, OpenJPHCompressNoArguments) {
  try {
    std::string result;
    EXPECT_EQ(execute(COMPRESS_EXECUTABLE, result), 1);
  }
  catch (const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
// Test ojph_expand on its own
TEST(TestExecutables, OpenJPHExpandNoArguments) {
  try {
    std::string result;
    EXPECT_EQ(execute(EXPAND_EXECUTABLE, result), 1);
  }
  catch (const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
//                               other tests
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64.jph -precise -quiet -rate 0.5 -full
TEST(TestExecutables, SimpleDecIrv9764x64) {
  double mse[3] = { 39.2812, 36.3819, 47.642};
  int pae[3] = { 74, 77, 73};
  run_ojph_expand("simple_dec_irv97_64x64", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_32x32.jph -precise -quiet -rate 1 Cblk={32,32} -full
TEST(TestExecutables, SimpleDecIrv9732x32) {
  double mse[3] = { 18.6979, 17.1208, 22.7539};
  int pae[3] = { 51, 48, 46};
  run_ojph_expand("simple_dec_irv97_32x32", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_32x32", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_16x16.jph -precise -quiet -rate 1 Cblk={16,16} -full
TEST(TestExecutables, SimpleDecIrv9716x16) {
  double mse[3] = { 20.1706, 18.5427, 24.6146};
  int pae[3] = { 53, 51, 47};
  run_ojph_expand("simple_dec_irv97_16x16", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_16x16", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_4x4.jph -precise -quiet -rate 1 Cblk={4,4} -full
TEST(TestExecutables, SimpleDecIrv974x4) {
  double mse[3] = { 40.8623, 37.9308, 49.7276};
  int pae[3] = { 75, 77, 80};
  run_ojph_expand("simple_dec_irv97_4x4", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_4x4", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_1024x4.jph -precise -quiet -rate 1 Cblk={1024,4} -full
TEST(TestExecutables, SimpleDecIrv971024x4) {
  double mse[3] = { 19.8275, 18.2511, 24.2832};
  int pae[3] = { 53, 52, 50};
  run_ojph_expand("simple_dec_irv97_1024x4", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_1024x4", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_4x1024.jph -precise -quiet -rate 1 Cblk={4,1024} -full
TEST(TestExecutables, SimpleDecIrv974x1024) {
  double mse[3] = { 19.9635, 18.4063, 24.1719};
  int pae[3] = { 51, 48, 51};
  run_ojph_expand("simple_dec_irv97_4x1024", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_4x1024", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_512x8.jph -precise -quiet -rate 1 Cblk={512,8} -full
TEST(TestExecutables, SimpleDecIrv97512x8) {
  double mse[3] = { 18.7929, 17.2026, 22.9922};
  int pae[3] = { 53, 52, 50};
  run_ojph_expand("simple_dec_irv97_512x8", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_512x8", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_8x512.jph -precise -quiet -rate 1 Cblk={8,512} -full
TEST(TestExecutables, SimpleDecIrv978x512) {
  double mse[3] = { 19.3661, 17.8067, 23.4574};
  int pae[3] = { 51, 48, 52};
  run_ojph_expand("simple_dec_irv97_8x512", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_8x512", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_256x16.jph -precise -quiet -rate 1 Cblk={256,16} -full
TEST(TestExecutables, SimpleDecIrv97256x16) {
  double mse[3] = { 18.6355, 17.0963, 22.6076};
  int pae[3] = { 54, 51, 48};
  run_ojph_expand("simple_dec_irv97_256x16", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_256x16", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_16x256.jph -precise -quiet -rate 1 Cblk={16,256} -full
TEST(TestExecutables, SimpleDecIrv9716x256) {
  double mse[3] = { 18.5933, 17.0208, 22.5709};
  int pae[3] = { 51, 48, 47};
  run_ojph_expand("simple_dec_irv97_16x256", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_16x256", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_128x32.jph -precise -quiet -rate 1 Cblk={128,32} -full
TEST(TestExecutables, SimpleDecIrv97128x32) {
  double mse[3] = { 18.4443, 16.9133, 22.4193};
  int pae[3] = { 52, 50, 46};
  run_ojph_expand("simple_dec_irv97_128x32", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_128x32", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_32x128.jph -precise -quiet -rate 1 Cblk={32,128} -full
TEST(TestExecutables, SimpleDecIrv9732x128) {
  double mse[3] = { 18.4874, 16.9379, 22.4855};
  int pae[3] = { 51, 48, 45};
  run_ojph_expand("simple_dec_irv97_32x128", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_32x128", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64.jph -precise -quiet Creversible=yes -full
TEST(TestExecutables, SimpleDecRev5364x64) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_64x64", "jph", "ppm");
  run_mse_pae("simple_dec_rev53_64x64", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_32x32.jph -precise -quiet Creversible=yes Cblk={32,32}
// -full
TEST(TestExecutables, SimpleDecRev5332x32) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_32x32", "jph", "ppm");
  run_mse_pae("simple_dec_rev53_32x32", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_4x4.jph -precise -quiet Creversible=yes Cblk={4,4}
// -full
TEST(TestExecutables, SimpleDecRev534x4) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_4x4", "jph", "ppm");
  run_mse_pae("simple_dec_rev53_4x4", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_1024x4.jph -precise -quiet Creversible=yes
// Cblk={1024,4} -full
TEST(TestExecutables, SimpleDecRev531024x4) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_1024x4", "jph", "ppm");
  run_mse_pae("simple_dec_rev53_1024x4", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_4x1024.jph -precise -quiet Creversible=yes
// Cblk={4,1024} -full
TEST(TestExecutables, SimpleDecRev534x1024) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_4x1024", "jph", "ppm");
  run_mse_pae("simple_dec_rev53_4x1024", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_yuv.jph -precise -quiet -rate 0.5
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
// Nprecision={8} Nsigned={no} -full
TEST(TestExecutables, SimpleDecIrv9764x64Yuv) {
  double mse[3] = { 20.2778, 6.27912, 4.15937};
  int pae[3] = { 52, 22, 31};
  run_ojph_expand("simple_dec_irv97_64x64_yuv", "jph", "yuv");
  run_mse_pae("simple_dec_irv97_64x64_yuv", "yuv", "foreman_420.yuv",
              ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_yuv.jph -precise -quiet Creversible=yes
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
// Nprecision={8} Nsigned={no} -full
TEST(TestExecutables, SimpleDecRev5364x64Yuv) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_64x64_yuv", "jph", "yuv");
  run_mse_pae("simple_dec_rev53_64x64_yuv", "yuv", "foreman_420.yuv",
              ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_yuv.jph -precise -quiet -rate 0.5
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
// Nprecision={8} Nsigned={no} Stiles={33,257} -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesYuv) {
  double mse[3] = { 34.4972, 10.1112, 7.96331};
  int pae[3] = { 67, 30, 39};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_yuv", "jph", "yuv");
  run_mse_pae("simple_dec_irv97_64x64_tiles_yuv", "yuv", "foreman_420.yuv",
              ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_tiles_yuv.jph -precise -quiet Creversible=yes
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
// Nprecision={8} Nsigned={no} Stiles={33,257} -full
TEST(TestExecutables, SimpleDecRev5364x64TilesYuv) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_64x64_tiles_yuv", "jph", "yuv");
  run_mse_pae("simple_dec_rev53_64x64_tiles_yuv", "yuv", "foreman_420.yuv",
              ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_LRCP.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=LRCP Cprecincts={2,256} Sorigin={374,1717}
// Stile_origin={374,1717} -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesLRCP) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_LRCP", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_LRCP", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RLCP.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RLCP Cprecincts={2,256} Sorigin={374,1717}
// Stile_origin={374,1717} -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesRLCP) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_RLCP", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_RLCP", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RPCL.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RPCL Cprecincts={2,256} Sorigin={374,1717}
// Stile_origin={374,1717} -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesRPCL) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_RPCL", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_RPCL", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_PCRL.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=PCRL Cprecincts={2,256} Sorigin={374,1717}
// Stile_origin={374,1717} -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesPCRL) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_PCRL", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_PCRL", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_CPRL.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=CPRL Cprecincts={2,256} Sorigin={374,1717}
// Stile_origin={374,1717} -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesCPRL) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_CPRL", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_CPRL", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_LRCP33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=LRCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesLRCP33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_LRCP33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_LRCP33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RLCP33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RLCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesRLCP33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_RLCP33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_RLCP33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RPCL33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RPCL Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesRPCL33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_RPCL33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_RPCL33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_PCRL33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=PCRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesPCRL33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_PCRL33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_PCRL33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_CPRL33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=CPRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesCPRL33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_CPRL33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_CPRL33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_LRCP33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=LRCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesLRCP33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_LRCP33x33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_LRCP33x33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RLCP33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RLCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesRLCP33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_RLCP33x33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_RLCP33x33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RPCL33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RPCL Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesRPCL33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_RPCL33x33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_RPCL33x33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_PCRL33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=PCRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesPCRL33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_PCRL33x33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_PCRL33x33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_CPRL33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=CPRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
// -full
TEST(TestExecutables, SimpleDecIrv9764x64TilesCPRL33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  run_ojph_expand("simple_dec_irv97_64x64_tiles_CPRL33x33", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_tiles_CPRL33x33", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_gray_tiles.jph -precise -quiet Creversible=yes
// Clevels=5 Stiles={33,257} -full
TEST(TestExecutables, SimpleDecRev5364x64GrayTiles) {
  double mse[1] = { 0};
  int pae[1] = { 0};
  run_ojph_expand("simple_dec_rev53_64x64_gray_tiles", "jph", "pgm");
  run_mse_pae("simple_dec_rev53_64x64_gray_tiles", "pgm", "monarch.pgm",
              "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_gray_tiles.jph -precise -quiet -rate 0.5
// Clevels=5 Stiles={33,257} -full
TEST(TestExecutables, SimpleDecIrv9764x64GrayTiles) {
  double mse[1] = { 18.9601};
  int pae[1] = { 56};
  run_ojph_expand("simple_dec_irv97_64x64_gray_tiles", "jph", "pgm");
  run_mse_pae("simple_dec_irv97_64x64_gray_tiles", "pgm", "monarch.pgm",
              "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_16bit.jph -precise -quiet -rate 0.5 -full
TEST(TestExecutables, SimpleDecIrv9764x6416bit) {
  double mse[3] = { 60507.2, 36672.5, 64809.8};
  int pae[3] = { 2547, 1974, 1922};
  run_ojph_expand("simple_dec_irv97_64x64_16bit", "jph", "ppm");
  run_mse_pae("simple_dec_irv97_64x64_16bit", "ppm", "mm.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_16bit_gray.jph -precise -quiet -rate 0.5 -full
TEST(TestExecutables, SimpleDecIrv9764x6416bitGray) {
  double mse[1] = { 19382.9};
  int pae[1] = { 1618};
  run_ojph_expand("simple_dec_irv97_64x64_16bit_gray", "jph", "pgm");
  run_mse_pae("simple_dec_irv97_64x64_16bit_gray", "pgm", "mm.pgm",
              "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_16bit.jph -precise -quiet Creversible=yes -full
TEST(TestExecutables, SimpleDecRev5364x6416bit) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_expand("simple_dec_rev53_64x64_16bit", "jph", "ppm");
  run_mse_pae("simple_dec_rev53_64x64_16bit", "ppm", "mm.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_16bit_gray.jph -precise -quiet Creversible=yes
// -full
TEST(TestExecutables, SimpleDecRev5364x6416bitGray) {
  double mse[1] = { 0};
  int pae[1] = { 0};
  run_ojph_expand("simple_dec_rev53_64x64_16bit_gray", "jph", "pgm");
  run_mse_pae("simple_dec_rev53_64x64_16bit_gray", "pgm", "mm.pgm",
              "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv53_bhvhb_low_latency.jph -quiet Corder=PCRL Clevels=5
// Cmodes=HT|CAUSAL -rate 2 Catk=2 Kkernels:I2=I5X3
// Cprecincts={16,8192},{8,8192},{4,8192} Cblk={8,256}
// Cdecomp=B(-:-:-),H(-),V(-),H(-),B(-:-:-) Qstep=0.0001 -precise -no_weights
// -tolerance 0
TEST(TestExecutables, SimpleDecIrv53BhvhbLowLatency) {
  double mse[3] = { 5.52392, 4.01405, 6.8166};
  int pae[3] = { 16, 17, 23};
  run_ojph_expand("simple_dec_irv53_bhvhb_low_latency", "jph", "ppm");
  run_mse_pae("simple_dec_irv53_bhvhb_low_latency", "ppm", "Malamute.ppm",
              "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_64x64.j2c -qstep 0.1
TEST(TestExecutables, SimpleEncIrv9764x64) {
  double mse[3] = { 46.2004, 43.622, 56.7452};
  int pae[3] = { 48, 46, 52};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_64x64", "", "j2c",
                    "-qstep 0.1");
  run_ojph_compress_expand("simple_enc_irv97_64x64", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_64x64", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_32x32.j2c -qstep 0.01 -block_size {32,32}
TEST(TestExecutables, SimpleEncIrv9732x32) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_32x32", "", "j2c",
                    "-qstep 0.01 -block_size \"{32,32}\"");
  run_ojph_compress_expand("simple_enc_irv97_32x32", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_32x32", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_16x16.j2c -qstep 0.01 -block_size {16,16}
TEST(TestExecutables, SimpleEncIrv9716x16) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_16x16", "", "j2c",
                    "-qstep 0.01 -block_size \"{16,16}\"");
  run_ojph_compress_expand("simple_enc_irv97_16x16", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_16x16", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_4x4.j2c -qstep 0.01 -block_size {4,4}
TEST(TestExecutables, SimpleEncIrv974x4) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_4x4", "", "j2c",
                    "-qstep 0.01 -block_size \"{4,4}\"");
  run_ojph_compress_expand("simple_enc_irv97_4x4", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_4x4", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_1024x4.j2c -qstep 0.01 -block_size {4,1024}
TEST(TestExecutables, SimpleEncIrv971024x4) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_1024x4", "", "j2c",
                    "-qstep 0.01 -block_size \"{4,1024}\"");
  run_ojph_compress_expand("simple_enc_irv97_1024x4", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_1024x4", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_4x1024.j2c -qstep 0.01 -block_size {1024,4}
TEST(TestExecutables, SimpleEncIrv974x1024) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_4x1024", "", "j2c",
                    "-qstep 0.01 -block_size \"{1024,4}\"");
  run_ojph_compress_expand("simple_enc_irv97_4x1024", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_4x1024", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_512x8.j2c -qstep 0.01 -block_size {8,512}
TEST(TestExecutables, SimpleEncIrv97512x8) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_512x8", "", "j2c",
                    "-qstep 0.01 -block_size \"{8,512}\"");
  run_ojph_compress_expand("simple_enc_irv97_512x8", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_512x8", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_8x512.j2c -qstep 0.01 -block_size {512,8}
TEST(TestExecutables, SimpleEncIrv978x512) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_8x512", "", "j2c",
                    "-qstep 0.01 -block_size \"{512,8}\"");
  run_ojph_compress_expand("simple_enc_irv97_8x512", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_8x512", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_256x16.j2c -qstep 0.01 -block_size {16,256}
TEST(TestExecutables, SimpleEncIrv97256x16) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_256x16", "", "j2c",
                    "-qstep 0.01 -block_size \"{16,256}\"");
  run_ojph_compress_expand("simple_enc_irv97_256x16", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_256x16", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_16x256.j2c -qstep 0.01 -block_size {256,16}
TEST(TestExecutables, SimpleEncIrv9716x256) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_16x256", "", "j2c",
                    "-qstep 0.01 -block_size \"{256,16}\"");
  run_ojph_compress_expand("simple_enc_irv97_16x256", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_16x256", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_128x32.j2c -qstep 0.01 -block_size {32,128}
TEST(TestExecutables, SimpleEncIrv97128x32) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_128x32", "", "j2c",
                    "-qstep 0.01 -block_size \"{32,128}\"");
  run_ojph_compress_expand("simple_enc_irv97_128x32", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_128x32", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_32x128.j2c -qstep 0.01 -block_size {128,32}
TEST(TestExecutables, SimpleEncIrv9732x128) {
  double mse[3] = { 1.78779, 1.26001, 2.38395};
  int pae[3] = { 7, 6, 9};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_32x128", "", "j2c",
                    "-qstep 0.01 -block_size \"{128,32}\"");
  run_ojph_compress_expand("simple_enc_irv97_32x128", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_32x128", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_64x64_tiles_33x33_d5.j2c -qstep 0.01 -tile_size {33,33}
// -num_decomps 5
TEST(TestExecutables, SimpleEncIrv9764x64Tiles33x33D5) {
  double mse[3] = { 1.88906, 1.30757, 2.5347};
  int pae[3] = { 9, 6, 10};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_64x64_tiles_33x33_d5", "", "j2c",
                    "-qstep 0.01 -tile_size \"{33,33}\" -num_decomps 5");
  run_ojph_compress_expand("simple_enc_irv97_64x64_tiles_33x33_d5", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_64x64_tiles_33x33_d5", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_64x64_tiles_33x33_d6.j2c -qstep 0.01 -tile_size {33,33}
// -num_decomps 6
TEST(TestExecutables, SimpleEncIrv9764x64Tiles33x33D6) {
  double mse[3] = { 1.88751, 1.30673, 2.53378};
  int pae[3] = { 8, 6, 10};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_irv97_64x64_tiles_33x33_d6", "", "j2c",
                    "-qstep 0.01 -tile_size \"{33,33}\" -num_decomps 6");
  run_ojph_compress_expand("simple_enc_irv97_64x64_tiles_33x33_d6", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_64x64_tiles_33x33_d6", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_64x64_16bit.j2c -qstep 0.01
TEST(TestExecutables, SimpleEncIrv9764x6416bit) {
  double mse[3] = { 51727.3, 32596.4, 45897.8};
  int pae[3] = { 1512, 1481, 1778};
  run_ojph_compress("mm.ppm",
                    "simple_enc_irv97_64x64_16bit", "", "j2c",
                    "-qstep 0.01");
  run_ojph_compress_expand("simple_enc_irv97_64x64_16bit", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_64x64_16bit", "ppm",
              "mm.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_64x64_16bit_gray.j2c -qstep 0.01
TEST(TestExecutables, SimpleEncIrv9764x6416bitGray) {
  double mse[1] = { 25150.6};
  int pae[1] = { 1081};
  run_ojph_compress("mm.pgm",
                    "simple_enc_irv97_64x64_16bit_gray", "", "j2c",
                    "-qstep 0.01");
  run_ojph_compress_expand("simple_enc_irv97_64x64_16bit_gray", "j2c", "pgm");
  run_mse_pae("simple_enc_irv97_64x64_16bit_gray", "pgm",
              "mm.pgm", "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_64x64_16bit.j2c -reversible true
TEST(TestExecutables, SimpleEncRev5364x6416bit) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("mm.ppm",
                    "simple_enc_rev53_64x64_16bit", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("simple_enc_rev53_64x64_16bit", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_64x64_16bit", "ppm",
              "mm.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_64x64_16bit_gray.j2c -reversible true
TEST(TestExecutables, SimpleEncRev5364x6416bitGray) {
  double mse[1] = { 0};
  int pae[1] = { 0};
  run_ojph_compress("mm.pgm",
                    "simple_enc_rev53_64x64_16bit_gray", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("simple_enc_rev53_64x64_16bit_gray", "j2c", "pgm");
  run_mse_pae("simple_enc_rev53_64x64_16bit_gray", "pgm",
              "mm.pgm", "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_64x64_16bit.j2c -reversible true
TEST(TestExecutables, SimpleEncRev5364x64) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_rev53_64x64", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("simple_enc_rev53_64x64", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_64x64", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_32x32.j2c -reversible true -block_size {32,32}
TEST(TestExecutables, SimpleEncRev5332x32) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_rev53_32x32", "", "j2c",
                    "-reversible true -block_size \"{32,32}\"");
  run_ojph_compress_expand("simple_enc_rev53_32x32", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_32x32", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_4x4.j2c -reversible true -block_size {4,4}
TEST(TestExecutables, SimpleEncRev534x4) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_rev53_4x4", "", "j2c",
                    "-reversible true -block_size \"{4,4}\"");
  run_ojph_compress_expand("simple_enc_rev53_4x4", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_4x4", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_1024x4.j2c -reversible true -block_size {4,1024}
TEST(TestExecutables, SimpleEncRev531024x4) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_rev53_1024x4", "", "j2c",
                    "-reversible true -block_size \"{4,1024}\"");
  run_ojph_compress_expand("simple_enc_rev53_1024x4", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_1024x4", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_4x1024.j2c -reversible true -block_size {1024,4}
TEST(TestExecutables, SimpleEncRev534x1024) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_rev53_4x1024", "", "j2c",
                    "-reversible true -block_size \"{1024,4}\"");
  run_ojph_compress_expand("simple_enc_rev53_4x1024", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_4x1024", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_64x64_tiles_33x33_d5.j2c -reversible true -tile_size
// {32,32} -num_decomps 5
TEST(TestExecutables, SimpleEncRev5364x64Tiles33x33D5) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_rev53_64x64_tiles_33x33_d5", "", "j2c",
                    "-reversible true -tile_size \"{32,32}\" -num_decomps 5");
  run_ojph_compress_expand("simple_enc_rev53_64x64_tiles_33x33_d5", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_64x64_tiles_33x33_d5", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_64x64_tiles_33x33_d6.j2c -reversible true -tile_size
// {32,32} -num_decomps 6
TEST(TestExecutables, SimpleEncRev5364x64Tiles33x33D6) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("Malamute.ppm",
                    "simple_enc_rev53_64x64_tiles_33x33_d6", "", "j2c",
                    "-reversible true -tile_size \"{32,32}\" -num_decomps 6");
  run_ojph_compress_expand("simple_enc_rev53_64x64_tiles_33x33_d6", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_64x64_tiles_33x33_d6", "ppm",
              "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_64x64_yuv.j2c -qstep 0.1 -dims {352,288} -num_comps 3
// -downsamp {1,1},{2,2},{2,2} -bit_depth 8,8,8 -signed false,false,false
TEST(TestExecutables, SimpleEncIrv9764x64Yuv) {
  double mse[3] = { 30.3548, 7.69602, 5.22246};
  int pae[3] = { 49, 27, 26};
  run_ojph_compress("foreman_420.yuv",
                    "simple_enc_irv97_64x64_yuv", "", "j2c",
                    "-qstep 0.1 -dims \"{352,288}\" -num_comps 3 -downsamp"
                    " \"{1,1}\",\"{2,2}\",\"{2,2}\" -bit_depth 8,8,8"
                    " -signed false,false,false");
  run_ojph_compress_expand("simple_enc_irv97_64x64_yuv", "j2c", "yuv");
  run_mse_pae("simple_enc_irv97_64x64_yuv", "yuv",
              "foreman_420.yuv", ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_64x64_yuv.j2c -reversible true -qstep 0.1 -dims
// {352,288} -num_comps 3 -downsamp {1,1},{2,2},{2,2} -bit_depth 8,8,8 -signed
// false,false,false
TEST(TestExecutables, SimpleEncRev5364x64Yuv) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("foreman_420.yuv",
                    "simple_enc_rev53_64x64_yuv", "", "j2c",
                    "-reversible true -qstep 0.1 -dims \"{352,288}\""
                    " -num_comps 3 -downsamp \"{1,1}\",\"{2,2}\",\"{2,2}\""
                    " -bit_depth 8,8,8 -signed false,false,false");
  run_ojph_compress_expand("simple_enc_rev53_64x64_yuv", "j2c", "yuv");
  run_mse_pae("simple_enc_rev53_64x64_yuv", "yuv",
              "foreman_420.yuv", ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_tall_narrow.j2c -qstep 0.1
TEST(TestExecutables, SimpleEncIrv97TallNarrow) {
  double mse[3] = { 112.097, 79.2214, 71.1367};
  int pae[3] = { 56, 41, 32};
  run_ojph_compress("tall_narrow.ppm",
                    "simple_enc_irv97_tall_narrow", "", "j2c",
                    "-qstep 0.1");
  run_ojph_compress_expand("simple_enc_irv97_tall_narrow", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_tall_narrow", "ppm",
              "tall_narrow.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the irv97 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_irv97_tall_narrow1.j2c -image_offset {1,0} -qstep 0.1
TEST(TestExecutables, SimpleEncIrv97TallNarrow1) {
  double mse[3] = { 100.906, 76.113, 72.8347};
  int pae[3] = { 39, 35, 34};
  run_ojph_compress("tall_narrow.ppm",
                    "simple_enc_irv97_tall_narrow1", "", "j2c",
                    "-image_offset \"{1,0}\" -qstep 0.1");
  run_ojph_compress_expand("simple_enc_irv97_tall_narrow1", "j2c", "ppm");
  run_mse_pae("simple_enc_irv97_tall_narrow1", "ppm",
              "tall_narrow.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_tall_narrow.j2c -reversible true
TEST(TestExecutables, SimpleEncRev53TallNarrow) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("tall_narrow.ppm",
                    "simple_enc_rev53_tall_narrow", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("simple_enc_rev53_tall_narrow", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_tall_narrow", "ppm",
              "tall_narrow.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o simple_enc_rev53_tall_narrow1.j2c -image_offset {1,0} -reversible true
TEST(TestExecutables, SimpleEncRev53TallNarrow1) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("tall_narrow.ppm",
                    "simple_enc_rev53_tall_narrow1", "", "j2c",
                    "-image_offset \"{1,0}\" -reversible true");
  run_ojph_compress_expand("simple_enc_rev53_tall_narrow1", "j2c", "ppm");
  run_mse_pae("simple_enc_rev53_tall_narrow1", "ppm",
              "tall_narrow.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o dpx_enc_1280x720_10bit_le_nuke11.j2c -reversible true
TEST(TestExecutables, DpxEnc1280x72010bitLeNuke11) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("dpx_1280x720_10bit.ppm",
                    "dpx_enc_1280x720_10bit_le_nuke11", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("dpx_enc_1280x720_10bit_le_nuke11", "j2c", "ppm");
  run_mse_pae("dpx_enc_1280x720_10bit_le_nuke11", "ppm",
              "dpx_1280x720_10bit.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o dpx_enc_1280x720_10bit_be_nuke11.j2c -reversible true
TEST(TestExecutables, DpxEnc1280x72010bitBeNuke11) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("dpx_1280x720_10bit.ppm",
                    "dpx_enc_1280x720_10bit_be_nuke11", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("dpx_enc_1280x720_10bit_be_nuke11", "j2c", "ppm");
  run_mse_pae("dpx_enc_1280x720_10bit_be_nuke11", "ppm",
              "dpx_1280x720_10bit.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o dpx_enc_1280x720_16bit_le_nuke11.j2c -reversible true
TEST(TestExecutables, DpxEnc1280x72016bitLeNuke11) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("dpx_1280x720_16bit.ppm",
                    "dpx_enc_1280x720_16bit_le_nuke11", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("dpx_enc_1280x720_16bit_le_nuke11", "j2c", "ppm");
  run_mse_pae("dpx_enc_1280x720_16bit_le_nuke11", "ppm",
              "dpx_1280x720_16bit.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o dpx_enc_1280x720_16bit_be_nuke11.j2c -reversible true
TEST(TestExecutables, DpxEnc1280x72016bitBeNuke11) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("dpx_1280x720_16bit.ppm",
                    "dpx_enc_1280x720_16bit_be_nuke11", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("dpx_enc_1280x720_16bit_be_nuke11", "j2c", "ppm");
  run_mse_pae("dpx_enc_1280x720_16bit_be_nuke11", "ppm",
              "dpx_1280x720_16bit.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o dpx_enc_1280x720_10bit_resolve18.j2c -reversible true
TEST(TestExecutables, DpxEnc1280x72010bitResolve18) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("dpx_1280x720_10bit.ppm",
                    "dpx_enc_1280x720_10bit_resolve18", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("dpx_enc_1280x720_10bit_resolve18", "j2c", "ppm");
  run_mse_pae("dpx_enc_1280x720_10bit_resolve18", "ppm",
              "dpx_1280x720_10bit.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_compress with codeblocks when the rev53 wavelet is used.
// We test by comparing MSE and PAE of decoded images. 
// The compressed file is obtained using these command-line options:
// -o dpx_enc_1280x720_16bit_resolve18.j2c -reversible true
TEST(TestExecutables, DpxEnc1280x72016bitResolve18) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  run_ojph_compress("dpx_1280x720_16bit.ppm",
                    "dpx_enc_1280x720_16bit_resolve18", "", "j2c",
                    "-reversible true");
  run_ojph_compress_expand("dpx_enc_1280x720_16bit_resolve18", "j2c", "ppm");
  run_mse_pae("dpx_enc_1280x720_16bit_resolve18", "ppm",
              "dpx_1280x720_16bit.ppm", "", 3, mse, pae);
}

////////////////////////////////////////////////////////////////////////////////
//                                   main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
