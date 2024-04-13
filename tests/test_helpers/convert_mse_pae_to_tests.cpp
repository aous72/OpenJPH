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
// File: convert_mse_pae_to_test.cpp
// Author: Aous Naman
// Date: 30 December 2022
//***************************************************************************/

#include <iostream>
#include <fstream>
#include <iomanip>

/******************************************************************************/
// This code is used to generate the tests in test_exceutables.cpp.
// It uses data from earlier tests, which have been retired, for this purpose.
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// Removes white space in the file buffer
void eat_white_spaces(std::ifstream& file) {
  int c = file.get();
  while(1)
  {
    if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
      c = file.get();
    else if (c == '#')
    {
      while (c != '\n') 
        c = file.get();
    }
    else
    {
      file.unget();
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Replaces double space with a single space
void remove_double_spaces(std::string& str)
{
  size_t pos = str.find("  ");
  while (pos != std::string::npos)
  {
    str.erase(pos, 1);
    pos = str.find("  ");
  }
}

////////////////////////////////////////////////////////////////////////////////
// Removes the back slash in \{ and \} sequences in strings
void remove_back_slashes(std::string& str)
{
  size_t pos;
  pos = str.find("\\{");
  while (pos != std::string::npos)
  {
    str.erase(pos, 1);
    pos = str.find("\\{");
  }
  pos = str.find("\\}");
  while (pos != std::string::npos)
  {
    str.erase(pos, 1);
    pos = str.find("\\}");
  }
}

////////////////////////////////////////////////////////////////////////////////
// Convert { and } to \"{ and }\"
void insert_quotes(std::string& str)
{
  size_t pos;
  pos = str.find("{");
  while (pos != std::string::npos)
  {
    str.insert(pos, "\\\"");
    pos = str.find("{", pos + 4);
  }
  pos = str.find("}");
  while (pos != std::string::npos)
  {
    str.insert(pos + 1, "\\\"");
    pos = str.find("}", pos + 5);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Removes underscores from name, and replaces the following letter with 
// the capital letter version of it
std::string prepare_test_name(std::string name)
{
  name[0] = toupper(name[0]);
  size_t pos = name.find("_");
  while (pos != std::string::npos)
  {
    name.erase(pos, 1);
    name[pos] = toupper(name[pos]);
    pos = name.find("_", pos);
  }
  return name;
}

////////////////////////////////////////////////////////////////////////////////
// Reads and processes ht_cmdlines.txt to extract the command line for
// base_filename, extracting the command line and yuv_specs if they exist.
// Extra_cmd_options are only useful for encoding (ojph_compress)
void process_cmdlines(std::ifstream& file, 
                      const std::string base_filename,
                      std::string& src_filename,
                      std::string& comment, std::string& yuv_specs,
                      std::string& extra_cmd_options) 
{
  file.seekg(std::ios_base::beg);
  while (file.good())
  {
    std::string line;
    std::getline(file, line);
    remove_double_spaces(line);

    size_t pos = line.find(base_filename);
    if (pos != std::string::npos)
    {
      size_t start_pos = line.find("-i");
      if (start_pos != std::string::npos) {
        start_pos = line.find("/", start_pos);
        if (start_pos == std::string::npos) {
          printf("Formatting error in cmdlines file, pos -1\n");
          exit(-1);
        }        
        size_t end_pos = line.find(" ", start_pos);
        if (start_pos == std::string::npos) {
          printf("Formatting error in cmdlines file, pos 0\n");
          exit(-1);
        }        
        src_filename = line.substr(start_pos + 1, end_pos - start_pos - 1);
      }

      start_pos = line.find("-o");
      if (start_pos != std::string::npos) {
        size_t end_pos = line.find("\"", start_pos);
        if (end_pos == std::string::npos) {
          printf("Formatting error in cmdlines file, pos 1\n");
          exit(-1);
        }
        // comment
        comment = line.substr(start_pos, end_pos - start_pos);
        remove_back_slashes(comment);

        // extra_cmd_options
        start_pos = 0;
        extra_cmd_options = comment;
        insert_quotes(extra_cmd_options);
        for (int i = 0; i < 2; ++i) // skip two spaces ("-o filename ")
          if (start_pos < extra_cmd_options.length())
            start_pos = extra_cmd_options.find(" ", start_pos) + 1;
          else
           {
              printf("Formatting error in cmdlines file, pos 2\n");
              exit(-1);
           }
        if (start_pos < extra_cmd_options.length())
          extra_cmd_options.erase(0, start_pos);
        else
          {
            printf("Formatting error in cmdlines file, pos 3\n");
            exit(-1);
          }
      }

      start_pos = line.find(":");
      if (start_pos != std::string::npos) {
        if (std::isdigit(line.at(start_pos + 1)))
        {
          size_t end_pos = line.find("\"", start_pos);
          yuv_specs = line.substr(start_pos, end_pos - start_pos);
        }
      }
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Write the run_ojph_expand command line for test_executables.cpp
void write_expand_test(std::ofstream& file, 
                       const std::string& base_filename,
                       const std::string& src_ext,
                       const std::string& out_ext, 
                       const std::string& ref_filename, 
                       const std::string& yuv_specs,
                       std::string comment,
                       int num_components, double* mse, int* pae)
{
  
  std::string wavelet, cb_dims;
  size_t start_pos = base_filename.find("_irv97_");
  if (start_pos != std::string::npos)
    wavelet = "irv97";
  else
    wavelet = "rev53";

  file << "/////////////////////////////////////////////////////////"
    << "//////////////////////" << std::endl;
  file << "// Test ojph_expand with " << "codeblocks when the "
    << wavelet << " wavelet is used." << std::endl;
  if (out_ext.compare("yuv") == 0)
    file << "// and the color components are subsampled." << std::endl;
  file << "// Command-line options used to obtain this file is:" << std::endl;
  while (comment.length() > 0)
  {
    const int len = 75;
    size_t pos = comment.rfind(' ', len);
    if (comment.length() > len && pos != std::string::npos) {
      file << "// " << comment.substr(0, pos) << std::endl;
      comment.erase(0, pos + 1);
    }
    else {
      file << "// " << comment << std::endl;
      comment.clear();
    }
  }

  file << "TEST(TestExecutables, " << prepare_test_name(base_filename) << ") {" 
    << std::endl;
  file << "  double mse[" << num_components << "] = { ";
  for (int i = 0; i < num_components; ++i) {
    file << std::setprecision(6) << mse[i];
    if (i < num_components - 1) 
      file << ", ";
  }
  file << "};" << std::endl;
  file << "  int pae[" << num_components << "] = { ";
  for (int i = 0; i < num_components; ++i) {
    file << pae[i];
    if (i < num_components - 1) 
      file << ", ";
  }
  file << "};" << std::endl;
  file << "  run_ojph_expand(\"" << base_filename << "\", \"" 
    << src_ext << "\", \"" << out_ext << "\");" << std::endl;
  file << "  run_mse_pae(\"" << base_filename << "\", \"" 
    << out_ext << "\", \"" << ref_filename << "\"," << std::endl;
  file << "              \"" << yuv_specs << "\", " 
    << num_components << ", mse, pae);" << std::endl;
  file << "}" << std::endl << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// Write the run_ojph_expand command line for test_executables.cpp
void write_compress_test(std::ofstream& file, 
                         const std::string& src_filename, 
                         const std::string& ref_filename, 
                         const std::string& base_filename,
                         const std::string& out_ext, 
                         const std::string& decode_ext,
                         const std::string& yuv_specs,
                         std::string comment,
                         std::string extra_cmd_options,
                         int num_components, double* mse, int* pae)
{
  std::string wavelet, cb_dims;
  size_t start_pos = base_filename.find("_irv97_");
  if (start_pos != std::string::npos)
    wavelet = "irv97";
  else
    wavelet = "rev53";

  // comment
  file << "/////////////////////////////////////////////////////////"
    << "//////////////////////" << std::endl;
  file << "// Test ojph_compress with " << "codeblocks when the "
    << wavelet << " wavelet is used";
  if (out_ext.compare("yuv") == 0) {
    file << "," << std::endl << "// and the color components are subsampled.";
    file << std::endl;
  } 
  else 
    file << "." << std::endl;
  file << "// We test by comparing MSE and PAE of decoded images. ";
  file << std::endl;

  file << "// The compressed file is obtained using these command-line "
    "options:" << std::endl;
  while (comment.length() > 0)
  {
    const int len = 75;
    size_t pos = comment.rfind(' ', len);
    if (comment.length() > len && pos < comment.length()) {
      file << "// " << comment.substr(0, pos) << std::endl;
      comment.erase(0, pos + 1);
    }
    else {
      file << "// " << comment << std::endl;
      comment.clear();
    }
  }

  // test
  file << "TEST(TestExecutables, " << prepare_test_name(base_filename) << ") {" 
    << std::endl;
  file << "  double mse[" << num_components << "] = { ";
  for (int i = 0; i < num_components; ++i) {
    file << std::setprecision(6) << mse[i];
    if (i < num_components - 1) 
      file << ", ";
  }
  file << "};" << std::endl;
  file << "  int pae[" << num_components << "] = { ";
  for (int i = 0; i < num_components; ++i) {
    file << pae[i];
    if (i < num_components - 1) 
      file << ", ";
  }
  file << "};" << std::endl;

  // compress
  file << "  run_ojph_compress(\"" << ref_filename << "\"," << std::endl;
  file << "                   ";
  file << " \"" << base_filename << "\", \"\", \"" << "j2c" << "\",";
  file << std::endl;

  while (extra_cmd_options.length() > 0)
  {
    const int len = 54;
    size_t pos = extra_cmd_options.rfind(' ', len);
    if (extra_cmd_options.length() > len && pos != std::string::npos) {
      file << "                    \"";
      file << extra_cmd_options.substr(0, pos) << "\"";
      extra_cmd_options.erase(0, pos);
      if (extra_cmd_options.empty())
        file << ");" << std::endl;
      else
        file << std::endl;
    }
    else {
      file << "                    \"";
      file << extra_cmd_options << "\");" << std::endl;
      extra_cmd_options.clear();
    }
  }
  
  // expand
  file << "  run_ojph_compress_expand(\"" << base_filename << "\", \"" 
    << "j2c" << "\", \"" << decode_ext << "\");" << std::endl;

  // error
  file << "  run_mse_pae(\"" << base_filename << "\", \"" 
    << decode_ext << "\"," << std::endl;
  file << "              \"" << ref_filename << "\", \"" << yuv_specs << "\", " 
    << num_components << ", mse, pae);" << std::endl;

  // end function
  file << "}" << std::endl << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// Write the run_compare_files command line for test_executables.cpp
void write_file_compare(std::ofstream& file, 
                        const std::string& ref_filename, 
                        const std::string& base_filename,
                        const std::string& out_ext, 
                        std::string comment,
                        std::string extra_cmd_options)
{
  std::string wavelet, cb_dims;
  size_t start_pos = base_filename.find("_irv97_");
  if (start_pos != std::string::npos)
    wavelet = "irv97";
  else
    wavelet = "rev53";

  // comment
  file << "/////////////////////////////////////////////////////////"
    << "//////////////////////" << std::endl;
  file << "// Test ojph_compress with " << "codeblocks when the "
    << wavelet << " wavelet is used";
  if (out_ext.compare("yuv") == 0) {
    file << "," << std::endl << "// and the color components are subsampled.";
    file << std::endl;
  } 
  else 
    file << "." << std::endl;
  file << "// We test by comparing coded images, ignoring comments. ";
  file << std::endl;

  file << "// The compressed file is obtained using these command-line "
    "options:" << std::endl;

  while (comment.length() > 0)
  {
    const int len = 75;
    size_t pos = comment.rfind(' ', len);
    if (comment.length() > len && pos < comment.length()) {
      file << "// " << comment.substr(0, pos) << std::endl;
      comment.erase(0, pos + 1);
    }
    else {
      file << "// " << comment << std::endl;
      comment.clear();
    }
  }  

  // test
  file << "TEST(TestExecutables, " << prepare_test_name(base_filename);
  file << "Compare) {" << std::endl;

  // compress
  file << "  run_ojph_compress(\"" << ref_filename << "\"," << std::endl;
  file << "                   ";
  file << " \"" << base_filename << "\", \"_compare\", \"" << "j2c";
  file << "\"," << std::endl;

  while (extra_cmd_options.length() > 0)
  {
    const int len = 54;
    size_t pos = extra_cmd_options.rfind(' ', len);
    if (extra_cmd_options.length() > len && pos != std::string::npos) {
      file << "                    \"";
      file << extra_cmd_options.substr(0, pos) << "\"";
      extra_cmd_options.erase(0, pos);
      if (extra_cmd_options.empty())
        file << ");" << std::endl;
      else
        file << std::endl;
    }
    else {
      file << "                    \"";
      file << extra_cmd_options << "\");" << std::endl;
      extra_cmd_options.clear();
    }
  }

  // call compare files
  file << "  compare_files(\"" << base_filename << "\", \"_compare\", \""; 
  file << "j2c" << "\");" << std::endl;

  // end function
  file << "}" << std::endl << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// main
int main(int argc, char *argv[])
{
  const char mse_pae_filename[] = 
    "../../build/tests/jp2k_test_codestreams/openjph/mse_pae.txt";
  const char cmdlines_filename[] = "ht_cmdlines.txt";
  const char out_filename[] = "openjph_tests.cpp";

  std::ifstream mse_pae_file;
  mse_pae_file.open(mse_pae_filename, std::ios_base::in);
  if (mse_pae_file.fail()) {
    std::cerr << "Failed to open " << mse_pae_filename << "." << std::endl;
    return -1;
  }

  std::ifstream cmdlines_file;
  cmdlines_file.open(cmdlines_filename, std::ios_base::in);
  if (cmdlines_file.fail()) {
    std::cerr << "Failed to open " << cmdlines_filename << "." << std::endl;
    return -1;
  }

  std::ofstream out_file;
  out_file.open(out_filename, std::ios_base::out);
  if (out_file.fail()) {
    std::cerr << "Failed to open " << out_filename << "." << std::endl;
    return -1;
  }

  while (mse_pae_file.good()) 
  {
    // read files line and process it
    std::string ht_filename, src_filename, ref_filename;
    mse_pae_file >> ht_filename;
    std::string base_filename = ht_filename.substr(0, ht_filename.rfind("."));
    std::string src_ext = ht_filename.substr(ht_filename.rfind(".") + 1);
    mse_pae_file >> ref_filename;
    std::string out_ext = ref_filename.substr(ref_filename.rfind(".") + 1);
    ref_filename = ref_filename.substr(ref_filename.rfind("/") + 1);

    // Uncomment to print values
    std::cout << "base_filename = " << base_filename << std::endl;
    std::cout << "src_ext = " << src_ext << std::endl;
    std::cout << "out_ext = " << out_ext << std::endl;
    std::cout << "ref_filename = " << ref_filename << std::endl;

    constexpr int max_components = 10;
    int num_components = 0;
    double mse[max_components];
    int pae[max_components];
    eat_white_spaces(mse_pae_file);
    int c = mse_pae_file.peek(); // check if next we have a number
    while (mse_pae_file.good() && std::isdigit(c))
    {
      if (num_components >= max_components)
        std::cerr << "More than " << max_components << " were found in "  
          << mse_pae_filename << "; this is not supported." <<  std::endl;
      mse_pae_file >> mse[num_components];
      mse_pae_file >> pae[num_components];
      std::cout << "mse = " << mse[num_components] << std::endl;
      std::cout << "pae = " << pae[num_components] << std::endl;
      ++num_components;
      eat_white_spaces(mse_pae_file);
      c = mse_pae_file.peek();
    }

    // write tests for write_expand_test
    if (base_filename.find("_dec_") != std::string::npos) 
    {
      std::string yuv_specs, comment, extra_cmd_options;
      process_cmdlines(cmdlines_file, base_filename, src_filename, comment, 
                       yuv_specs, extra_cmd_options);
      write_expand_test(out_file, base_filename, src_ext, out_ext, 
        ref_filename, yuv_specs, comment, num_components, mse, pae);
    }

    // write tests for test_ojph_compress and test_compare_files
    // to be written
    if (base_filename.find("_enc_") != std::string::npos) 
    {
      std::string yuv_specs, comment, extra_cmd_options;
      process_cmdlines(cmdlines_file, base_filename, src_filename, comment, 
                       yuv_specs, extra_cmd_options);
      std::cout << "src_filename = " << src_filename << std::endl;
      write_compress_test(out_file, src_filename, ref_filename, base_filename, 
        src_ext, out_ext, yuv_specs, comment, extra_cmd_options, num_components,
        mse, pae);
      // write_file_compare(out_file, ref_filename, base_filename, out_ext,
      //   comment, extra_cmd_options);
    }

  }
  
  return 0;
}