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
// File: ojph_img_io.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_IMG_IO_H
#define OJPH_IMG_IO_H

#include <cstdio>
#include <cassert>

#include "ojph_base.h"
#include "ojph_defs.h"

#ifdef OJPH_ENABLE_TIFF_SUPPORT
  #include "tiffio.h"
#endif /* OJPH_ENABLE_TIFF_SUPPORT */

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  // defined elsewhere
  class mem_fixed_allocator;
  struct line_buf;

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
  class image_in_base
  {
  public:
    virtual ~image_in_base() {}
    virtual ui32 read(const line_buf* line, ui32 comp_num) = 0;
    virtual void close() {}
  };

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
  class ppm_in : public image_in_base
  {
  public:
    ppm_in(mem_fixed_allocator *p = NULL)
    {
      fh = 0;
      fname = NULL;
      alloc_p = p;
      temp_buf = NULL;
      width = height = num_comps = max_val = max_val_num_bits = 0;
      bytes_per_sample = num_ele_per_line = 0;
      temp_buf_byte_size = 0;

      cur_line = 0;
      start_of_data = 0;
      planar = false;

      bit_depth[2] = bit_depth[1] = bit_depth[0] = 0;
      is_signed[2] = is_signed[1] = is_signed[0] = false;
      subsampling[2] = subsampling[1] = subsampling[0] = point(1,1);
    }
    virtual ~ppm_in()
    {
      close();
      if (alloc_p == NULL && temp_buf)
        free(temp_buf);
    }

    void open(const char* filename);
    void finalize_alloc();
    virtual ui32 read(const line_buf* line, ui32 comp_num);
    void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }
    void set_planar(bool planar) { this->planar = planar; }

    //size get_size() { assert(fh); return size(width, height); }
    ui32 get_width() { assert(fh); return width; }
    ui32 get_height() { assert(fh); return height; }
    ui32 get_max_val() { assert(fh); return max_val; }
    ui32 get_num_components() { assert(fh); return num_comps; }
    ui32 get_bit_depth(ui32 comp_num)
    { assert(fh && comp_num < num_comps); return bit_depth[comp_num]; }
    bool get_is_signed(ui32 comp_num)
    { assert(fh && comp_num < num_comps); return is_signed[comp_num]; }
    point get_comp_subsampling(ui32 comp_num)
    { assert(fh && comp_num < num_comps); return subsampling[comp_num]; }

  private:
    FILE *fh;
    const char *fname;
    mem_fixed_allocator *alloc_p;
    void *temp_buf;
    ui32 width, height, num_comps, max_val, max_val_num_bits;
    ui32 bytes_per_sample, num_ele_per_line;
    ui32 temp_buf_byte_size;

    ui32 cur_line;
    si64 start_of_data;
    int planar;
    ui32 bit_depth[3];
    bool is_signed[3];
    point subsampling[3];
  };

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_ENABLE_TIFF_SUPPORT
  class tif_in : public image_in_base
  {
  public:
    tif_in()
    {
      tiff_handle = NULL;
      fname = NULL;
      line_buffer = NULL;
      line_buffer_for_planar_support_uint8 = NULL;
      line_buffer_for_planar_support_uint16 = NULL;

      width = height = num_comps = 0;
      bytes_per_sample = 0;

      bytes_per_line = 0;
      planar_configuration = 0;

      cur_line = 0;

      bit_depth[3] = bit_depth[2] = bit_depth[1] = bit_depth[0] = 0;
      is_signed[3] = is_signed[2] = is_signed[1] = is_signed[0] = false;
      subsampling[3] = subsampling[2] = point(1, 1);
      subsampling[1] = subsampling[0] = point(1, 1);
    }
    virtual ~tif_in()
    {
      close();
      if (line_buffer)
        free(line_buffer);
      if (line_buffer_for_planar_support_uint8)
        free(line_buffer_for_planar_support_uint8);
      if (line_buffer_for_planar_support_uint16)
        free(line_buffer_for_planar_support_uint16);
    }

    void open(const char* filename);
    virtual ui32 read(const line_buf* line, ui32 comp_num);
    void close() { 
      if (tiff_handle) { 
        TIFFClose(tiff_handle); 
        tiff_handle = NULL; 
      } 
      fname = NULL; 
    }

    size get_size() { assert(tiff_handle); return size(width, height); }
    ui32 get_num_components() { assert(tiff_handle); return num_comps; }
    void set_bit_depth(ui32 num_bit_depths, ui32* bit_depth);
    ui32 get_bit_depth(ui32 comp_num)
    {
      assert(tiff_handle && comp_num < num_comps); return bit_depth[comp_num];
    }
    bool get_is_signed(ui32 comp_num)
    {
      assert(tiff_handle && comp_num < num_comps); return is_signed[comp_num];
    }
    point get_comp_subsampling(ui32 comp_num)
    {
      assert(tiff_handle && comp_num < num_comps); return subsampling[comp_num];
    }

  private:
    TIFF* tiff_handle;
    size_t bytes_per_line;
    ui16 planar_configuration;

    const char* fname;
    void* line_buffer;
    ui8* line_buffer_for_planar_support_uint8;
    ui16* line_buffer_for_planar_support_uint16;
    ui32 width, height;
    ui32 num_comps;
    ui32 bytes_per_sample;
    ui32 cur_line;
    ui32 bit_depth[4];
    bool is_signed[4];
    point subsampling[4];
  };
#endif /* OJPH_ENABLE_TIFF_SUPPORT */

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
  class yuv_in : public image_in_base
  {
  public:
    yuv_in()
    {
      fh = NULL;
      fname = NULL;
      temp_buf = NULL;
      for (int i = 0; i < 3; ++i)
      {
        width[i] = height[i] = bit_depth[i] = 0;
        is_signed[i] = false;
        subsampling[i] = point(1,1);
        comp_address[i] = 0;
        bytes_per_sample[i] = 0;
      }
      num_com = 0;

      cur_line = 0;
      last_comp = 0;
      planar = false;
    }
    virtual ~yuv_in()
    {
      close();
      if (temp_buf)
        free(temp_buf);
    }

    void open(const char* filename);
    virtual ui32 read(const line_buf* line, ui32 comp_num);
    void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }

    void set_bit_depth(ui32 num_bit_depths, ui32* bit_depth);
    void set_img_props(const size& s, ui32 num_components,
                       ui32 num_downsampling, const point *downsampling);

    ui32 get_num_components() { assert(fh); return num_com; }
    ui32 *get_bit_depth() { assert(fh); return bit_depth; }
    bool *get_is_signed() { assert(fh); return is_signed; }
    point *get_comp_subsampling() { assert(fh); return subsampling; }

  private:
    FILE *fh;
    const char *fname;
    void *temp_buf;
    ui32 width[3], height[3], num_com;
    ui32 bytes_per_sample[3];
    ui32 comp_address[3];

    ui32 cur_line, last_comp;
    bool planar;
    ui32 bit_depth[3];
    bool is_signed[3];
    point subsampling[3];
  };

  ////////////////////////////////////////////////////////////////////////////
  // Accelerators (defined in ojph_img_io_*)
  typedef void (*conversion_fun)(const line_buf *ln0, const line_buf *ln1, 
                                 const line_buf *ln2, void *dp, 
                                 int bit_depth, int count);

  void gen_cvrt_32b1c_to_8ub1c(const line_buf *ln0, const line_buf *ln1, 
                               const line_buf *ln2, void *dp, 
                               int bit_depth, int count);
  void gen_cvrt_32b3c_to_8ub3c(const line_buf *ln0, const line_buf *ln1, 
                               const line_buf *ln2, void *dp, 
                               int bit_depth, int count);
  void gen_cvrt_32b1c_to_16ub1c_le(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   int bit_depth, int count);
  void gen_cvrt_32b3c_to_16ub3c_le(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   int bit_depth, int count);
  void gen_cvrt_32b1c_to_16ub1c_be(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   int bit_depth, int count);
  void gen_cvrt_32b3c_to_16ub3c_be(const line_buf *ln0, const line_buf *ln1, 
                                   const line_buf *ln2, void *dp, 
                                   int bit_depth, int count);

  void sse41_cvrt_32b1c_to_8ub1c(const line_buf *ln0, const line_buf *ln1, 
                                 const line_buf *ln2, void *dp, 
                                 int bit_depth, int count);
  void sse41_cvrt_32b3c_to_8ub3c(const line_buf *ln0, const line_buf *ln1, 
                                 const line_buf *ln2, void *dp, 
                                 int bit_depth, int count);
  void sse41_cvrt_32b1c_to_16ub1c_le(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     int bit_depth, int count);
  void sse41_cvrt_32b3c_to_16ub3c_le(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     int bit_depth, int count);
  void sse41_cvrt_32b1c_to_16ub1c_be(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     int bit_depth, int count);
  void sse41_cvrt_32b3c_to_16ub3c_be(const line_buf *ln0, const line_buf *ln1, 
                                     const line_buf *ln2, void *dp, 
                                     int bit_depth, int count);

  void avx2_cvrt_32b1c_to_8ub1c(const line_buf *ln0, const line_buf *ln1, 
                                const line_buf *ln2, void *dp, 
                                int bit_depth, int count);
  void avx2_cvrt_32b3c_to_8ub3c(const line_buf *ln0, const line_buf *ln1, 
                                const line_buf *ln2, void *dp, 
                                int bit_depth, int count);
  void avx2_cvrt_32b1c_to_16ub1c_le(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    int bit_depth, int count);
  void avx2_cvrt_32b1c_to_16ub1c_be(const line_buf *ln0, const line_buf *ln1, 
                                    const line_buf *ln2, void *dp, 
                                    int bit_depth, int count);

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
  class image_out_base
  {
  public:
    virtual ~image_out_base() {}
    virtual ui32 write(const line_buf* line, ui32 comp_num) = 0;
    virtual void close() {}
  };

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
  class ppm_out : public image_out_base
  {
  public:
    ppm_out()
    {
      fh = NULL;
      fname = NULL;
      buffer = NULL;
      width = height = num_components = 0;
      bit_depth = bytes_per_sample = 0;
      buffer_size = 0;
      cur_line = samples_per_line = bytes_per_line = 0;
      converter = NULL;
      lptr[0] = lptr[1] = lptr[2] = 0;
    }
    virtual ~ppm_out()
    {
      close();
      if (buffer)
        free(buffer);
    }

    void open(char* filename);
    void configure(ui32 width, ui32 height, ui32 num_components, 
                   ui32 bit_depth);
    virtual ui32 write(const line_buf* line, ui32 comp_num);
    virtual void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }

  private:
    FILE *fh;
    const char *fname;
    ui32 width, height, num_components;
    ui32 bit_depth, bytes_per_sample;
    ui8* buffer;
    ui32 buffer_size;
    ui32 cur_line, samples_per_line, bytes_per_line;
    conversion_fun converter;
    const line_buf *lptr[3];
  };

////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_ENABLE_TIFF_SUPPORT
  class tif_out : public image_out_base
  {
  public:
    tif_out()
    {
      tiff_handle = NULL;
      fname = NULL;
      buffer = NULL;
      width = height = num_components = 0;
      bytes_per_sample = 0;
	    bit_depth_of_data[0] = bit_depth_of_data[1] = 0;
      bit_depth_of_data[2] = bit_depth_of_data[3] = 0;
      buffer_size = 0;
      cur_line = samples_per_line = 0;
      bytes_per_line = 0;

      planar_configuration = 0;
    }
    virtual ~tif_out()
    {
      close();
      if (buffer)
        free(buffer);
    }

    void open(char* filename);
    void configure(ui32 width, ui32 height, ui32 num_components,
      ui32 *bit_depth);
    virtual ui32 write(const line_buf* line, ui32 comp_num);
    virtual void close() { 
      if (tiff_handle) { 
        TIFFClose(tiff_handle); 
        tiff_handle = NULL; 
      } 
      fname = NULL; 
    }

  private:
    TIFF* tiff_handle;
    size_t bytes_per_line;
    unsigned short planar_configuration;

    const char* fname;
    ui32 width, height, num_components;
    ui32 bit_depth_of_data[4]; 
    ui32 bytes_per_sample;
    ui8* buffer;
    ui32 buffer_size;
    ui32 cur_line, samples_per_line;
  };
#endif /* OJPH_ENABLE_TIFF_SUPPORT */


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////
  class yuv_out : public image_out_base
  {
  public:
    yuv_out()
    {
      fh = NULL;
      fname = NULL;
      width = num_components = 0;
      bit_depth = 0;
      comp_width = NULL;
      buffer = NULL;
      buffer_size = 0;
    }
    virtual ~yuv_out();

    void open(char* filename);
    void configure(ui32 bit_depth, ui32 num_components, ui32 *comp_width);
    virtual ui32 write(const line_buf* line, ui32 comp_num);
    virtual void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }

  private:
    FILE *fh;
    const char *fname;
    ui32 width;
    ui32 num_components;
    ui32 bit_depth;
    ui32 *comp_width;
    ui8 *buffer;
    ui32 buffer_size;
  };


}

#endif // !OJPH_IMG_IO_H
