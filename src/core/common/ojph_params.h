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
// File: ojph_params.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_PARAMS_H
#define OJPH_PARAMS_H

#include "ojph_arch.h"
#include "ojph_base.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //prototyping from local
  namespace local {
    struct param_siz;
    struct param_cod;
    struct param_qcd;
    struct param_qcc;
    struct param_cap;
    class codestream;
  }

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT param_siz
  {
  public:
    param_siz(local::param_siz *p) : state(p) {}

    //setters
    void set_image_extent(point extent);
    void set_tile_size(size s);
    void set_image_offset(point offset);
    void set_tile_offset(point offset);
    void set_num_components(ui32 num_comps);
    void set_component(ui32 comp_num, const point& downsampling,
                       ui32 bit_depth, bool is_signed);

    //getters
    point get_image_extent() const;
    point get_image_offset() const;
    size get_tile_size() const;
    point get_tile_offset() const;
    ui32 get_num_components() const;
    ui32 get_bit_depth(ui32 comp_num) const;
    bool is_signed(ui32 comp_num) const;
    point get_downsampling(ui32 comp_num) const;

    //deeper getters
    ui32 get_recon_width(ui32 comp_num) const;
    ui32 get_recon_height(ui32 comp_num) const;

  private:
    local::param_siz* state;
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT param_cod
  {
  public:
    param_cod(local::param_cod* p) : state(p) {}

    void set_num_decomposition(ui32 num_decompositions);
    void set_block_dims(ui32 width, ui32 height);
    void set_precinct_size(int num_levels, size* precinct_size);
    void set_progression_order(const char *name);
    void set_color_transform(bool color_transform);
    void set_reversible(bool reversible);

    ui32 get_num_decompositions() const;
    size get_block_dims() const;
    size get_log_block_dims() const;
    bool is_reversible() const;
    size get_precinct_size(ui32 level_num) const;
    size get_log_precinct_size(ui32 level_num) const;
    int get_progression_order() const;
    const char* get_progression_order_as_string() const;
    int get_num_layers() const;
    bool is_using_color_transform() const;
    bool packets_may_use_sop() const;
    bool packets_use_eph() const;
    bool get_block_vertical_causality() const;

  private:
    local::param_cod* state;
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT param_qcd
  {
  public:
    param_qcd(local::param_qcd* p) : state(p) {}

    void set_irrev_quant(float delta);

  private:
    local::param_qcd* state;
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT comment_exchange
  {
    friend class local::codestream;
  public:
    comment_exchange() : data(NULL), len(0), Rcom(0) {}
    void set_string(const char* str);
    void set_data(const char* data, ui16 len);

  private:
    const char* data;
    ui16 len;
    ui16 Rcom;
  };

}

#endif // !OJPH_PARAMS_H
