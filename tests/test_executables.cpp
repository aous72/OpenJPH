
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

#define SRC_FILE_DIR "../_deps/jp2k_test_codestreams-src/openjph/"
#define OUT_FILE_DIR "./"
#define REF_FILE_DIR "../_deps/jp2k_test_codestreams-src/openjph/references/"
#define MSE_PAE_PATH  "../../bin/mse_pae"
#define EXPAND_EXECUTABLE "ojph_expand"
#define COMPRESS_EXECUTABLE "ojph_compress"
#define TOL_DOUBLE 0.01
#define TOL_INTEGER 1

////////////////////////////////////////////////////////////////////////////////
//                            test_ojph_expand
////////////////////////////////////////////////////////////////////////////////
void test_ojph_expand(std::string src_filename,
                      std::string out_filename,
                      std::string ref_filename,
                      std::string yuv_specs,
                      int num_components, double* mse, int* pae) 
{
  try {
    std::string result, command;
    command = std::string(EXPAND_EXECUTABLE) 
      + " -i " + SRC_FILE_DIR + src_filename
      + " -o " + OUT_FILE_DIR + out_filename;
    EXPECT_EQ(execute(command, result), 0);
    std::cerr << command << std::endl << result << std::endl;
    command = std::string(MSE_PAE_PATH) 
      + " " + OUT_FILE_DIR + out_filename + yuv_specs
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
    EXPECT_EQ(execute("ojph_compress", result), 1);
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
    EXPECT_EQ(execute("ojph_expand", result), 1);
  }
  catch(const std::runtime_error& error) {
    FAIL() << error.what();
  }
}

////////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with 64x64 codeblocks when the irv97 wavelet is used
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_64x64.jph -precise -quiet -rate 0.5 -full
TEST(TestExecutables, simple_dec_irv97_64x64) {
  double mse[3] = { 39.239422, 36.324543, 47.574749 };
  int pae[3] = { 74, 77, 73 };
  test_ojph_expand("simple_dec_irv97_64x64.jph",
                   "simple_dec_irv97_64x64.ppm",
                   "Malamute.ppm", "", 3, mse, pae);
}

////////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with 16x16 codeblocks when the irv97 wavelet is used
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_16x16.jph -precise -quiet -rate 1 Cblk={16,16} -full
TEST(TestExecutables, simple_dec_irv97_16x16) {
  double mse[3] = { 20.258595, 18.633598, 24.716270 };
  int pae[3] = { 53, 51, 47 };
  test_ojph_expand("simple_dec_irv97_16x16.jph",
                   "simple_dec_irv97_16x16.ppm",
                   "Malamute.ppm", "", 3, mse, pae);
}

////////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with 4x4 codeblocks when the irv97 wavelet is used
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_4x4.jph -precise -quiet -rate 1 Cblk={4,4} -full
TEST(TestExecutables, simple_dec_irv97_4x4) {
  double mse[3] = { 41.224403, 38.277267, 50.179729 };
  int pae[3] = { 75, 77, 80 };
  test_ojph_expand("simple_dec_irv97_4x4.jph",
                   "simple_dec_irv97_4x4.ppm",
                   "Malamute.ppm", "", 3, mse, pae);
}

////////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with 1024x4 codeblocks when the irv97 wavelet is used
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_1024x4.jph -precise -quiet -rate 1 Cblk={1024,4} -full
TEST(TestExecutables, simple_dec_irv97_1024x4) {
  double mse[3] = { 19.827452, 18.251141, 24.283169 };
  int pae[3] = { 53, 52, 50 };
  test_ojph_expand("simple_dec_irv97_1024x4.jph",
                   "simple_dec_irv97_1024x4.ppm",
                   "Malamute.ppm", "", 3, mse, pae);
}

////////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with 4x1024 codeblocks when the irv97 wavelet is used
// Command-line options used to obtain this file is:
// -o simple_dec_irv97_4x1024.jph -precise -quiet -rate 1 Cblk={4,1024} -full
TEST(TestExecutables, simple_dec_irv97_4x1024) {
  double mse[3] = { 19.988697, 18.432861, 24.209379 };
  int pae[3] = { 51, 48, 51 };
  test_ojph_expand("simple_dec_irv97_4x1024.jph",
                   "simple_dec_irv97_4x1024.ppm",
                   "Malamute.ppm", "", 3, mse, pae);
}

 
 
 



////////////////////////////////////////////////////////////////////////////////
// Test ojph_expand with 64x64 codeblocks when the irv97 wavelet is used
// when color components are subsampled.
// Command-line options used to obtain this file is:
// -i foreman_420y.rawl,foreman_420u.rawl,foreman_420v.rawl 
// -o simple_dec_irv97_64x64_yuv.jph -precise -quiet -rate 0.5 
// Sdims={288,352},{144,176},{144,176} Ssampling={1,1},{2,2},{2,2} 
// Nprecision={8} Nsigned={no} -full
TEST(TestExecutables, simple_dec_irv97_64x64_yuv) {
  double mse[3] = { 20.277807, 6.279119, 4.159367 };
  int pae[3] = { 52, 22, 31 };
  test_ojph_expand("simple_dec_irv97_64x64_yuv.jph",
                   "simple_dec_irv97_64x64_yuv.yuv",
                   "foreman_420.yuv", ":352x288x8x420", 3, mse, pae);
}


////////////////////////////////////////////////////////////////////////////////
//                                   main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
