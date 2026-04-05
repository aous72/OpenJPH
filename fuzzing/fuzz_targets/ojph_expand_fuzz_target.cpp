//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman 
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
// File: ojph_expand_fuzz_target.cpp
// Author: Pierre-Anthony Lemieux
// Date: 17 February 2026
//***************************************************************************/

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <ojph_arch.h>
#include <ojph_file.h>
#include <ojph_params.h>
#include <ojph_mem.h>
#include <ojph_codestream.h>
#include <ojph_message.h>
#include <exception>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
  // The first 2 bytes are used to control decoder options:
  //   byte 0 bit 1: force planar mode
  //   byte 0 bit 2: force interleaved mode
  //   byte 1: number of resolutions to skip (0-7)
  if (Size < 3)
    return 0;

  uint8_t opts = Data[0];
  uint8_t skip_res = Data[1] & 0x07;
  Data += 2;
  Size -= 2;

  bool force_planar = (opts & 0x02) != 0;
  bool force_interleaved = (opts & 0x04) != 0;

  try
  {
    ojph::mem_infile infile;
    infile.open(reinterpret_cast<const ojph::ui8 *>(Data), Size);

    ojph::codestream cs;

    // Always enable resilience: all fuzzer inputs are untrusted/mutated,
    // so the decoder must use its error-recovery path.
    cs.enable_resilience();

    cs.read_headers(&infile);

    // Guard against inputs that cause excessive decoding work.
    {
      ojph::param_siz siz = cs.access_siz();
      ojph::point extent = siz.get_image_extent();
      ojph::point offset = siz.get_image_offset();
      ojph::ui64 w = extent.x - offset.x;
      ojph::ui64 h = extent.y - offset.y;
      if (w * h * siz.get_num_components() > 65536)
      {
        cs.close();
        return 0;
      }

      ojph::param_cod cod = cs.access_cod();
      if (cod.get_num_decompositions() > 5)
      {
        cs.close();
        return 0;
      }

      // Large precincts cause huge internal buffers and very expensive
      // per-row wavelet transforms even for small images.
      for (ojph::ui32 lev = 0; lev <= cod.get_num_decompositions(); ++lev)
      {
        ojph::size psiz = cod.get_precinct_size(lev);
        if (psiz.w > 256 || psiz.h > 256)
        {
          cs.close();
          return 0;
        }
      }
    }

    if (skip_res > 0)
      cs.restrict_input_resolution(skip_res, skip_res);

    if (force_planar)
      cs.set_planar(true);
    else if (force_interleaved)
      cs.set_planar(false);

    cs.create();

    ojph::param_siz siz = cs.access_siz();

    // Second guard: cap reconstructed dimensions after create().
    {
      ojph::ui64 total_recon = 0;
      for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
        total_recon += (ojph::ui64)siz.get_recon_width(c)
                     * (ojph::ui64)siz.get_recon_height(c);
      if (total_recon > 65536)
      {
        cs.close();
        return 0;
      }
    }

    // Time budget: abort if decoding takes too long.
    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    ojph::ui32 pull_count = 0;
    const ojph::ui32 MAX_SECONDS = 10;
    bool timed_out = false;

    if (cs.is_planar())
    {
      for (ojph::ui32 c = 0; c < siz.get_num_components() && !timed_out; ++c)
      {
        ojph::ui32 height = siz.get_recon_height(c);
        for (ojph::ui32 i = height; i > 0 && !timed_out; --i)
        {
          ojph::ui32 comp_num;
          cs.pull(comp_num);
          if (++pull_count % 64 == 0)
          {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            if ((ojph::ui32)(now.tv_sec - start_ts.tv_sec) >= MAX_SECONDS)
              timed_out = true;
          }
        }
      }
    }
    else
    {
      ojph::ui32 height = siz.get_recon_height(0);
      for (ojph::ui32 i = 0; i < height && !timed_out; ++i)
      {
        for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
        {
          ojph::ui32 comp_num;
          cs.pull(comp_num);
          if (++pull_count % 64 == 0)
          {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            if ((ojph::ui32)(now.tv_sec - start_ts.tv_sec) >= MAX_SECONDS)
              timed_out = true;
          }
        }
      }
    }

    cs.close();
  }
  catch (const std::exception &)
  {
  }

  return 0;
}

#ifdef OJPH_FUZZ_TARGET_MAIN
int main(int argc, char **argv) {
  if (argc != 2) {
    return -1;
  }
  FILE *f = fopen(argv[1], "rb");
  if (!f) { return -1; }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  if (len < 0) {
    return -1;
  }
  rewind(f);
  // Prepend 2 control bytes (default: no skip)
  std::vector<uint8_t> buf(len + 2);
  buf[0] = 0;
  buf[1] = 0;
  size_t n = fread(buf.data() + 2, 1, len, f);
  if(n != static_cast<size_t>(len)) {
    return -1;
  }
  fclose(f);
  LLVMFuzzerTestOneInput(buf.data(), buf.size());
  return 0;
}
#endif
