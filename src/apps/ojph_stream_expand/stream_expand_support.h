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
// File: stream_expand_support.cpp
// Author: Aous Naman
// Date: 18 April 2024
//***************************************************************************/

#ifndef OJPH_STR_EX_SUPPORT_H
#define OJPH_STR_EX_SUPPORT_H

#include <atomic>
#include <cassert>
#include "ojph_base.h"
#include "ojph_file.h"
#include "ojph_sockets.h"

namespace ojph
{
  namespace thds 
  { class thread_pool; }

namespace stex // stream expand
{

// defined here
class packets_handler;
class frames_handler;

// defined elsewhere
struct j2k_frame_storer;

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** @brief interprets RTP header and payload, and holds received packets.
 * 
 *  This object interpret RFC 3550 and draft-ietf-avtcore-rtp-j2k-scl-00.
 *  The implementation is not complete, but it is sufficient for the time 
 *  being.
 *  
 */
struct rtp_packet
{
  /**
   *  @brief packet types based on the main header of 
   *         draft-ietf-avtcore-rtp-j2k-scl-00
   */
  enum packet_type : ui32
  {
    PT_BODY                  = 0, // this is body packet
    PT_MAIN_FOLLOWED_BY_MAIN = 1,
    PT_MAIN_FOLLOWED_BY_BODY = 2, 
    PT_MAIN                  = 3, // frame has only one main packet
  };
public:
  /**
   *  @brief default constructor
   */
  rtp_packet() { num_bytes = 0; next = NULL; }

  /**
   *  @brief Call this to link packets.
   * 
   *  @param next pointer to next packet
   */
  void init(rtp_packet* next) { this->next = next; }

public:
  // RTP header
  ui32 get_rtp_version() { return ((ui32)data[0]) >> 6; }
  bool is_padded() { return (data[0] & 0x20) != 0; }
  bool is_extended() { return (data[0] & 0x10) != 0; }
  ui32 get_csrc_count() { return (ui32)(data[0]) & 0xF; }
  bool is_marked() { return (data[1] & 0x80) != 0; }
  ui32 get_payload_type() { return (ui32)(data[1]) & 0x7F; }
  ui32 get_seq_num() {
    ui32 result = ntohs(*(ui16*)(data + 2));
    result |= ((ui32)data[15]) << 16;   // extended sequence (ESEQ)
    return result;
  }
  ui32 get_time_stamp() 
  { return ntohl(*(ui32*)(data + 4)); }
  ui32 get_ssrc()             // not used for the time being
  { return ntohl(*(ui32*)(data + 8)); }

  // common in main and body payload headers
  ui32 get_packet_type() 
  { return ((ui32)data[12]) >> 6; }
  ui32 get_TP()
  { return (((ui32)data[12]) >> 3) & 0x7; }
  ui32 get_ORDH() { 
    if (get_packet_type() != PT_BODY) return ((ui32)data[12]) & 0x7; 
    else return (((ui32)data[13]) >> 7) & 0x1; 
  }
  ui32 get_PTSTAMP() {
    ui32 result = (((ui32)data[13]) & 0xF) << 8;
    result |= (ui32)data[14];
    return result; 
  }
  ui8* get_data()
  { return data + 20; }
  ui32 get_data_size()
  { return (ui32)num_bytes - 20; }

  // only in main payload header
  bool is_PTSTAMP_used() { 
    assert(get_packet_type() != PT_BODY);
    return (((ui32)data[13]) & 0x80) != 0; 
  }
  ui32 get_XTRAC() { 
    assert(get_packet_type() != PT_BODY);
    return (((ui32)data[13]) >> 4) & 0x7; 
  }
  bool is_codestream_header_reusable() { 
    assert(get_packet_type() != PT_BODY);
    return (((ui32)data[16]) & 0x80) != 0;
  }
  bool is_component_colorimetry_used() { 
    assert(get_packet_type() != PT_BODY);    
    return (((ui32)data[16]) & 0x40) != 0;
  }
  bool is_codeblock_caching_used() {
    assert(get_packet_type() != PT_BODY);
    return (((ui32)data[16]) & 0x20) != 0;
  }
  bool is_RANGE() {
    assert(get_packet_type() != PT_BODY); 
    return ((ui32)data[16] & 1) != 0; 
  }
  ui32 get_PRIMS(){
    assert(get_packet_type() != PT_BODY); 
    return (ui32)data[17]; 
  }
  ui32 get_TRANS() { 
    assert(get_packet_type() != PT_BODY); 
    return (ui32)data[18]; 
  }
  ui32 get_MAT() { 
    assert(get_packet_type() != PT_BODY); 
    return (ui32)data[19]; 
  }

  // only in body payload header
  ui32 get_RES() { 
    assert(get_packet_type() == PT_BODY); 
    return ((ui32)data[12]) & 0x7; 
  }
  ui32 get_QUAL() { 
    assert(get_packet_type() == PT_BODY); 
    return (((ui32)data[13]) >> 4) & 0x7; 
  }
  ui32 get_data_pos() {
    ui32 result = 0;
    if (get_packet_type() == PT_BODY) { 
      result = ((ui32)data[16]) << 4;
      result |= (((ui32)data[17]) >> 4) & 0xF;
    }
    return result;
  }
  ui32 get_PID() {
    assert(get_packet_type() == PT_BODY);     
    ui32 result = (((ui32)data[17]) & 0xF) << 16;
    result |= ((ui32)data[18]) << 8;
    result |= ((ui32)data[19]);
    return result;
  }
  

public:
  static constexpr int max_size = 2048; //!<maximum packet size
                                        // ethernet packet are only 1500
  ui8 data[max_size];                   //!<data in the packet
  ui32 num_bytes;                       //!<number of bytes 
  rtp_packet* next;                     //!<used for linking packets
};

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** @brief Interprets new packets, buffers them if needed.
 * 
 *  This object primarily attempts to process the RTP packet.
 *  The main purpose is to buffer received packets if it is not clear where
 *  they fit. It also drops packets if they become old.
 * 
 *  This object basically works as follows.
 *  The object buffers out-of-order packets, i.e., those with a sequence 
 *  number higher than expected.  Then, the object tries to push these 
 *  packets when their sequence number comes.  Packets are pushed to the 
 *  frames_handler, using the "push" member function.
 * 
 *  The buffer has limited size, when it becomes full, the oldest packet is 
 *  pushed; this basically means that all missing packets are considered
 *  lost.
 *  
 *  When a new packet is pushed, the object looks if it has the next packet
 *  in its buffer, if so, then it pushes one more packet.  It does not 
 *  attempt to push more than one packet from its buffer, because this
 *  might delay picking up the next packet from the operating system network
 *  stack.
 * 
 *  Packets in the buffer are arranged according to their sequence number.
 *  
 */
class packets_handler
{
public:
  /**
   *  @brief default constructor
   */
  packets_handler()
  {
    quiet = false;
    avail = in_use = NULL; 
    last_seq_num = lost_packets = 0;
    frames = NULL;
    num_packets = 0;
    packet_store = NULL;
  }
  /**
   *  @brief default destructor
   */
  ~packets_handler()
  { if (packet_store) delete[] packet_store; }

public:
  /**
   *  @brief call this to initialize packets_handler
   *
   *  This function creates a chain of packets that is for packet re-ordering
   *
   *  @param quiet no messages are printed when true -- as of this writing
   *         the object prints no messages
   *  @param num_packets the number of packets in the chain
   *  @param frames a pointer to the frames_handler object that will be 
   *         receive the packets
   */
  void init(bool quiet, ui32 num_packets, frames_handler* frames);

  /**
   *  @brief Call this function to get a packet from the packet chain.
   *
   *  This function is an input-output function.  First time call to this 
   *  function passes a null pointer, and gets a pointer to use. Subsequent
   *  calls passes the pointer that was obtained earlier to get a new pointer.
   *  This function supplies one pointer only.
   *
   *  @param  p a pointer to a packet that was previously obtained by calling
   *          this function.
   *  @return returns a pointer to a packet
   */
  rtp_packet* exchange(rtp_packet* p);

  /**
   *  @brief This function provides information about the observed number 
   *          of lost packets
   *
   *  @return returns number of lost packets up to the time of the call
   */
  ui32 get_num_lost_packets() const { return lost_packets; }

  /**
   *  @brief This function is not used, and therefore it is not clear how to 
   *         use it.
   */
  void flush();

private:
  /**
   *  @brief This function sends the packet in in_use (oldest) to frames 
   *         handler object.
   * 
   */
  void consume_packet();

private:
  bool quiet;                //!<no informational info is printed when true
  rtp_packet* avail;         //!<start of available packets chain
  rtp_packet* in_use;        //!<start of used packet chain
  ui32 last_seq_num;         //!<the last observed sequence number
  ui32 lost_packets;         //!<number of lost packets -- just statistics
  frames_handler* frames;    //!<frames object

  ui32 num_packets;          //!<maximum number of packets in packet_store
  rtp_packet* packet_store;  //!<address of packet memory allocation
};

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** @brief holds in memory j2k codestream together with other info
 * 
 *  This objects holds a j2k codestream file.  The codestream is identified 
 *  by its timestamp. Once complete the file is pushed to saver.
 * 
 *  File chains can be created using the \"next\" member variable.
 * 
 *  This object is handled by frames_handler, and therefore, it does not 
 *  have many functions.  stex_file does not create any objects of its own.
 * 
 *  The object also serves to pass information to the j2k_frame_storer, 
 *  which is run by another thread
 * 
 */
struct stex_file {
public:
  /**
   *  @brief default constructor
   */
  stex_file() 
  { 
    time_stamp = last_seen_seq = 0; 
    done.store(0, std::memory_order_relaxed);
    frame_idx = 0;
    parent = NULL;
    name_template = NULL;
    storer = NULL;
    next = NULL; 
  }

public:
  /**
   *  @brief call this function to initialize stex_file
   * 
   *  It just copies parameters to the object.
   * 
   *  @param parent is a pointer to the object holding this file, which is
   *         frames_handler
   *  @param next is used to chain files
   *  @param storer this object is used to store j2k codestreams
   *  @param name_template file name template to use for storeing files
   */
  void init(frames_handler* parent, stex_file* next, j2k_frame_storer *storer,
            const char *name_template)
  {
    this->parent = parent;
    this->name_template = name_template;
    this->next = next;
    this->storer = storer;
  }

  /**
   *  @brief other threads can call this function to signal completion of 
   *         processing.  
   *
   *  This function basically reduces \"done\", and when 0 is reached
   *  the function will let the parent know that there is a stex_file 
   *  waiting removal.
   */
  void notify_file_completion();

public:  
  ojph::mem_outfile f;    //!<holds in-memory j2k codestream
  ui32 time_stamp;        //!<time stamp at which this file must be displayed
  ui32 last_seen_seq;     //!<the last seen RTP sequence number
  std::atomic_int done;   //!<saving is completed when 0 is reached
  ui32 frame_idx;         //!<frame number in the sequence
  frames_handler* parent; //!<the object holding this frame

  const char *name_template; //!<name template for saved files
  j2k_frame_storer* storer;  //!<stores a j2k frame using another thread

  stex_file* next;        //!<used to create files chain
};

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** @brief 
 * 
 *  Assumes packets arrive in order.
 * 
 */
class frames_handler
{
public:
  /**
   *  @brief default construction
   */
  frames_handler()
  { 
    quiet = false;
    num_threads = 0;
    target_name = NULL;
    num_files = 0;
    last_seq_number = last_time_stamp = 0;
    total_frames = trunc_frames = lost_frames = 0;
    files_store = in_use = avail = processing = NULL;
    num_complete_files.store(0);
    thread_pool = NULL;
    storers_store = NULL;
  }
  /**
   *  @brief default destructor
   */  
  ~frames_handler();

public:
  /**
   *  @brief call this function to initialize this object
   *
   *  The function just copies collected statistics
   *
   *  @param quiet when true, no messages are printed -- as of this writing
   *         the object prints no messages
   *  @param target_name a template for the saved file names
   *  @param thread_pool a thread pool for processing j2k codestreams
   *         (saving)
   * 
   */
  void init(bool quiet, const char *target_name, 
            thds::thread_pool* thread_pool);

  /**
   *  @brief call this function to push rtp_packets to this object
   *
   *  Packets received by this object has to be sequentially increasing; 
   *  older packets are ignored.  That is, a packet with a sequential number 
   *  smaller than the last observed sequential number is ignored.
   *
   *  @param p returns a pointer to the packet.
   */
  void push(rtp_packet* p);

  /**
   *  @brief call this function to collect statistics about frames
   *
   *  The function just copies collected statistics
   *
   *  @param total_frames returns the number of observed total frames
   *  @param trunc_frames returns the number of truncated frames
   *  @param lost_frames returns the number of lost frames -- for which the
   *                     main header payload packet was not received, but
   *                     time stamp was observed
   */
  void get_stats(ui32& total_frames, ui32& trunc_frames, ui32& lost_frames);

  /**
   *  @brief This function is not used, and therefore it is not clear how to
   *         use it.
   */
  bool flush();

  /**
   *  @brief other threads call this function to let frames_handler know that 
   *         processing is done.
   *
   *  This function basically increment the number of objects that need to 
   *  be moved from processing to avail.
   *
   */
  void increment_num_complete_files()
  { num_complete_files.fetch_add(1, std::memory_order_release); }

private:
  /**
   *  @brief call this function to process stex_file for which processing is
   *         complete
   *
   *  This function moves stex_file from processing to avail if storing 
   *  is complete.
   * 
   */
  void check_files_in_processing();

  /**
   *  @brief Handles complete/truncated files and send them for storing
   *
   *  This function moves stex_file from in_use to processing if there are
   *  further processors (such as a storer) or to avail if there are no 
   *  processors.
   */
  void send_to_processing();

private:
  bool quiet;               //!<no informational info is printed when true
  ui32 num_threads;         //!<number of threads used for saving
  const char *target_name;  //!<target file name template
  ui32 num_files;           //!<maximum number of in-flight files.
  ui32 last_seq_number;     //!<last observed sequence number
  ui32 last_time_stamp;     //!<last observed time stamp
  ui32 total_frames;        //!<total number of frames that were observed
  ui32 trunc_frames;        //!<truncated frames (because of a packet lostt)
  ui32 lost_frames;         //!<frames for which main header was not received
  stex_file* files_store;   //!<address for allocated files
  stex_file* in_use;        //!<the frame that is being filled with data
  stex_file* avail;         //!<available frames structures
  stex_file* processing;    //!<frames that are being saved
  std::atomic_int32_t 
    num_complete_files;     //<!num. of files for which processing is complete
  thds::thread_pool* 
    thread_pool;            //!<thread pool for processing frames
  j2k_frame_storer* 
    storers_store;          //!<address for allocated frame storers
};

} // !stex namespace
} // !ojph namespace

#endif //!OJPH_STR_EX_SUPPORT_H