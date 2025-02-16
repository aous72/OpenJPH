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
// File: ojph_stream_expand.cpp
// Author: Aous Naman
// Date: 17 April 2024
//***************************************************************************/

#include <iostream>
#include "ojph_message.h"
#include "ojph_arg.h"
#include "ojph_sockets.h"
#include "ojph_threads.h"
#include "stream_expand_support.h"

#ifdef OJPH_OS_WINDOWS

#else
  #include <arpa/inet.h>
#endif

//////////////////////////////////////////////////////////////////////////////
static
bool get_arguments(int argc, char *argv[],
                   char *&recv_addr, char *&recv_port, 
                   char *&src_addr, char *&src_port, 
                   char *&target_name, ojph::ui32& num_threads, 
                   ojph::ui32& num_inflight_packets,
                   ojph::ui32& recvfrm_buf_size, bool& blocking,
                   bool& quiet)
{
  ojph::cli_interpreter interpreter;
  interpreter.init(argc, argv);

  interpreter.reinterpret("-addr", recv_addr);
  interpreter.reinterpret("-port", recv_port);
  interpreter.reinterpret("-src_addr", src_addr);
  interpreter.reinterpret("-src_port", src_port);
  interpreter.reinterpret("-o", target_name);
  interpreter.reinterpret("-num_threads", num_threads);
  interpreter.reinterpret("-num_packets", num_inflight_packets);
  interpreter.reinterpret("-recv_buf_size", recvfrm_buf_size);

  blocking = interpreter.reinterpret("-blocking");
  quiet = interpreter.reinterpret("-quiet");

  if (interpreter.is_exhausted() == false) {
    printf("The following arguments were not interpreted:\n");
    ojph::argument t = interpreter.get_argument_zero();
    t = interpreter.get_next_avail_argument(t);
    while (t.is_valid()) {
      printf("%s\n", t.arg);
      t = interpreter.get_next_avail_argument(t);
    }
    return false;
  }

  if (recv_addr == NULL)
  {
    printf("Please use \"-addr\" to provide a receiving address, "
      "\"localhost\" or a local network card IPv4 address.\n");
    return false;
  }
  if (recv_port == NULL)
  {
    printf("Please use \"-port\" to provide a port number.\n");
    return false;
  }
  if (num_threads < 1)
  {
    printf("Please set \"-num_threads\" to 1 or more.\n");
    return false;
  }
  if (num_inflight_packets < 1)
  {
    printf("Please set \"-num_packets\" to 1 or more.\n");
    return false;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  char *recv_addr = NULL;
  char *recv_port = NULL;
  char *src_addr = NULL;
  char *src_port = NULL;
  char *target_name = NULL;
  ojph::ui32 num_threads = 2;
  ojph::ui32 num_inflight_packets = 5;
  ojph::ui32 recvfrm_buf_size = 65536;
  bool blocking = false;
  bool quiet = false;
	
  if (argc <= 1) {
    printf(
    "\n"
    "The following arguments are necessary:\n"
    " -addr          <receiving IPv4 address>, or\n"
    "                The address should be either localhost, or\n"
    "                a local network card IPv4 address\n"
    "                example: -addr 127.0.0.1\n"
    " -port          <listening port>\n"
    "\n"
    "The following arguments are options:\n"
    " -src_addr      <source ipv4 address>, packets from other sources\n"
    "                will be ignored. If not specified, then packets\n"
    "                from any source are accepted.\n"
    " -src_port      <source port>, packets from other source ports are\n"    
    "                ignored. If not specified, then packets from any\n"
    "                port are accepted -- I would recommend not leaving\n"
    "                this one out.\n"
    " -recv_buf_size <integer> recvfrom buffer size; default is 65536.\n"
    "                This is the size of the operating system's receive\n"
    "                buffer, before packets are picked by the program.\n"
    "                Larger buffers reduces the likelihood that a packet\n"
    "                is dropped before the program has a chance to pick it.\n"
    " -blocking      sets the receiving socket blocking mode to blocking.\n"
    "                The default mode is non-blocking. A blocking socket\n"
    "                increases the likelihood of not receiving some\n"
    "                packets; this is because the thread get into sleep\n"
    "                state, and therefore takes sometime to wakeup. A\n"
    "                non-blocking socket increase power consumption,\n"
    "                because it prevents the thread from sleeping.\n"
    " -num_threads   <integer> number of threads for decoding and\n"
    "                displaying files.  This number also determines the\n"
    "                number of in-flight files, not completely\n"
    "                saved/processed yet. The number of files is set to\n"
    "                number of threads + 1\n"
    " -num_packets   <integer> number of in-flight packets; this is a\n"
    "                window of packets in which packets can be re-ordered.\n"
    " -o             <string> target file name without extension; the same\n"
    "                printf formating can be used. For example,\n"
    "                output_%%05d. An extension will be added, either .j2c\n"
    "                for original frames, or .ppm for decoded images.\n"
    " -quiet         use to stop printing informative messages.\n."
    "\n"
    );
    exit(-1);
  }
  if (!get_arguments(argc, argv, recv_addr, recv_port, src_addr, src_port,
                     target_name, num_threads, num_inflight_packets,
                     recvfrm_buf_size, blocking, quiet))
  {
    exit(-1);
  }

  try {
    ojph::thds::thread_pool thread_pool;
    thread_pool.init(num_threads);
    ojph::stex::frames_handler frames_handler;
    frames_handler.init(quiet, target_name, &thread_pool);
    ojph::stex::packets_handler packets_handler;
    packets_handler.init(quiet, num_inflight_packets, &frames_handler);
    ojph::net::socket_manager smanager;

    // listening address/port
    struct sockaddr_in server;
    {
      server.sin_family = AF_INET;
      const char *p = recv_addr;
      const char localhost[] = "127.0.0.1";
      if (strcmp(recv_addr, "localhost") == 0)
        p = localhost;
      int result = inet_pton(AF_INET, p, &server.sin_addr);
      if (result != 1)
        OJPH_ERROR(0x02000001, "Please provide a valid IPv4 address when "
          "using \"-addr,\" the provided address %s is not valid", 
          recv_addr);
      ojph::ui16 port_number = 0;
      port_number = (ojph::ui16)atoi(recv_port);
      if (port_number == 0)
        OJPH_ERROR(0x02000002, "Please provide a valid port number. "
            "The number you provided is %d", recv_port);
      server.sin_port = htons(port_number);
    }

    // create a socket
    ojph::net::socket s;
    s = smanager.create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s.intern() == OJPH_INVALID_SOCKET)
    {
      std::string err = smanager.get_last_error_message();
      OJPH_ERROR(0x02000003, "Could not create socket: %s", err.data());
    }

    // change recv buffer size; default is 65536
    if (::setsockopt(s.intern(), SOL_SOCKET, SO_RCVBUF,
                   (char*)&recvfrm_buf_size, sizeof(recvfrm_buf_size)) == -1)
    {
      std::string err = smanager.get_last_error_message();
      OJPH_INFO(0x02000001,
        "Failed to expand receive buffer: %s", err.data());
    }

    // set socket to non-blocking
    if (s.set_blocking_mode(blocking) == false)
    {
      std::string err = smanager.get_last_error_message();
      OJPH_INFO(0x02000002,
        "Failed to set the socket's blocking mode to %s, with error %s", 
        blocking ? "blocking" : "non-blocking", err.data());
    }

    // bind to listening address
    if(bind(s.intern(), (struct sockaddr *)&server, sizeof(server)) == -1)
    {
      std::string err = smanager.get_last_error_message();
      OJPH_ERROR(0x02000004, 
        "Could not bind address to socket: %s", err.data());
    }

    if (!quiet) {
      constexpr int buf_size = 128;
      char buf[buf_size];
      ojph::ui32 addr = smanager.get_addr(server);
      const char* t = inet_ntop(AF_INET, &addr, buf, buf_size);
      if (t == NULL) {
        std::string err = smanager.get_last_error_message();
        OJPH_INFO(0x02000005,
          "Error converting source address: %s", err.data());
      }
      printf("Listening on %s, port %d\n", t, ntohs(server.sin_port));
    }

    // process the source IPv4 address and port
    ojph::ui32 saddr = 0;
    if (src_addr)
    {
      const char *p = src_addr;
      const char localhost[] = "127.0.0.1";
      if (strcmp(src_addr, "localhost") == 0)
        p = localhost;
      struct sockaddr_in t;
      int result = inet_pton(AF_INET, p, &t.sin_addr);
      if (result != 1)
        OJPH_ERROR(0x02000005, "Please provide a valid IPv4 address when "
          "using \"-src_addr,\" the provided address %s is not valid", 
          src_addr);
      saddr = smanager.get_addr(t);
    }
    ojph::ui16 sport = 0;
    if (src_addr)
    {
      sport = (ojph::ui16)atoi(src_port);
      if (sport == 0)
        OJPH_ERROR(0x02000006, "Please provide a valid port number. "
            "The number you provided is %d", src_port);
    }

    // listen to incoming data, and forward it to packet_handler
    struct sockaddr_in si_other;
    socklen_t socklen = sizeof(si_other);
    bool src_printed = false;
    ojph::stex::rtp_packet* packet = NULL;
    ojph::ui32 last_time_stamp = 0;
    while (1)
    {
      if (packet == NULL || packet->num_bytes != 0)
        packet = packets_handler.exchange(packet);
      if (packet == NULL)
        continue;
      packet->num_bytes = 0;

      // receive data
      int num_bytes = (int)recvfrom(s.intern(), (char*)packet->data,
        packet->max_size, 0, (struct sockaddr*)&si_other, &socklen);

      if (num_bytes < 0) // error or non-blocking call
      {
        int last_error = smanager.get_last_error();
        if (last_error != OJPH_EWOULDBLOCK)
        {
          std::string err = smanager.get_error_message(last_error);
          OJPH_INFO(0x02000003, "Failed to receive data: %s", err.data());
        }
        continue; // if we wish to continue
      }

      if ((src_addr && saddr != smanager.get_addr(si_other)) ||
        (src_port && sport != si_other.sin_port)) {
        constexpr int buf_size = 128;
        char buf[buf_size];
        ojph::ui32 addr = smanager.get_addr(si_other);
        const char* t = inet_ntop(AF_INET, &addr, buf, buf_size);
        if (t == NULL) {
          std::string err = smanager.get_last_error_message();
          OJPH_INFO(0x02000004,
            "Error converting source address: %s", err.data());
        }
        printf("Source mismatch %s, port %d\n",
          t, ntohs(si_other.sin_port));
        continue;
      }

      packet->num_bytes = (ojph::ui32)num_bytes;

      if (last_time_stamp == 0)
        last_time_stamp = packet->get_time_stamp();

      if (!quiet && !src_printed)
      {
        constexpr int buf_size = 128;
        char buf[buf_size];
        ojph::ui32 addr = smanager.get_addr(si_other);
        const char* t = inet_ntop(AF_INET, &addr, buf, buf_size);
        if (t == NULL) {
          std::string err = smanager.get_last_error_message();
          OJPH_INFO(0x02000005, 
            "Error converting source address: %s", err.data());
        }
        printf("Receiving data from %s, port %d\n",
          t, ntohs(si_other.sin_port));
        src_printed = true;
      }

      if (!quiet)
        if (packet->get_time_stamp() >= last_time_stamp + 45000)
        { // One second is 90000
          last_time_stamp = packet->get_time_stamp();
          ojph::ui32 lost_packets = packets_handler.get_num_lost_packets();
          ojph::ui32 total_frames = 0, trunc_frames = 0, lost_frames = 0;
          frames_handler.get_stats(total_frames, trunc_frames, lost_frames);

          printf("Total frame %d, truncated frames %d, lost frames %d, "
            "packets lost %d\n",
            total_frames, trunc_frames, lost_frames, lost_packets);
        }
    }
    s.close();    
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
    exit(-1);
  }    

  return 0;
}

