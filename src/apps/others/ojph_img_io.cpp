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
// File: ojph_img_io.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <cstdlib>
#include <cstring>

#include "ojph_file.h"
#include "ojph_img_io.h"
#include "ojph_mem.h"
#include "ojph_message.h"

namespace ojph {

  /////////////////////////////////////////////////////////////////////////////
  // Static functions
  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
  static
  ui16 be2le(const ui16 v)
  {
    return (ui16)((v<<8) | (v>>8));
  }

  //////////////////////////////////////////////////////////////////////////
  static inline
  ui32 be2le(const ui32 t)
  {
    ui32 u = be2le((ui16)(t & 0xFFFFu));
    u <<= 16;
    u |= be2le((ui16)(t >> 16));
    return u;
  }

  /////////////////////////////////////////////////////////////////////////////
  static
  void eat_white_spaces(FILE *fh)
  {
    int c = fgetc(fh);
    while(1)
    {
      if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
        c = fgetc(fh);
      else if (c == '#')
      {
        while (c != '\n') c = fgetc(fh);
      }
      else
      {
        ungetc(c, fh);
        break;
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  // Accelerators -- non-accelerating
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  void gen_cvrt_32b1c_to_8ub1c(const line_buf *ln0, const line_buf *ln1, 
                               const line_buf *ln2, void *dp, 
                               ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);
    
    int max_val = (1 << bit_depth) - 1;
    const si32 *sp = ln0->i32;
    ui8* p = (ui8 *)dp;
    for ( ; count > 0; --count)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8)val;
    }
  }

  void gen_cvrt_32b3c_to_8ub3c(const line_buf *ln0, const line_buf *ln1, 
                               const line_buf *ln2, void *dp, 
                               ui32 bit_depth, ui32 count)
  {
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui8* p = (ui8 *)dp;
    for (; count > 0; --count)
    {
      int val;
      val = *sp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
      val = *sp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
      val = *sp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui8) val;
    }
  }

  void gen_cvrt_32b1c_to_16ub1c_le(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp = ln0->i32;
    ui16* p = (ui16*)dp;
    for (; count > 0; --count)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
    }
  }

  void gen_cvrt_32b3c_to_16ub3c_le(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   ui32 bit_depth, ui32 count)
  {
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui16* p = (ui16*)dp;
    for (; count > 0; --count)
    {
      int val;
      val = *sp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
      val = *sp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
      val = *sp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = (ui16) val;
    }
  }

  void gen_cvrt_32b1c_to_16ub1c_be(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   ui32 bit_depth, ui32 count)
  {
    ojph_unused(ln1);
    ojph_unused(ln2);
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp = ln0->i32;
    ui16* p = (ui16*)dp;
    for (; count > 0; --count)
    {
      int val = *sp++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
    }
  }

  void gen_cvrt_32b3c_to_16ub3c_be(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   ui32 bit_depth, ui32 count)
  {
    int max_val = (1<<bit_depth) - 1;
    const si32 *sp0 = ln0->i32;
    const si32 *sp1 = ln1->i32;
    const si32 *sp2 = ln2->i32;
    ui16* p = (ui16*)dp;
    for (; count > 0; --count)
    {
      int val;
      val = *sp0++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
      val = *sp1++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
      val = *sp2++;
      val = val >= 0 ? val : 0;
      val = val <= max_val ? val : max_val;
      *p++ = be2le((ui16) val);
    }
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
  void ppm_in::open(const char *filename)
  {
    assert(fh == 0);
    fh = fopen(filename, "rb");
    if (fh == 0)
      OJPH_ERROR(0x03000001, "Unable to open file %s", filename);
    fname = filename;

    // read magic number
    char t[2];
    if (fread(t, 1, 2, fh) != 2)
    {
      close();
      OJPH_ERROR(0x03000002, "Error reading file %s", filename);
    }

    // check magic number
    if (t[0] != 'P' || (t[1] != '5' && t[1] != '6'))
    {
      close();
      OJPH_ERROR(0x03000003, "unknown file type for file %s", filename);
    }

    size_t len = strlen(filename);
    if (t[1] == '5' && strncmp(filename + len - 4, ".pgm", 4) != 0)
    {
      close();
      OJPH_ERROR(0x03000004, "wrong file extension, a file with "
        "keyword P5 must have a .pgm extension for file %s", filename);
    }
    if (t[1] == '6' && strncmp(filename + len - 4, ".ppm", 4) != 0)
    {
      close();
      OJPH_ERROR(0x03000005, "wrong file extension, a file with keyword P6 "
        "must have a .ppm extension for file %s", filename);
    }

    // set number of components based on file-type
    num_comps = t[1] == '5' ? 1 : 3;
    eat_white_spaces(fh);

    // read width, height and max value in header
    if (fscanf(fh, "%d %d %d", &width, &height, &max_val) != 3)
    {
      close();
      OJPH_ERROR(0x03000006, "error in file format for file %s", filename);
    }
    num_ele_per_line = num_comps * width;
    bytes_per_sample = max_val > 255 ? 2 : 1;
    max_val_num_bits = 32u - count_leading_zeros(max_val);
    bit_depth[2] = bit_depth[1] = bit_depth [0] = max_val_num_bits;
    fgetc(fh);
    start_of_data = ojph_ftell(fh);

    // allocate linebuffer to hold a line of image data
    if (temp_buf_byte_size < num_comps * width * bytes_per_sample)
    {
      if (alloc_p == NULL)
      {
        temp_buf_byte_size = num_comps * width * bytes_per_sample;
        void* t = temp_buf;
        if (temp_buf)
          temp_buf = realloc(temp_buf, temp_buf_byte_size);
        else
          temp_buf = malloc(temp_buf_byte_size);
        if (temp_buf == NULL) { // failed to allocate memory
          if (t) free(t); // the original buffer is still valid
          OJPH_ERROR(0x03000007, "error allocating memory");
        }
      }
      else
      {
        assert(temp_buf_byte_size == 0); //cannot reallocate the buffer
        temp_buf_byte_size = num_comps * width * bytes_per_sample;
        alloc_p->pre_alloc_data<ui8>(temp_buf_byte_size, 0);
      }
    }
    cur_line = 0;
  }

  /////////////////////////////////////////////////////////////////////////////
  void ppm_in::finalize_alloc()
  {
    if (alloc_p == NULL)
      return;
      
    if (bytes_per_sample == 1)
      temp_buf = alloc_p->post_alloc_data<ui8>(num_comps * (size_t)width, 0);
    else
      temp_buf = alloc_p->post_alloc_data<ui16>(num_comps * (size_t)width, 0);
  }

  /////////////////////////////////////////////////////////////////////////////
  ui32 ppm_in::read(const line_buf* line, ui32 comp_num)
  {
    assert(temp_buf_byte_size != 0 && fh != 0 && comp_num < num_comps);
    assert(line->size >= width);

    if (planar || comp_num == 0)
    {
      size_t result = fread(
        temp_buf, bytes_per_sample, num_ele_per_line, fh);
      if (result != num_ele_per_line)
      {
        close();
        OJPH_ERROR(0x03000011, "not enough data in file %s", fname);
      }
      if (++cur_line >= height)
      {
        cur_line = 0;
        ojph_fseek(fh, start_of_data, SEEK_SET); //handles plannar reading
      }
    }

    if (bytes_per_sample == 1)
    {
      const ui8* sp = (ui8*)temp_buf + comp_num;
      si32* dp = line->i32;
      for (ui32 i = width; i > 0; --i, sp+=num_comps)
        *dp++ = (si32)*sp;
    }
    else
    {
      const ui16* sp = (ui16*)temp_buf + comp_num;
      si32* dp = line->i32;
      for (ui32 i = width; i > 0; --i, sp+=num_comps)
        *dp++ = (si32)be2le(*sp);
    }

    return width;
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void ppm_out::open(char* filename)
  {
    assert(fh == NULL && buffer == NULL);
    if (num_components == 1)
    {
      size_t len = strlen(filename);
      if (len >= 4)
      {
        if (strncmp(".ppm", filename + len - 4, 4) == 0)
        {
          filename[len - 2] = 'g'; 
          OJPH_WARN(0x03000021, "file was renamed %s\n", filename);
        }
        if (strncmp(".PPM", filename + len - 4, 4) == 0)
        {
          filename[len - 2] = 'G';
          OJPH_WARN(0x03000022, "file was renamed %s\n", filename);
        }
      }
      fh = fopen(filename, "wb");
      if (fh == NULL)
        OJPH_ERROR(0x03000023,
          "unable to open file %s for writing", filename);

      fprintf(fh, "P5\n%d %d\n%d\n", width, height, (1 << bit_depth) - 1);
      buffer_size = (size_t)width * bytes_per_sample;
      buffer = (ui8*)malloc(buffer_size);
    }
    else
    {
      size_t len = strlen(filename);
      if (len >= 4)
      {
        if (strncmp(".pgm", filename + len - 4, 4) == 0)
        {
          filename[len - 2] = 'p';
          OJPH_WARN(0x03000024, "file was renamed %s\n", filename);
        }
        if (strncmp(".PGM", filename + len - 4, 4) == 0)
        {
          filename[len - 2] = 'P';
          OJPH_WARN(0x03000025, "file was renamed %s\n", filename);
        }
      }
      fh = fopen(filename, "wb");
      if (fh == NULL)
        OJPH_ERROR(0x03000026,
          "unable to open file %s for writing", filename);
      int result = //the number of written characters
        fprintf(fh, "P6\n%d %d\n%d\n", width, height, (1 << bit_depth) - 1);
      if (result == 0)
        OJPH_ERROR(0x03000027, "error writing to file %s", filename);
      buffer_size = (size_t)width * num_components * (size_t)bytes_per_sample;
      buffer = (ui8*)malloc(buffer_size);
    }
    fname = filename;
    cur_line = 0;
  }

  ////////////////////////////////////////////////////////////////////////////
  void ppm_out::configure(ui32 width, ui32 height, ui32 num_components,
                          ui32 bit_depth)
  {
    assert(fh == NULL); //configure before opening
    if (num_components != 1 && num_components != 3)
      OJPH_ERROR(0x03000031,
        "ppm supports 3 colour components, while pgm supports 1");
    this->width = width;
    this->height = height;
    this->num_components = num_components;
    this->bit_depth = bit_depth;
    bytes_per_sample = 1 + (bit_depth > 8 ? 1 : 0);
    samples_per_line = num_components * width;
    bytes_per_line = bytes_per_sample * samples_per_line;
    
#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

    if (bytes_per_sample == 1) {
      if (num_components == 1) 
        converter = gen_cvrt_32b1c_to_8ub1c;
      else
        converter = gen_cvrt_32b3c_to_8ub3c;
    }
    else {
      if (num_components == 1) 
        converter = gen_cvrt_32b1c_to_16ub1c_be;
      else
        converter = gen_cvrt_32b3c_to_16ub3c_be;
    }

  #ifndef OJPH_DISABLE_SIMD

    #if (defined(OJPH_ARCH_X86_64) || defined(OJPH_ARCH_I386))

      #ifndef OJPH_DISABLE_SSE4
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE41) {
          if (bytes_per_sample == 1) {
            if (num_components == 1) 
              converter = sse41_cvrt_32b1c_to_8ub1c;
            else
              converter = sse41_cvrt_32b3c_to_8ub3c;
          }
          else {
            if (num_components == 1) 
              converter = sse41_cvrt_32b1c_to_16ub1c_be;
            else
              converter = sse41_cvrt_32b3c_to_16ub3c_be;
          }
        }
      #endif // !OJPH_DISABLE_SSE4

      #ifndef OJPH_DISABLE_AVX2
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX2) {
          if (bytes_per_sample == 1) {
            if (num_components == 1) 
              converter = avx2_cvrt_32b1c_to_8ub1c;
            else
              converter = avx2_cvrt_32b3c_to_8ub3c;
          }
          else {
            if (num_components == 1) 
              converter = avx2_cvrt_32b1c_to_16ub1c_be;
            else
              { } // did not find an implementation better than sse41
          }
        }
      #endif // !OJPH_DISABLE_AVX2

    #elif defined(OJPH_ARCH_ARM)

    #endif // !(defined(OJPH_ARCH_X86_64) || defined(OJPH_ARCH_I386))

  #endif // !OJPH_DISABLE_SIMD

#else // OJPH_ENABLE_WASM_SIMD

    if (bytes_per_sample == 1) {
      if (num_components == 1) 
        converter = sse41_cvrt_32b1c_to_8ub1c;
      else
        converter = sse41_cvrt_32b3c_to_8ub3c;
    }
    else {
      if (num_components == 1) 
        converter = sse41_cvrt_32b1c_to_16ub1c_be;
      else
        converter = sse41_cvrt_32b3c_to_16ub3c_be;
    }
  
#endif // !OJPH_ENABLE_WASM_SIMD
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 ppm_out::write(const line_buf* line, ui32 comp_num)
  {
    assert(fh);

    lptr[comp_num] = line;
    if (comp_num == num_components - 1)
    {
      assert(lptr[0] != lptr[1]);
      assert((lptr[1]!=lptr[2] && num_components==3) || num_components==1);
      converter(lptr[0], lptr[1], lptr[2], buffer, bit_depth, width);
      size_t result = fwrite(buffer,
                              bytes_per_sample, samples_per_line, fh);
      if (result != samples_per_line)
        OJPH_ERROR(0x03000041, "error writing to file %s", fname);
    }
    return 0;
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
  void pfm_in::open(const char *filename)
  {
    assert(fh == 0);
    fh = fopen(filename, "rb");
    if (fh == 0)
      OJPH_ERROR(0x03000051, "Unable to open file %s", filename);
    fname = filename;

    // read magic number
    char t[2];
    if (fread(t, 1, 2, fh) != 2)
    {
      close();
      OJPH_ERROR(0x03000052, "Error reading file %s", filename);
    }

    // check magic number
    if (t[0] != 'P' || (t[1] != 'F' && t[1] != 'f'))
    {
      close();
      OJPH_ERROR(0x03000053, "Unknown file type for file %s", filename);
    }

    // set number of components based on file-type
    num_comps = t[1] == 'f' ? 1 : 3;
    eat_white_spaces(fh);

    // read width, height and max value in header
    if (fscanf(fh, "%d %d", &width, &height) != 2)
    {
      close();
      OJPH_ERROR(0x03000054, 
        "Error reading width and height in file %s", filename);
    }
    eat_white_spaces(fh);

    // little or big-endian
    if (fscanf(fh, "%f", &scale) != 1)
    {
      close();
      OJPH_ERROR(0x03000055, "Error reading scale in file %s", filename);
    }
    little_endian = scale < 0.0f;
    scale = std::abs(scale);

    fgetc(fh);
    start_of_data = ojph_ftell(fh);

    // alloc. linebuffer to hold a line of image data, if more than 1 comp.
    if (temp_buf_byte_size < num_comps * (size_t)width * sizeof(float))
    {
      if (alloc_p == NULL)
      {
        temp_buf_byte_size = num_comps * (size_t)width * sizeof(float);
        void* t = temp_buf;
        if (temp_buf)
          temp_buf = (float*)realloc(temp_buf, temp_buf_byte_size);
        else
          temp_buf = (float*)malloc(temp_buf_byte_size);
        if (temp_buf == NULL) { // failed to allocate memory
          if (t) free(t); // the original buffer is still valid
          OJPH_ERROR(0x03000056, "Error allocating memory");
        }
      }
      else
      {
        assert(temp_buf_byte_size == 0); //cannot reallocate the buffer
        temp_buf_byte_size = num_comps * (size_t)width * sizeof(float);
        alloc_p->pre_alloc_data<float>(temp_buf_byte_size, 0);
      }
    }
    cur_line = 0;
  }

  /////////////////////////////////////////////////////////////////////////////
  void pfm_in::finalize_alloc()
  {
    if (alloc_p == NULL)
      return;
    temp_buf = alloc_p->post_alloc_data<float>(num_comps * (size_t)width, 0);
  }

  /////////////////////////////////////////////////////////////////////////////
  ui32 pfm_in::read(const line_buf* line, ui32 comp_num)
  {
    assert(temp_buf_byte_size != 0 );
    assert(fh != 0 && comp_num < num_comps);
    assert(line->size >= width);

    if (comp_num == 0)
    {
      si64 loc = start_of_data;
      loc += (size_t)(height-1 - cur_line) * (size_t)num_comps 
           * (size_t)width * sizeof(float);
      if (ojph_fseek(fh, loc, SEEK_SET) != 0)
      {
        close();
        OJPH_ERROR(0x03000061, "Error seeking in file %s", fname);
      }
      size_t result = 
        fread(temp_buf, sizeof(float), (size_t)num_comps * (size_t)width, fh);
      if (result != (size_t)num_comps * (size_t)width)
      {
        close();
        OJPH_ERROR(0x03000062, "Not enough data in file %s", fname);
      }
      if (++cur_line >= height)
        cur_line = 0;
    }

    union {
      si32* s;
      ui32* u;
      float* f;
    } sp, dp;

    if (little_endian)
    {
      ui32 shift = 32 - bit_depth[comp_num];
      sp.f = temp_buf + comp_num;
      dp.f = line->f32;
      if (shift)
        for (ui32 i = width; i > 0; --i, sp.f += num_comps) 
        {
          si32 s = *sp.s;
          s >>= shift;
          *dp.s++ = s;
        }
      else
        for (ui32 i = width; i > 0; --i, sp.f += num_comps)
          *dp.f++ = *sp.f;
    }
    else {
      ui32 shift = 32 - bit_depth[comp_num];
      sp.f = temp_buf + comp_num;
      dp.f = line->f32;
      if (shift)
        for (ui32 i = width; i > 0; --i, sp.f += num_comps) {
          ui32 u = be2le(*sp.u);
          si32 s = *(si32*)&u;
          s >>= shift;
          *dp.s++ = s;
        }
      else
        for (ui32 i = width; i > 0; --i, sp.f += num_comps)
          *dp.u++ = be2le(*sp.u);
    }

    return width;
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void pfm_out::open(char* filename)
  {
    assert(fh == NULL && buffer == NULL);
    fh = fopen(filename, "wb");
    if (fh == NULL)
      OJPH_ERROR(0x03000071,
        "Unable to open file %s for writing", filename);
    int result = //the number of written characters
      fprintf(fh, "P%c\n%d %d\n%f\n", 
        num_components > 1 ? 'F' : 'f', width, height, scale);
    if (result == 0)
      OJPH_ERROR(0x03000072, "error writing to file %s", filename);
    buffer_size = (size_t)width * num_components * sizeof(float);
    buffer = (float*)malloc(buffer_size);
    fname = filename;
    cur_line = 0;
    start_of_data = ojph_ftell(fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  void pfm_out::configure(ui32 width, ui32 height, ui32 num_components, 
                          float scale, ui32* bit_depth)
  {
    assert(fh == NULL); //configure before opening
    if (num_components != 1 && num_components != 3)
      OJPH_ERROR(0x03000081,
        "pfm supports 1 or 3 colour components, not %d", num_components);
    this->width = width;
    this->height = height;
    this->num_components = num_components;
    this->scale = scale < 0.0f ? scale : -scale;
    for (ui32 c = 0; c < num_components; ++c)
      this->bit_depth[c] = bit_depth[c];
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 pfm_out::write(const line_buf* line, ui32 comp_num)
  {
    assert(fh);

    ui32 shift = 32 - bit_depth[comp_num];
    union {
      ui32* u;
      float* f;
    } sp, dp;

    dp.f = buffer + comp_num;
    sp.f = line->f32;

    if (shift)
      for (ui32 i = width; i > 0; --i, dp.f += num_components, ++sp.f)
      {
        ui32 u = *sp.u;
        u <<= shift;
        *dp.u = u;
      }
    else
      for (ui32 i = width; i > 0; --i, dp.f += num_components)
        *dp.f = *sp.f++;

    if (comp_num == num_components - 1)
    {
      size_t samples_per_line = num_components * (size_t)width;
      si64 loc = start_of_data;
      loc += (height - 1 - cur_line)* samples_per_line * sizeof(float);
      if (ojph_fseek(fh, loc, SEEK_SET) != 0)
        OJPH_ERROR(0x03000082, "Error seeking in file %s", fname);
      size_t result = fwrite(buffer, sizeof(float), samples_per_line, fh);
      if (result != samples_per_line)
        OJPH_ERROR(0x03000083, "error writing to file %s", fname);
      ++cur_line;
    }

    return 0;
  }

 ////////////////////////////////////////////////////////////////////////////
 //
 //
 //
 //
 //
 ////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_ENABLE_TIFF_SUPPORT
 /////////////////////////////////////////////////////////////////////////////
  void tif_in::open(const char* filename)
  {
    tiff_handle = NULL;
    if ((tiff_handle = TIFFOpen(filename, "r")) == NULL)
      OJPH_ERROR(0x03000091, "Unable to open file %s", filename);
    fname = filename;

    ui32 tiff_width = 0;
    ui32 tiff_height = 0; 
    TIFFGetField(tiff_handle, TIFFTAG_IMAGEWIDTH, &tiff_width);
    TIFFGetField(tiff_handle, TIFFTAG_IMAGELENGTH, &tiff_height);

    ui16 tiff_bits_per_sample = 0;
    ui16 tiff_samples_per_pixel = 0;
    TIFFGetField(tiff_handle, TIFFTAG_BITSPERSAMPLE, &tiff_bits_per_sample);
    TIFFGetField(tiff_handle, TIFFTAG_SAMPLESPERPIXEL, &tiff_samples_per_pixel);
    // some TIFs have tiff_samples_per_pixel=0 when it is a single channel 
    // image - set to 1
    tiff_samples_per_pixel = 
      (tiff_samples_per_pixel < 1) ? 1 : tiff_samples_per_pixel;

    ui16 tiff_planar_configuration = 0;
    ui16 tiff_photometric = 0;
    TIFFGetField(tiff_handle, TIFFTAG_PLANARCONFIG, &tiff_planar_configuration);
    TIFFGetField(tiff_handle, TIFFTAG_PHOTOMETRIC, &tiff_photometric);

    planar_configuration = tiff_planar_configuration;

    ui16 tiff_compression = 0;
    ui32 tiff_rows_per_strip = 0;
    TIFFGetField(tiff_handle, TIFFTAG_COMPRESSION, &tiff_compression);
    TIFFGetField(tiff_handle, TIFFTAG_ROWSPERSTRIP, &tiff_rows_per_strip);

    if (tiff_planar_configuration == PLANARCONFIG_SEPARATE)
    {
      bytes_per_line = tiff_samples_per_pixel * TIFFScanlineSize64(tiff_handle);
    }
    else
    {
      bytes_per_line = TIFFScanlineSize64(tiff_handle);
    }
    // allocate linebuffer to hold a line of image data
    line_buffer = malloc(bytes_per_line);
    if (NULL == line_buffer)
      OJPH_ERROR(0x03000092, "Unable to allocate %d bytes for line_buffer[] "
        "for file %s", bytes_per_line, filename);
      
    cur_line = 0;

    // Error on known incompatilbe input formats
    if( tiff_bits_per_sample != 8 && tiff_bits_per_sample != 16 )
    {
      OJPH_ERROR(0x03000093, "\nTIFF IO is currently limited"
        " to files with TIFFTAG_BITSPERSAMPLE=8 and TIFFTAG_BITSPERSAMPLE=16 \n"
        "input file = %s has TIFFTAG_BITSPERSAMPLE=%d", 
        filename, tiff_bits_per_sample);
    }

    if( TIFFIsTiled( tiff_handle ) )
    {
      OJPH_ERROR(0x03000094, "\nTIFF IO is currently limited to TIF files "
        "without tiles. \nInput file %s has been detected as tiled", filename);
    }

    if(PHOTOMETRIC_RGB != tiff_photometric && 
       PHOTOMETRIC_MINISBLACK != tiff_photometric )
    {
      OJPH_ERROR(0x03000095, "\nTIFF IO is currently limited to "
        "TIFFTAG_PHOTOMETRIC=PHOTOMETRIC_MINISBLACK=%d and "
        "PHOTOMETRIC_RGB=%d. \nInput file %s has been detected "
        "TIFFTAG_PHOTOMETRIC=%d", 
      PHOTOMETRIC_MINISBLACK, PHOTOMETRIC_RGB, filename, tiff_photometric);
    }

    if( tiff_samples_per_pixel > 4 )
    {
      OJPH_ERROR(0x03000096, "\nTIFF IO is currently limited to "
        "TIFFTAG_SAMPLESPERPIXEL=4 \nInput file %s has been detected with "
        "TIFFTAG_SAMPLESPERPIXEL=%d",
        filename, tiff_samples_per_pixel);
    }

    // set number of components based on tiff_samples_per_pixel
    width = tiff_width;
    height = tiff_height;
    num_comps = tiff_samples_per_pixel;
    bytes_per_sample = (tiff_bits_per_sample + 7) / 8;
    for (ui32 comp_num = 0; comp_num < num_comps; comp_num++)
      bit_depth[comp_num] = tiff_bits_per_sample;

    // allocate intermediate linebuffers to hold a line of a single component 
    // of image data
    if (tiff_planar_configuration == PLANARCONFIG_SEPARATE && 
        bytes_per_sample == 1)
    {
      line_buffer_for_planar_support_uint8 = 
        (uint8_t*)calloc(width, sizeof(uint8_t));
      if (NULL == line_buffer_for_planar_support_uint8)
        OJPH_ERROR(0x03000097, "Unable to allocate %d bytes for "
          "line_buffer_for_planar_support_uint8[] for file %s", 
          width * sizeof(uint8_t), filename);
    }
    if (tiff_planar_configuration == PLANARCONFIG_SEPARATE && 
        bytes_per_sample == 2)
    {
      line_buffer_for_planar_support_uint16 = 
        (uint16_t*)calloc(width, sizeof(uint16_t));
      if (NULL == line_buffer_for_planar_support_uint16)
        OJPH_ERROR(0x03000098, "Unable to allocate %d bytes for "
          "line_buffer_for_planar_support_uint16[] for file %s", 
          width * sizeof(uint16_t), filename);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void tif_in::set_bit_depth(ui32 num_bit_depths, ui32* bit_depth)
  {
    if (num_bit_depths < 1)
      OJPH_ERROR(0x030000A1, "one or more bit_depths must be provided");
    ui32 last_bd_idx = 0;
    for (ui32 i = 0; i < 4; ++i)
    {
      ui32 bd = bit_depth[i < num_bit_depths ? i : last_bd_idx];
      last_bd_idx += last_bd_idx + 1 < num_bit_depths ? 1 : 0;

      if (bd > 32 || bd < 1)
      {
        OJPH_ERROR(0x030000A2, 
          "bit_depth = %d, this must be an integer from 1-32", bd);
      }
      this->bit_depth[i] = bd;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  ui32 tif_in::read(const line_buf* line, ui32 comp_num)
  {
    assert(bytes_per_line != 0 && tiff_handle != 0 && comp_num < num_comps);
    assert((ui32)line->size >= width);

    // do a read from the file if this is the first component and therefore 
    // the first time trying to access this line
    if (PLANARCONFIG_SEPARATE == planar_configuration && 0 == comp_num )
    {
      for (ui32 color = 0; color < num_comps; color++)
      {
        if (bytes_per_sample == 1)
        {
          TIFFReadScanline(tiff_handle, line_buffer_for_planar_support_uint8, 
            cur_line, (ui16)color);
          ui32 x = color;
          uint8_t* line_buffer_of_interleaved_components = 
            (uint8_t*)line_buffer;
          for (ui32 i = 0; i < width; i++, x += num_comps)
          {
            line_buffer_of_interleaved_components[x] = 
              line_buffer_for_planar_support_uint8[i];
          }
        }
        else if (bytes_per_sample == 2)
        {
          TIFFReadScanline(tiff_handle, line_buffer_for_planar_support_uint16, 
            cur_line, (ui16)color);
          ui32 x = color;
          ui16* line_buffer_of_interleaved_components = (ui16*)line_buffer;
          for (ui32 i = 0; i < width; i++, x += num_comps)
          {
            line_buffer_of_interleaved_components[x] = 
              line_buffer_for_planar_support_uint16[i];
          }
        }
      }
      cur_line++;
      
    }
    else if (planar_configuration == PLANARCONFIG_CONTIG && 0 == comp_num)
    {
      TIFFReadScanline(tiff_handle, line_buffer, cur_line++);
    }
    if (cur_line >= height)
    {
      cur_line = 0;
    }

    if (bytes_per_sample == 1)
    {
      const ui8* sp = (ui8*)line_buffer + comp_num;
      si32* dp = line->i32;
      if (bit_depth[comp_num] == 8)
      {
        for (ui32 i = width; i > 0; --i, sp += num_comps)
          *dp++ = (si32)*sp;
      }
      else if (bit_depth[comp_num] < 8)
      {
        // read the desired precision from the MSBs
        const int bits_to_shift = 8 - (int)bit_depth[comp_num];
        const int bit_mask = (1 << bit_depth[comp_num]) - 1;
        for (ui32 i = width; i > 0; --i, sp += num_comps)
          *dp++ = (si32) (((*sp) >> bits_to_shift) & bit_mask);
      }
      else if (bit_depth[comp_num] > 8)
      {
        const int bits_to_shift = (int)bit_depth[comp_num] - 8;
        const int bit_mask = (1 << bit_depth[comp_num]) - 1;
        for (ui32 i = width; i > 0; --i, sp += num_comps)
          *dp++ = (si32)(((*sp) << bits_to_shift) & bit_mask);
      }
    }
    else if(bytes_per_sample == 2)
    {
      if (bit_depth[comp_num] == 16)
      {
        const ui16* sp = (ui16*)line_buffer + comp_num;
        si32* dp = line->i32;
        for (ui32 i = width; i > 0; --i, sp += num_comps)
          *dp++ = (si32)*sp;
      }
      else if (bit_depth[comp_num] < 16)
      {
        // read the desired precision from the MSBs
        const int bits_to_shift = 16 - (int)bit_depth[comp_num];
        const int bit_mask = (1 << bit_depth[comp_num]) - 1;
        const ui16* sp = (ui16*)line_buffer + comp_num;
        si32* dp = line->i32;
        for (ui32 i = width; i > 0; --i, sp += num_comps)
          *dp++ = (si32)(((*sp) >> bits_to_shift) & bit_mask);
      }
      else if (bit_depth[comp_num] > 16)
      {
        const int bits_to_shift = (int)bit_depth[comp_num] - 16;
        const int bit_mask = (1 << bit_depth[comp_num]) - 1;
        const ui16* sp = (ui16*)line_buffer + comp_num;
        si32* dp = line->i32;
        for (ui32 i = width; i > 0; --i, sp += num_comps)
          *dp++ = (si32)(((*sp) << bits_to_shift) & bit_mask);
      }
      
    }

    return width;
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void tif_out::open(char* filename)
  {
    // Error on known incompatilbe output formats
    ui32 max_bitdepth = 0;
    for (ui32 c = 0; c < num_components; c++)
    {
      if (bit_depth_of_data[c] > max_bitdepth)
        max_bitdepth = bit_depth_of_data[c];
    }
    if (max_bitdepth > 16)
    {
      OJPH_WARN(0x030000B1, "TIFF output is currently limited to files "
        "with max_bitdepth = 16, the source codestream has max_bitdepth=%d"
        ", the decoded data will be truncated to 16 bits", max_bitdepth);
    }
    if (num_components > 4)
    {
      OJPH_ERROR(0x030000B2, "TIFF IO is currently limited to files with "
        "num_components=1 to 4");
    }

    assert(tiff_handle == NULL && buffer == NULL);
    if ((tiff_handle = TIFFOpen(filename, "w")) == NULL)
    {
      OJPH_ERROR(0x030000B3, "unable to open file %s for writing", filename);
    }

    buffer_size = width * (size_t)num_components * (size_t)bytes_per_sample;
    buffer = (ui8*)malloc(buffer_size);
    fname = filename;
    cur_line = 0;

    // set tiff fields

    // Write the tiff tags to the file
    TIFFSetField(tiff_handle, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tiff_handle, TIFFTAG_IMAGELENGTH, height);

    TIFFSetField(tiff_handle, TIFFTAG_BITSPERSAMPLE, bytes_per_sample * 8);
    TIFFSetField(tiff_handle, TIFFTAG_SAMPLESPERPIXEL, num_components);

    planar_configuration = PLANARCONFIG_CONTIG;
    TIFFSetField(tiff_handle, TIFFTAG_PLANARCONFIG, planar_configuration);

    if (num_components == 1)
    {
      TIFFSetField(tiff_handle, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    }
    else if (num_components == 2)
    {
      TIFFSetField(tiff_handle, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
      // possible values are EXTRASAMPLE_UNSPECIFIED = 0; 
      // EXTRASAMPLE_ASSOCALPHA = 1; EXTRASAMPLE_UNASSALPHA = 2;
      const ui16 extra_samples_description[1] = { EXTRASAMPLE_ASSOCALPHA }; 
      TIFFSetField(tiff_handle, TIFFTAG_EXTRASAMPLES, (uint16_t)1, 
        &extra_samples_description);
    }
    else if (num_components == 3)
    {
      TIFFSetField(tiff_handle, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    }
    else if (num_components == 4)
    {
      TIFFSetField(tiff_handle, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      // possible values are EXTRASAMPLE_UNSPECIFIED = 0; 
      // EXTRASAMPLE_ASSOCALPHA = 1; EXTRASAMPLE_UNASSALPHA = 2;
      const ui16 extra_samples_description[1] = { EXTRASAMPLE_ASSOCALPHA }; 
      TIFFSetField(tiff_handle, TIFFTAG_EXTRASAMPLES, (uint16_t)1, 
        &extra_samples_description);
    }
      
    TIFFSetField(tiff_handle, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tiff_handle, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    //TIFFSetField(tiff_handle, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tiff_handle, TIFFTAG_ROWSPERSTRIP, height);
    
  }

  ////////////////////////////////////////////////////////////////////////////
  void tif_out::configure(ui32 width, ui32 height, ui32 num_components,
    ui32 *bit_depth)
  {
    assert(tiff_handle == NULL); //configure before opening

    this->width = width;
    this->height = height;
    this->num_components = num_components;
    ui32 max_bitdepth = 0;
    for (ui32 c = 0; c < num_components; c++)
    {
      this->bit_depth_of_data[c] = bit_depth[c];
      if (bit_depth[c] > max_bitdepth)
        max_bitdepth = bit_depth[c];
    }

    bytes_per_sample = (max_bitdepth + 7) / 8;  // round up
    if (bytes_per_sample > 2)
    {
      // TIFF output is currently limited to files with max_bitdepth = 16, 
      // the decoded data will be truncated to 16 bits
      bytes_per_sample = 2;
    }
    samples_per_line = num_components * width;
    bytes_per_line = bytes_per_sample * (size_t)samples_per_line;

  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 tif_out::write(const line_buf* line, ui32 comp_num)
  {
    assert(tiff_handle);
    
    if (bytes_per_sample == 1)
    {
      int max_val = (1 << bit_depth_of_data[comp_num]) - 1;
      const si32* sp = line->i32;
      ui8* dp = buffer + comp_num;
      if (bit_depth_of_data[comp_num] == 8)
      {
        for (ui32 i = width; i > 0; --i, dp += num_components)
        {
          // clamp the decoded sample to the allowed range
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp = (ui8)val;
        }
      }
      else if (bit_depth_of_data[comp_num] < 8)
      {
        const int bits_to_shift = 8 - (int)bit_depth_of_data[comp_num];
        const int bit_mask = (1 << bit_depth_of_data[comp_num]) - 1;
        for (ui32 i = width; i > 0; --i, dp += num_components)
        {
          // clamp the decoded sample to the allowed range
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          // shift the decoded data so the data's MSB is aligned with the 
          // 8 bit MSB
          *dp = (ui8)((val & bit_mask) << bits_to_shift);
        }
      }
      else if (bit_depth_of_data[comp_num] > 8)
      {
        const int bits_to_shift = (int)bit_depth_of_data[comp_num] - 8;
        const int bit_mask = (1 << bit_depth_of_data[comp_num]) - 1;
        for (ui32 i = width; i > 0; --i, dp += num_components)
        {
          // clamp the decoded sample to the allowed range
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          // shift the decoded data so the data's MSB is aligned with the 
          // 8 bit MSB
          *dp = (ui8)((val >> bits_to_shift) & bit_mask);
        }
      }
      
    }
    else if(bytes_per_sample == 2)
    {
      int max_val = (1 << bit_depth_of_data[comp_num]) - 1;
      const si32* sp = line->i32;
      ui16* dp = (ui16*)buffer + comp_num;

      if (bit_depth_of_data[comp_num] == 16)
      {
        for (ui32 i = width; i > 0; --i, dp += num_components)
        {
          // clamp the decoded sample to the allowed range
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp = (ui16)val;
        }
      }
      else if (bit_depth_of_data[comp_num] < 16)
      {
        const int bits_to_shift = 16 - (int)bit_depth_of_data[comp_num];
        const int bit_mask = (1 << bit_depth_of_data[comp_num]) - 1;
        for (ui32 i = width; i > 0; --i, dp += num_components)
        {
          // clamp the decoded sample to the allowed range
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;

          // shift the decoded data so the data's MSB is aligned with the 
          // 16 bit MSB
          *dp = (ui16)((val & bit_mask) << bits_to_shift);
        }
      }
      else if (bit_depth_of_data[comp_num] > 16)
      {
        const int bits_to_shift = (int)bit_depth_of_data[comp_num] - 16;
        const int bit_mask = (1 << bit_depth_of_data[comp_num]) - 1;
        for (ui32 i = width; i > 0; --i, dp += num_components)
        {
          // clamp the decoded sample to the allowed range
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;

          // shift the decoded data so the data's MSB is aligned with the 
          // 16 bit MSB
          *dp = (ui16)((val >> bits_to_shift) & bit_mask);
        }
      }
      
    }
      // write scanline when the last component is reached 
      if (comp_num == num_components-1)
      {
        int result = TIFFWriteScanline(tiff_handle, buffer, cur_line++);
        if (result != 1)
          OJPH_ERROR(0x030000C1, "error writing to file %s", fname);
      }
    return 0;
  }
  #endif /* OJPH_ENABLE_TIFF_SUPPORT */

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void yuv_in::open(const char* filename)
  {
    assert(fh == NULL);
    fh = fopen(filename, "rb");
    if (fh == 0)
      OJPH_ERROR(0x030000D1, "Unable to open file %s", filename);

    //need to extract info from filename

    assert(num_com == 1 || num_com == 3);
    for (ui32 i = 0; i < num_com; ++i)
      bytes_per_sample[i] = bit_depth[i] > 8 ? 2 : 1;
    ui32 max_byte_width = width[0] * bytes_per_sample[0];
    comp_address[0] = 0;
    for (ui32 i = 1; i < num_com; ++i)
    {
      comp_address[i] = comp_address[i - 1];
      comp_address[i] += width[i-1] * height[i-1] * bytes_per_sample[i-1];
      max_byte_width = ojph_max(max_byte_width, width[i]*bytes_per_sample[i]);
    }
    temp_buf = malloc(max_byte_width);
    fname = filename;
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 yuv_in::read(const line_buf* line, ui32 comp_num)
  {
    assert(comp_num < num_com);
    size_t result = fread(temp_buf, bytes_per_sample[comp_num],
                          width[comp_num], fh);
    if (result != width[comp_num])
    {
      close();
      OJPH_ERROR(0x030000E1, "not enough data in file %s", fname);
    }

    if (bytes_per_sample[comp_num] == 1)
    {
      const ui8* sp = (ui8*)temp_buf;
      si32* dp = line->i32;
      for (ui32 i = width[comp_num]; i > 0; --i, ++sp)
        *dp++ = (si32)*sp;
    }
    else
    {
      const ui16* sp = (ui16*)temp_buf;
      si32* dp = line->i32;
      for (ui32 i = width[comp_num]; i > 0; --i, ++sp)
        *dp++ = (si32)*sp;
    }

    return width[comp_num];
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_in::set_img_props(const size& s, ui32 num_components,
                             ui32 num_downsamplings, const point *subsampling)
  {
    if (num_components != 1 && num_components !=3)
      OJPH_ERROR(0x030000F1, "yuv_in support 1 or 3 components");
    this->num_com = num_components;

    if (num_downsamplings < 1)
      OJPH_ERROR(0x030000F2, "one or more downsampling must be provided");

    ui32 last_downsamp_idx = 0;
    for (ui32 i = 0; i < num_components; ++i)
    {
      point cp_ds = subsampling[i<num_downsamplings ? i : last_downsamp_idx];
      last_downsamp_idx += last_downsamp_idx + 1 < num_downsamplings ? 1 : 0;

      this->subsampling[i] = cp_ds;
    }

    for (ui32 i = 0; i < num_components; ++i)
    {
      width[i] = ojph_div_ceil(s.w, this->subsampling[i].x);
      height[i] = ojph_div_ceil(s.h, this->subsampling[i].y);
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_in::set_bit_depth(ui32 num_bit_depths, ui32* bit_depth)
  {
    if (num_bit_depths < 1)
      OJPH_ERROR(0x03000101, "one or more bit_depths must be provided");
    ui32 last_bd_idx = 0;
    for (ui32 i = 0; i < 3; ++i)
    {
      ui32 bd = bit_depth[i < num_bit_depths ? i : last_bd_idx];
      last_bd_idx += last_bd_idx + 1 < num_bit_depths ? 1 : 0;

      this->bit_depth[i] = bd;
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  yuv_out::~yuv_out()
  {
    close();
    if (buffer)
    {
      free(buffer);
      buffer = NULL;
      buffer_size = 0;
    }
    if (comp_width)
    {
      delete [] comp_width;
      comp_width = NULL;
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_out::open(char *filename)
  {
    assert(fh == NULL); //configure before open
    fh = fopen(filename, "wb");
    if (fh == 0)
      OJPH_ERROR(0x03000111, "Unable to open file %s", filename);
    fname = filename;
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_out::configure(ui32 bit_depth, ui32 num_components, 
                          ui32* comp_width)
  {
    assert(fh == NULL);
    this->num_components = num_components;
    this->bit_depth = bit_depth;
    this->comp_width = new ui32[num_components];
    ui32 tw = 0;
    for (ui32 i = 0; i < num_components; ++i)
    {
      this->comp_width[i] = comp_width[i];
      tw = ojph_max(tw, this->comp_width[i]);
    }
    this->width = tw;
    buffer_size = tw * (bit_depth > 8 ? 2 : 1);
    buffer = (ui8*)malloc(buffer_size);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 yuv_out::write(const line_buf* line, ui32 comp_num)
  {
    assert(fh);
    assert(comp_num < num_components);

    int max_val = (1<<bit_depth) - 1;
    ui32 w = comp_width[comp_num];
    if (bit_depth > 8)
    {
      const si32 *sp = line->i32;
      ui16 *dp = (ui16 *)buffer;
      for (ui32 i = w; i > 0; --i)
      {
        int val = *sp++;
        val = val >= 0 ? val : 0;
        val = val <= max_val ? val : max_val;
        *dp++ = (ui16)val;
      }
      if (fwrite(buffer, 2, w, fh) != w)
        OJPH_ERROR(0x03000121, "unable to write to file %s", fname);
    }
    else
    {
      const si32 *sp = line->i32;
      ui8 *dp = (ui8 *)buffer;
      for (ui32 i = w; i > 0; --i)
      {
        int val = *sp++;
        val = val >= 0 ? val : 0;
        val = val <= max_val ? val : max_val;
        *dp++ = (ui8)val;
      }
      if (fwrite(buffer, 1, w, fh) != w)
        OJPH_ERROR(0x03000122, "unable to write to file %s", fname);
    }

    return w;
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void raw_in::open(const char* filename)
  {
    assert(fh == NULL);
    fh = fopen(filename, "rb");
    if (fh == NULL)
      OJPH_ERROR(0x03000131, "Unable to open file %s", filename);

    cur_line = 0;
    bytes_per_sample = (bit_depth + 7) >> 3;
    buffer_size = (size_t)width * bytes_per_sample;
    buffer = (ui8*)malloc(buffer_size);
    fname = filename;
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 raw_in::read(const line_buf* line, ui32 comp_num)
  {
    ojph_unused(comp_num);
    assert(comp_num == 0);
    size_t result = fread(buffer, bytes_per_sample, width, fh);
    if (result != width)
    {
      close();
      OJPH_ERROR(0x03000132, "not enough data in file %s", fname);
    }

    if (bytes_per_sample > 3)
    {
      si32* dp = line->i32;
      if (is_signed) {
        const si32* sp = (si32*)buffer;
        for (ui32 i = width; i > 0; --i, ++sp)
          *dp++ = *sp;
      }
      else {
        si32* dp = line->i32;
        const ui32* sp = (ui32*)buffer;
        for (ui32 i = width; i > 0; --i, ++sp)
          *dp++ = (si32)*sp;
      }
    }
    else if (bytes_per_sample > 2)
    {
      si32* dp = line->i32;
      if (is_signed) {
        const si32* sp = (si32*)buffer;
        for (ui32 i = width; i > 0; --i) {
          si32 val = *sp & 0xFFFFFF;
          val |= (val & 0x800000) ? 0xFF000000 : 0;
          *dp++ = val;
          // this only works for little endian architecture
          sp = (si32*)((si8*)sp + 3);
        }
      }
      else {
        const ui32* sp = (ui32*)buffer;
        for (ui32 i = width; i > 0; --i) {
          *dp++ = (si32)(*sp & 0xFFFFFFu);
          // this only works for little endian architecture
          sp = (ui32*)((ui8*)sp + 3);
        }
      }
    }
    else if (bytes_per_sample > 1)
    {
      si32* dp = line->i32;
      if (is_signed) {
        const si16* sp = (si16*)buffer;
        for (ui32 i = width; i > 0; --i, ++sp)
          *dp++ = *sp;
      }
      else {
        const ui16* sp = (ui16*)buffer;
        for (ui32 i = width; i > 0; --i, ++sp)
          *dp++ = (si32)*sp;
      }
    }
    else
    {
      si32* dp = line->i32;
      if (is_signed) {
        const si8* sp = (si8*)buffer;
        for (ui32 i = width; i > 0; --i, ++sp)
          *dp++ = *sp;
      }
      else {
        const ui8* sp = (ui8*)buffer;
        for (ui32 i = width; i > 0; --i, ++sp)
          *dp++ = (si32)*sp;
      }
    }

    return width;
  }

  ////////////////////////////////////////////////////////////////////////////
  void raw_in::set_img_props(const size& s, ui32 bit_depth, bool is_signed)
  {
    assert(fh == NULL);
    //need to extract this info from filename
    this->width = s.w;
    this->height = s.h;
    this->bit_depth = bit_depth;
    this->is_signed = is_signed;
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  raw_out::~raw_out()
  {
    close();
    if (buffer)
    {
      free(buffer);
      buffer = NULL;
      buffer_size = 0;
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  void raw_out::open(char *filename)
  {
    assert(fh == NULL); //configure before open
    fh = fopen(filename, "wb");
    if (fh == 0)
      OJPH_ERROR(0x03000141, "Unable to open file %s", filename);
    fname = filename;
  }

  ////////////////////////////////////////////////////////////////////////////
  void raw_out::configure(bool is_signed, ui32 bit_depth, ui32 width)
  {
    assert(fh == NULL);
    this->is_signed = is_signed;
    this->bit_depth = bit_depth;
    this->width = width;

    if (is_signed) { 
      upper_val = ((si64)1 << (bit_depth - 1));
      lower_val = -((si64)1 << (bit_depth - 1));
    } else {
      upper_val = (si64)1 << bit_depth;
      lower_val = (si64)0;
    }

    bytes_per_sample = (bit_depth + 7) >> 3;
    buffer_size = width * bytes_per_sample;
    buffer = (ui8*)malloc(buffer_size);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 raw_out::write(const line_buf* line, ui32 comp_num)
  {
    ojph_unused(comp_num);
    assert(fh);
    assert(comp_num == 0);

    if (is_signed) 
    {
      if (bytes_per_sample > 3)
      {
        const si32* sp = line->i32;
        si32* dp = (si32*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp++ = (si32)val;
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000151, "unable to write to file %s", fname);
      }
      else if (bytes_per_sample > 2)
      {
        const si32* sp = line->i32;
        si32* dp = (si32*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp = (si32)val;
          // this only works for little endian architecture
          dp = (si32*)((ui8*)dp + 3);
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000152, "unable to write to file %s", fname);
      }
      else if (bytes_per_sample > 1)
      {
        const si32* sp = line->i32;
        si16* dp = (si16*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp++ = (si16)val;
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000153, "unable to write to file %s", fname);
      }
      else
      {
        const si32* sp = line->i32;
        si8* dp = (si8*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp++ = (si8)val;
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000154, "unable to write to file %s", fname);
      }
    }
    else 
    {
      if (bytes_per_sample > 3)
      {
        const ui32* sp = (ui32*)line->i32;
        ui32* dp = (ui32*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp++ = (ui32)val;
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000155, "unable to write to file %s", fname);
      }
      else if (bytes_per_sample > 2)
      {
        const ui32* sp = (ui32*)line->i32;
        ui32* dp = (ui32*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp = (ui32)val;
          // this only works for little endian architecture
          dp = (ui32*)((ui8*)dp + 3);
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000156, "unable to write to file %s", fname);
      }
      else if (bytes_per_sample > 1)
      {
        const ui32* sp = (ui32*)line->i32;
        ui16* dp = (ui16*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp++ = (ui16)val;
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000157, "unable to write to file %s", fname);
      }
      else
      {
        const ui32* sp = (ui32*)line->i32;
        ui8* dp = (ui8*)buffer;
        for (ui32 i = width; i > 0; --i)
        {
          si64 val = *sp++;
          val = val < upper_val ? val : upper_val;
          val = val >= lower_val ? val : lower_val;
          *dp++ = (ui8)val;
        }
        if (fwrite(buffer, bytes_per_sample, width, fh) != width)
          OJPH_ERROR(0x03000158, "unable to write to file %s", fname);
      }
    }

    return width;
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
  
  ////////////////////////////////////////////////////////////////////////////

  void dpx_in::open(const char* filename)
  {
    assert(file_handle == 0);
    file_handle = fopen(filename, "rb");
    if (0 == file_handle)
      OJPH_ERROR(0x03000161, "Unable to open file %s", filename);
    fname = filename;

    // read magic number
    ui32 magic_number;
    if (fread(&magic_number, sizeof(ui32), 1, file_handle) != 1)
    {
      close();
      OJPH_ERROR(0x03000162, "Error reading file %s", filename);
    }

    // check magic number
    const ui32 dpx_magic_number = 0x53445058;
    if (dpx_magic_number == magic_number)
    {
      // magic number is a match - no byte swapping necessary
      is_byte_swapping_necessary = false;
    }
    else if (dpx_magic_number == be2le(magic_number))
    {
      // magic number is a match after bytes swapping - 
      // the data read from this file needs byte swapping
      is_byte_swapping_necessary = true;
    }
    else
    {
      close();
      OJPH_ERROR(0x03000163, "Error reading file %s - this does not appear "
        "to be a valid DPX file.  It has magic number = 0x%08X.  The magic "
        "number of a DPX file is 0x%08X.", filename, magic_number, 
        dpx_magic_number);
    }

    // read offset to data
    if (fread(&offset_to_image_data_in_bytes, sizeof(ui32), 1, file_handle) 
        != 1)
    {
      close();
      OJPH_ERROR(0x03000164, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      offset_to_image_data_in_bytes = be2le(offset_to_image_data_in_bytes);
    // read version
    if (fread(version, sizeof(uint8_t), 8, file_handle) != 8)
    {
      close();
      OJPH_ERROR(0x03000165, "Error reading file %s", filename);
    }
    // read image file size in bytes
    if (fread(&total_image_file_size_in_bytes, sizeof(ui32), 1, file_handle) 
        != 1)
    {
      close();
      OJPH_ERROR(0x03000166, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      total_image_file_size_in_bytes = be2le(total_image_file_size_in_bytes);
    
    // seek to image info header
    if (fseek(file_handle,768, SEEK_SET) != 0)
    {
      close();
      OJPH_ERROR(0x03000167, "Error reading file %s", filename);
    }

    // read image_orientation
    if (fread(&image_orientation, sizeof(uint16_t), 1, file_handle) != 1)
    {
      close();
      OJPH_ERROR(0x03000168, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      image_orientation = be2le(image_orientation);

    // read number of image elements
    if (fread(&number_of_image_elements, sizeof(uint16_t), 1, file_handle) 
        != 1)
    {
      close();
      OJPH_ERROR(0x03000169, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      number_of_image_elements = be2le(number_of_image_elements);

    // read pixels per line
    if (fread(&pixels_per_line, sizeof(ui32), 1, file_handle) != 1)
    {
      close();
      OJPH_ERROR(0x0300016A, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      pixels_per_line = be2le(pixels_per_line);

    // read lines per image element
    if (fread(&lines_per_image_element, sizeof(ui32), 1, file_handle) != 1)
    {
      close();
      OJPH_ERROR(0x0300016B, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      lines_per_image_element = be2le(lines_per_image_element);

    // seek to data structure for image element 1
    if (fseek(file_handle, 780, SEEK_SET) != 0)
    {
      close();
      OJPH_ERROR(0x0300016C, "Error reading file %s", filename);
    }

    // read data sign for image element
    if (fread(&data_sign_for_image_element_1, sizeof(ui32), 1, file_handle) 
        != 1)
    {
      close();
      OJPH_ERROR(0x0300016E, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      data_sign_for_image_element_1 = be2le(data_sign_for_image_element_1);

    // seek to core data elements in image element 1
    if (fseek(file_handle, 800, SEEK_SET) != 0)
    {
      close();
      OJPH_ERROR(0x0300016F, "Error reading file %s", filename);
    }

    // read descriptor
    if (fread(&descriptor_for_image_element_1, sizeof(uint8_t), 1, file_handle)
        != 1)
    {
      close();
      OJPH_ERROR(0x03000170, "Error reading file %s", filename);
    }

    // read transfer characteristic
    if (fread(&transfer_characteristic_for_image_element_1, sizeof(uint8_t),
              1, file_handle) != 1)
    {
      close();
      OJPH_ERROR(0x03000171, "Error reading file %s", filename);
    }

    // read colorimetric specification
    if (fread(&colormetric_specification_for_image_element_1, sizeof(uint8_t), 
        1, file_handle) != 1)
    {
      close();
      OJPH_ERROR(0x03000172, "Error reading file %s", filename);
    }

    // read bit depth
    if (fread(&bitdepth_for_image_element_1, sizeof(uint8_t), 1, file_handle) 
        != 1)
    {
      close();
      OJPH_ERROR(0x03000173, "Error reading file %s", filename);
    }

    // read packing
    if (fread(&packing_for_image_element_1, sizeof(uint16_t), 1, file_handle) 
        != 1)
    {
      close();
      OJPH_ERROR(0x03000174, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      packing_for_image_element_1 = be2le(packing_for_image_element_1);

    // read encoding
    if (fread(&encoding_for_image_element_1, sizeof(uint16_t), 1, file_handle) 
        != 1)
    {
      close();
      OJPH_ERROR(0x03000175, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      encoding_for_image_element_1 = be2le(encoding_for_image_element_1);
      
    // read offset to data
    if (fread(&offset_to_data_for_image_element_1, sizeof(ui32), 1, 
              file_handle) != 1)
    {
      close();
      OJPH_ERROR(0x03000176, "Error reading file %s", filename);
    }
    if (is_byte_swapping_necessary)
      offset_to_data_for_image_element_1 = 
        be2le(offset_to_data_for_image_element_1);

    // set to starting point of image data
    if (fseek(file_handle, (long)offset_to_image_data_in_bytes, SEEK_SET) != 0)
    {
      close();
      OJPH_ERROR(0x03000177, "Error reading file %s", filename);
    }

    // set ojph properties
    width = pixels_per_line;
    height = lines_per_image_element;
    num_comps = 3;  // descriptor field can indicate 1, 3, or 4 comps
    for ( ojph::ui32 c = 0; c < get_num_components(); c++)
    {
      bit_depth[c] = bitdepth_for_image_element_1;
      is_signed[c] = false;
      subsampling[c] = point(1,1);
    }

    // handle DPX image data packing in file 
    ui32 number_of_samples_per_32_bit_word = 32 / bitdepth_for_image_element_1;
    number_of_samples_per_line = width * num_comps;
    number_of_32_bit_words_per_line =
      (number_of_samples_per_line + (number_of_samples_per_32_bit_word - 1))
      / number_of_samples_per_32_bit_word;

    cur_line = 0;

    // allocate linebuffer to hold a line of image data from the file
    line_buffer = malloc(number_of_32_bit_words_per_line * sizeof(ui32) );
    if (NULL == line_buffer)
      OJPH_ERROR(0x03000178, "Unable to allocate %d bytes for line_buffer[] "
        "for file %s", 
        number_of_32_bit_words_per_line * sizeof(ui32), filename);

    // allocate line_buffer_16bit_samples to hold a line of image data in memory
    line_buffer_16bit_samples = 
      (ui16*) malloc((size_t)width * num_comps * sizeof(ui16));
    if (NULL == line_buffer_16bit_samples)
      OJPH_ERROR(0x03000179, "Unable to allocate %d bytes for "
        "line_buffer_16bit_samples[] for file %s", 
        (size_t)width * num_comps * sizeof(ui16), filename);

    cur_line = 0;

    return;
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 dpx_in::read(const line_buf* line, ui32 comp_num)
  {
    assert(file_handle != 0 && comp_num < num_comps);
    assert((ui32)line->size >= width);

    // read from file if trying to read the first component
    if (0 == comp_num)
    {
      if (fread(line_buffer, sizeof(ui32), number_of_32_bit_words_per_line, 
          file_handle) != number_of_32_bit_words_per_line)
      {
        close();
        OJPH_ERROR(0x03000181, "Error reading file %s", fname);
      }

      if (true == is_byte_swapping_necessary)
      {
        if (16 == bitdepth_for_image_element_1)
        {
          ui16* line_buffer_ptr = (ui16*)line_buffer;
          for (size_t i = 0; i < 2*number_of_32_bit_words_per_line; i++)
          {
            line_buffer_ptr[i] = be2le(line_buffer_ptr[i]);
          }
        }
        else
        {
          ui32* line_buffer_ptr = (ui32*)line_buffer;
          for (size_t i = 0; i < number_of_32_bit_words_per_line; i++)
          {
            line_buffer_ptr[i] = be2le(line_buffer_ptr[i]);
          }
        }
      }

      // extract samples from 32bit words from file read into 
      // RGB ordered buffer
      ui32 word_index = 0;
      if (10 == bitdepth_for_image_element_1 && 3 == num_comps 
          && packing_for_image_element_1 == 1)
      {
        ui32* line_buffer_ptr = (ui32*)line_buffer;
        for (ui32 i = 0; i < number_of_samples_per_line; i += 3)
        {
          // R
          line_buffer_16bit_samples[i + 0] = 
            (ui16) ((line_buffer_ptr[word_index] & 0xFFC00000) >> 22);
          // G
          line_buffer_16bit_samples[i + 1] = 
            (ui16) ((line_buffer_ptr[word_index] & 0x003FF000) >> 12);
          // B
          line_buffer_16bit_samples[i + 2] = 
            (ui16) ((line_buffer_ptr[word_index] & 0x00000FFC) >>  2);
          word_index++;
        }
      }
      else if (16 == bitdepth_for_image_element_1 && 3 == num_comps)
      {
        ui16* line_buffer_ptr = (ui16*)line_buffer;
        for (ui32 i = 0; i < number_of_samples_per_line; i++)
        {
          line_buffer_16bit_samples[i] = line_buffer_ptr[i];
        }
      }
      else
      {
        OJPH_ERROR(0x03000182, "file %s uses DPX image formats that are not "
          "yet supported by this software\n bitdepth_for_image_element_1 = "
          "%d\n num_comps=%d\npacking_for_image_element_1=%d\n "
          "descriptor_for_image_element_1=%d", fname, 
          bitdepth_for_image_element_1, num_comps, 
          packing_for_image_element_1, descriptor_for_image_element_1);
      }
      
      cur_line++;
    }

    // copy sample data from the unpacked line buffer into a 
    // single-component buffer to be used by the openjph core
    const ui16* sp = (ui16*)line_buffer_16bit_samples + comp_num;
    si32* dp = line->i32;
    for (ui32 i = width; i > 0; --i, sp += num_comps)
      *dp++ = (si32)*sp;

    return width;
  }

} 
