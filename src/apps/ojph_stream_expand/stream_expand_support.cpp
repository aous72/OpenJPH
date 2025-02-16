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
// static comparison functions
//
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Compares two 32 bit values, A with B, with the possibility A or B has 
// undergone overflow. This problem has no proper solution, but here we 
// assume that the value B approximately divides the space into two regions, 
// a region larger than B and a region smaller than B.  This leaves one 
// undetermined value that lies at the opposite end of B, a case we just 
// ignore -- it is part of smaller.
// NB: This is my current thinking -- I might be wrong
static inline bool is_greater32(ui32 a, ui32 b)
{ 
  ui32 c = a - b; 
  return (c > 0u && c <= 0x7FFFFFFFu);
}

///////////////////////////////////////////////////////////////////////////////
// Compares two 32 bit values, A with B, with the possibility A or B has 
// undergone overflow. This problem has no proper solution, but here we 
// assume that the value B approximately divides the space into two regions, 
// a region larger than B and a region smaller than B.  This leaves one 
// undetermined value that lies at the opposite end of B, a case we just 
// ignore -- it is part of smaller.
// NB: This is my current thinking -- I might be wrong
static inline bool is_smaller32(ui32 a, ui32 b)
{ 
  ui32 c = a - b;
  return (c >= 0x80000000u && c <= 0xFFFFFFFFu);
}

///////////////////////////////////////////////////////////////////////////////
static inline bool is_greater24(ui32 a, ui32 b)
{ return is_greater32(a << 8, b << 8); }

///////////////////////////////////////////////////////////////////////////////
static inline bool is_smaller24(ui32 a, ui32 b)
{ return is_smaller32(a << 8, b << 8); }

///////////////////////////////////////////////////////////////////////////////
static inline ui32 clip_seq_num(ui32 n) { return (n & 0xFFFFFF); }

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
  ui32 i = 0;
  for (; i < num_packets - 1; ++i)
    packet_store[i].init(packet_store + i + 1);
  packet_store[i].init(NULL);
  this->quiet = quiet;
  this->num_packets = num_packets; 
  this->frames = frames;
}

///////////////////////////////////////////////////////////////////////////////
rtp_packet* packets_handler::exchange(rtp_packet* p)
{
  assert(num_packets > 0 && p == in_use);

  if (p != NULL) {
    if (p->num_bytes == 0)
      return p;

    if (last_seq_num == 0) // initialization
      last_seq_num = clip_seq_num(p->get_seq_num() - 1);

    // packet is old, and is ignored -- no need to included it in the 
    // lost packets, because this packet was considered lost previously.
    // This also captures the case where the previous packet and this packet
    // has the same sequence number, which is rather weird but possible
    // if some intermediate network unit retransmits packets.
    if (is_smaller24(p->get_seq_num(), clip_seq_num(last_seq_num + 1)))
      return p;
    else if (p->get_seq_num() == clip_seq_num(last_seq_num + 1))
    {
      consume_packet();
      // see if we can push one packet from the top of the buffer
      if (in_use && in_use->get_seq_num() == clip_seq_num(last_seq_num + 1))
        consume_packet();
    }
    else // sequence larger than expected
    {
      // Place the packet in the in_use queue according to its sequence
      // number; we may have to move it down the queue. The in_use queue is 
      // always arranged in an ascending order, where the top of the queue 
      // (pointed to by in_use) has the smallest sequence number.
      if (in_use->next != NULL) // we have more than 1 packet in queue
      { 
        rtp_packet* t = in_use;
        while (t->next != NULL && 
          is_greater24(p->get_seq_num(), t->next->get_seq_num()))
          t = t->next;

        if (t->next != NULL && p->get_seq_num() == t->next->get_seq_num())
        { // this is a repeated packet and must be removed
          in_use = in_use->next;
          p->next = avail;
          avail = p;
        }
        else {
          if (t == in_use) // at front of queue -- exactly where it should be
          { } // do nothing
          else if (t->next == NULL) { // at the end of queue
            in_use = in_use->next; // remove p from the queue
            t->next = p;
            p->next = NULL;
          }
          else { // in the middle of the queue
            in_use = in_use->next; // p removed from the start of queue
            p->next = t->next;
            t->next = p;
          }
        }
      }

      // If avail == NULL, all packets are being used (in_use), meaning 
      // the queue is already full. We push packets from to the top of in_use
      // queue.
      // If avail != NULL, we push one packet from the top of the buffer, 
      // if it has the correct sequence number.
      if (avail == NULL || 
          in_use->get_seq_num() == clip_seq_num(last_seq_num + 1))
      {
        if (avail == NULL)
          lost_packets += 
            in_use->get_seq_num() - clip_seq_num(last_seq_num + 1);
        consume_packet();
        if (in_use && in_use->get_seq_num() == clip_seq_num(last_seq_num + 1))
            consume_packet();
      }
    }
  }

  // move from avail to in_use -- there must be at least one packet in avail
  assert(avail != NULL);
  p = avail;
  avail = avail->next;
  p->next = in_use;
  in_use = p;
  return p;
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
void packets_handler::consume_packet()
{
  last_seq_num = in_use->get_seq_num();
  frames->push(in_use);
  // move pack from in_use to avail; the packet must be equal to in_use
  rtp_packet* p = in_use;
  in_use = in_use->next;
  p->next = avail;
  avail = p;
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
  if (storers_store)
    delete[] storers_store;
  if (files_store) 
    delete[] files_store; 
}

///////////////////////////////////////////////////////////////////////////////
void frames_handler::init(bool quiet, const char *target_name, 
                          thds::thread_pool* thread_pool)
{
  this->quiet = quiet;
  this->num_threads = (ui32)thread_pool->get_num_threads();
  this->target_name = target_name;
  num_files = num_threads + 1;
  avail = files_store = new stex_file[num_files];
  storers_store = new j2k_frame_storer[num_files];
  ui32 i = 0;
  for (; i < num_files - 1; ++i) {
    files_store[i].f.open(2 << 20, false); 
    files_store[i].f.close();
    files_store[i].init(this, files_store + i + 1, storers_store + i,
      target_name);
    storers_store[i].init(files_store + i, target_name);
  }
  files_store[i].f.open(2 << 20, false); 
  files_store[i].f.close();
  files_store[i].init(this, NULL, storers_store + i, target_name);
  storers_store[i].init(files_store + i, target_name);
  this->thread_pool = thread_pool;
}

///////////////////////////////////////////////////////////////////////////////
void frames_handler::push(rtp_packet* p)
{
  assert(!is_smaller32(p->get_time_stamp(), last_time_stamp));
  assert(!is_smaller24(p->get_seq_num(), last_seq_number));
  last_seq_number = p->get_seq_num();

  // check if any of the frames processed in other threads are done
  check_files_in_processing();

  // process newly received packet
  if (p->get_packet_type() != rtp_packet::PT_BODY)
  { // main packet payload

    // The existence of a previous frame means we did not get the marked
    // packet.  Here, we close the frame and move it to processing
    if (in_use) {
      ++trunc_frames;
      send_to_processing();
    }

    // This is where we process a new frame, if there is space
    if (avail)
    {
      // move from avail to in_use
      in_use = avail;
      avail = avail->next;
      in_use->next = NULL;

      assert(in_use->done.load(std::memory_order_acquire) == 0);
      in_use->time_stamp = p->get_time_stamp();
      in_use->last_seen_seq = p->get_seq_num();
      in_use->frame_idx = total_frames;
      in_use->f.open();
      in_use->f.write(p->get_data(), p->get_data_size());
    }
    else
      ++lost_frames;

    ++total_frames;
    last_time_stamp = p->get_time_stamp();
  }
  else 
  { // body packet payload
    if (in_use != NULL)
    {
      if (p->get_time_stamp() == in_use->time_stamp)
      { // this is a continuation of a previous frame
        if (p->get_seq_num() == clip_seq_num(in_use->last_seen_seq + 1))
        {
          in_use->last_seen_seq = p->get_seq_num();
          in_use->f.write(p->get_data(), p->get_data_size());
          if (p->is_marked())
            send_to_processing();
        }
        else {
          // we must have missed packets
          ++trunc_frames;
          send_to_processing();
        }
      }
      else
      {
        // This is a different frame and we did not get the marked packet.
        // We close the older frame and send it for processing
        ++trunc_frames;
        send_to_processing();

        if (is_greater32(p->get_time_stamp(), last_time_stamp))
        {
          ++total_frames;
          last_time_stamp = p->get_time_stamp();
        }
      }
    }
    else // no frame is being written
    {
      if (is_greater32(p->get_time_stamp(), last_time_stamp))
      {
        ++total_frames;
        last_time_stamp = p->get_time_stamp();
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void frames_handler::get_stats(ui32& total_frames, ui32& trunc_frames, 
                               ui32& lost_frames)
{
  total_frames = this->total_frames;
  trunc_frames = this->trunc_frames;
  lost_frames = this->lost_frames;
}

///////////////////////////////////////////////////////////////////////////////
bool frames_handler::flush()
{
  // check if any of the frames processed in other threads are done
  check_files_in_processing();

  // check the file in in_use and terminate it
  if (in_use != NULL)
  {
    // move from in_use to avail    
    in_use->f.close();
    in_use->next = avail;
    avail = in_use;
    in_use = NULL;
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
        f->time_stamp = 0;
        f->last_seen_seq = 0;
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
          f = pf->next;         // for next test
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

///////////////////////////////////////////////////////////////////////////////
void frames_handler::send_to_processing()
{
  in_use->f.close();
  if (target_name) {
    in_use->next = processing;
    processing = in_use;
    in_use->done.store(1, std::memory_order_relaxed);
    thread_pool->add_task(in_use->storer);
  }
  else {
    in_use->next = avail;
    avail = in_use;
  }
  in_use = NULL;
}

} // !stex namespace
} // !ojph namespace