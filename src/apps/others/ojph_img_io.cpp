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
      OJPH_ERROR(0x030000001, "Unable to open file %s", filename);
    fname = filename;

    char t[2];
    if (fread(t, 1, 2, fh) != 2)
    {
      close();
      OJPH_ERROR(0x030000002, "Error reading file %s", filename);
    }

    if (t[0] != 'P' || (t[1] != '5' && t[1] != '6'))
    {
      close();
      OJPH_ERROR(0x030000003, "unknown file type for file %s", filename);
    }

    size_t len = strlen(filename);
    if (t[1] == '5' && strncmp(filename + len - 4, ".pgm", 4) != 0)
    {
      close();
      OJPH_ERROR(0x030000004, "wrong file extension, a file with "
        "keyword P5 must have a .pgm extension for file %s", filename);
    }
    if (t[1] == '6' && strncmp(filename + len - 4, ".ppm", 4) != 0)
    {
      close();
      OJPH_ERROR(0x030000005, "wrong file extension, a file with keyword P6 "
        "must have a .pgm extension fir file %s", filename);
    }

    num_comps = t[1] == '5' ? 1 : 3;
    eat_white_spaces(fh);

    if (fscanf(fh, "%d %d %d\n", &width, &height, &max_val) != 3)
    {
      close();
      OJPH_ERROR(0x030000006, "error in file format for file %s", filename);
    }
    num_ele_per_line = num_comps * width;
    bytes_per_sample = max_val > 255 ? 2 : 1;
    max_val_num_bits = 32 - count_leading_zeros(max_val);
    bit_depth[2] = bit_depth[1] = bit_depth [0] = max_val_num_bits;
    start_of_data = ojph_ftell(fh);

    if (temp_buf_byte_size < num_comps * width * bytes_per_sample)
    {
      if (alloc_p == NULL)
      {
        temp_buf_byte_size = num_comps * width * bytes_per_sample;
        if (temp_buf)
          temp_buf = realloc(temp_buf, temp_buf_byte_size);
        else
          temp_buf = malloc(temp_buf_byte_size);
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
      temp_buf = alloc_p->post_alloc_data<ui8>(num_comps * width, 0);
    else
      temp_buf = alloc_p->post_alloc_data<ui16>(num_comps * width, 0);
  }

  /////////////////////////////////////////////////////////////////////////////
  int ppm_in::read(const line_buf* line, int comp_num)
  {
    assert(temp_buf_byte_size != 0 && fh != 0 && comp_num < num_comps);
    assert((int)line->size >= width);

    if (planar || comp_num == 0)
    {
      int result = (int)fread(
        temp_buf, bytes_per_sample, num_ele_per_line, fh);
      if (result != num_ele_per_line)
      {
        close();
        OJPH_ERROR(0x030000011, "not enough data in file %s", fname);
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
      for (int i = width; i > 0; --i, sp+=num_comps)
        *dp++ = (si32)*sp;
    }
    else
    {
      const ui16* sp = (ui16*)temp_buf + comp_num;
      si32* dp = line->i32;
      for (int i = width; i > 0; --i, sp+=num_comps)
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
          OJPH_WARN(0x03000001, "file was renamed %s\n", filename);
        }
        if (strncmp(".PPM", filename + len - 4, 4) == 0)
        {
          filename[len - 2] = 'G';
          OJPH_WARN(0x03000002, "file was renamed %s\n", filename);
        }
      }
      fh = fopen(filename, "wb");
      if (fh == NULL)
        OJPH_ERROR(0x030000021,
          "unable to open file %s for writing", filename);

      fprintf(fh, "P5\n%d %d\n%d\n", width, height, (1 << bit_depth) - 1);
      buffer_size = width * bytes_per_sample;
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
          OJPH_WARN(0x03000003, "file was renamed %s\n", filename);
        }
        if (strncmp(".PGM", filename + len - 4, 4) == 0)
        {
          filename[len - 2] = 'P';
          OJPH_WARN(0x03000004, "file was renamed %s\n", filename);
        }
      }
      fh = fopen(filename, "wb");
      if (fh == NULL)
        OJPH_ERROR(0x030000022,
          "unable to open file %s for writing", filename);
      int result = //the number of written characters
        fprintf(fh, "P6\n%d %d\n%d\n", width, height, (1 << bit_depth) - 1);
      if (result == 0)
        OJPH_ERROR(0x030000023, "error writing to file %s", filename);
      buffer_size = width * num_components * bytes_per_sample;
      buffer = (ui8*)malloc(buffer_size);
    }
    fname = filename;
    cur_line = 0;
  }

  ////////////////////////////////////////////////////////////////////////////
  void ppm_out::configure(ui32 width, ui32 height, int num_components,
                          int bit_depth)
  {
    assert(fh == NULL); //configure before opening
    if (num_components != 1 && num_components != 3)
      OJPH_ERROR(0x030000031,
        "ppm supports 3 colour components, while pgm supports 1");
    this->width = width;
    this->height = height;
    this->num_components = num_components;
    this->bit_depth = bit_depth;
    bytes_per_sample = 1 + (bit_depth > 8 ? 1 : 0);
    samples_per_line = num_components * width;
    bytes_per_line = bytes_per_sample * samples_per_line;
  }

  ////////////////////////////////////////////////////////////////////////////
  int ppm_out::write(const line_buf* line, int comp_num)
  {
    assert(fh);
    if (num_components == 1)
    {
      assert(comp_num == 0);

      if (bit_depth <= 8)
      {
        int max_val = (1<<bit_depth) - 1;
        const si32 *sp = line->i32;
        ui8* dp = buffer;
        for (int i = width; i > 0; --i)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp++ = (ui8)val;
        }
      }
      else
      {
        int max_val = (1<<bit_depth) - 1;
        const si32 *sp = line->i32;
        ui16* dp = (ui16*)buffer;
        for (int i = width; i > 0; --i)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp++ = be2le((ui16) val);
        }
      }
      if ((ui32)fwrite(buffer, bytes_per_sample, width, fh) != width)
        OJPH_ERROR(0x030000041, "error writing to file %s", fname);
    }
    else
    {
      assert(num_components == 3);

      if (bit_depth <= 8)
      {
        int max_val = (1<<bit_depth) - 1;
        const si32 *sp = line->i32;
        ui8* dp = buffer + comp_num;
        for (int i = width; i > 0; --i, dp += 3)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp = (ui8) val;
        }
      }
      else
      {
        int max_val = (1<<bit_depth) - 1;
        const si32 *sp = line->i32;
        ui16* dp = (ui16*)buffer + comp_num;
        for (int i = width; i > 0; --i, dp += 3)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp = be2le((ui16) val);
        }
      }
      if (comp_num == 2)
      {
        int result = (int)fwrite(buffer,
                               bytes_per_sample, samples_per_line, fh);
        if (result != samples_per_line)
          OJPH_ERROR(0x030000042, "error writing to file %s", fname);
      }
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

  ////////////////////////////////////////////////////////////////////////////
  void yuv_in::open(const char* filename)
  {
    assert(fh == NULL);
    fh = fopen(filename, "rb");
    if (fh == 0)
      OJPH_ERROR(0x03000051, "Unable to open file %s", filename);

    //need to extract info from filename

    assert(num_com == 1 || num_com == 3);
    for (int i = 0; i < num_com; ++i)
      bytes_per_sample[i] = bit_depth[i] > 8 ? 2 : 1;
    int max_byte_width = width[0] * bytes_per_sample[0];
    comp_address[0] = 0;
    for (int i = 1; i < num_com; ++i)
    {
      comp_address[i] = comp_address[i - 1];
      comp_address[i] += width[i-1] * height[i-1] * bytes_per_sample[i-1];
      max_byte_width = ojph_max(max_byte_width, width[i]*bytes_per_sample[i]);
    }
    temp_buf = malloc(max_byte_width);
    fname = filename;
  }

  ////////////////////////////////////////////////////////////////////////////
  int yuv_in::read(const line_buf* line, int comp_num)
  {
    assert(comp_num < num_com);
    int result = (int)fread(temp_buf, bytes_per_sample[comp_num],
                          width[comp_num], fh);
    if (result != width[comp_num])
    {
      close();
      OJPH_ERROR(0x03000061, "not enough data in file %s", fname);
    }

    if (bytes_per_sample[comp_num] == 1)
    {
      const ui8* sp = (ui8*)temp_buf;
      si32* dp = line->i32;
      for (int i = width[comp_num]; i > 0; --i, ++sp)
        *dp++ = (si32)*sp;
    }
    else
    {
      const ui16* sp = (ui16*)temp_buf;
      si32* dp = line->i32;
      for (int i = width[comp_num]; i > 0; --i, ++sp)
        *dp++ = (si32)*sp;
    }

    return width[comp_num];
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_in::set_img_props(const size& s, int num_components,
                             int num_downsamplings, const point *subsampling)
  {
    if (num_components != 1 && num_components !=3)
      OJPH_ERROR(0x03000071, "yuv_in support 1 or 3 components");
    this->num_com = num_components;

    if (num_downsamplings < 1)
      OJPH_ERROR(0x03000072, "one or more downsampling must be provided");

    int last_downsamp_idx = 0;
    for (int i = 0; i < num_components; ++i)
    {
      point cp_ds = subsampling[i<num_downsamplings ? i : last_downsamp_idx];
      last_downsamp_idx += last_downsamp_idx + 1 < num_downsamplings ? 1 : 0;

      this->subsampling[i] = cp_ds;
    }

    for (int i = 0; i < num_components; ++i)
    {
      width[i] = ojph_div_ceil(s.w, this->subsampling[i].x);
      height[i] = ojph_div_ceil(s.h, this->subsampling[i].y);
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_in::set_bit_depth(int num_bit_depths, int* bit_depth)
  {
    if (num_bit_depths < 1)
      OJPH_ERROR(0x03000081, "one or more bit_depths must be provided");
    int last_bd_idx = 0;
    for (int i = 0; i < 3; ++i)
    {
      int bd = bit_depth[i < num_bit_depths ? i : last_bd_idx];
      last_bd_idx += last_bd_idx + 1 < num_bit_depths ? 1 : 0;

      this->bit_depth[i] = bd;
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  size yuv_in::get_comp_size(int c)
  {
    assert(c < num_com);
    return size(width[c], height[c]);
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
      OJPH_ERROR(0x03000091, "Unable to open file %s", filename);
    fname = filename;
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_out::configure(int image_x_extent, int image_x_offset,
                          int bit_depth, int num_components,
                          point *downsampling)
  {
    assert(fh == NULL);
    this->width = image_x_extent - image_x_offset;
    this->num_components = num_components;
    this->bit_depth = bit_depth;
    this->comp_width = new ui32[num_components];
    ui32 tw = 0;
    for (int i = 0; i < num_components; ++i)
    {
      this->comp_width[i] = ojph_div_ceil(image_x_extent, downsampling[i].x)
                          - ojph_div_ceil(image_x_offset, downsampling[i].x);
      tw = ojph_max(tw, this->comp_width[i]);
    }
    buffer_size = tw * (bit_depth > 8 ? 2 : 1);
    buffer = (ui8*)malloc(buffer_size);
  }

  ////////////////////////////////////////////////////////////////////////////
  void yuv_out::configure(int bit_depth, int num_components, ui32* comp_width)
  {
    assert(fh == NULL);
    this->num_components = num_components;
    this->bit_depth = bit_depth;
    this->comp_width = new ui32[num_components];
    ui32 tw = 0;
    for (int i = 0; i < num_components; ++i)
    {
      this->comp_width[i] = comp_width[i];
      tw = ojph_max(tw, this->comp_width[i]);
    }
    this->width = tw;
    buffer_size = tw * (bit_depth > 8 ? 2 : 1);
    buffer = (ui8*)malloc(buffer_size);
  }

  ////////////////////////////////////////////////////////////////////////////
  int yuv_out::write(const line_buf* line, int comp_num)
  {
    assert(fh);
    assert(comp_num < num_components);

    int max_val = (1<<bit_depth) - 1;
    int w = comp_width[comp_num];
    if (bit_depth > 8)
    {
      const si32 *sp = line->i32;
      ui16 *dp = (ui16 *)buffer;
      for (int i = w; i > 0; --i)
      {
        int val = *sp++;
        val = val >= 0 ? val : 0;
        val = val <= max_val ? val : max_val;
        *dp++ = (ui16)val;
      }
      if ((int)fwrite(buffer, 2, w, fh) != w)
        OJPH_ERROR(0x030000A1, "unable to write to file %s", fname);
    }
    else
    {
      const si32 *sp = line->i32;
      ui8 *dp = (ui8 *)buffer;
      for (int i = w; i > 0; --i)
      {
        int val = *sp++;
        val = val >= 0 ? val : 0;
        val = val <= max_val ? val : max_val;
        *dp++ = (ui8)val;
      }
      if ((int)fwrite(buffer, 1, w, fh) != w)
        OJPH_ERROR(0x030000A2, "unable to write to file %s", fname);
    }

    return w;
  }
}
