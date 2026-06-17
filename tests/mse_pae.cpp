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
// File: mse_pae.cpp
// Author: Aous Naman
// Date: 18 March 2021
//***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cctype>
#include "ojph_img_io.h"
#include "ojph_mem.h"

using namespace ojph;
using namespace std;

enum : ui32 {
  UNDEFINED = 0,
  FORMAT444 = 1,
  FORMAT422 = 2,
  FORMAT420 = 3,
  FORMAT400 = 4,
};

struct img_info {
  img_info() { 
    num_comps = 0;
    width = height = 0;
    comps[0] = comps[1] = comps[2] = 0;
    format = UNDEFINED;
    bit_depth = 0;
    is_signed = false;
  }
  ~img_info() {
    for (ui32 i = 0; i < num_comps; ++i)
    {
      if (comps[i]) delete[] comps[i];
      comps[i] = NULL;
    }
  }
  
  void init(ui32 num_comps, size_t width, size_t height, ui32 bit_depth,
            bool is_signed, ui32 format=FORMAT444)
  {
    assert(num_comps <= 3 && comps[0] == NULL);
    this->num_comps = num_comps;
    this->width = width;
    this->height = height;
    this->format = format;
    this->bit_depth = bit_depth;
    this->is_signed = is_signed;
    for (ui32 i = 0; i < num_comps; ++i)
      switch (format)
      {
        case FORMAT444:
        case FORMAT400:        
          downsampling[i].x = downsampling[i].y = 1;
          break;
        case FORMAT422:
          downsampling[i].x = i == 0 ? 1 : 2;
          downsampling[i].y = 1;
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
  
  bool exist() {
    return comps[0] != NULL;
  }
  
  ui32 num_comps;
  size_t width, height;
  point downsampling[3];
  si32 *comps[3];
  ui32 format;
  ui32 bit_depth;
  bool is_signed;
};

bool is_pnm(const char *filename)
{
  size_t len = strlen(filename);
  if (len >= 4 && filename[len - 4] == '.' && 
      toupper(filename[len - 3]) == 'P' && 
      (toupper(filename[len - 2])== 'P' || toupper(filename[len - 2]) == 'G') &&
      toupper(filename[len - 1]) == 'M')
    return true;
  return false;
}

void load_ppm(const char *filename, img_info& img)
{
  ppm_in ppm;
  ppm.set_planar(true);
  ppm.open(filename);

  ui32 num_comps = ppm.get_num_components();
  size_t width = ppm.get_width();
  size_t height = ppm.get_height();
  img.init(num_comps, width, height, ppm.get_bit_depth(0), false);
  
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

bool is_yuv(const char *filename)
{
  const char *p = strchr(filename, ':'); // p is either NULL or pointing to ':'
  if (p != NULL && p - filename >= 4 && p[-4] == '.' && 
      toupper(p[-3]) == 'Y' && toupper(p[-2])== 'U' && toupper(p[-1]) == 'V')
    return true;
  return false;
}

void load_yuv(const char *filename, img_info& img)
{  
  const char *p = strchr(filename, ':'); // p is either NULL or pointing to ':'
  const char *name_end = p;
  if (p == NULL) {
    printf("A .yuv that does not have the expected format, which is\n");
    printf(".yuv:widthxheightxbitdepthxformat, where format is\n");
    printf("either 444, 422, or 420\n");
    exit(-1);
  }
  
  ojph::size s;
  s.w = (ui32)atoi(++p);
  p = strchr(p, 'x'); // p is either NULL or pointing to ':'
  if (p == NULL) {
    printf("Expecting image height.\n");
    printf("A .yuv that does not have the expected format, which is\n");
    printf(".yuv:widthxheightxbitdepthxformat, where format is\n");
    printf("either 444, 422, or 420\n");
    exit(-1);
  }
  s.h = (ui32)atoi(++p);
  p = strchr(p, 'x'); // p is either NULL or pointing to ':'
  if (p == NULL) {
    printf("Expecting image bitdepth.\n");
    printf("A .yuv that does not have the expected format, which is\n");
    printf(".yuv:widthxheightxbitdepthxformat, where format is\n");
    printf("either 444, 422, or 420\n");
    exit(-1);
  }
  ui32 bit_depth = (ui32)atoi(++p);
  p = strchr(p, 'x'); // p is either NULL or pointing to ':'
  if (p == NULL) {
    printf("Expecting color subsampling format.\n");
    printf("A .yuv that does not have the expected format, which is\n");
    printf(".yuv:widthxheightxbitdepthxformat, where format is\n");
    printf("either 444, 422, or 420\n");
    exit(-1);
  }
  // p must be pointing to color subsampling format
  ++p;
  size_t len = strlen(p);
  if (len != 3)
  {
    printf("Image color format must have 3 characters, %s was supplied.\n", p);
    printf("A .yuv that does not have the expected format, which is\n");
    printf(".yuv:widthxheightxbitdepthxformat, where format is\n");
    printf("either 444, 422, or 420\n");
    exit(-1);
  }
  ui32 num_comps;
  point downsampling[3] = { point(1,1), point(1,1), point(1,1)};
  ui32 format;
  if (strcmp(p, "444") == 0)
  {
    num_comps = 3;
    format = FORMAT444;
  }
  else if (strcmp(p, "422") == 0)
  {
    num_comps = 3;
    format = FORMAT422;
    downsampling[1].x = downsampling[2].x = 2;
  }
  else if (strcmp(p, "420") == 0)
  {
    num_comps = 3;
    format = FORMAT420;
    downsampling[1].x = downsampling[2].x = 2;
    downsampling[1].y = downsampling[2].y = 2;
  }
  else if (strcmp(p, "400") == 0)
  {
    num_comps = 1;
    format = FORMAT400;
  }
  else {
    printf("Unknown image color format, %s.\n", p);
    exit(-1);
  }

  char name_buf[2048];
  ptrdiff_t cpy_len = name_end - filename > 2047 ? 2047 : name_end - filename;
  strncpy(name_buf, filename, (size_t)cpy_len);
  name_buf[cpy_len] = 0;
  
  yuv_in yuv;
  ui32 depths[3] = {bit_depth, bit_depth, bit_depth};
  yuv.set_bit_depth(num_comps, depths);
  yuv.set_img_props(s, num_comps, num_comps, downsampling);  
  yuv.open(name_buf);
  
  img.init(num_comps, s.w, s.h, bit_depth, false, format);
  
  size_t w = calc_aligned_size<si32, byte_alignment>(s.w);
  si32 *buffer = new si32[w];
  line_buf line;
  line.wrap(buffer, w, 0);
  
  for (ui32 c = 0; c < num_comps; ++c)
  {  
    si32 *p = img.comps[c];
    ui32 height = (s.h + img.downsampling[c].y - 1) / img.downsampling[c].y;
    for (ui32 h = 0; h < height; ++h)
    {
      ui32 w = yuv.read(&line, c);
      memcpy(p, line.i32, w * sizeof(si32));
      p += w;
    }
  }

  delete[] buffer;
}

bool is_rawl(const char *filename)
{
  const char *p = strchr(filename, ':'); // p is either NULL or pointing to ':'
  if (p != NULL && p - filename >= 5 && p[-5] == '.' && 
      toupper(p[-4]) == 'R' && toupper(p[-3])== 'A' && 
      toupper(p[-2]) == 'W' && toupper(p[-1]) == 'L')
    return true;
  return false;
}

void load_rawl(const char *filename, img_info& img)
{  
  const char *p = strchr(filename, ':'); // p is either NULL or pointing to ':'
  const char *name_end = p;
  if (p == NULL) {
    printf("A .rawl that does not have the expected format, which is\n");
    printf(".rawl:widthxheightxbitdepthxsignedxnum_comp\n");
    exit(-1);
  }
  ojph::size s;
  ++p;
  s.w = (ui32)atoi(p);
  p = strchr(p, 'x'); // p is either NULL or pointing to ':'
  if (p == NULL) {
    printf("Expecting image height.\n");
    printf("A .rawl that does not have the expected format, which is\n");
    printf(".rawl:widthxheightxbitdepthxsignedxnum_comp\n");
    exit(-1);
  }
  ++p;
  s.h = (ui32)atoi(p);
  p = strchr(p, 'x'); // p is either NULL or pointing to ':'
  if (p == NULL) {
    printf("Expecting image bitdepth.\n");
    printf("A .rawl that does not have the expected format, which is\n");
    printf(".rawl:widthxheightxbitdepthxsignedxnum_comp\n");
    exit(-1);
  }
  ++p;
  ui32 bit_depth = (ui32)atoi(p);
  p = strchr(p, 'x'); // p is either NULL or pointing to ':'
  if (p == NULL) {
    printf("Expecting signedness information (either 0 or 1).\n");
    printf("A .rawl that does not have the expected format, which is\n");
    printf(".rawl:widthxheightxbitdepthxsignedxnum_comp, where num_comp is\n");
    printf("either 1 or 3\n");
    exit(-1);
  }
  ++p;
  bool is_signed = *p != '0';
  p = strchr(p, 'x'); // p is either NULL or pointing to ':'
  if (p == NULL) {
    printf("Expecting number of components.\n");
    printf("A .rawl that does not have the expected format, which is\n");
    printf(".rawl:widthxheightxbitdepthxsignedxnum_comp, where num_comp is\n");
    printf("either 1 or 3\n");
    exit(-1);
  }
  ++p;
  ui32 num_comps = (ui32)atoi(p);
  if (num_comps != 1 && num_comps != 3)
  {
    printf("num_comp must be either 1 or 3, %s was supplied.\n", p);
    printf("A .rawl that does not have the expected format, which is\n");
    printf(".rawl:widthxheightxbitdepthxsignedxnum_comp, where format is\n");
    printf("either 1 or 3\n");
    exit(-1);
  }

  char name_buf[2048];
  ptrdiff_t cpy_len = name_end - filename > 2047 ? 2047 : name_end - filename;
  strncpy(name_buf, filename, (size_t)cpy_len);
  name_buf[cpy_len] = 0;

  if (num_comps == 3)
    img.init(num_comps, s.w, s.h, bit_depth, is_signed, FORMAT444);
  else
    img.init(num_comps, s.w, s.h, bit_depth, is_signed, FORMAT400);

  if (is_signed)
  {
    if (bit_depth <= 8)
    {
      si8 *buffer = new si8[s.w *  s.h];      
      FILE *f = fopen(name_buf, "rb");
      if (f == NULL) {
        printf("Error opening file %s\n", name_buf);
        exit(-1);
      }

      for (ui32 i = 0; i < num_comps; ++i)
      {
        si8 *sp = buffer;
        si32 *dp = img.comps[i];
        if (fread(buffer, 1, s.w * s.h, f) != s.w * s.h) {
          printf("Error reading from file %s\n", name_buf);
          exit(-1);
        }
        for (ui32 j = s.w * s.h; j > 0; --j)
          *dp++ = *sp++;
      }
      fclose(f);
      delete[] buffer;
    }
    else if (bit_depth <= 16)
    {
      si16 *buffer = new si16[s.w *  s.h];      
      FILE *f = fopen(name_buf, "rb");
      if (f == NULL) {
        printf("Error opening file %s\n", name_buf);
        exit(-1);
      }

      for (ui32 i = 0; i < num_comps; ++i)
      {
        si16 *sp = buffer;
        si32 *dp = img.comps[i];
        if (fread(buffer, 2, s.w * s.h, f) != s.w * s.h) {
          printf("Error reading from file %s\n", name_buf);
          exit(-1);
        }
        for (ui32 j = s.w * s.h; j > 0; --j)
          *dp++ = *sp++;
      }
      fclose(f);
      delete[] buffer;
    }
    else
    {
      si32 *buffer = new si32[s.w *  s.h];      
      FILE *f = fopen(name_buf, "rb");
      if (f == NULL) {
        printf("Error opening file %s\n", name_buf);
        exit(-1);
      }

      for (ui32 i = 0; i < num_comps; ++i)
      {
        si32 *sp = buffer;
        si32 *dp = img.comps[i];
        if (fread(buffer, 4, s.w * s.h, f) != s.w * s.h) {
          printf("Error reading from file %s\n", name_buf);
          exit(-1);
        }
        for (ui32 j = s.w * s.h; j > 0; --j)
          *dp++ = *sp++;
      }
      fclose(f);
      delete[] buffer;
    }
  }
  else
  {
    if (bit_depth <= 8)
    {
      ui8 *buffer = new ui8[s.w *  s.h];      
      FILE *f = fopen(name_buf, "rb");
      if (f == NULL) {
        printf("Error opening file %s\n", name_buf);
        exit(-1);
      }

      for (ui32 i = 0; i < num_comps; ++i)
      {
        ui8 *sp = buffer;
        si32 *dp = img.comps[i];
        if (fread(buffer, 1, s.w * s.h, f) != s.w * s.h) {
          printf("Error reading from file %s\n", name_buf);
          exit(-1);
        }
        for (ui32 j = s.w * s.h; j > 0; --j)
          *dp++ = *sp++;
      }
      fclose(f);
      delete[] buffer;
    }
    else if (bit_depth <= 16)
    {
      ui16 *buffer = new ui16[s.w *  s.h];      
      FILE *f = fopen(name_buf, "rb");
      if (f == NULL) {
        printf("Error opening file %s\n", name_buf);
        exit(-1);
      }

      for (ui32 i = 0; i < num_comps; ++i)
      {
        ui16 *sp = buffer;
        si32 *dp = img.comps[i];
        if (fread(buffer, 2, s.w * s.h, f) != s.w * s.h) {
          printf("Error reading from file %s\n", name_buf);
          exit(-1);
        }
        for (ui32 j = s.w * s.h; j > 0; --j)
          *dp++ = *sp++;
      }
      fclose(f);
      delete[] buffer;
    }
    else
    {
      ui32 *buffer = new ui32[s.w *  s.h];      
      FILE *f = fopen(name_buf, "rb");
      if (f == NULL) {
        printf("Error opening file %s\n", name_buf);
        exit(-1);
      }

      for (ui32 i = 0; i < num_comps; ++i)
      {
        ui32 *sp = buffer;
        si32 *dp = img.comps[i];
        if (fread(buffer, 4, s.w * s.h, f) != s.w * s.h) {
          printf("Error reading from file %s\n", name_buf);
          exit(-1);
        }
        for (ui32 j = s.w * s.h; j > 0; --j)
          *dp++ = (si32)*sp++;
      }
      fclose(f);
      delete[] buffer;
    }
  }
}

void find_mse_pae(const img_info& img1, const img_info& img2, 
                  float mse[3], ui32 pae[3])
{
  if (img1.num_comps != img2.num_comps || img1.format != img2.format ||
      img1.width != img2.width || img1.height != img2.height ||
      img1.bit_depth != img2.bit_depth || img1.is_signed != img2.is_signed)
  {
    printf("Error: mismatching images\n");
    exit(-1);
  }
  for (ui32 c = 0; c < img1.num_comps; ++c)
  {
    size_t w, h;
    w = (img1.width + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
    h = (img1.height + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
    double se = 0;
    ui32 lpae = 0;
    if (img1.is_signed)
      for (ui32 v = 0; v < h; ++v)
      {
        si32 *p0 = img1.comps[c] + w * v;
        si32 *p1 = img2.comps[c] + w * v;
        for (ui32 s = 0; s < w; ++s)
        {
          si32 err = *p0++ - *p1++;
          ui32 ae = (ui32)(err > 0 ? err : -err);
          lpae = ae > lpae ? ae : lpae;
          se += (double)err * (double)err;
        }
      }
    else
      for (ui32 v = 0; v < h; ++v)
      {
        ui32 *p0 = (ui32*)img1.comps[c] + w * v;
        ui32 *p1 = (ui32*)img2.comps[c] + w * v;
        for (ui32 s = 0; s < w; ++s)
        {
          ui32 a = *p0++;
          ui32 b = *p1++;
          ui32 err = a > b ? a - b : b - a;
          lpae = err > lpae ? err : lpae;
          se += (double)err * (double)err;
        }
      }
    mse[c] = (float)se / (float)(w * h);
    pae[c] = lpae;
  }
}

void find_nlt_mse_pae(const img_info& img1, const img_info& img2, 
                      float mse[3], ui32 pae[3])
{
  if (img1.num_comps != img2.num_comps || img1.format != img2.format ||
      img1.width != img2.width || img1.height != img2.height ||
      img1.bit_depth != img2.bit_depth || img1.is_signed != img2.is_signed)
  {
    printf("Error: mismatching images\n");
    exit(-1);
  }
  if (img1.is_signed)
    for (ui32 c = 0; c < img1.num_comps; ++c)
    {
      size_t w, h;
      w = (img1.width + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
      h = (img1.height + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
      double se = 0;
      ui32 lpae = 0;
      si32 bias = (si32)((1ULL << (img1.bit_depth - 1)) + 1);
      for (ui32 v = 0; v < h; ++v)
      {
        si32 *p0 = img1.comps[c] + w * v;
        si32 *p1 = img2.comps[c] + w * v;
        for (ui32 s = 0; s < w; ++s)
        {
          si32 a = *p0++;
          si32 b = *p1++;
          a = (a >= 0) ? a : (- a - bias);
          b = (b >= 0) ? b : (- b - bias);
          ui32 err = (ui32)(a > b ? a - b : b - a);
          lpae = err > lpae ? err : lpae;
          se += (double)err * (double)err;
        }
      }
      mse[c] = (float)se / (float)(w * h);
      pae[c] = lpae;
    }
  else
    for (ui32 c = 0; c < img1.num_comps; ++c)
    {
      size_t w, h;
      w = (img1.width + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
      h = (img1.height + img1.downsampling[c].x - 1) / img1.downsampling[c].x;
      double se = 0;
      ui32 lpae = 0;
      for (ui32 v = 0; v < h; ++v)
      {
        ui32 *p0 = (ui32*)img1.comps[c] + w * v;
        ui32 *p1 = (ui32*)img2.comps[c] + w * v;
        for (ui32 s = 0; s < w; ++s)
        {
          ui32 a = *p0++;
          ui32 b = *p1++;
          ui32 err = a > b ? a - b : b - a;
          lpae = err > lpae ? err : lpae;
          se += (double)err * (double)err;
        }
      }
      mse[c] = (float)se / (float)(w * h);
      pae[c] = lpae;
    }
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("mse_pae expects two arguments <filename1, filename2>\n");
    printf("A third optional argment is \"-nlt\".\n");
    exit(-1);
  }

  bool nlt = false;
  if (argc == 4)
  {
    if (strcmp("-nlt", argv[3]) == 0)
      nlt = true;
    else {
      printf("unknown 4th parameter %s\n", argv[3]);
      exit(-1);      
    }
  }


  img_info img1, img2;
  try {
    if (is_pnm(argv[1]))
      load_ppm(argv[1], img1);
    else if (is_yuv(argv[1]))
      load_yuv(argv[1], img1);
    else if (is_rawl(argv[1]))
      load_rawl(argv[1], img1);
    else {
      printf("mse_pae does not know file format of %s\n", argv[1]);
      printf("or a .yuv that does not have the expected format, which is\n");
      printf(".yuv:widthxheightxbitdepthxformat, where format is\n");
      printf("either 444, 422, or 420, or wrongly format .rawl, which has\n");
      printf(".rawl:widthxheightxbitdepthxsignedxnum_comp format.\n");
      exit(-1);  
    }
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
    exit(-1);
  } 

  try {  
    if (is_pnm(argv[2]))
      load_ppm(argv[2], img2);
    else if (is_yuv(argv[2]))
      load_yuv(argv[2], img2);
    else if (is_rawl(argv[2]))
      load_rawl(argv[2], img2);
    else {
      printf("mse_pae does not know file format of %s\n", argv[2]);
      printf("or a .yuv that does not have the expected format, which is\n");
      printf(".yuv:widthxheightxbitdepthxformat, where format is\n");
      printf("either 444, 422, or 420, or wrongly format .rawl, which has\n");
      printf(".rawl:widthxheightxbitdepthxsignedxnum_comp format.\n");
      exit(-1);  
    }
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
    exit(-1);
  }  
  
  float mse[3]; ui32 pae[3];
  if (!nlt)
    find_mse_pae(img1, img2, mse, pae);
  else
    find_nlt_mse_pae(img1, img2, mse, pae);
  
  for (ui32 c = 0; c < img1.num_comps; ++c)
    printf("%f %d\n", mse[c], pae[c]);
  
  return 0;
}


