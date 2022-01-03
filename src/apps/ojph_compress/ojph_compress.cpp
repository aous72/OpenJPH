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
// File: ojph_compress.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <ctime>
#include <iostream>

#include "ojph_arg.h"
#include "ojph_mem.h"
#include "ojph_img_io.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "ojph_message.h"

/////////////////////////////////////////////////////////////////////////////
struct size_list_interpreter : public ojph::cli_interpreter::arg_inter_base
{
  size_list_interpreter(const int max_num_elements, int& num_elements,
                        ojph::size* list)
  : max_num_eles(max_num_elements), sizelist(list), num_eles(num_elements)
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

      if (*next_char != '{')
        throw "size must start with {";
      next_char++;
      char *endptr;
      sizelist[num_eles].w = (ojph::ui32)strtoul(next_char, &endptr, 10);
      if (endptr == next_char)
        throw "size number is improperly formatted";
      next_char = endptr;
      if (*next_char != ',')
        throw "size must have a "","" between the two numbers";
      next_char++;
      sizelist[num_eles].h = (ojph::ui32)strtoul(next_char, &endptr, 10);
      if (endptr == next_char)
        throw "number is improperly formatted";
      next_char = endptr;
      if (*next_char != '}')
        throw "size must end with }";
      next_char++;

      ++num_eles;
    }
    while (*next_char == ',' && num_eles < max_num_eles);
    if (num_eles < max_num_eles)
    {
      if (*next_char)
        throw "size elements must separated by a "",""";
    }
    else if (*next_char)
        throw "there are too many elements in the size list";
  }

  const int max_num_eles;
  ojph::size* sizelist;
  int& num_eles;
};

/////////////////////////////////////////////////////////////////////////////
struct point_list_interpreter : public ojph::cli_interpreter::arg_inter_base
{
  point_list_interpreter(const ojph::ui32 max_num_elements, 
                         ojph::ui32& num_elements,
                         ojph::point* list)
  : max_num_eles(max_num_elements), pointlist(list), num_eles(num_elements)
  { }

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

      if (*next_char != '{')
        throw "size must start with {";
      next_char++;
      char *endptr;
      pointlist[num_eles].x = (ojph::ui32)strtoul(next_char, &endptr, 10);
      if (endptr == next_char)
        throw "point number is improperly formatted";
      next_char = endptr;
      if (*next_char != ',')
        throw "point must have a "","" between the two numbers";
      next_char++;
      pointlist[num_eles].y = (ojph::ui32)strtoul(next_char, &endptr, 10);
      if (endptr == next_char)
        throw "number is improperly formatted";
      next_char = endptr;
      if (*next_char != '}')
        throw "point must end with }";
      next_char++;

      ++num_eles;
    }
    while (*next_char == ',' && num_eles < max_num_eles);
    if (num_eles < max_num_eles)
    {
      if (*next_char)
        throw "size elements must separated by a "",""";
    }
    else if (*next_char)
        throw "there are too many elements in the size list";
  }

  const ojph::ui32 max_num_eles;
  ojph::point* pointlist;
  ojph::ui32& num_eles;
};

/////////////////////////////////////////////////////////////////////////////
struct size_interpreter : public ojph::cli_interpreter::arg_inter_base
{
  size_interpreter(ojph::size& val) : val(val) {}
  virtual void operate(const char *str)
  {
    const char *next_char = str;
    if (*next_char != '{')
      throw "size must start with {";
    next_char++;
    char *endptr;
    val.w = (ojph::ui32)strtoul(next_char, &endptr, 10);
    if (endptr == next_char)
      throw "size number is improperly formatted";
    next_char = endptr;
    if (*next_char != ',')
      throw "size must have a "","" between the two numbers";
    next_char++;
    val.h = (ojph::ui32)strtoul(next_char, &endptr, 10);
    if (endptr == next_char)
      throw "number is improperly formatted";
    next_char = endptr;
    if (*next_char != '}')
      throw "size must end with }";
    next_char++;
    if (*next_char != '\0') //must be end of string
      throw "size has extra characters";
  }
  ojph::size& val;
};

/////////////////////////////////////////////////////////////////////////////
struct point_interpreter : public ojph::cli_interpreter::arg_inter_base
{
  point_interpreter(ojph::point& val) : val(val) {}
  virtual void operate(const char *str)
  {
    const char *next_char = str;
    if (*next_char != '{')
      throw "size must start with {";
    next_char++;
    char *endptr;
    val.x = (ojph::ui32)strtoul(next_char, &endptr, 10);
    if (endptr == next_char)
      throw "size number is improperly formatted";
    next_char = endptr;
    if (*next_char != ',')
      throw "size must have a "","" between the two numbers";
    next_char++;
    val.y = (ojph::ui32)strtoul(next_char, &endptr, 10);
    if (endptr == next_char)
      throw "number is improperly formatted";
    next_char = endptr;
    if (*next_char != '}')
      throw "size must end with }";
    next_char++;
    if (*next_char != '\0') //must be end of string
      throw "size has extra characters";
  }
  ojph::point& val;
};


/////////////////////////////////////////////////////////////////////////////
struct ui32_list_interpreter : public ojph::cli_interpreter::arg_inter_base
{
  ui32_list_interpreter(const ojph::ui32 max_num_elements, 
                        ojph::ui32& num_elements,
                        ojph::ui32* list)
  : max_num_eles(max_num_elements), ui32list(list), num_eles(num_elements)
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
      ui32list[num_eles] = (ojph::ui32)strtoul(next_char, &endptr, 10);
      if (endptr == next_char)
        throw "size number is improperly formatted";
      next_char = endptr;
      ++num_eles;
    }
    while (*next_char == ',' && num_eles < max_num_eles);
    if (num_eles < max_num_eles)
    {
      if (*next_char)
        throw "list elements must separated by a "",""";
    }
    else if (*next_char)
        throw "there are too many elements in the size list";
  }

  const ojph::ui32 max_num_eles;
  ojph::ui32* ui32list;
  ojph::ui32& num_eles;
};

/////////////////////////////////////////////////////////////////////////////
struct si32_to_bool_list_interpreter
: public ojph::cli_interpreter::arg_inter_base
{
  si32_to_bool_list_interpreter(const ojph::ui32 max_num_elements, 
                                ojph::ui32& num_elements,
                                ojph::si32* list)
  : max_num_eles(max_num_elements), boollist(list), num_eles(num_elements) {}

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
      if (strncmp(next_char, "true", 4) == 0)
      {
        boollist[num_eles] = 1;
        next_char += 4;
      }
      else if (strncmp(next_char, "false", 5) == 0)
      {
        boollist[num_eles] = 0;
        next_char += 5;
      }
      else
        throw "unknown bool value";
      ++num_eles;
    }
    while (*next_char == ',' && num_eles < max_num_eles);
    if (num_eles < max_num_eles)
    {
      if (*next_char)
        throw "size elements must separated by a "",""";
    }
    else if (*next_char)
        throw "there are too many elements in the size list";
  }

  ojph::ui32 get_num_elements() { return num_eles; }

  const ojph::ui32 max_num_eles;
  ojph::si32* boollist;
  ojph::ui32& num_eles;
};



//////////////////////////////////////////////////////////////////////////////
bool get_arguments(int argc, char *argv[], char *&input_filename,
                   char *&output_filename, char *&progression_order,
                   char *&profile_string, ojph::ui32 &num_decompositions,
                   float &quantization_step, bool &reversible,
                   int &employ_color_transform,
                   const int max_num_precincts, int &num_precincts,
                   ojph::size *precinct_size, ojph::size& block_size,
                   ojph::size& dims, ojph::point& image_offset,
                   ojph::size& tile_size, ojph::point& tile_offset,
                   ojph::ui32& max_num_comps, ojph::ui32& num_comps,
                   ojph::ui32& num_comp_downsamps, ojph::point*& comp_downsamp,
                   ojph::ui32& num_bit_depths, ojph::ui32*& bit_depth,
                   ojph::ui32& num_is_signed, ojph::si32*& is_signed)
{
  ojph::cli_interpreter interpreter;
  interpreter.init(argc, argv);

  interpreter.reinterpret("-i", input_filename);
  interpreter.reinterpret("-o", output_filename);
  interpreter.reinterpret("-prog_order", progression_order);
  interpreter.reinterpret("-profile", profile_string);
  interpreter.reinterpret("-num_decomps", num_decompositions);
  interpreter.reinterpret("-qstep", quantization_step);
  interpreter.reinterpret("-reversible", reversible);
  interpreter.reinterpret_to_bool("-colour_trans", employ_color_transform);
  interpreter.reinterpret("-num_comps", num_comps);

  size_interpreter block_interpreter(block_size);
  size_interpreter dims_interpreter(dims);
  size_list_interpreter sizelist(max_num_precincts, num_precincts,
                                 precinct_size);

  if (num_comps > 255)
    throw "more than 255 components is not supported";
  if (num_comps > max_num_comps)
  {
    max_num_comps = num_comps;
    comp_downsamp = new ojph::point[num_comps];
    bit_depth = new ojph::ui32[num_comps];
    is_signed = new ojph::si32[num_comps];
    for (ojph::ui32 i = 0; i < num_comps; ++i)
    {
      comp_downsamp[i] = ojph::point(0, 0);
      bit_depth[i] = 0;
      is_signed[i] = -1;
    }
  }

  point_list_interpreter pointlist(max_num_comps, num_comp_downsamps,
                                   comp_downsamp);
  ui32_list_interpreter ilist(max_num_comps, num_bit_depths, bit_depth);
  si32_to_bool_list_interpreter blist(max_num_comps, num_is_signed, is_signed);
  point_interpreter img_off_interpreter(image_offset);
  size_interpreter tile_size_interpreter(tile_size);
  point_interpreter tile_off_interpreter(tile_offset);

  try
  {
    interpreter.reinterpret("-block_size", &block_interpreter);
    interpreter.reinterpret("-dims", &dims_interpreter);
    interpreter.reinterpret("-image_offset", &img_off_interpreter);
    interpreter.reinterpret("-tile_size", &tile_size_interpreter);
    interpreter.reinterpret("-tile_offset", &tile_off_interpreter);
    interpreter.reinterpret("-precincts", &sizelist);
    interpreter.reinterpret("-downsamp", &pointlist);
    interpreter.reinterpret("-bit_depth", &ilist);
    interpreter.reinterpret("-signed", &blist);
  }
  catch (const char *s)
  {
    printf("%s\n",s);
    return false;
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

//////////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[]) {
  char *input_filename = NULL;
  char *output_filename = NULL;
  char prog_order_store[] = "RPCL";
  char *prog_order = prog_order_store;
  char profile_string_store[] = "";
  char *profile_string = profile_string_store;
  ojph::ui32 num_decompositions = 5;
  float quantization_step = -1.0f;
  bool reversible = false;
  int employ_color_transform = -1;

  const int max_precinct_sizes = 33; //maximum number of decompositions is 32
  ojph::size precinct_size[max_precinct_sizes];
  int num_precincts = -1;

  ojph::size block_size(64,64);
  ojph::size dims(0, 0);
  ojph::size tile_size(0, 0);
  ojph::point tile_offset(0, 0);
  ojph::point image_offset(0, 0);
  const ojph::ui32 initial_num_comps = 4;
  ojph::ui32 max_num_comps = initial_num_comps;
  ojph::ui32 num_components = 0;
  ojph::ui32 num_is_signed = 0;
  ojph::si32 is_signed_store[initial_num_comps] = {-1, -1, -1, -1};
  ojph::si32 *is_signed = is_signed_store;
  ojph::ui32 num_bit_depths = 0;
  ojph::ui32 bit_depth_store[initial_num_comps] = {0, 0, 0, 0};
  ojph::ui32 *bit_depth = bit_depth_store;
  ojph::ui32 num_comp_downsamps = 0;
  ojph::point downsampling_store[initial_num_comps];
  ojph::point *comp_downsampling = downsampling_store;

  if (argc <= 1) {
    std::cout <<
    "\nThe following arguments are necessary:\n"
#ifdef OJPH_ENABLE_TIFF_SUPPORT
    " -i input file name (either pgm, ppm, tif(f), or raw(yuv))\n"
#else
    " -i input file name (either pgm, ppm, or raw(yuv))\n"
#endif // !OJPH_ENABLE_TIFF_SUPPORT
    " -o output file name\n\n"

    "The following option has a default value (optional):\n"
    " -num_decomps  (5) number of decompositions\n"
    " -qstep        (0.00001...0.5) quantization step size for lossy\n"
    "               compression; quantization steps size for all subbands are\n"
    "               derived from this value. {The default value for 8bit\n"
    "               images is 0.0039}\n"
    " -reversible   (false) for irreversible; this should be false to perform\n"
    "               lossy compression using the 9/7 wavelet transform;\n" 
    "               or true to perform reversible compression, where\n"
    "               the 5/3 wavelet is employed with lossless compression.\n"
    " -colour_trans (true) this option employs a color transform, to\n"
    "               transform RGB color images into the YUV domain.\n"
    "               This option should not be used with YUV images, because\n"
    "               they have already been transformed.\n"
    "               If there are three color components that are\n"
    "               downsampled by the same amount then the color transform\n"
    "               can be true or false. This option is also available\n"
    "               when there are more than three colour components,\n"
    "               where it is applied to the first three colour\n"
    "               components.\n"
    "               it has already been applied to convert the original RGB\n"
    "               or whatever the original format to YUV.\n"
    " -prog_order   (RPCL) is the progression order, and can be one of:\n"
    "               LRCP, RLCP, RPCL, PCRL, CPRL\n"
    " -block_size   {x,y} (64,64) where x and y are the height and width of\n"
    "               a codeblock. In unix-like environment, { and } must be\n"
    "               proceeded by a ""\\""\n"
    " -precincts    {x,y},{x,y},...,{x,y} where {x,y} is the precinct size\n"
    "               starting from the coarest resolution; the last precinct\n"
    "               is repeated for all finer resolutions\n"
    " -tile_offset  {x,y} tile offset. \n"
    " -tile_size    {x,y} tile width and height. \n"
    " -image_offset {x,y} image offset from origin. \n"
    " -profile      (None) is the profile, the code will check if the \n"
    "               selected options meet the profile.  Currently only \n"
    "               BROADCAST and IMF are supported\n"
    "\n"

    "When the input file is a YUV file, these arguments need to be \n"
    " supplied: \n"
    " -dims      {x,y} x is image width, y is height\n"
    " -num_comps number of components\n"
    " -signed    a comma-separated list of true or false parameters, one\n"
    "            for each component; for example: true,false,false\n"
    " -bit_depth a comma-separated list of bit depth values, one per \n"
    "            component; for example: 12,10,10\n"
    " -downsamp  {x,y},{x,y},...,{x,y} a list of x,y points, one for each\n"
    "            component; for example {1,1},{2,2},{2,2}\n\n"
    ;
    return -1;
  }
  if (!get_arguments(argc, argv, input_filename, output_filename,
                     prog_order, profile_string, num_decompositions,
                     quantization_step, reversible, employ_color_transform,
                     max_precinct_sizes, num_precincts, precinct_size,
                     block_size, dims, image_offset, tile_size, tile_offset,
                     max_num_comps, num_components,
                     num_comp_downsamps, comp_downsampling,
                     num_bit_depths, bit_depth, num_is_signed, is_signed))
  {
    return -1;
  }

  clock_t begin = clock();

  try
  {
    ojph::codestream codestream;

    ojph::ppm_in ppm;
    ojph::yuv_in yuv;
#ifdef OJPH_ENABLE_TIFF_SUPPORT
    ojph::tif_in tif;
#endif // !OJPH_ENABLE_TIFF_SUPPORT
    ojph::image_in_base *base = NULL;
    if (input_filename == NULL)
      OJPH_ERROR(0x01000007, "please specify an input file name using"
        " the -i command line option");
    if (output_filename == NULL)
      OJPH_ERROR(0x01000008, "please specify an output file name using"
        " the -o command line option");
    const char *v = get_file_extension(input_filename);

    if (v)
    {
      if (strncmp(".pgm", v, 4) == 0)
      {
        ppm.open(input_filename);
        ojph::param_siz siz = codestream.access_siz();
        siz.set_image_extent(ojph::point(image_offset.x + ppm.get_width(),
          image_offset.y + ppm.get_height()));
        ojph::ui32 num_comps = ppm.get_num_components();
        assert(num_comps == 1);
        siz.set_num_components(num_comps);
        for (ojph::ui32 c = 0; c < num_comps; ++c)
          siz.set_component(c, ppm.get_comp_subsampling(c),
            ppm.get_bit_depth(c), ppm.get_is_signed(c));
        siz.set_image_offset(image_offset);
        siz.set_tile_size(tile_size);
        siz.set_tile_offset(tile_offset);

        ojph::param_cod cod = codestream.access_cod();
        cod.set_num_decomposition(num_decompositions);
        cod.set_block_dims(block_size.w, block_size.h);
        if (num_precincts != -1)
          cod.set_precinct_size(num_precincts, precinct_size);
        cod.set_progression_order(prog_order);
        cod.set_color_transform(false);
        cod.set_reversible(reversible);
        if (!reversible && quantization_step != -1.0f)
          codestream.access_qcd().set_irrev_quant(quantization_step);
        if (profile_string[0] != '\0')
          codestream.set_profile(profile_string);

        if (employ_color_transform != -1)
          OJPH_WARN(0x01000001,
            "-colour_trans option is not needed and was not used\n");
        if (dims.w != 0 || dims.h != 0)
          OJPH_WARN(0x01000002,
            "-dims option is not needed and was not used\n");
        if (num_components != 0)
          OJPH_WARN(0x01000003,
            "-num_comps is not needed and was not used\n");
        if (is_signed[0] != -1)
          OJPH_WARN(0x01000004,
            "-signed is not needed and was not used\n");
        if (bit_depth[0] != 0)
          OJPH_WARN(0x01000005,
            "-bit_depth is not needed and was not used\n");
        if (comp_downsampling[0].x != 0 || comp_downsampling[0].y != 0)
          OJPH_WARN(0x01000006,
            "-downsamp is not needed and was not used\n");

        base = &ppm;
      }
      else if (strncmp(".ppm", v, 4) == 0)
      {
        ppm.open(input_filename);
        ojph::param_siz siz = codestream.access_siz();
        siz.set_image_extent(ojph::point(image_offset.x + ppm.get_width(),
          image_offset.y + ppm.get_height()));
        ojph::ui32 num_comps = ppm.get_num_components();
        assert(num_comps == 3);
        siz.set_num_components(num_comps);
        for (ojph::ui32 c = 0; c < num_comps; ++c)
          siz.set_component(c, ppm.get_comp_subsampling(c),
            ppm.get_bit_depth(c), ppm.get_is_signed(c));
        siz.set_image_offset(image_offset);
        siz.set_tile_size(tile_size);
        siz.set_tile_offset(tile_offset);

        ojph::param_cod cod = codestream.access_cod();
        cod.set_num_decomposition(num_decompositions);
        cod.set_block_dims(block_size.w, block_size.h);
        if (num_precincts != -1)
          cod.set_precinct_size(num_precincts, precinct_size);
        cod.set_progression_order(prog_order);
        if (employ_color_transform == -1)
          cod.set_color_transform(true);
        else
          cod.set_color_transform(employ_color_transform == 1);
        cod.set_reversible(reversible);
        if (!reversible && quantization_step != -1.0f)
          codestream.access_qcd().set_irrev_quant(quantization_step);
        codestream.set_planar(false);
        if (profile_string[0] != '\0')
          codestream.set_profile(profile_string);

        if (dims.w != 0 || dims.h != 0)
          OJPH_WARN(0x01000011,
            "-dims option is not needed and was not used\n");
        if (num_components != 0)
          OJPH_WARN(0x01000012,
            "-num_comps is not needed and was not used\n");
        if (is_signed[0] != -1)
          OJPH_WARN(0x01000013,
            "-signed is not needed and was not used\n");
        if (bit_depth[0] != 0)
          OJPH_WARN(0x01000014,
            "-bit_depth is not needed and was not used\n");
        if (comp_downsampling[0].x != 0 || comp_downsampling[0].y != 0)
          OJPH_WARN(0x01000015,
            "-downsamp is not needed and was not used\n");

        base = &ppm;
      }
#ifdef OJPH_ENABLE_TIFF_SUPPORT
      else if (strncmp(".tif", v, 4) == 0 || strncmp(".tiff", v, 5) == 0)
      {
        tif.open(input_filename);
        ojph::param_siz siz = codestream.access_siz();
        siz.set_image_extent(ojph::point(image_offset.x + tif.get_size().w,
          image_offset.y + tif.get_size().h));
        ojph::ui32 num_comps = tif.get_num_components();
        siz.set_num_components(num_comps);
        if(num_bit_depths > 0 )
          tif.set_bit_depth(num_bit_depths, bit_depth);
        for (ojph::ui32 c = 0; c < num_comps; ++c)
          siz.set_component(c, tif.get_comp_subsampling(c),
            tif.get_bit_depth(c), tif.get_is_signed(c));
        siz.set_image_offset(image_offset);
        siz.set_tile_size(tile_size);
        siz.set_tile_offset(tile_offset);

        ojph::param_cod cod = codestream.access_cod();
        cod.set_num_decomposition(num_decompositions);
        cod.set_block_dims(block_size.w, block_size.h);
        if (num_precincts != -1)
          cod.set_precinct_size(num_precincts, precinct_size);
        cod.set_progression_order(prog_order);
        if (employ_color_transform == -1 && num_comps >= 3)
          cod.set_color_transform(true);
        else
          cod.set_color_transform(employ_color_transform == 1);
        cod.set_reversible(reversible);
        if (!reversible && quantization_step != -1)
          codestream.access_qcd().set_irrev_quant(quantization_step);
        codestream.set_planar(false);
        if (profile_string[0] != '\0')
          codestream.set_profile(profile_string);

        if (dims.w != 0 || dims.h != 0)
          OJPH_WARN(0x01000061,
            "-dims option is not needed and was not used\n");
        if (num_components != 0)
          OJPH_WARN(0x01000062,
            "-num_comps is not needed and was not used\n");
        if (is_signed[0] != -1)
          OJPH_WARN(0x01000063,
            "-signed is not needed and was not used\n");
        if (comp_downsampling[0].x != 0 || comp_downsampling[0].y != 0)
          OJPH_WARN(0x01000065,
            "-downsamp is not needed and was not used\n");

        base = &tif;
      }
#endif // !OJPH_ENABLE_TIFF_SUPPORT
      else if (strncmp(".yuv", v, 4) == 0 || strncmp(".raw", v, 4) == 0)
      {
        ojph::param_siz siz = codestream.access_siz();
        if (dims.w == 0 || dims.h == 0)
          OJPH_ERROR(0x01000021,
            "-dims option must have positive dimensions\n");
        siz.set_image_extent(ojph::point(image_offset.x + dims.w,
          image_offset.y + dims.h));
        if (num_components <= 0)
          OJPH_ERROR(0x01000022,
            "-num_comps option is missing and must be provided\n");
        if (num_is_signed <= 0)
          OJPH_ERROR(0x01000023,
            "-signed option is missing and must be provided\n");
        if (num_bit_depths <= 0)
          OJPH_ERROR(0x01000024,
            "-bit_depth option is missing and must be provided\n");
        if (num_comp_downsamps <= 0)
          OJPH_ERROR(0x01000025,
            "-downsamp option is missing and must be provided\n");

        yuv.set_img_props(dims, num_components, num_comp_downsamps,
          comp_downsampling);
        yuv.set_bit_depth(num_bit_depths, bit_depth);

        ojph::ui32 last_signed_idx = 0, last_bit_depth_idx = 0;
        ojph::ui32 last_downsamp_idx = 0;
        siz.set_num_components(num_components);
        for (ojph::ui32 c = 0; c < num_components; ++c)
        {
          ojph::point cp_ds = comp_downsampling
              [c < num_comp_downsamps ? c : last_downsamp_idx];
          last_downsamp_idx += last_downsamp_idx+1 < num_comp_downsamps ? 1:0;
          ojph::ui32 bd = bit_depth[c<num_bit_depths ? c : last_bit_depth_idx];
          last_bit_depth_idx += last_bit_depth_idx + 1 < num_bit_depths ? 1:0;
          int is = is_signed[c < num_is_signed ? c : last_signed_idx];
          last_signed_idx += last_signed_idx + 1 < num_is_signed ? 1 : 0;
          siz.set_component(c, cp_ds, bd, is == 1);
        }
        siz.set_image_offset(image_offset);
        siz.set_tile_size(tile_size);
        siz.set_tile_offset(tile_offset);

        ojph::param_cod cod = codestream.access_cod();
        cod.set_num_decomposition(num_decompositions);
        cod.set_block_dims(block_size.w, block_size.h);
        if (num_precincts != -1)
          cod.set_precinct_size(num_precincts, precinct_size);
        cod.set_progression_order(prog_order);
        if (employ_color_transform == -1)
          cod.set_color_transform(false);
        else
          OJPH_ERROR(0x01000031,
            "We currently do not support color transform on raw(yuv) files."
            " In any case, this not a normal usage scenario.  The OpenJPH "
            "library however does support that, but ojph_compress.cpp must be "
            "modified to send all lines from one component before moving to "
            "the next component;  this requires buffering components outside"
            " of the OpenJPH library");
        cod.set_reversible(reversible);
        if (!reversible && quantization_step != -1.0f)
          codestream.access_qcd().set_irrev_quant(quantization_step);
        codestream.set_planar(true);
        if (profile_string[0] != '\0')
          codestream.set_profile(profile_string);

        yuv.open(input_filename);
        base = &yuv;
      }
      else
#ifdef OJPH_ENABLE_TIFF_SUPPORT
        OJPH_ERROR(0x01000041,
          "unknown input file extension; only pgm, ppm, tif(f), or"
          " raw(yuv) are supported\n");
#else
        OJPH_ERROR(0x01000041,
          "unknown input file extension; only pgm, ppm, and raw(yuv)) are"
          " supported\n");
#endif // !OJPH_ENABLE_TIFF_SUPPORT
    }
    else
      OJPH_ERROR(0x01000051,
        "Please supply a proper input filename with a proper three-letter "
        "extension\n");

    ojph::j2c_outfile j2c_file;
    j2c_file.open(output_filename);
    codestream.write_headers(&j2c_file);

    ojph::ui32 next_comp;
    ojph::line_buf* cur_line = codestream.exchange(NULL, next_comp);
    if (codestream.is_planar())
    {
      ojph::param_siz siz = codestream.access_siz();
      for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
      {
        ojph::point p = siz.get_downsampling(c);
        ojph::ui32 height = ojph_div_ceil(siz.get_image_extent().y, p.y);
        height -= ojph_div_ceil(siz.get_image_offset().y, p.y);
        for (ojph::ui32 i = height; i > 0; --i)
        {
          assert(c == next_comp);
          base->read(cur_line, next_comp);
          cur_line = codestream.exchange(cur_line, next_comp);
        }
      }
    }
    else
    {
      ojph::param_siz siz = codestream.access_siz();
      ojph::ui32 height = siz.get_image_extent().y; 
      height -= siz.get_image_offset().y;
      for (ojph::ui32 i = 0; i < height; ++i)
      {
        for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
        {
          assert(c == next_comp);
          base->read(cur_line, next_comp);
          cur_line = codestream.exchange(cur_line, next_comp);
        }
      }
    }

    codestream.flush();
    codestream.close();
    base->close();

    if (max_num_comps != initial_num_comps)
    {
      delete[] comp_downsampling;
      delete[] bit_depth;
      delete[] is_signed;
    }
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
