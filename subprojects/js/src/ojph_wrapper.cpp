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
#include "ojph_img_io.h"
#include "ojph_message.h"

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
void cpp_parse_j2c_data(j2c_t *j2c)
{
  try {
    j2c->codestream.set_planar(true);
    j2c->codestream.create();
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
signed int* cpp_pull_j2c_line(j2c_t* j2c)
{
  try {
    int comp_num;
    ojph::line_buf *line = j2c->codestream.pull(comp_num);
    return line->i32;
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
  }
  return NULL;
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
  int get_j2c_bit_depth(j2c_t* j2c, int comp_num)
  {
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < siz.get_num_components())
      return siz.get_bit_depth(comp_num);
    else
      return -1;
  }
  
  ////////////////////////////////////////////////////////////////////////////
  int get_j2c_is_signed(j2c_t* j2c, int comp_num)
  {
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < siz.get_num_components())
      return siz.is_signed(comp_num) ? 1 : 0;
    else
      return -1;
  }
  
  ////////////////////////////////////////////////////////////////////////////
  int get_j2c_num_components(j2c_t* j2c)
  {
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    return siz.get_num_components();
  }

  ////////////////////////////////////////////////////////////////////////////
  int get_j2c_downsampling_x(j2c_t* j2c, int comp_num)
  {
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < siz.get_num_components())
    { return siz.get_downsampling(comp_num).x; }
    else
    { return -1; }
  }

  ////////////////////////////////////////////////////////////////////////////
  int get_j2c_downsampling_y(j2c_t* j2c, int comp_num)
  {
    ojph::param_siz_t siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < siz.get_num_components())
    { return siz.get_downsampling(comp_num).y; }
    else
    { return -1; }
  }
  
  ////////////////////////////////////////////////////////////////////////////
  void parse_j2c_data(j2c_t *j2c)
  {
    cpp_parse_j2c_data(j2c);
  }
  
  ////////////////////////////////////////////////////////////////////////////
  signed int* pull_j2c_line(j2c_t* j2c)
  {
    return cpp_pull_j2c_line(j2c);
  }
  
  ////////////////////////////////////////////////////////////////////////////
  void release_j2c_data(j2c_t* j2c)
  {
    cpp_release_j2c_data(j2c);
  }

  //////////
  // ENCODER
  //////////

  const size_t MAX_NUM_COMPONENTS=255; // from hard coded value in ojph_compress.cpp

  struct j2c_format_t {
    int num_decomps; // 5?
    int width;
    int height;
    int num_components;
    int is_signed;
    int num_bit_depths;
    int bit_depths[4];
    int num_downsamplings;
    int downsampling_x[4];
    int downsampling_y[4];
    int reversible; // 0 = lossy, 1 = lossless
    float quantization_step; // ?
  };

  struct j2c_encoder_t {
    ojph::codestream codestream;
    ojph::mem_outfile mem_outfile;
  };

  //////////////////////////////////////////////////////////////////////////////
  j2c_encoder_t* cpp_create_j2c_encoder(void)
  {
    return new j2c_encoder_t;
  }

  j2c_format_t* cpp_create_j2c_format(void)
  {
    return new j2c_format_t;
  }

  void get_arguments(const j2c_format_t* j2c_format,
                    char *&progression_order, int &num_decompositions,
                    float &quantization_step, bool &reversible,
                    int &employ_color_transform,
                    const int max_num_precincts, int &num_precincts,
                    ojph::size *precinct_size, ojph::size& block_size,
                    ojph::size& dims, ojph::point& image_offset,
                    ojph::size& tile_size, ojph::point& tile_offset,
                    int& max_num_comps, int& num_comps,
                    int& num_comp_downsamps, ojph::point*& comp_downsamp,
                    int& num_bit_depths, ojph::si32*& bit_depth,
                    int& num_is_signed, int*& is_signed)
  {
    //progression_order - todo
    num_decompositions = j2c_format->num_decomps;
    quantization_step = j2c_format->quantization_step;
    reversible = j2c_format->reversible;
    //employ_color_transform
    num_comps = j2c_format->num_components;

    //block_size - todo
    dims = ojph::size(j2c_format->width, j2c_format->height);
    //num_precincts - todo
    //precint_size - todo

    if (num_comps > 255)
      throw "more than 255 components is not supported";
    if (num_comps > max_num_comps)
    {
      max_num_comps = num_comps;
      comp_downsamp = new ojph::point[num_comps];
      bit_depth = new ojph::si32[num_comps];
      is_signed = new int[num_comps];
      for (int i = 0; i < num_comps; ++i)
      {
        comp_downsamp[i] = ojph::point(-1, -1);
        bit_depth[i] = -1;
        is_signed[i] = -1;
      }
    }

    num_comp_downsamps = num_comps;
    num_bit_depths = num_comps;

    //block_size - todo
    dims = ojph::size(j2c_format->width, j2c_format->height);
    //image_offset
    //tile_size - todo
    //tile_offset
    //precintcs - todo
    for(int cdi=0; cdi < j2c_format->num_downsamplings; cdi++) {
      comp_downsamp[cdi] = ojph::point(j2c_format->downsampling_x[cdi], j2c_format->downsampling_y[cdi]);
    }
    for(int bdi=0; bdi < j2c_format->num_bit_depths; bdi++) {
      bit_depth[bdi] = j2c_format->bit_depths[bdi];
    }
    is_signed[0] = j2c_format->is_signed;
  }

  //////////////////////////////////////////////////////////////////////////////
  uint8_t * cpp_init_j2c_encoder(j2c_encoder_t *j2c, j2c_format_t* j2c_format, const uint8_t *data, size_t size, size_t& compressed_size)
  {
    try {
      char prog_order_store[] = "RPCL";
      char *prog_order = prog_order_store;
      int num_decompositions = 5;
      float quantization_step = -1.0;
      bool reversible = false;
      int employ_color_transform = -1;
      const int max_precinct_sizes = 33; //maximum number of decompositions is 32
      ojph::size precinct_size[max_precinct_sizes];
      int num_precints = -1;

      ojph::size block_size(64,64);
      ojph::size dims(-1, -1);
      ojph::size tile_size(0, 0);
      ojph::point tile_offset(0, 0);
      ojph::point image_offset(0, 0);
      const int initial_num_comps = 4;
      int max_num_comps = initial_num_comps;
      int num_components = -1;
      int num_is_signed = 0;
      int is_signed_store[initial_num_comps] = {-1, -1, -1, -1};
      int *is_signed = is_signed_store;
      int num_bit_depths = 0;
      ojph::si32 bit_depth_store[initial_num_comps] = {-1, -1, -1, -1};
      ojph::si32 *bit_depth = bit_depth_store;
      int num_comp_downsamps = 0;
      ojph::point downsampling_store[initial_num_comps] = { ojph::point(0,0),
        ojph::point(0,0), ojph::point(0,0), ojph::point(0,0) };
      ojph::point *comp_downsampling = downsampling_store;

      get_arguments(j2c_format,
                        prog_order, num_decompositions,
                        quantization_step, reversible, employ_color_transform,
                        max_precinct_sizes, num_precints, precinct_size,
                        block_size, dims, image_offset, tile_size, tile_offset,
                        max_num_comps, num_components,
                        num_comp_downsamps, comp_downsampling,
                        num_bit_depths, bit_depth, num_is_signed, is_signed);
      
      ojph::codestream codestream;
      ojph::raw_in raw_image;

      // BEGIN YUV BASED CODE
        ojph::param_siz_t siz = codestream.access_siz();
      if (dims.w < 0 || dims.h < 0)
        OJPH_ERROR(0x01000021,
          "width and height must be provided and >= 0\n");
      siz.set_image_extent(ojph::point(image_offset.x + dims.w,
        image_offset.y + dims.h));
      if (num_components <= 0)
        OJPH_ERROR(0x01000022,
          "num_components must be provided and > 0\n");
      //if (num_is_signed <= 0)
      // OJPH_ERROR(0x01000023,
      //    "-signed option is missing and must be provided\n");
      if (num_bit_depths <= 0)
        OJPH_ERROR(0x01000024,
          "num_bit_depths must be > 0\n");
      if (num_comp_downsamps <= 0)
        OJPH_ERROR(0x01000025,
          "num_downsamplings must be > 0\n");

      raw_image.set_img_props(dims, num_components, num_comp_downsamps,
        comp_downsampling);
      raw_image.set_bit_depth(num_bit_depths, bit_depth);

      int last_signed_idx = 0, last_bit_depth_idx = 0, last_downsamp_idx = 0;
      siz.set_num_components(num_components);
      for (int c = 0; c < num_components; ++c)
      {
        ojph::point cp_ds = comp_downsampling
            [c < num_comp_downsamps ? c : last_downsamp_idx];
        last_downsamp_idx += last_downsamp_idx+1 < num_comp_downsamps ? 1:0;
        int bd = bit_depth[c < num_bit_depths ? c : last_bit_depth_idx];
        last_bit_depth_idx += last_bit_depth_idx + 1 < num_bit_depths ? 1:0;
        int is = is_signed[c < num_is_signed ? c : last_signed_idx];
        last_signed_idx += last_signed_idx + 1 < num_is_signed ? 1 : 0;
        siz.set_component(c, cp_ds, bd, is == 1);
      }

      siz.set_image_offset(image_offset);
      siz.set_tile_size(tile_size);
      siz.set_tile_offset(tile_offset);

      ojph::param_cod_t cod = codestream.access_cod();
      cod.set_num_decomposition(num_decompositions);
      cod.set_block_dims(block_size.w, block_size.h);
      if (num_precints != -1)
        cod.set_precinct_size(num_precints, precinct_size);
      cod.set_progression_order(prog_order);
      if (employ_color_transform == -1)
        cod.set_color_transform(false);
      else
        OJPH_ERROR(0x01000031,
          "color transform not currently supported\n");

      cod.set_reversible(reversible);
      if (!reversible && quantization_step != -1)
        codestream.access_qcd().set_irrev_quant(quantization_step);
      codestream.set_planar(true);

      raw_image.open(data, size);
      // END YUV BASED CODE

      ojph::mem_outfile j2c_file;
      j2c_file.open();
      codestream.write_headers(&j2c_file);
      int next_comp;
      ojph::line_buf* cur_line = codestream.exchange(NULL, next_comp);
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
            assert(c == next_comp);
            raw_image.read(cur_line, next_comp);
            cur_line = codestream.exchange(cur_line, next_comp);
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
            assert(c == next_comp);
            raw_image.read(cur_line, next_comp);
            cur_line = codestream.exchange(cur_line, next_comp);
          }
        }
      }

      codestream.flush();
      codestream.close();
      raw_image.close();

      if (max_num_comps != initial_num_comps)
      {
        delete[] comp_downsampling;
        delete[] bit_depth;
        delete[] is_signed;
      }
      // return compressed data stream from j2c_file
      compressed_size = j2c_file.get_size();
      return j2c_file.release_data();
    }
    catch (const std::exception& e)
    {
      const char *p = e.what();
      if (strncmp(p, "ojph error", 10) != 0)
        printf("%s\n", p);
      return NULL;
    }
}

//#ifdef BUILD_TEST
#include <cstdio>
int main(int argc, const char* argv[])
{
  /*
  const char filename[] = "test.j2c";
  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  long int size = ftell(f);
  ojph::ui8 *compressed_data = (ojph::ui8*)malloc(size);
  fseek(f, 0, SEEK_SET);
  fread(compressed_data, 1, size, f);
  fclose(f);
  */
// recompress
    j2c_encoder_t* j2cIn = cpp_create_j2c_encoder();
    j2c_format_t* j2c_format = cpp_create_j2c_format();
    j2c_format->num_decomps = 5;
    j2c_format-> width = 1024;
    j2c_format-> height = 1024;
    j2c_format-> num_components = 1;
    j2c_format-> is_signed = 0;
    j2c_format->num_bit_depths = 1;
    j2c_format->bit_depths[0] = 16;
    j2c_format-> num_downsamplings = 1;
    j2c_format-> downsampling_x[0] = 1;
    j2c_format-> downsampling_y[0] = 1;
    j2c_format->reversible = 1; // 0 = lossy, 1 = lossless
    //j2c_format->quantization_step = 0.05; // ?

    size_t bytesPerSample = j2c_format->bit_depths[0] >> 3;
    printf("bytesPerSample=%d\n", bytesPerSample);
    size_t srcSize = j2c_format->height * j2c_format->width * j2c_format-> num_components *  bytesPerSample;
    printf("srcSize=%d\n", srcSize);
    uint8_t* srcin = new uint8_t[srcSize];
    for(int y=0; y < j2c_format-> height; y++) {
      unsigned short * pOut = (unsigned short*)srcin + (y * j2c_format->width * j2c_format-> num_components);
      for(int c=0; c < j2c_format-> num_components; c++) {
        for(int x =0; x < j2c_format-> width; x++) {
          *pOut = rand() % 256;//x;
          //printf("%d ", *pOut);
          pOut++;
        }
      }
    }

    size_t compressed_size;
    uint8_t *  result = cpp_init_j2c_encoder(
      j2cIn,
      j2c_format,
      (uint8_t*)srcin,
      srcSize,
      compressed_size);
    printf("compressed result = %p, size = %d\n", result, compressed_size);

  ojph::ui8 *compressed_data = result;
  long int size = compressed_size;
  
  struct j2c_t* j2c = create_j2c_data();
  init_j2c_data(j2c, compressed_data, size);
  int width = get_j2c_width(j2c);
  int height = get_j2c_height(j2c);
  int num_comps = get_j2c_num_components(j2c);
  int bit_depth = get_j2c_bit_depth(j2c, 0);
  int is_signed = get_j2c_is_signed(j2c, 0);
  printf("width = %d, height = %d\n", width, height);
  printf("num_comps = %d\n", num_comps);
  printf("bit_depth = %d\n", bit_depth);
  printf("is_signed = %d\n", is_signed);

  size_t bytesPerSampleD = bit_depth >> 3;
  printf("bytesPerSampleD=%d\n", bytesPerSampleD);
  size_t buf_size = width * height * num_comps *bytesPerSampleD;
  printf("buf_size=%d\n", buf_size);
  ojph::ui8* buf = (ojph::ui8*)malloc(buf_size * 2); // WARNING - *2 does not make any sense, but we get seg fault if we dont!!!
  parse_j2c_data(j2c);

  int shift = (bit_depth >= 8 ? bit_depth - 8 : 0) | 0;
  int half = (bit_depth > 8 ? (1<<(shift-1)) : 0) | 0;
  ojph::ui8* dst = buf;
  if (num_comps == 1)
  {
    int max_val = 65535; // TODO: Caculate
    for(int y=0; y < height; y++) {
      unsigned short * pOut = (unsigned short*)srcin + (y * width);
      ojph::si32* sp = pull_j2c_line(j2c);
      unsigned short* dp = (unsigned short*)(buf) + (y * width);
      for (int x = 0; x < width; ++x, dp++)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp = (unsigned short)val;
          if(val != *pOut) {
            printf("difference at %d,%d", x, y);
          }
          //printf("%d,%d=%d,%d\n", x, y, val, *pOut);
          pOut++;
        }
    }

    /*
    for (int y = 0; y < height; y++)
    {
      //src is an address in memory, but since we are 
      // dealing with integers (i.e, each entry is 4 bytes 
      // away from the previous sample, we need to divide by 4
      int* src = (int*)((int)pull_j2c_line(j2c) >> 2); 
      int didx = y * width * 4;
      for (int x = 0; x < width; x++)
      {
        var val = (heap[src + x] + half) >> shift;
        dst[didx + x * 4    ] = val;
        dst[didx + x * 4 + 1] = val;
        dst[didx + x * 4 + 2] = val;
        dst[didx + x * 4 + 3] = 255;
      }
    }
    */
  }
  else if (num_comps == 3)
  {
    int max_val = 255;
    for (int y = 0; y < height; y++)
    {
      unsigned short * pOut = (unsigned short*)srcin + (y * j2c_format->width * j2c_format-> num_components);
      for (int c = 0; c < num_comps; c++)
      {
        //int *src = pull_j2c_line(j2c); 
        ojph::si32* sp = pull_j2c_line(j2c);
        ojph::ui8* dp = buf + c + y * width * num_comps;

        for (int x = 0; x < width; ++x, dp+=num_comps)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0;
          val = val <= max_val ? val : max_val;
          *dp = (ojph::ui8)val;
          if(*dp != *pOut) {
            printf("difference at %d,%d", x, y);
          }
          pOut++;
        }
    }
    }

    
  }
  else {
    printf("Unsupported number of components %d\n", num_comps);
    exit(1);
  }
  
  release_j2c_data(j2c);

  /*decode_j2c_data(j2c);
  int max_val = 255;
  for (int y = 0; y < height; ++y)
  {
    for (int c = 0; c < num_components; ++c)
    {
      ojph::si32* sp = pull_j2c_line(j2c);
      ojph::ui8* dp = buf + c + y * width * num_components;
      for (int x = 0; x < width; ++x, dp+=num_components)
      {
        int val = *sp++;
        val = val >= 0 ? val : 0;
        val = val <= max_val ? val : max_val;
        *dp = (ojph::ui8)val;
      }
    }
  }
  
  release_j2c_data(j2c);

  f = fopen("test_out.ppm", "wb");
  if (num_components == 1)
    fprintf(f, "P5 %d %d %d\n", width, height, max_val);
  else
    fprintf(f, "P6 %d %d %d\n", width, height, max_val);
  fwrite(buf, width * height * num_components, 1, f);
  fclose(f);
  */

  return 0;
}
}
//#endif


