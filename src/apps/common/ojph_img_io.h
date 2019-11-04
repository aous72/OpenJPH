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
// File: ojph_img_io.h
// Author: Aous Naman
// Date: 28 August 2019
/****************************************************************************/


#ifndef OJPH_IMG_IO_H
#define OJPH_IMG_IO_H

#include <cstdio>
#include <cassert>

#include "ojph_base.h"
#include "ojph_defs.h"

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
    virtual int read(const line_buf* line, int comp_num) = 0;
    virtual void close() {}
  };

  struct image_properties
  {
    image_properties();

    /**
     * width of image
     */
    size_t width;
    /**
     * height of image
     */
    size_t height;
    /**
     * number of components per pixel - 1 for grayscale, 3 for color/rgb
     */
    size_t num_components;
    
    /**
     * number of values in bit_depths array
     */
    size_t num_bit_depths;
    /**
     * bit depth of each component - 8 for 8 bit, 16 for 16 bit, etc
     */
    ui32 bit_depth[3];

    /**
     * number of entries in is_signed array
     */
    size_t num_signed;
    /**
     * the sign of each component - false for unsigned, true for signed data
     */
    bool is_signed[3];

    /**
     * the number of entries in downsamplings array
     */
    size_t num_downsamplings;
    /**
     * downsamplings?  not sure what this does...
     */
    point downsamplings[3];
  };

  /**
   * Implementation of reading streams of raw pixel values
   * 
   */
  class raw_in : public image_in_base
  {
  public:
    raw_in();
    virtual ~raw_in();

    /**
     * Read raw pixels from the specified file name.
     * 
     * @param image_props[in] the image properties (size, num_components, bit depth, signedness)
     * @param filename[in] the filename to read
     */
    void open(const image_properties& image_props, const char* filename);

    /**
     * Read raw pixels from the memory buffer.  Note that the object does not make a copy of the
     * data so the caller must make sure the data is not free()d until the object is close()d
     * 
     * @param image_props[in] the image properties (size, num_components, bit depth, signedness)
     * @param data[in] pointer to the memory buffer
     * @param data_size[in] size of the memory buffer
     */
    void open(const image_properties& image_props, const uint8_t *data, size_t data_size);
    
    virtual int read(const line_buf* line, int comp_num);
    /**
     * closes the object once reading is done. 
     */
    void close();

    /**
     * returns the image_properties
     */
    const image_properties& get_image_properties() const {return this->image_props;}

  private:
    uint8_t *file_data; // buffer for holding data read for file based open() method

    const uint8_t *data; // const pointer to raw pixel data
    size_t data_size; // size of raw pixel data
    const uint8_t *cur_pos; // current read position in raw pixel data

    void *temp_buf; // temporary buffer used for reading a line of raw pixel data

    image_properties image_props; // the image_properties for this raw pixel data

    int bytes_per_sample[3]; // how many bytes per sample based on bit depth per component
    int width[3], height[3]; // component width/height based on downsamplings
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
    virtual int read(const line_buf* line, int comp_num);
    void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }
    void set_plannar(bool planar) { this->planar = planar; }

    size get_size() { assert(fh); return size(width, height); }
    int get_max_val() { assert(fh); return max_val; }
    int get_num_components() { assert(fh); return num_comps; }
    ui32 get_bit_depth(int comp_num)
    { assert(fh && comp_num < num_comps); return bit_depth[comp_num]; }
    bool get_is_signed(int comp_num)
    { assert(fh && comp_num < num_comps); return is_signed[comp_num]; }
    point get_comp_subsampling(int comp_num)
    { assert(fh && comp_num < num_comps); return subsampling[comp_num]; }

  private:
    FILE *fh;
    const char *fname;
    mem_fixed_allocator *alloc_p;
    void *temp_buf;
    int width, height, num_comps, max_val, max_val_num_bits;
    int bytes_per_sample, num_ele_per_line;
    int temp_buf_byte_size;

    int cur_line;
    ui64 start_of_data;
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
    virtual int read(const line_buf* line, int comp_num);
    void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }

    void set_bit_depth(int num_bit_depths, int* bit_depth);
    void set_img_props(const size& s, int num_components,
                       int num_downsampling, const point *downsampling);

    size get_size() { assert(fh); return size(width[0], height[0]); }
    int get_num_components() { assert(fh); return num_com; }
    ui32 *get_bit_depth() { assert(fh); return bit_depth; }
    bool *get_is_signed() { assert(fh); return is_signed; }
    point *get_comp_subsampling() { assert(fh); return subsampling; }
    size get_comp_size(int c);

  private:
    FILE *fh;
    const char *fname;
    void *temp_buf;
    int width[3], height[3], num_com;
    int bytes_per_sample[3];
    int comp_address[3];

    int cur_line, last_comp;
    bool planar;
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
  class image_out_base
  {
  public:
    virtual ~image_out_base() {}
    virtual int write(const line_buf* line, int comp_num) = 0;
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
    }
    virtual ~ppm_out()
    {
      close();
      if (buffer)
        free(buffer);
    }

    void open(char* filename);
    void configure(int width, int height, int num_components, int bit_depth);
    virtual int write(const line_buf* line, int comp_num);
    virtual void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }

  private:
    FILE *fh;
    const char *fname;
    int width, height, num_components;
    int bit_depth, bytes_per_sample;
    ui8* buffer;
    int buffer_size;
    int cur_line, samples_per_line, bytes_per_line;
  };


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
      downsampling = NULL;
      comp_width = NULL;
      buffer = NULL;
      buffer_size = 0;
    }
    virtual ~yuv_out();

    void open(char* filename);
    void configure(int image_x_extent, int image_x_offset,
                   int bit_depth, int num_components, point *downsampling);
    virtual int write(const line_buf* line, int comp_num);
    virtual void close() { if(fh) { fclose(fh); fh = NULL; } fname = NULL; }

  private:
    FILE *fh;
    const char *fname;
    int width, num_components;
    int bit_depth;
    point *downsampling;
    int *comp_width;
    ui8 *buffer;
    int buffer_size;
  };




}

#endif // !OJPH_IMG_IO_H
