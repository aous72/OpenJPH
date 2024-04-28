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
      last_seq_num = p->get_sequence_number() - 1;

    // packet is old, and is ignored -- no need to included it in the 
    // lost packets, because this packet was considered lost previously.
    // This also captures the case where the previous packet and this packet
    // has the same sequence number, which is rather weird but possible
    // if some intermediate network unit retransmits packets.
    if (p->get_sequence_number() < last_seq_num + 1)
      return p;
    else if (p->get_sequence_number() == last_seq_num + 1)
    {
      consume_packet(p);
      // see if we can push one packet from the top of the buffer
      if (in_use && in_use->get_sequence_number() == last_seq_num + 1)
        consume_packet(in_use);
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
          p->get_sequence_number() > t->next->get_sequence_number())
          t = t->next;

        if (t->next != NULL &&
          p->get_sequence_number() == t->next->get_sequence_number())
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
      if (avail == NULL || in_use->get_sequence_number() == last_seq_num + 1) 
      {
        if (avail == NULL)
          lost_packets += in_use->get_sequence_number() - last_seq_num - 1;
        consume_packet(in_use);
        if (in_use && in_use->get_sequence_number() == last_seq_num + 1)
          consume_packet(in_use);
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
void packets_handler::consume_packet(rtp_packet* p)
{
  last_seq_num = p->get_sequence_number();
  frames->push(p);
  // move pack from in_use to avail; the packet must be equal to in_use
  assert(p == in_use);
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
    files_store[i].f.open(2<<20, false); files_store[i].f.close();
    files_store[i].init(this, files_store + i + 1, storers_store + i,
      renderers_store + i, target_name);
    storers_store[i].init(files_store + i, target_name);
    renderers_store[i].init(files_store + i, target_name);
  }
  files_store[i].f.open(2<<20, false); files_store[i].f.close();
  files_store[i].init(this, NULL, storers_store + i, renderers_store + i, 
    target_name);
  storers_store[i].init(files_store + i, target_name);
  renderers_store[i].init(files_store + i, target_name);
  this->thread_pool = thread_pool;
}

///////////////////////////////////////////////////////////////////////////////
void frames_handler::push(rtp_packet* p)
{
  assert(p->get_time_stamp() >= last_time_stamp);
  assert(p->get_sequence_number() >= last_seq_number);
  last_seq_number = p->get_sequence_number();

  // check if any of the frames processed in other threads are done
  check_files_in_processing();

  // process newly received packet
  if (p->get_packet_type() != rtp_packet::PT_BODY)
  { // main packet payload

    // The existance of a previous frame means we did not get the marked
    // packet.  Here, we close the frame and move it to processing
    if (in_use)
      truncate_and_process();

    // This is where we process a new frame, if there is space
    if (avail)
    {
      // move from avail to in_use
      stex_file* f = avail;
      avail = avail->next;
      f->next = in_use;
      in_use = f;

      assert(f->done.load(std::memory_order_acquire) == 0);
      f->timestamp = p->get_time_stamp();
      f->last_seen_seq = p->get_sequence_number();
      f->frame_idx = total_frames;
      f->f.open();
      f->f.write(p->get_data(), p->get_data_size());
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
      stex_file* f = in_use;
      if (last_time_stamp == f->timestamp)
      { // this is a continuation of a previous frame
        if (p->get_sequence_number() == f->last_seen_seq + 1)
        {
          f->last_seen_seq = p->get_sequence_number();
          f->f.write(p->get_data(), p->get_data_size());
          if (p->is_marked())
          { // move from from in_use to processing
            assert(in_use->next == NULL);
            f->f.close();
            in_use = in_use->next;

            if (target_name) {
              f->next = processing;
              processing = f;
              f->done.store(1, std::memory_order_relaxed);
              thread_pool->add_task(f->storer);
            }
            else {
              f->next = avail;
              avail = f;
            }
          }
        }
        else
          truncate_and_process();
      }
      else
      {
        // This is a different frame, and we did not get the marked packet.
        // We close the older frame and send it for processing
        truncate_and_process();

        if (p->get_time_stamp() > last_time_stamp)
        {
          ++total_frames;
          last_time_stamp = p->get_time_stamp();
        }
      }
    }
    else // no frames in_use
    {
      if (p->get_time_stamp() > last_time_stamp)
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

  // check files in_use and move them to processing
  while (in_use != NULL)
  {
    // move from in_use to avail    
    stex_file* f = in_use;
    in_use = in_use->next;
    f->next = avail;
    avail = f;
    f->f.close();
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

///////////////////////////////////////////////////////////////////////////////
void frames_handler::truncate_and_process()
{
  assert(in_use != NULL && in_use->next == NULL);
  ++trunc_frames;
  stex_file* f = in_use;
  f->f.close();
  in_use = in_use->next;

  if (target_name) {
    f->next = processing;
    processing = f;
    f->done.store(1, std::memory_order_relaxed);
    thread_pool->add_task(f->storer);
  }
  else {
    f->next = avail;
    avail = f;
  }
}


} // !stex namespace
} // !ojph namespace