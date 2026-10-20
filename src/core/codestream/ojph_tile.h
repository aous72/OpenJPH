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
// File: ojph_tile.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_TILE_H
#define OJPH_TILE_H

#include "ojph_defs.h"
#include "ojph_file.h"
#include "ojph_params_local.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class line_buf;
  class codestream;
  class mem_elastic_allocator;

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    //defined here
    class tile_comp;

    //////////////////////////////////////////////////////////////////////////
    class tile
    {
    public:
      static void pre_alloc(codestream *codestream, const rect& tile_rect,
                            const rect& recon_tile_rect, ui32 &num_tileparts);
      void finalize_alloc(codestream *codestream, const rect& tile_rect,
                          ui32 tile_idx, ui32& offset, ui32 &num_tileparts);

      bool push(line_buf *line, ui32 comp_num);
      void prepare_for_flush(mem_elastic_allocator *elastic);
      void fill_tlm(param_tlm* tlm);
      void flush(outfile_base *file);
      void parse_tile_header(const param_sot& sot, infile_base *file,
                             const ui64& tile_start_location);
      bool pull(line_buf *, ui32 comp_num);
      rect get_tile_rect() { return tile_rect; }

    private:
      //codestream *parent;
      rect tile_rect;
      ui32 num_comps;
      tile_comp *comps;
      ui32 num_lines;
      line_buf* lines;
      bool employ_color_transform, resilient;
      bool *reversible;
      rect *comp_rects, *recon_comp_rects;
      ui32 *line_offsets;
      ui32 skipped_res_for_read;

      ui32 *num_bits;
      bool *is_signed;
      ui32 *cur_line;
      const nlt_rec **nlt_ptr;
      int prog_order;

    private:
      param_sot sot;
      int next_tile_part;

    private:
      bool start_tilepart(outfile_base *file, ui32 bytes, ui8 TPsot,
                          ui8 TNsot);
      void write_precincts(ui32 comp_num, ui32 res_num, outfile_base *file);
      void write_one_precinct(ui32 comp_num, ui32 res_num,
                              outfile_base *file);
      void record_packet_lengths(mem_elastic_allocator *elastic);
      void reserve_lengths(ui32 count) const;
      ui32 end_of_plt_segment(ui32 first, ui32 end, ui32 &seg_bytes) const;
      ui32 plt_size(ui32 part) const;
      bool write_plt(outfile_base *file, ui32 part) const;
      ui32 plt_bytes(ui32 part) const
      { return need_plt ? plt_size(part) : 0; }

    private:
      int profile;
      ui32 tilepart_div;    // tilepart division value
      bool need_tlm;        // true if tlm markers are needed
      bool need_plt;        // true if plt markers are needed

      bool recording;       // true while packet lengths are recorded
      ui32 *plt_lengths;    // packet lengths, in codestream order
      ui32 *plt_end;        // end of plt_lengths
      ui32 *plt_part_start; // index of first packet of each tile-part
      ui32 *plt_cursor;     // where the next packet length is recorded
      ui32 cur_part;        // tile-part being recorded or written

      ui32 num_bytes; // number of bytes in this tile
                      // used for tile length
    };

  }
}

#endif // !OJPH_TILE_H