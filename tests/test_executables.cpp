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
FILE *ojph_popen(const char *command, const char *modes) 
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
int ojph_pclose(FILE *stream) 
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

#define SRC_FILE_DIR "./jp2k_test_codestreams/openjph/"
#define OUT_FILE_DIR "./"
#define REF_FILE_DIR "./jp2k_test_codestreams/openjph/references/"
#define MSE_PAE_PATH  "./mse_pae"
#define EXPAND_EXECUTABLE "ojph_expand"
#define COMPRESS_EXECUTABLE "ojph_compress"
#define TOL_DOUBLE 0.01
#define TOL_INTEGER 1

////////////////////////////////////////////////////////////////////////////////
//                            test_ojph_expand
////////////////////////////////////////////////////////////////////////////////
void test_ojph_expand(std::string base_filename, std::string src_ext,
                      std::string out_ext, std::string ref_filename,
                      std::string yuv_specs,
                      int num_components, double* mse, int* pae) 
{
  try {
    std::string result, command;
    command = std::string(EXPAND_EXECUTABLE) 
      + " -i " + SRC_FILE_DIR + base_filename + "." + src_ext +
      + " -o " + OUT_FILE_DIR + base_filename + "." + out_ext;
    EXPECT_EQ(execute(command, result), 0);
    std::cerr << command << std::endl << result << std::endl;
    command = std::string(MSE_PAE_PATH) 
      + " " + OUT_FILE_DIR + base_filename + "." + out_ext + yuv_specs
      + " " + REF_FILE_DIR + ref_filename + yuv_specs;
    EXPECT_EQ(execute(command, result), 0);
    std::cerr << command << std::endl << result << std::endl;

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
  catch(const std::runtime_error& error) {
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
  catch(const std::runtime_error& error) {
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
  catch(const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_64x64) {
  double mse[3] = { 39.2812, 36.3819, 47.642};
  int pae[3] = { 74, 77, 73};
  test_ojph_expand("simple_dec_irv97_64x64", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_32x32) {
  double mse[3] = { 18.6979, 17.1208, 22.7539};
  int pae[3] = { 51, 48, 46};
  test_ojph_expand("simple_dec_irv97_32x32", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_16x16) {
  double mse[3] = { 20.1706, 18.5427, 24.6146};
  int pae[3] = { 53, 51, 47};
  test_ojph_expand("simple_dec_irv97_16x16", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_4x4) {
  double mse[3] = { 40.8623, 37.9308, 49.7276};
  int pae[3] = { 75, 77, 80};
  test_ojph_expand("simple_dec_irv97_4x4", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_1024x4) {
  double mse[3] = { 19.8275, 18.2511, 24.2832};
  int pae[3] = { 53, 52, 50};
  test_ojph_expand("simple_dec_irv97_1024x4", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_4x1024) {
  double mse[3] = { 19.9635, 18.4063, 24.1719};
  int pae[3] = { 51, 48, 51};
  test_ojph_expand("simple_dec_irv97_4x1024", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_512x8) {
  double mse[3] = { 18.7929, 17.2026, 22.9922};
  int pae[3] = { 53, 52, 50};
  test_ojph_expand("simple_dec_irv97_512x8", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_8x512) {
  double mse[3] = { 19.3661, 17.8067, 23.4574};
  int pae[3] = { 51, 48, 52};
  test_ojph_expand("simple_dec_irv97_8x512", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_256x16) {
  double mse[3] = { 18.6355, 17.0963, 22.6076};
  int pae[3] = { 54, 51, 48};
  test_ojph_expand("simple_dec_irv97_256x16", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_16x256) {
  double mse[3] = { 18.5933, 17.0208, 22.5709};
  int pae[3] = { 51, 48, 47};
  test_ojph_expand("simple_dec_irv97_16x256", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_128x32) {
  double mse[3] = { 18.4443, 16.9133, 22.4193};
  int pae[3] = { 52, 50, 46};
  test_ojph_expand("simple_dec_irv97_128x32", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_32x128) {
  double mse[3] = { 18.4874, 16.9379, 22.4855};
  int pae[3] = { 51, 48, 45};
  test_ojph_expand("simple_dec_irv97_32x128", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_rev53_64x64) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_64x64", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_32x32.jph -precise -quiet Creversible=yes Cblk={32,32}
TEST(TestExecutables, simple_dec_rev53_32x32) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_32x32", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_4x4.jph -precise -quiet Creversible=yes Cblk={4,4}
TEST(TestExecutables, simple_dec_rev53_4x4) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_4x4", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_1024x4.jph -precise -quiet Creversible=yes
TEST(TestExecutables, simple_dec_rev53_1024x4) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_1024x4", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_4x1024.jph -precise -quiet Creversible=yes
TEST(TestExecutables, simple_dec_rev53_4x1024) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_4x1024", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_yuv.jph -precise -quiet -rate 0.5
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
TEST(TestExecutables, simple_dec_irv97_64x64_yuv) {
  double mse[3] = { 20.2778, 6.27912, 4.15937};
  int pae[3] = { 52, 22, 31};
  test_ojph_expand("simple_dec_irv97_64x64_yuv", "jph", "yuv",
                    "foreman_420.yuv", ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_yuv.jph -precise -quiet Creversible=yes
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
TEST(TestExecutables, simple_dec_rev53_64x64_yuv) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_64x64_yuv", "jph", "yuv",
                    "foreman_420.yuv", ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_yuv.jph -precise -quiet -rate 0.5
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_yuv) {
  double mse[3] = { 34.4972, 10.1112, 7.96331};
  int pae[3] = { 67, 30, 39};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_yuv", "jph", "yuv",
                    "foreman_420.yuv", ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// and the color components are subsampled.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_tiles_yuv.jph -precise -quiet Creversible=yes
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2}
TEST(TestExecutables, simple_dec_rev53_64x64_tiles_yuv) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_64x64_tiles_yuv", "jph", "yuv",
                    "foreman_420.yuv", ":352x288x8x420", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_LRCP.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=LRCP Cprecincts={2,256} Sorigin={374,1717}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_LRCP) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_LRCP", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RLCP.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RLCP Cprecincts={2,256} Sorigin={374,1717}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_RLCP) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_RLCP", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RPCL.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RPCL Cprecincts={2,256} Sorigin={374,1717}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_RPCL) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_RPCL", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_PCRL.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=PCRL Cprecincts={2,256} Sorigin={374,1717}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_PCRL) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_PCRL", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_CPRL.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=CPRL Cprecincts={2,256} Sorigin={374,1717}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_CPRL) {
  double mse[3] = { 71.8149, 68.7115, 89.4001};
  int pae[3] = { 78, 78, 83};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_CPRL", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_LRCP33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=LRCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_LRCP33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_LRCP33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RLCP33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RLCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_RLCP33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_RLCP33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RPCL33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RPCL Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_RPCL33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_RPCL33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_PCRL33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=PCRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_PCRL33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_PCRL33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_CPRL33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=CPRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,257}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_CPRL33) {
  double mse[3] = { 56.2139, 51.4121, 69.0107};
  int pae[3] = { 80, 81, 98};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_CPRL33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_LRCP33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=LRCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_LRCP33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_LRCP33x33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RLCP33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RLCP Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_RLCP33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_RLCP33x33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_RPCL33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=RPCL Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_RPCL33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_RPCL33x33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_PCRL33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=PCRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_PCRL33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_PCRL33x33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_tiles_CPRL33x33.jph -precise -quiet -rate 0.5
// Clevels=5 Corder=CPRL Sorigin={5,33} Stile_origin={5,10} Stiles={33,33}
TEST(TestExecutables, simple_dec_irv97_64x64_tiles_CPRL33x33) {
  double mse[3] = { 210.283, 210.214, 257.276};
  int pae[3] = { 165, 161, 166};
  test_ojph_expand("simple_dec_irv97_64x64_tiles_CPRL33x33", "jph", "ppm",
                    "Malamute.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_gray_tiles.jph -precise -quiet Creversible=yes
TEST(TestExecutables, simple_dec_rev53_64x64_gray_tiles) {
  double mse[1] = { 0};
  int pae[1] = { 0};
  test_ojph_expand("simple_dec_rev53_64x64_gray_tiles", "jph", "pgm",
                    "monarch.pgm", "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64_gray_tiles.jph -precise -quiet -rate 0.5
TEST(TestExecutables, simple_dec_irv97_64x64_gray_tiles) {
  double mse[1] = { 18.9601};
  int pae[1] = { 56};
  test_ojph_expand("simple_dec_irv97_64x64_gray_tiles", "jph", "pgm",
                    "monarch.pgm", "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_64x64_16bit) {
  double mse[3] = { 60507.2, 36672.5, 64809.8};
  int pae[3] = { 2547, 1974, 1922};
  test_ojph_expand("simple_dec_irv97_64x64_16bit", "jph", "ppm",
                    "mm.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the irv97 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_irv97_64x64_16bit_gray) {
  double mse[1] = { 19382.9};
  int pae[1] = { 1618};
  test_ojph_expand("simple_dec_irv97_64x64_16bit_gray", "jph", "pgm",
                    "mm.pgm", "", 1, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
TEST(TestExecutables, simple_dec_rev53_64x64_16bit) {
  double mse[3] = { 0, 0, 0};
  int pae[3] = { 0, 0, 0};
  test_ojph_expand("simple_dec_rev53_64x64_16bit", "jph", "ppm",
                    "mm.ppm", "", 3, mse, pae);
}

///////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with codeblocks when the rev53 wavelet is used.
// Command-line options used to obtain this file is:
// -o simple_dec_rev53_64x64_16bit_gray.jph -precise -quiet Creversible=yes
TEST(TestExecutables, simple_dec_rev53_64x64_16bit_gray) {
  double mse[1] = { 0};
  int pae[1] = { 0};
  test_ojph_expand("simple_dec_rev53_64x64_16bit_gray", "jph", "pgm",
                    "mm.pgm", "", 1, mse, pae);
}

////////////////////////////////////////////////////////////////////////////////
//                                   main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
