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
#include "ojph_socket.h"
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
                   bool& quiet, bool& display, bool& decode)
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

  quiet = interpreter.reinterpret("-quiet");
  display = interpreter.reinterpret("-display");
  decode = interpreter.reinterpret("-decode");

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
      "\"localhost\" or a local network card IP address.\n");
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
  if (decode && target_name == NULL)
  {
    printf("Since \"-decode\" was specified, please set \"-target_name\" "
      "for the target name of decoded files.\n");
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
  ojph::ui32 num_threads = 1;
  ojph::ui32 num_inflight_packets = 5;
  bool quiet = false;
  bool display = false;
  bool decode = false;
	
  if (argc <= 1) {
    printf(
    "\n"
    "The following arguments are necessary:\n"
    " -addr         <receiving ipv4 address>, or\n"
    "               The address should be either localhost, or\n"
    "               a local network card IP address\n"
    "               example: -addr 127.0.0.1\n"
    " -port         <listening port>\n"
    "\n"
    "The following arguments are options:\n"
    " -src_addr     <source ipv4 address>, packets from other sources\n"
    "               will be ignored. If not specified, then packets\n"
    "               from any source are accepted.\n"
    " -src_port     <source port>, packets from other source ports are\n"    
    "               ignored. If not specified, then packets from any\n"
    "               port are accepted -- I would recommend not leaving\n"
    "               this one out."
    " -num_threads  <integer> number of threads for decoding and\n"
    "               displaying files.  It is also the number of files that\n"
    "               are in flight; i.e., not completely saved yet.\n"
    " -num_packets  <integer> number of in-flight packets; this is the\n"
    "               maximum number of packets to wait before an out-of-order\n"
    "               or lost packet is considered lost.\n"
    " -o            <string> target file name without extension; the same\n"
    "               printf formating can be used. For example, output_%%05d.\n"
    "               An extension will be added, either .j2c for original\n"
    "               frames, or .ppm for decoded images.\n"
    " -display      use this to display decoded frames.\n"
    " -quiet        use to stop printing informative messages.\n."
    "\n"
    );
    exit(-1);
  }
  if (!get_arguments(argc, argv, recv_addr, recv_port, src_addr, src_port,
                     target_name, num_threads, num_inflight_packets,
                     quiet, display, decode))
  {
    exit(-1);
  }

  ojph::stex::frames_handler frames_handler;
  frames_handler.init(quiet, display, decode, num_threads, target_name);
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
      OJPH_ERROR(0x02000001, "Please provide a valid IP address when "
        "using \"-addr,\" the provided address %s is not valid\n", 
        recv_addr);
    ojph::ui16 port_number = 0;
    port_number = (ojph::ui16)atoi(recv_port);
    if (port_number == 0)
      OJPH_ERROR(0x02000003, "Please provide a valid port number. "
          "The number you provided is %d\n", recv_port);
    server.sin_port = htons(port_number);
  }

  // create a socket
  ojph::net::socket s;
  s = smanager.create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(s.intern() == OJPH_INVALID_SOCKET)
  {
    std::string err = smanager.get_last_error_message();
    OJPH_ERROR(0x02000004, "Could not create socket : %s\n", err.data());
  }

  // bind to listening address
  if( bind(s.intern(), (struct sockaddr *)&server, sizeof(server)) == -1)
  {
    std::string err = smanager.get_last_error_message();
    OJPH_ERROR(0x02000005, 
      "Could not bind address to socket : %s\n", err.data());
  }

  // listen to incoming data, and forward it to packet_handler
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
      OJPH_ERROR(0x02000006, "Please provide a valid IP address when "
        "using \"-src_addr,\" the provided address %s is not valid\n", 
        src_addr);
    saddr = smanager.get_addr(t);
  }
  ojph::ui16 sport = 0;
  if (src_addr)
  {
    sport = (ojph::ui16)atoi(src_port);
    if (sport == 0)
      OJPH_ERROR(0x02000007, "Please provide a valid port number. "
          "The number you provided is %d\n", src_port);
  }

  bool src_printed = false;
  ojph::stex::rtp_packet* packet = NULL;
  while (1)
  {
    if (packet == NULL || packet->num_bytes != 0) // num_bytes == 0
      packet = packets_handler.exchange(packet);  // if packet was ignored

    struct sockaddr_in si_other;
    socklen_t socklen = sizeof(si_other);
    // receive data -- this is a blocking call
    packet->num_bytes = 0; // if we ignore the packet, we can continue
    int num_bytes = (int)recvfrom(s.intern(), (char*)packet->data, 
      packet->max_size, 0, (struct sockaddr *) &si_other, &socklen);
    if (num_bytes < 0)
    {
      std::string err = smanager.get_last_error_message();
      OJPH_INFO(0x02000008, "Failed to receive data : %s\n", err.data());
      continue; // if we wish to continue
    }
    if ((src_addr && saddr != smanager.get_addr(si_other)) ||
        (src_port && sport != si_other.sin_port)) {
      continue;
    }
    packet->num_bytes = (ojph::ui32)num_bytes;    

    if (!quiet && !src_printed)
    {
      constexpr int buf_size = 128;
      char buf[buf_size];
      if (!inet_ntop(AF_INET, &si_other, buf, buf_size)) {
        std::string err = smanager.get_last_error_message();
        OJPH_INFO(0x02000009, 
          "Error converting source address.\n", err.data());
      }
      printf("Receiving data from %s, port %d\n",
        buf, ntohs(si_other.sin_port));
      src_printed = true;
    }
  }

  s.close();
  return 0;
}

