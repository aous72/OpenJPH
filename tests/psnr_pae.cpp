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
// This file is part of the testing routines for the OpenJPH software 
// implementation.
// File: psnr_pae.cpp
// Author: Aous Naman
// Date: 18 March 2021
//***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include "../common/ojph_img_io.h"
#include "../common/ojph_mem.h"

using namespace ojph;

enum : ui32 {
  UNDEFINED = 0,
  FORMAT444 = 1,
  FORMAT422 = 2,
  FORMAT420 = 3,
};

struct img_info {
  img_info() { 
    num_comps = 0;
    width = height = 0;
    comps[0] = comps[1] = comps[2] = 0;
    format = UNDEFINED;
    max_val = 0;
  }
  ~img_info() {
    for (ui32 i = 0; i < num_comps; ++i)
    {
      if (comps[i]) delete[] comps[i];
      comps[i] = NULL;
    }
  }
  
  void init(ui32 num_comps, size_t width, size_t height, ui32 max_val,
            ui32 format=FORMAT444)
  {
    assert(num_comps <= 3 && comps[0] == NULL);
    this->num_comps = num_comps;
    this->width = width;
    this->height = height;
    this->format = format;
    this->max_val = max_val;
    for (ui32 i = 0; i < num_comps; ++i)
      switch (format)
      {
        case FORMAT444:
          downsampling[i].x = downsampling[i].y = 1;
          break;
        case FORMAT422:
          downsampling[i].x = 1;
          downsampling[i].y = i == 0 ? 1 : 2;
          break;
        case FORMAT420:
          downsampling[i].x = i == 0 ? 1 : 2;
          downsampling[i].y = i == 0 ? 1 : 2;
          break;
        default:
          assert(0);
      };
    for (ui32 i = 0; i < num_comps; ++i)
    {
      size_t w = (this->width + downsampling[i].x - 1) / downsampling[i].x;
      size_t h = (this->height + downsampling[i].x - 1) / downsampling[i].x;
      comps[i] = new si32[w * h];
    }
  }
  
  ui32 num_comps;
  size_t width, height;
  point downsampling[3];
  si32 *comps[3];
  ui32 format;
  ui32 max_val;
};

//     void open(const char* filename);
//     void finalize_alloc();
//     virtual ui32 read(const line_buf* line, ui32 comp_num);
//     void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }
//     void set_plannar(bool planar) { this->planar = planar; }


void load_ppm(const char *filename, img_info& img)
{
  ppm_in ppm;
  ppm.set_planar(true);
  ppm.open(filename);

  ui32 num_comps = ppm.get_num_components();
  size_t width = ppm.get_width();
  size_t height = ppm.get_height();
  img.init(num_comps, width, height, ppm.get_max_val());
  
  width = calc_aligned_size<si32, byte_alignment>(width);
  si32 *buffer = new si32[width];
  line_buf line;
  line.wrap(buffer, width, 0);
  
  for (ui32 c = 0; c < num_comps; ++c)
  {  
    si32 *p = img.comps[c];
    for (ui32 h = 0; h < height; ++h)
    {
      ui32 w = ppm.read(&line, c);
      memcpy(p, line.i32, w * sizeof(si32));
      p += w;
    }
  }

  delete[] buffer;
}

void find_psnr_pae(const img_info& img1, const img_info& img2, 
                   float &psnr, ui32 &pae)
{
  if (img1.num_comps != img2.num_comps || img1.format != img2.format ||
      img1.width != img2.width || img1.height != img2.height ||
      img1.max_val != img2.max_val)
  {
    printf("Error: mismatching images\n");
    exit(-1);
  }
  size_t mse[3] = { 0 };
  pae = 0;
  size_t num_pixels = 0;
  for (ui32 c = 0; c < img1.num_comps; ++c)
  {
    size_t w, h;
    w = (img1.width + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
    h = (img1.height + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
    num_pixels += w * h;
    for (ui32 v = 0; v < h; ++v)
    {
      si32 *p0 = img1.comps[c] + w * v;
      si32 *p1 = img2.comps[c] + w * v;
      for (ui32 s = 0; s < w; ++s)
      {
        si32 err = *p0 - *p1;
        ui32 ae = (ui32)(err > 0 ? err : -err);
        mse[c] += ae * ae;
        pae = ae > pae ? ae : pae;
      }
    }
  }
  float t = 0;
  for (ui32 c = 0; c < img1.num_comps; ++c)
    t += mse[c];
  t /= num_pixels;
  psnr = 10.0f * log10f(img1.max_val * img1.max_val / t);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("psnr_pae expects two arguments <filename1, filename2>\n");
    exit(-1);
  }
    
  img_info img1, img2;
  load_ppm(argv[1], img1);
  load_ppm(argv[2], img2);
  
  float psnr; ui32 pae;
  find_psnr_pae(img1, img2, psnr, pae);
  
  printf("%f %d\n", psnr, pae);
  
  return 0;
}


