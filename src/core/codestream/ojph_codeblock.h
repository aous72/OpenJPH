
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
// File: ojph_codeblock.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_CODEBLOCK_H
#define OJPH_CODEBLOCK_H

#include "ojph_defs.h"
#include "ojph_file.h"
#include "ojph_codeblock_fun.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class line_buf;
  class mem_elastic_allocator;
  class codestream;
  struct coded_lists;

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    //defined here
    struct precinct;
    class subband;
    struct coded_cb_header;

    //////////////////////////////////////////////////////////////////////////
    class codeblock
    {
      friend struct precinct;
      enum : ui32 {
        BUF32 = 4,
        BUF64 = 8,
      };

    public:
      static void pre_alloc(codestream *codestream, const size& nominal,
                            ui32 precision);
      void finalize_alloc(codestream *codestream, subband* parent,
                          const size& nominal, const size& cb_size,
                          coded_cb_header* coded_cb, ui32 K_max,
                          int tbx0, ui32 precision, ui32 comp_idx);
      void push(line_buf *line);
      void encode(mem_elastic_allocator *elastic);
      void recreate(const size& cb_size, coded_cb_header* coded_cb);

      void decode();
      void pull_line(line_buf *line);

    private:
      ui32 precision;
      union {
        ui32* buf32;
        ui64* buf64;
      };
      size nominal_size;
      size cb_size;
      ui32 stride;
      ui32 buf_size;
      subband* parent;
      int line_offset;
      ui32 cur_line;
      float delta, delta_inv;
      ui32 K_max;
      bool reversible;
      bool resilient;
      bool stripe_causal;
      bool zero_block; // true when the decoded block is all zero
      union {
        ui32 max_val32[8]; // supports up to 256 bits
        ui64 max_val64[4]; // supports up to 256 bits
      };
      coded_cb_header* coded_cb;
      codeblock_fun codeblock_functions;
    };

    //////////////////////////////////////////////////////////////////////////
    struct coded_cb_header
    {
      ui32 pass_length[2];
      ui32 num_passes;       // number of passes to be decoded
      ui32 Kmax;
      ui32 missing_msbs;
      // Parsing state, live while the packets of a precinct are read. It has
      // to survive from one quality layer of a codeblock to the next, which
      // is why it lives here and not in precinct::parse.
      // placeholder_passes : 3*P0 (T.814 B.1), or all ones while the
      //                      placeholder run is still open
      // pass_index         : index of the codeblock's next coding pass
      // pkt_*              : the byte counts the packet in hand contributes,
      //                      handed from its header to its data
      // sets_seen          : HT sets seen so far
      // decoded_set_skip   : S_skip of the set being decoded (T.814 B.3)
      // included           : has contributed to an earlier quality layer
      // in_layer           : contributes to the packet being read now
      // has_cleanup        : holds a real HT cleanup segment
      // starts_new_set     : this packet began a new HT set
      // Lblock_m3          : Lblock biased by 3, so zero means 3
      // set_passes         : coding passes gathered in the current HT set
      // zero_bitplanes     : P, the missing MSBs the packet header codes
      ui32 placeholder_passes;
      ui32 pass_index;
      ui32 pkt_pre_skip;
      ui32 pkt_cleanup;
      ui32 pkt_refine;
      ui16 sets_seen;
      ui16 decoded_set_skip;
      ui8 included;
      ui8 in_layer;
      ui8 has_cleanup;
      ui8 starts_new_set;
      ui8 Lblock_m3;
      ui8 set_passes;
      ui8 zero_bitplanes;
      coded_lists *next_coded;

      static const int prefix_buf_size = 8;
      static const int suffix_buf_size = 16;
    };

  }
}

#endif // !OJPH_CODEBLOCK_H