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
// File: ojph_codestream.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream.h"
#include "ojph_codestream_local.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  codestream::~codestream()
  {
    if (state)
      delete state;
    state = NULL;
  }

  ////////////////////////////////////////////////////////////////////////////
  codestream::codestream()
  {
    state = new local::codestream;
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::restart()
  {
    assert(state != NULL);
    state->restart();
  }

  ////////////////////////////////////////////////////////////////////////////
  param_siz codestream::access_siz()
  {
    return param_siz(&state->siz);
  }

  ////////////////////////////////////////////////////////////////////////////
  param_cod codestream::access_cod()
  {
    return param_cod(&state->cod);
  }

  ////////////////////////////////////////////////////////////////////////////
  param_qcd codestream::access_qcd()
  {
    return param_qcd(&state->qcd);
  }

  ////////////////////////////////////////////////////////////////////////////
  param_nlt codestream::access_nlt()
  {
    return param_nlt(&state->nlt);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::set_planar(bool planar)
  {
    state->set_planar(planar ? 1 : 0);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::set_profile(const char *s)
  {
    state->set_profile(s);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::set_tilepart_divisions(bool at_resolutions,
                                          bool at_components)
  {
    ui32 value = 0;
    if (at_resolutions)
      value |= OJPH_TILEPART_RESOLUTIONS;
    if (at_components)
      value |= OJPH_TILEPART_COMPONENTS;
    state->set_tilepart_divisions(value);
  }

  ////////////////////////////////////////////////////////////////////////////
  bool codestream::is_tilepart_division_at_resolutions()
  {
    ui32 res = state->get_tilepart_div() & OJPH_TILEPART_RESOLUTIONS;
    return res ? true : false;
  }

  ////////////////////////////////////////////////////////////////////////////
  bool codestream::is_tilepart_division_at_components()
  {
    ui32 comp = state->get_tilepart_div() & OJPH_TILEPART_COMPONENTS;
    return comp ? true : false;
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::request_tlm_marker(bool needed)
  {
    state->request_tlm_marker(needed);
  }

  ////////////////////////////////////////////////////////////////////////////
  bool codestream::is_tlm_requested()
  {
    return state->is_tlm_needed();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool codestream::is_planar() const
  {
    return state->is_planar();
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::write_headers(outfile_base *file,
                                 const comment_exchange* comments,
                                 ui32 num_comments)
  {
    state->write_headers(file, comments, num_comments);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::enable_resilience()
  {
    state->enable_resilience();
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::read_headers(infile_base *file)
  {
    state->read_headers(file);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::restrict_input_resolution(ui32 skipped_res_for_read,
                                             ui32 skipped_res_for_recon)
  {
    state->restrict_input_resolution(skipped_res_for_read,
      skipped_res_for_recon);
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::create()
  {
    state->read();
  }

  ////////////////////////////////////////////////////////////////////////////
  line_buf* codestream::pull(ui32 &comp_num)
  {
    return state->pull(comp_num);
  }


  ////////////////////////////////////////////////////////////////////////////
  void codestream::flush()
  {
    state->flush();
  }

  ////////////////////////////////////////////////////////////////////////////
  void codestream::close()
  {
    state->close();
  }

  ////////////////////////////////////////////////////////////////////////////
  line_buf* codestream::exchange(line_buf* line, ui32& next_component)
  {
    return state->exchange(line, next_component);
  }

  /////////////////////////////////////////////////////////////////////////////
  void codestream::send_image(void* component_bufs[], uint32_t row_strides[], uint32_t sample_strides[], uint8_t sample_widths[]) {
    ojph::ui32 next_comp;
    ojph::line_buf* cur_line = this->exchange(NULL, next_comp);
    ojph::param_siz siz = this->access_siz();
    if (this->is_planar())
    {
      for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
      {
        ojph::point p = siz.get_downsampling(c);
        ojph::ui32 height = ojph_div_ceil(siz.get_image_extent().y, p.y);
        height -= ojph_div_ceil(siz.get_image_offset().y, p.y);
        ojph::ui32 width = ojph_div_ceil(siz.get_image_extent().x, p.x);
        width -= ojph_div_ceil(siz.get_image_offset().x, p.x);
        uint8_t* cur_row = (uint8_t*) component_bufs[c];
        for (ojph::ui32 i = height; i > 0; --i)
        {
          uint8_t* cur_sample = cur_row;
          switch (sample_widths[c]) {
            case 8:
              for (ojph::ui32 j = 0; j < width; ++j) {
                cur_line->i32[j] = *(uint8_t*)cur_sample;
                cur_sample = cur_sample + sample_strides[c];
              }
              break;
            case 16:
              for (ojph::ui32 j = 0; j < width; ++j) {
                cur_line->i32[j] = *(uint16_t*)cur_sample;
                cur_sample = cur_sample + sample_strides[c];

              }
              break;
            case 32:
              for (ojph::ui32 j = 0; j < width; ++j) {
                cur_line->i32[j] = *(uint32_t*)cur_sample;
                cur_sample = cur_sample + sample_strides[c];
              }
              break;
            default:
              OJPH_ERROR(0x000300A1, "Sample widths must be 8, 16 or 32");
          }
          cur_row += row_strides[c];
          cur_line = this->exchange(cur_line, next_comp);
        }
      }
    }
    else
    {
      ojph::ui32 height = siz.get_image_extent().y - siz.get_image_offset().y; 
      ojph::ui32 width = siz.get_image_extent().x - siz.get_image_offset().x;
      for (ojph::ui32 i = 0; i < height; ++i)
      {
        for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
        {
          uint8_t* cur_sample = (uint8_t*) component_bufs[c] + i * row_strides[c];
          switch (sample_widths[c]) {
            case 8:
              for (ojph::ui32 j = 0; j < width; ++j) {
                cur_line->i32[j] = *(uint8_t*)cur_sample;
                cur_sample = cur_sample + sample_strides[c];
              }
              break;
            case 16:
              for (ojph::ui32 j = 0; j < width; ++j) {
                cur_line->i32[j] = *(uint16_t*)cur_sample;
                cur_sample = cur_sample + sample_strides[c];

              }
              break;
            case 32:
              for (ojph::ui32 j = 0; j < width; ++j) {
                cur_line->i32[j] = *(uint32_t*)cur_sample;
                cur_sample = cur_sample + sample_strides[c];
              }
              break;
            default:
              OJPH_ERROR(0x000300A1, "Sample widths must be 8, 16 or 32");
          }
          cur_line = this->exchange(cur_line, next_comp);
        }
      }
    }
  }

}
