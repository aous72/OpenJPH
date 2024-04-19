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
// File: ojph_str_ex_support.cpp
// Author: Aous Naman
// Date: 18 April 2024
//***************************************************************************/

#ifndef OJPH_STR_EX_SUPPORT_H
#define OJPH_STR_EX_SUPPORT_H

#include "ojph_base.h"
#include "ojph_file.h"
#include "ojph_socket.h"

namespace ojph
{
namespace stex // stream expand
{
  class packets_handler;
  class frames_handler;

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** @brief inteprets RTP and j2k header, and holds received packets.
 * 
 *  This object interpret RFC 3550 and draft-ietf-avtcore-rtp-j2k-scl-00.
 */
struct rtp_packet
{
  enum packet_type : ui32
  {
    PT_BODY                  = 0, // this is body packet
    PT_MAIN_FOLLOWED_BY_MAIN = 1,
    PT_MAIN_FOLLOWED_BY_BODY = 2, 
    PT_MAIN                  = 3, // frame has only one main packet
  };
public:
  rtp_packet() { num_bytes = 0; next = NULL; }

public:
  ui32 get_rtp_version() { return ((ui32)data[0]) >> 6; }
  bool is_padded() { return (data[0] & 0x20) != 0; }
  bool is_extended() { return (data[0] & 0x10) != 0; }
  ui32 get_csrc_count() { return (ui32)(data[0]) & 0xF; }
  bool is_marked() { return (data[1] & 0x80) != 0; }
  ui32 get_payload_type() { return (ui32)(data[1]) & 0x7F; }
  ui32 get_sequence_number() {
    ui32 result = ntohs(*(ui16*)(data + 2));
    result |= ((ui32)data[15]) << 16;   // extended sequence (ESEQ)
    return result;
  }
  ui32 get_time_stamp() 
  { return ntohl(*(ui32*)(data + 4)); }
  ui32 get_ssrc()             // not used for the time being
  { return ntohl(*(ui32*)(data + 8)); }
  ui32 get_packet_type() 
  { return ((ui32)data[12]) >> 6; }
  ui32 get_TP() 
  { return (((ui32)data[12]) >> 3) & 0x7; }
  ui32 get_ORDH() 
  { return ((ui32)data[12]) & 0x7; }
  bool is_PTSTAMP_used() 
  { return (((ui32)data[13]) & 0x80) != 0; }
  ui32 get_XTRAC() 
  { return (((ui32)data[13]) >> 4) & 0x7; }
  ui32 get_PTSTAMP() { 
    ui32 result = (((ui32)data[13]) & 0xF) << 8;
    result |= (ui32)data[14];
    return result; 
  }
  bool is_codestream_header_reusable() 
  { return (((ui32)data[16] >> 7) & 1) != 0; }
  bool is_component_colorimetry_used() 
  { return (((ui32)data[16] >> 6) & 1) != 0; }
  bool is_codeblock_caching_used() 
  { return (((ui32)data[16] >> 5) & 1) != 0; }
  bool is_RANGE() 
  { return ((ui32)data[16] & 1) != 0; }
  ui32 get_PRIMS()
  { return (ui32)data[17]; }
  ui32 get_TRANS()
  { return (ui32)data[18]; }
  ui32 get_MAT()
  { return (ui32)data[19]; }

public:
  static constexpr int max_size = 2048; //!<maximum packet size
                                        // ethernet packet are only 1500
  ui8 data[max_size];                   //!<data in the packet
  int num_bytes;                        //!<number of bytes 
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
 *  It receives new packets, identifying main vs body J2K packets.
 *  It can buffer packets that are out of order and has no clear file
 *  associated with them.
 *  It also initiates creation of new files, when they are encountered.
 */
class packets_handler
{
public:
  packets_handler()
  { avail = in_use = NULL; num_packets = 0; packet_store = NULL; }
  ~packets_handler()
  { if (packet_store) delete[] packet_store; }

public:
  void init(bool quiet, ui32 num_packets, frames_handler* frames);
  rtp_packet* exchange(rtp_packet* p);

private:
  bool quiet;                //!<no informational info is printed when true
  rtp_packet* avail;         //!<start of available packets chain
  rtp_packet* in_use;        //!<start of used packet chain
  ui32 num_packets;          //!<maximum number of packets in packet_store
  rtp_packet* packet_store;  //!<address of packet memory allocation
  frames_handler* frames;    //!<frames object
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
 *  This objects holds a j2k codestream with source sequence number.
 *  It can be used to create a chain of files.
 */
struct stex_file {
public:
  stex_file() 
  { ssrc = 0; next = NULL; }

public:  
  ojph::mem_infile f;  //!<holds in-memory j2k codestream
  ui32 ssrc;           //!<source sequence number associated with this file
  ui32 timestamp;      //!<time stamp at which this file must be displayed
  stex_file* next;     //!<used to create files chain
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
 */
class frames_handler
{
public:
  frames_handler()
  { 
    quiet = false;
    num_threads = 0;
    target_name = NULL;
    num_files = 0;
    files = NULL; 
  }
  ~frames_handler();

public:
  void init(bool quiet, ui32 num_threads, const char *target_name, 
            bool display);

private:
  bool quiet;               //!<no informational info is printed when true
  ui32 num_threads;         //!<max number of threads used for decoding/display
  const char *target_name;  //!<target file name template
  bool display;             //!<images are displayed when true
  ui32 num_files;           //!<maximum number of in-flight files.
  stex_file* files;         //!<address for allocated files
};

} // !stex namespace
} // !ojph namespace

#endif //!OJPH_STR_EX_SUPPORT_H