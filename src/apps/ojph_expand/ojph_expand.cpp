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
#include <chrono>

//////////////////////////////////////////////////////////////////////////////
bool get_arguments(int argc, char *argv[],
                   char *&input_filename, char *&output_filename)
{
  ojph::cli_interpreter interpreter;
  interpreter.init(argc, argv);

  interpreter.reinterpret("-i", input_filename);
  interpreter.reinterpret("-o", output_filename);

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
const char *get_file_extension(const char *filename)
{
  size_t len = strlen(filename);
  return filename + ojph_max(0, len - 4);
}

/////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {

  char *input_filename = NULL;
  char *output_filename = NULL;

  if (argc <= 1) {
    std::cout <<
    "\nThe following arguments are necessary:\n"
    " -i input file name\n"
    " -o output file name (either pgm, ppm, or yuv)\n\n"
    ;
    return -1;
  }
  if (!get_arguments(argc, argv, input_filename, output_filename))
  {
    return -1;
  }

  auto start = std::chrono::high_resolution_clock::now();

  const uint32_t numReps = 200;
  for (int p=0; p < numReps; ++p){
  try {
    ojph::j2c_infile j2c_file;
    j2c_file.open(input_filename);
    ojph::codestream codestream;

    ojph::ppm_out ppm;
    ojph::yuv_out yuv;
    ojph::image_out_base *base = NULL;
    const char *v = get_file_extension(output_filename);
    if (v)
    {
      codestream.read_headers(&j2c_file);
      ojph::param_siz_t siz = codestream.access_siz();

      if (strncmp(".pgm", v, 4) == 0)
      {

        if (siz.get_num_components() != 1)
          OJPH_ERROR(0x020000001,
            "The file has more than one color component, but .pgm can "
            "contain only on color component\n");
        ppm.configure(siz.get_image_extent().x - siz.get_image_offset().x,
                      siz.get_image_extent().y - siz.get_image_offset().y,
                      siz.get_num_components(), siz.get_bit_depth(0));
        ppm.open(output_filename);
        base = &ppm;
      }
      else if (strncmp(".ppm", v, 4) == 0)
      {
        codestream.set_planar(false);
        ojph::param_siz_t siz = codestream.access_siz();

        if (siz.get_num_components() != 3)
          OJPH_ERROR(0x020000002,
            "The file has %d color components; this cannot be saved to"
            " a .ppm file\n", siz.get_num_components());
        bool all_same = true;
        ojph::point p = siz.get_downsampling(0);
        for (int i = 1; i < siz.get_num_components(); ++i)
        {
          ojph::point p1 = siz.get_downsampling(i);
          all_same = all_same && (p1.x == p.x) && (p1.y == p.y);
        }
        if (!all_same)
          OJPH_ERROR(0x020000003,
            "To save an image to ppm, all the components must have the "
            "downsampling ratio\n");
        ppm.configure(siz.get_image_extent().x - siz.get_image_offset().x,
                      siz.get_image_extent().y - siz.get_image_offset().y,
                      siz.get_num_components(), siz.get_bit_depth(0));
        ppm.open(output_filename);
        base = &ppm;
      }
      else if (strncmp(".yuv", v, 4) == 0)
      {
        codestream.set_planar(true);
        ojph::param_siz_t siz = codestream.access_siz();

        if (siz.get_num_components() != 3 && siz.get_num_components() != 1)
          OJPH_ERROR(0x020000004,
            "The file has %d color components; this cannot be saved to"
             " a .yuv file\n", siz.get_num_components());
        ojph::param_cod_t cod = codestream.access_cod();
        if (cod.is_using_color_transform())
          OJPH_ERROR(0x020000005,
            "The current implementation of yuv file object does not "
            "support saving a file when conversion from yuv to rgb is needed; "
            "In any case, this is not the normal usage of a yuv file");
        ojph::point points[3];
        int max_bit_depth = 0;
        for (int i = 0; i < siz.get_num_components(); ++i)
        {
          points[i] = siz.get_downsampling(i);
          max_bit_depth = ojph_max(max_bit_depth, siz.get_bit_depth(i));
        }
        codestream.set_planar(true);
        yuv.configure(siz.get_image_extent().x, siz.get_image_offset().x,
          max_bit_depth, siz.get_num_components(), points);
        yuv.open(output_filename);
        base = &yuv;
      }
      else
        OJPH_ERROR(0x020000006,
          "unknown output file extension; only (pgm, ppm, and yuv) are"
          " supported\n");
    }
    else
      OJPH_ERROR(0x020000007,
        "Please supply a proper output filename with a proper three-letter"
        " extension\n");

    codestream.create();

    if (codestream.is_planar())
    {
      ojph::param_siz_t siz = codestream.access_siz();
      for (int c = 0; c < siz.get_num_components(); ++c)
      {
        ojph::point p = siz.get_downsampling(c);
        int height = ojph_div_ceil(siz.get_image_extent().y, p.y)
                   - ojph_div_ceil(siz.get_image_offset().y, p.y);
        for (int i = height; i > 0; --i)
        {
          int comp_num;
          ojph::line_buf *line = codestream.pull(comp_num);
          assert(comp_num == c);
          base->write(line, comp_num);
        }
      }
    }
    else
    {
      ojph::param_siz_t siz = codestream.access_siz();
      int height = siz.get_image_extent().y - siz.get_image_offset().y;
      for (int i = 0; i < height; ++i)
      {
        for (int c = 0; c < siz.get_num_components(); ++c)
        {
          int comp_num;
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

  }

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
  printf("Elapsed time = %f\n", elapsed.count()/numReps);

  return 0;
}
