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
// File: stream_expand_support.h
// Author: Aous Naman
// Date: 18 April 2024
//***************************************************************************/

#include <cassert>
#include <cstddef>
#include "ojph_threads.h"
#include "threaded_frame_processors.h"
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
  if (result == false) // cannot use the packet for the time being
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
  else 
  {
    // sequence number of the most recent packet
    ui32 seq = p->get_sequence_number();

    // move packet to avail
    assert(p == in_use);
    in_use = in_use->next;
    p->next = avail;
    avail = p;

    // test if you can push more packets, also remove old packets
    p = in_use;
    rtp_packet *pp = p; // previous p -- will be updated before use
    while (p != NULL)
    {
      // if packet is used or it is old
      result = frames->push(p);
      result = result | (seq > p->get_sequence_number() + num_packets);
      if (result)
      {
        // move packet from in_use to avail
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
void packets_handler::flush()
{
  // move all packets from in_use to avail
  while (in_use)
  {
    rtp_packet *p = in_use;
    in_use = in_use->next;
    p->next = avail;
    avail = p;
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
void stex_file::notify_file_completion()
{ 
  int t = done.fetch_add(-1, std::memory_order_acq_rel);
  if (t == 1) // done is 0
    parent->increment_num_complete_files();
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
  if (renderers_store)
    delete[] renderers_store;
  if (storers_store)
    delete[] storers_store;
  if (files_store) 
    delete[] files_store; 
}

///////////////////////////////////////////////////////////////////////////////
void frames_handler::init(bool quiet, bool display, bool decode, 
                          ui32 packet_queue_length, ui32 num_threads, 
                          const char *target_name, 
                          thds::thread_pool* thread_pool)
{
  this->quiet = quiet;
  this->display = display;
  this->decode = decode;
  this->packet_queue_length = packet_queue_length;
  this->num_threads = num_threads;
  this->target_name = target_name;
  num_files = num_threads + 1;
  avail = files_store = new stex_file[num_files];
  storers_store = new j2k_frame_storer[num_files];
  renderers_store = new j2k_frame_renderer[num_files];
  ui32 i = 0;
  for (; i < num_files - 1; ++i) {
    files_store[i].init(this, files_store + i + 1, storers_store + i,
      renderers_store + i, target_name);
    storers_store[i].init(files_store + i, target_name);
    renderers_store[i].init(files_store + i, target_name);
  }
  files_store[i].init(this, NULL, storers_store + i, renderers_store + i, 
    target_name);
  storers_store[i].init(files_store + i, target_name);
  renderers_store[i].init(files_store + i, target_name);
  this->thread_pool = thread_pool;
}

///////////////////////////////////////////////////////////////////////////////
bool frames_handler::push(rtp_packet* p)
{
  // check if any of the frames processed in other threads are done
  check_files_in_processing();

  // check if we have any old files that have no hope is updating
  if (in_use)
  {
    ui32 seq = p->get_sequence_number();    
    stex_file* f = in_use, *pf = NULL;
    while (f != NULL)
    {
      if (seq > f->last_seen_seq + packet_queue_length)
      {
        // move from in_use to processing
        if (f == in_use)
        {
          in_use = in_use->next;
          f->next = processing;
          processing = f;
          if (target_name != NULL)
          {
            f->f.close();
            thread_pool->add_task(f->storer);
          }
          f = in_use;
        }
        else {
          pf->next = f->next;
          f->next = processing;
          processing = f;
          if (target_name != NULL)
          {
            f->f.close();
            thread_pool->add_task(f->storer);
          }
          f = pf->next;
        }
      }
      else {
        pf = f;
        f = f->next;
      }
    }
  }

  // process newly received packet
  if (p->get_packet_type() != rtp_packet::PT_BODY)
  { // main payload header
    printf("A new file %d\n", p->get_time_stamp());
    if (avail)
    {
      // move from avail to in_use
      stex_file* f = avail;
      avail = avail->next;
      f->next = in_use;
      in_use = f;
      f->timestamp = p->get_time_stamp();
      f->last_seen_seq = p->get_sequence_number();
      f->marked = false;
      f->estimated_size = f->actual_size = 0;
      f->frame_idx = frame_idx++;
      f->f.open(1<<20, true); // start with 1MB
      f->write(p);
      return true;
    }
    else 
      return false;
  }
  else 
  { // body payload header
    stex_file* f = in_use, *pf;
    while (f != NULL && f->timestamp != p->get_time_stamp()) {
      pf = f;
      f = f->next;
    }
    if (f == NULL)
      return false;

    f->last_seen_seq = ojph_max(f->last_seen_seq, p->get_sequence_number());
    f->write(p);
    
    if (p->is_marked())
      f->marked = true;

    if (f->marked && f->are_packets_missing() == false)
    {
      // move from from in_use to processing
      if (f == in_use)
        in_use = in_use->next;
      else
        pf->next = f->next;
      f->next = processing;
      processing = f;

      f->f.close();
      thread_pool->add_task(f->storer);
    }
    // else
    //   printf("%02x %02x\n", p->data[0], p->data[1]);

    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
bool frames_handler::flush()
{
  // check if any of the frames processed in other threads are done
  check_files_in_processing();

  // check files in_use and move them to processing
  while (in_use != NULL)
  {
    // move from in_use to processing    
    stex_file* f = in_use;
    in_use = in_use->next;
    f->next = processing;
    processing = f;
    if (target_name != NULL)
    {
      f->f.close();
      thread_pool->add_task(f->storer);
    }
  }

  return (processing != NULL);
}

///////////////////////////////////////////////////////////////////////////////
void frames_handler::check_files_in_processing()
{
  // check if any of the frames processed in other threads are done
  int nf = num_complete_files.load(std::memory_order_acquire);
  if (nf > 0)
  {
    stex_file* f = processing, *pf = NULL;
    while(f != NULL && nf > 0)
    {
      num_complete_files.fetch_add(-1, std::memory_order_relaxed);

      if (f->done.load(std::memory_order_acquire) == 0)
      {
        // move f from processing to avail
        f->timestamp = 0;
        f->last_seen_seq = 0;
        f->marked = false;
        f->estimated_size = f->actual_size = 0;
        f->frame_idx = 0;
        if (f == processing)
        {
          processing = processing->next;
          f->next = avail;
          avail = f;
          f = processing;        // for next test
        }
        else {
          pf->next = f->next;
          f->next = avail;
          avail = f;
          f = pf->next;
        }
      }
      else 
      {
        pf = f;
        f = f->next;
      }
      nf = num_complete_files.load(std::memory_order_acquire);
    }
  }
}

} // !stex namespace
} // !ojph namespace