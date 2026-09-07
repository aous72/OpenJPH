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
// File: ojph_precinct.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_PRECINCT_H
#define OJPH_PRECINCT_H

#include "ojph_defs.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class mem_elastic_allocator;
  struct coded_lists;

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    //defined here
    class subband;
    
    //////////////////////////////////////////////////////////////////////////
    struct precinct
    {
      precinct() {
        scratch = NULL; bands = NULL; coded = NULL;
        may_use_sop = uses_eph = false;
      }
      enum : ui32 {
        // The tag trees the packets of a precinct are parsed with: the
        // quality layer of first inclusion and the number of missing MSBs,
        // each paired with a tree of which of its nodes are already known,
        // for each of the four subbands.
        num_trees_per_band = 4,
        num_parse_tag_trees = 4 * num_trees_per_band,
      };

      // The number of levels the tag trees of a precinct spanning num_cbs
      // codeblocks in one subband need: one per power of two of the wider
      // span, plus the root level. Only meaningful for a subband that
      // exists and that the precinct holds codeblocks of.
      static ui32 num_tag_tree_levels(size num_cbs);
      // The leaves of one tag tree of that many levels, and the bytes it
      // occupies: a quad tree needs 4/3 of its leaf count, rounded up, and
      // the rounding leaves the one extra entry the level above the root
      // takes. resolution::pre_alloc has to reserve a precinct's tag tree
      // storage before the codeblocks the precinct holds are known, so it
      // asks for the levels the precinct to codeblock ratio allows, which
      // is what a precinct the resolution holds in full spans;
      // resolution::finalize_alloc then lays the trees out with the levels
      // its precincts turned out to need. Both go through here, so the two
      // cannot drift apart.
      static ui32 num_tag_tree_leaves(ui32 num_levels)
      { return 1u << ((num_levels - 1) << 1); }
      static ui32 num_tag_tree_bytes(ui32 num_levels)
      { return (num_tag_tree_leaves(num_levels) * 4 + 2) / 3; }
      ui32 prepare_precinct(int tag_tree_size, ui32* lev_idx,
                            mem_elastic_allocator *elastic);
      void write(outfile_base *file);
      // Parses the packet carrying one quality layer of this precinct. The
      // packets of a precinct arrive in order of increasing quality layer in
      // every progression order, but not necessarily one after the other.
      void parse(int tag_tree_size, ui32* lev_idx,
                 mem_elastic_allocator *elastic,
                 ui32& data_left, infile_base *file, bool skipped,
                 ui32 layer);

      ui8 *scratch;    //storage for num_parse_tag_trees tag trees; the
                       //precinct's own when it holds more than one packet,
                       //otherwise the buffer shared by the whole codestream
      point img_point; //the precinct projected to full resolution
      rect cb_idxs[4]; //indices of codeblocks
      subband *bands;  //the subbands
      coded_lists* coded;
      bool may_use_sop, uses_eph;
    };

  }
}

#endif // !OJPH_PRECINCT_H