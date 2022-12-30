
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
#define TOL_DOUBLE 0.1
#define TOL_INTEGER 1

////////////////////////////////////////////////////////////////////////////////
//                            test_ojph_expand
////////////////////////////////////////////////////////////////////////////////
void test_ojph_expand(std::string src_filename, std::string src_ext,
                      std::string out_filename, std::string out_ext,
                      std::string ref_filename,
                      int num_components, double* mse, int* pae) 
{
  try {
    std::string result, command;
    command = std::string(EXPAND_EXECUTABLE) 
      + " -i " + SRC_FILE_DIR + src_filename + "." + src_ext
      + " -o " + OUT_FILE_DIR + out_filename + "." + out_ext;
    EXPECT_EQ(execute(command, result), 0);
    command = std::string(MSE_PAE_PATH) 
      + " " + OUT_FILE_DIR + out_filename + "." + out_ext
      + " " + REF_FILE_DIR + ref_filename;
    EXPECT_EQ(execute(command, result), 0);

    size_t pos = 0;
    for (int c = 0; c < num_components; ++c) {
      if (pos < result.length()) {
        double valf = atof(result.c_str() + pos);
        EXPECT_NEAR(valf, mse[c], TOL_DOUBLE);
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
// Test ojph_expand with 1024x4 codeblocks when the irv97 wavelet is used
TEST(TestExecutables, simple_dec_irv97_1024x4) {
  double mse[3] = { 19.967075, 18.398159, 24.487680 };
  int pae[3] = { 53, 52, 50 };
  test_ojph_expand("simple_dec_irv97_1024x4", "jph",
                   "simple_dec_irv97_1024x4", "ppm",
                   "Malamute.ppm", 3, mse, pae);
}

////////////////////////////////////////////////////////////////////////////////
//                                   main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
