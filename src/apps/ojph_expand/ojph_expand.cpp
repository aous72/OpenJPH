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
// File: ojph_expand.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#include <ctime>
#include <iostream>
#include <cstdlib>

#include "ojph_arg.h"
#include "ojph_mem.h"
#include "ojph_img_io.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "ojph_message.h"

/////////////////////////////////////////////////////////////////////////////
struct ui32_list_interpreter : public ojph::cli_interpreter::arg_inter_base
{
  ui32_list_interpreter(const int max_num_elements, int& num_elements,
                        ojph::ui32* list)
  : max_num_eles(max_num_elements), si32list(list), num_eles(num_elements)
  {}

  virtual void operate(const char *str)
  {
    const char *next_char = str;
    num_eles = 0;
    do
    {
      if (num_eles)
      {
        if (*next_char != ',') //separate sizes by a comma
          throw "sizes in a sizes list must be separated by a comma";
        next_char++;
      }
      char *endptr;
      si32list[num_eles] = (ojph::ui32)strtoul(next_char, &endptr, 10);
      if (endptr == next_char)
        throw "size number is improperly formatted";
      next_char = endptr;
      ++num_eles;
    }
    while (*next_char == ',' && num_eles < max_num_eles);
    if (num_eles + 1 < max_num_eles)
    {
      if (*next_char)
        throw "list elements must separated by a "",""";
    }
    else if (*next_char)
        throw "there are too many elements in the size list";
  }

  const int max_num_eles;
  ojph::ui32* si32list;
  int& num_eles;
};

//////////////////////////////////////////////////////////////////////////////
bool get_arguments(int argc, char *argv[],
                   char *&input_filename, char *&output_filename,
                   ojph::ui32& skipped_res_for_read, 
                   ojph::ui32& skipped_res_for_recon,
                   bool& resilient)
{
  ojph::cli_interpreter interpreter;
  interpreter.init(argc, argv);

  ojph::ui32 skipped_res[2] = {0, 0};
  int num_skipped_res = 0;
  ui32_list_interpreter ilist(2, num_skipped_res, skipped_res);

  interpreter.reinterpret("-i", input_filename);
  interpreter.reinterpret("-o", output_filename);
  interpreter.reinterpret("-skip_res", &ilist);
  interpreter.reinterpret("-resilient", resilient);

  //interpret skipped_string
  if (num_skipped_res > 0)
  {
    skipped_res_for_read = skipped_res[0];
    if (num_skipped_res > 1)
      skipped_res_for_recon = skipped_res[1];
    else
      skipped_res_for_recon = skipped_res_for_read;
  }

  if (interpreter.is_exhausted() == false) {
    printf("The following arguments were not interpreted:\n");
    ojph::argument t = interpreter.get_argument_zero();
    t = interpreter.get_next_avail_argument(t);
    while (t.is_valid()) {
      printf("%s\n", t.arg);
      t = interpreter.get_next_avail_argument(t);
    }
    return false;
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////////
const char* get_file_extension(const char* filename)
{
  size_t len = strlen(filename);
  const char* p = strrchr(filename, '.');
  if (p == NULL || p == filename + len - 1)
    OJPH_ERROR(0x01000071,
      "no file extension is found, or there are no characters "
      "after the dot \'.\' for filename \"%s\" \n", filename);
  return p;
}

/////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {

  char *input_filename = NULL;
  char *output_filename = NULL;
  ojph::ui32 skipped_res_for_read = 0;
  ojph::ui32 skipped_res_for_recon = 0;
  bool resilient = false;

  if (argc <= 1) {
    std::cout <<
    "\nThe following arguments are necessary:\n"
    " -i input file name\n"
#ifdef OJPH_ENABLE_TIFF_SUPPORT
    " -o output file name (either pgm, ppm, tif(f), or raw(yuv))\n\n"
#else
    " -o output file name (either pgm, ppm, or raw(yuv))\n\n"
#endif // !OJPH_ENABLE_TIFF_SUPPORT
    "The following arguments are options:\n"
    " -skip_res  x,y a comma-separated list of two elements containing the\n"
    "            number of resolutions to skip. You can specify 1 or 2\n"
    "            parameters; the first specifies the number of resolution\n"
    "            for which data reading is skipped. The second is the\n"
    "            number of skipped resolution for reconstruction, which is\n"
    "            either equal to the first or smaller. If the second is not\n"
    "            specified, it is made to equal to the first.\n"
    " -resilient true if you want the decoder to be more tolerant of errors\n"
    "            in the codestream\n\n"
    ;
    return -1;
  }
  if (!get_arguments(argc, argv, input_filename, output_filename,
                     skipped_res_for_read, skipped_res_for_recon,
                     resilient))
  {
    return -1;
  }

  clock_t begin = clock();

  try {
    if (output_filename == NULL)
      OJPH_ERROR(0x020000008,
                 "Please provide and output file using the -o option\n");

    ojph::j2c_infile j2c_file;
    j2c_file.open(input_filename);
    ojph::codestream codestream;

    ojph::ppm_out ppm;
    #ifdef OJPH_ENABLE_TIFF_SUPPORT
    ojph::tif_out tif;
    #endif /* OJPH_ENABLE_TIFF_SUPPORT */
    ojph::yuv_out yuv;
    ojph::image_out_base *base = NULL;
    const char *v = get_file_extension(output_filename);
    if (v)
    {
      if (resilient)
        codestream.enable_resilience();
      codestream.read_headers(&j2c_file);
      codestream.restrict_input_resolution(skipped_res_for_read, 
        skipped_res_for_recon);
      ojph::param_siz siz = codestream.access_siz();

      if (strncmp(".pgm", v, 4) == 0)
      {

        if (siz.get_num_components() != 1)
          OJPH_ERROR(0x020000001,
            "The file has more than one color component, but .pgm can "
            "contain only on color component\n");
        ppm.configure(siz.get_recon_width(0), siz.get_recon_height(0),
                      siz.get_num_components(), siz.get_bit_depth(0));
        ppm.open(output_filename);
        base = &ppm;
      }
      else if (strncmp(".ppm", v, 4) == 0)
      {
        codestream.set_planar(false);
        ojph::param_siz siz = codestream.access_siz();

        if (siz.get_num_components() != 3)
          OJPH_ERROR(0x020000002,
            "The file has %d color components; this cannot be saved to"
            " a .ppm file\n", siz.get_num_components());
        bool all_same = true;
        ojph::point p = siz.get_downsampling(0);
        for (ojph::ui32 i = 1; i < siz.get_num_components(); ++i)
        {
          ojph::point p1 = siz.get_downsampling(i);
          all_same = all_same && (p1.x == p.x) && (p1.y == p.y);
        }
        if (!all_same)
          OJPH_ERROR(0x020000003,
            "To save an image to ppm, all the components must have the "
            "downsampling ratio\n");
        ppm.configure(siz.get_recon_width(0), siz.get_recon_height(0),
                      siz.get_num_components(), siz.get_bit_depth(0));
        ppm.open(output_filename);
        base = &ppm;
      }
#ifdef OJPH_ENABLE_TIFF_SUPPORT
      else if (strncmp(".tif", v, 4) == 0 || strncmp(".tiff", v, 5) == 0)
      {
        codestream.set_planar(false);
        ojph::param_siz siz = codestream.access_siz();

        bool all_same = true;
        ojph::point p = siz.get_downsampling(0);
        for (unsigned int i = 1; i < siz.get_num_components(); ++i)
        {
          ojph::point p1 = siz.get_downsampling(i);
          all_same = all_same && (p1.x == p.x) && (p1.y == p.y);
        }
        if (!all_same)
          OJPH_ERROR(0x020000008,
            "To save an image to tif(f), all the components must have the "
            "downsampling ratio\n");
        ojph::ui32 bit_depths[4] = { 0, 0, 0, 0 };
        for (ojph::ui32 c = 0; c < siz.get_num_components(); c++)
        {
          bit_depths[c] = siz.get_bit_depth(c);
        }
        tif.configure(siz.get_recon_width(0), siz.get_recon_height(0),
          siz.get_num_components(), bit_depths);
        tif.open(output_filename);
        base = &tif;
      }
#endif // !OJPH_ENABLE_TIFF_SUPPORT
      else if (strncmp(".yuv", v, 4) == 0 || strncmp(".raw", v, 4) == 0)
      {
        codestream.set_planar(true);
        ojph::param_siz siz = codestream.access_siz();

        if (siz.get_num_components() != 3 && siz.get_num_components() != 1)
          OJPH_ERROR(0x020000004,
            "The file has %d color components; this cannot be saved to"
             " .raw(yuv) file\n", siz.get_num_components());
        ojph::param_cod cod = codestream.access_cod();
        if (cod.is_using_color_transform())
          OJPH_ERROR(0x020000005,
            "The current implementation of raw(yuv) file object does not"
            " support saving file when conversion from raw(yuv) to rgb is"
            " needed; in any case, this is not the normal usage of raw(yuv)"
            "file.");
        ojph::ui32 comp_widths[3];
        ojph::ui32 max_bit_depth = 0;
        for (ojph::ui32 i = 0; i < siz.get_num_components(); ++i)
        {
          comp_widths[i] = siz.get_recon_width(i);
          max_bit_depth = ojph_max(max_bit_depth, siz.get_bit_depth(i));
        }
        codestream.set_planar(true);
        yuv.configure(max_bit_depth, siz.get_num_components(), comp_widths);
        yuv.open(output_filename);
        base = &yuv;
      }
      else
#ifdef OJPH_ENABLE_TIFF_SUPPORT
        OJPH_ERROR(0x020000006,
          "unknown output file extension; only pgm, ppm, tif(f) and raw(yuv))"
          " are supported\n");
#else
        OJPH_ERROR(0x020000006,
          "unknown output file extension; only pgm, ppm, and raw(yuv) are"
          " supported\n");
#endif // !OJPH_ENABLE_TIFF_SUPPORT
    }
    else
      OJPH_ERROR(0x020000007,
        "Please supply a proper output filename with a proper extension\n");

    codestream.create();

    if (codestream.is_planar())
    {
      ojph::param_siz siz = codestream.access_siz();
      for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
      {
        ojph::ui32 height = siz.get_recon_height(c);
        for (ojph::ui32 i = height; i > 0; --i)
        {
          ojph::ui32 comp_num;
          ojph::line_buf *line = codestream.pull(comp_num);
          assert(comp_num == c);
          base->write(line, comp_num);
        }
      }
    }
    else
    {
      ojph::param_siz siz = codestream.access_siz();
      ojph::ui32 height = siz.get_recon_height(0);
      for (ojph::ui32 i = 0; i < height; ++i)
      {
        for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
        {
          ojph::ui32 comp_num;
          ojph::line_buf *line = codestream.pull(comp_num);
          assert(comp_num == c);
          base->write(line, comp_num);
        }
      }
    }

    base->close();
    codestream.close();
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
    exit(-1);
  }

  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  printf("Elapsed time = %f\n", elapsed_secs);

  return 0;
}
