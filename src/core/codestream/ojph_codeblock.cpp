
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
// File: ojph_codeblock.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_codeblock.h"
#include "ojph_subband.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    void codeblock::pre_alloc(codestream *codestream,
                              const size& nominal)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      assert(byte_alignment / sizeof(ui32) > 1);
      const ui32 f = byte_alignment / sizeof(ui32) - 1;
      ui32 stride = (nominal.w + f) & ~f; // a multiple of 8
      allocator->pre_alloc_data<ui32>(nominal.h * stride, 0);
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::finalize_alloc(codestream *codestream,
                                   subband *parent, const size& nominal,
                                   const size& cb_size,
                                   coded_cb_header* coded_cb,
                                   ui32 K_max, int line_offset)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      const ui32 f = byte_alignment / sizeof(ui32) - 1;
      this->stride = (nominal.w + f) & ~f; // a multiple of 8
      this->buf_size = this->stride * nominal.h;
      this->buf = allocator->post_alloc_data<ui32>(this->buf_size, 0);

      this->nominal_size = nominal;
      this->cb_size = cb_size;
      this->parent = parent;
      this->line_offset = line_offset;
      this->cur_line = 0;
      this->delta = parent->get_delta();
      this->delta_inv = 1.0f / this->delta;
      this->K_max = K_max;
      for (int i = 0; i < 8; ++i)
        this->max_val[i] = 0;
      ojph::param_cod cod = codestream->access_cod();
      this->reversible = cod.is_reversible();
      this->resilient = codestream->is_resilient();
      this->stripe_causal = cod.get_block_vertical_causality();
      this->zero_block = false;
      this->coded_cb = coded_cb;

      this->codeblock_functions.init(reversible);
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::push(line_buf *line)
    {
      // convert to sign and magnitude and keep max_val
      const si32 *sp = line->i32 + line_offset;
      ui32 *dp = buf + cur_line * stride;
      this->codeblock_functions.tx_to_cb(sp, dp, K_max, delta_inv, cb_size.w, 
        max_val);
      ++cur_line;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::encode(mem_elastic_allocator *elastic)
    {
      ui32 mv = this->codeblock_functions.find_max_val(max_val);
      if (mv >= 1u<<(31 - K_max))
      {
        coded_cb->missing_msbs = K_max - 1;
        assert(coded_cb->missing_msbs > 0);
        assert(coded_cb->missing_msbs < K_max);
        coded_cb->num_passes = 1;
        
        this->codeblock_functions.encode_cb(buf, K_max-1, 1,
          cb_size.w, cb_size.h, stride, coded_cb->pass_length,
          elastic, coded_cb->next_coded);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::recreate(const size &cb_size, coded_cb_header* coded_cb)
    {
      assert(cb_size.h * stride <= buf_size && cb_size.w <= stride);
      this->cb_size = cb_size;
      this->coded_cb = coded_cb;
      this->cur_line = 0;
      for (int i = 0; i < 8; ++i)
        this->max_val[i] = 0;
      this->zero_block = false;
    }

    //////////////////////////////////////////////////////////////////////////
    void codeblock::decode()
    {
      if (coded_cb->pass_length[0] > 0 && coded_cb->num_passes > 0 &&
          coded_cb->next_coded != NULL)
      {
        bool result = this->codeblock_functions.decode_cb(
            coded_cb->next_coded->buf + coded_cb_header::prefix_buf_size,
            buf, coded_cb->missing_msbs, coded_cb->num_passes,
            coded_cb->pass_length[0], coded_cb->pass_length[1],
            cb_size.w, cb_size.h, stride, stripe_causal);

        if (result == false)
          {
            if (resilient == true)
              zero_block = true;
            else
              OJPH_ERROR(0x000300A1, "Error decoding a codeblock\n");
          }
      }
      else
        zero_block = true;
    }


    //////////////////////////////////////////////////////////////////////////
    void codeblock::pull_line(line_buf *line)
    {
      si32 *dp = line->i32 + line_offset;
      if (!zero_block)
      {
        //convert to sign and magnitude
        const ui32 *sp = buf + cur_line * stride;
        this->codeblock_functions.tx_from_cb(sp, dp, K_max, delta, cb_size.w);
      }
      else
        this->codeblock_functions.mem_clear(dp, cb_size.w * sizeof(*dp));
      ++cur_line;
      assert(cur_line <= cb_size.h);
    }

  }
}