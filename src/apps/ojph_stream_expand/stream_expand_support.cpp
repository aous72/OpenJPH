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
  for (ui32 i = 0; i < num_packets - 1; ++i)
    packet_store[i].next = packet_store + i + 1;
  this->quiet = quiet;
  this->num_packets = num_packets; 
  this->frames = frames;
}

///////////////////////////////////////////////////////////////////////////////
rtp_packet* packets_handler::exchange(rtp_packet* p)
{
  if (p == NULL) {
    assert(in_use == NULL && num_packets > 0);
    // move from avail to in_use
    rtp_packet* p = avail;
    avail = avail->next;
    p->next = in_use;    
    in_use = p;
    return p;
  }
  if (p->num_bytes == 0)
    return p;
  
  // We can a series of test to remove/warn about unsupported options
  // but we currently do not do that yet

  bool result = frames->push(p);
  if (result == false)
  {
    if (avail)
    { // move from avail to in_use
      p = avail;
      avail = avail->next;
      p->next = in_use;    
      in_use = p;
    }
    else
    { 
      assert(p->next != NULL || num_packets == 1);
      if (p->next != NULL)
      { // use the oldest/last packet in in_use
        assert(p == in_use);
        rtp_packet *pp = p; // previous p
        p = p->next;
        while(p->next != NULL) { pp = p; p = p->next; }
        pp->next = NULL;
        p->next = in_use;
        in_use = p;
      }
    }
    return p;
  }
  else {
    // move packet to avail
    assert(p == in_use);
    in_use = in_use->next;
    p->next = avail;
    avail = p;

    // test if you can push more packets
    p = in_use;
    rtp_packet *pp = p; // previous p
    while (p != NULL)
    {
      result = frames->push(p);
      if (result)
      {
        // move packet to avail
        if (p == in_use)
        {
          in_use = in_use->next;
          p->next = avail;
          avail = p;
          p = in_use;
        }
        else
        {
          pp->next = p->next;
          p->next = avail;
          avail = p;
          p = pp->next;
        }
      }
      else {
        pp = p;
        p = p->next;
      }
    }

    // get one from avail and move it to in_use
    p = avail;
    avail = avail->next;
    p->next = in_use;
    in_use = p;
    return p;
  }
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
void frames_handler::init(bool quiet, bool display, bool decode,
                          ui32 num_threads, const char *target_name)
{
  this->quiet = quiet;
  this->display = display;
  this->decode = decode;
  this->num_threads = num_threads;
  this->target_name = target_name;
  num_files = num_threads + 1;
  files = new stex_file[num_files];
}

///////////////////////////////////////////////////////////////////////////////
bool frames_handler::push(rtp_packet* p)
{

  return false;
}

} // !stex namespace
} // !ojph namespace