/****************************************************************************/
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
/****************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_wrapper.cpp
// Author: Aous Naman
// Date: 22 October 2019
/****************************************************************************/

#include <exception>

#include "ojph_arch.h"
#include "ojph_file.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream.h"

//////////////////////////////////////////////////////////////////////////////
struct j2c_t
{
  ojph::codestream codestream;
  ojph::mem_infile mem_file;
};

//////////////////////////////////////////////////////////////////////////////
j2c_t* cpp_create_j2c_data(void)
{
  return new j2c_t;
}

//////////////////////////////////////////////////////////////////////////////
void cpp_init_j2c_data(j2c_t *j2c, const uint8_t *data, size_t size)
{
  try {
    j2c->mem_file.open(data, size);
    j2c->codestream.read_headers(&j2c->mem_file);
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
  }
}

//////////////////////////////////////////////////////////////////////////////
void cpp_release_j2c_data(j2c_t* j2c)
{
  if (j2c)
  {
    delete j2c;
  }
}

//////////////////////////////////////////////////////////////////////////////
extern "C"
{
  ////////////////////////////////////////////////////////////////////////////
  j2c_t* create_j2c_data(void)
  {
    return cpp_create_j2c_data();
  }
  
  ////////////////////////////////////////////////////////////////////////////
  void init_j2c_data(j2c_t *j2c, const uint8_t *data, size_t size)
  {
    cpp_init_j2c_data(j2c, data, size);
  }
  
  ////////////////////////////////////////////////////////////////////////////
  int get_j2c_width(j2c_t* j2c)
  {
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    return siz.get_image_extent().x - siz.get_image_offset().x;
  }

  ////////////////////////////////////////////////////////////////////////////
  int get_j2c_height(j2c_t* j2c)
  {
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    return siz.get_image_extent().y - siz.get_image_offset().y;
  }
  
  ////////////////////////////////////////////////////////////////////////////
  void expand_j2c_data(j2c_t* j2c, ojph::ui8* buf, 
                       int buf_width, int buf_height)
  {
    j2c->codestream.set_planar(false);
    j2c->codestream.create();
    
    int max_val = 255;
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    int height = siz.get_image_extent().y - siz.get_image_offset().y;
    height = ojph_min(height, buf_height);
    int width = siz.get_image_extent().x - siz.get_image_offset().x;
    width = ojph_min(width, buf_width);
    if (siz.get_num_components() == 1)
      for (int i = 0; i < height; ++i)
      {
        int comp_num;
        ojph::line_buf *line = j2c->codestream.pull(comp_num);
        const ojph::si32 *sp = line->i32;
        ojph::ui8 *dp = buf + i * width * 4;
        for (int j = 0; j < width; ++j)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp++ = (ojph::ui8)val;
          *dp++ = (ojph::ui8)val;
          *dp++ = (ojph::ui8)val;
          *dp++ = (ojph::ui8)255;
        }
      }
    else
      for (int i = 0; i < height; ++i)
      {
        for (int c = 0; c < siz.get_num_components(); ++c)
        {
          int comp_num;
          ojph::line_buf *line = j2c->codestream.pull(comp_num);
          const ojph::si32 *sp = line->i32;
          ojph::ui8 *dp = buf + i * width * 4 + c;
          for (int j = 0; j < width; ++j, dp+=4)
          {
            int val = *sp++;
            val = val >= 0 ? val : 0;
            val = val <= max_val ? val : max_val;
            *dp = (ojph::ui8)val;
          }
        }
        ojph::ui8 *dp = buf + i * width * 4 + 3;
        for (int j = 0; j < width; ++j, dp+=4)
        {
          *dp = (ojph::ui8)255;
        }
      }
    j2c->codestream.close();
  }
  
  ////////////////////////////////////////////////////////////////////////////
  void release_j2c_data(j2c_t* j2c)
  {
    cpp_release_j2c_data(j2c);
  }
}

#ifdef BUILD_TEST
#include <cstdio>
int main(int argc, const char* argv[])
{
  const char filename[] = "test.j2c";
  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  long int size = ftell(f);
  ojph::ui8 *compressed_data = (ojph::ui8*)malloc(size);
  fseek(f, 0, SEEK_SET);
  fread(compressed_data, 1, size, f);
  fclose(f);
  
  struct j2c_t* j2c = create_j2c_data();
  init_j2c_data(j2c, compressed_data, size);
  int width = get_j2c_width(j2c);
  int height = get_j2c_height(j2c);
  printf("width = %d, height = %d\n", width, height);
  
  ojph::ui8* buf = (ojph::ui8*)malloc(width * height * 4); //assuming RGBA
   
  expand_j2c_data(j2c, buf, width, height);
  release_j2c_data(j2c);

  f = fopen("test_out.ppm", "wb");
  fprintf(f, "P6 %d %d %d\n", width, height, 255);
  for (int i = 0; i < height; ++i)
  {
    ojph::ui8* sp = buf + i * width * 4;
    for (int j = 0; j < width; ++j, sp+=4)
      fwrite(sp, 3, 1, f);
  }
  fclose(f);

  return 0;
}
#endif


