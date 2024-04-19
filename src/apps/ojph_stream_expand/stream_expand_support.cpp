//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2024, Aous Naman
// Copyright (c) 2024, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2024, The University of New South Wales, Australia
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
// File: ojph_str_ex_support.h
// Author: Aous Naman
// Date: 18 April 2024
//***************************************************************************/

#include <cassert>
#include <cstddef>
#include "stream_expand_support.h"

namespace ojph
{
namespace stex
{

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void packets_handler::init(bool quiet, ui32 num_packets,
                           frames_handler* frames)
{ 
  assert(this->num_packets == 0);
  avail = packet_store = new rtp_packet[num_packets]; 
  this->quiet = quiet;
  this->num_packets = num_packets; 
  this->frames = frames;
}

///////////////////////////////////////////////////////////////////////////////
rtp_packet* packets_handler::exchange(rtp_packet* p)
{

  if (p != NULL)
  { // check validity/supported features in p
  }

  if (p)
  {
    ui32 packet_type = p->get_packet_type();
    if (packet_type == rtp_packet::packet_type::PT_MAIN)
    printf("A new frame %d\n", p->get_time_stamp());
  }


  if (avail != NULL)
  {
  }
  else 
  {
  }
  return packet_store;
}

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
frames_handler::~frames_handler()
{ 
  if (files) 
    delete[] files; 
}

///////////////////////////////////////////////////////////////////////////////
void frames_handler::init(bool quiet, ui32 num_threads,
                          const char *target_name, bool display)
{
  this->quiet = quiet;
  this->num_threads = num_threads;
  this->target_name = target_name;
  this->display = display;

  num_files = num_threads + 1;
  files = new stex_file[num_files];
}

///////////////////////////////////////////////////////////////////////////////

} // !stex namespace
} // !ojph namespace