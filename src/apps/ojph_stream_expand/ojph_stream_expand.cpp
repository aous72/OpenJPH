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
#include "ojph_str_ex_support.h"

#ifdef OJPH_OS_WINDOWS

#else
  #include <arpa/inet.h>
#endif

//////////////////////////////////////////////////////////////////////////////
static
bool get_arguments(int argc, char *argv[],
                   char *&recv_addr, char *&recv_port, char *&target_name,
                   ojph::ui32& num_threads, 
                   ojph::ui32& num_inflight_packets,
                   bool& display, bool& decode, bool& store)
{
  ojph::cli_interpreter interpreter;
  interpreter.init(argc, argv);

  interpreter.reinterpret("-addr", recv_addr);
  interpreter.reinterpret("-port", recv_port);
  interpreter.reinterpret("-o", target_name);
  interpreter.reinterpret("-num_threads", num_threads);
  interpreter.reinterpret("-num_packets", num_inflight_packets);

  display = interpreter.reinterpret("-display");
  decode = interpreter.reinterpret("-decode");
  store = interpreter.reinterpret("-store");

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
  if (store && target_name == NULL)
  {
    printf("Please use \"-o\" to provide a target file name.\n");
    return false;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  constexpr int buffer_size = 2048;	// buffer size

  char *recv_addr = NULL;
  char *recv_port = NULL;
  char *target_name = NULL;
  ojph::ui32 num_threads = 0;
  ojph::ui32 num_inflight_packets = 5;
  bool display = false;
  bool decode = false;
  bool store = false;
	
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
    " -num_threads  <integer> number of threads for decoding and\n"
    "               displaying files.  This number is ignored unless\n"
    "               decode or display is selected, where it represents the\n"
    "               number of working threads."
    " -num_packets  <integer> number of in-flight packets; this is the\n"
    "               maximum number of packets to wait before an out-of-order\n"
    "               or lost packet is considered lost.\n"
    " -target_name  <string> target file name without extension; the same\n"
    "               printf formating can be used. For example, output_%%05d.\n"
    "               An extension will be added, either .j2c for original\n"
    "               frames, or .ppm for decoded images.\n"
    " -display      use this to display decoded frames.\n"
    " -decode       use this to decode files and store them.\n"
    " -store        use this to store encoded files.\n."
    "\n"
    );
    exit(-1);
  }
  if (!get_arguments(argc, argv, recv_addr, recv_port, 
                     target_name, num_threads, num_inflight_packets,
                     display, decode, store))
  {
    exit(-1);
  }

  ojph::str_ex::ojph_frames_handler frames_handler;
  frames_handler.init(num_threads);
  ojph::str_ex::ojph_packets_handler packets_handler;
  packets_handler.init(num_inflight_packets, &frames_handler);
  ojph::net::socket_manager smanager;

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  int result = inet_pton(AF_INET, recv_addr, &server.sin_addr);
  if (result != 1)
    OJPH_ERROR(0x02000001, "Please provide a valid ip address, "
      "the provided address %s is not valid\n", recv_addr);

  {
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

  // bind
  if( bind(s.intern(), (struct sockaddr *)&server, sizeof(server)) == -1)
  {
    std::string err = smanager.get_last_error_message();
    OJPH_ERROR(0x02000005, 
      "Could not bind address to socket : %s\n", err.data());
  }

  // listen to incoming data, and forward it to packet_handler
  bool first_packet = true;
  ULONG src_addr;
  USHORT src_port;
  ojph::str_ex::packet* pac = NULL;
  while(1)
  {
    pac = packets_handler.exchange(pac);

    // receive data -- this is a blocking call
    bool success = true;
    do {
      struct sockaddr_in si_other;
      socklen_t socklen = sizeof(si_other);  
      pac->num_bytes = (int)recvfrom(s.intern(), pac->data, buffer_size, 0, 
        (struct sockaddr *) &si_other, &socklen);
      if (pac->num_bytes < 0)
      {
        std::string err = smanager.get_last_error_message();
        OJPH_INFO(0x02000006, "Could not receive data : %s\n", err.data());
        continue; // if we wish to continue
      }
      if (first_packet) {
        // this is to ignore packets from source other than the first source
        first_packet = false;
        src_addr = si_other.sin_addr.S_un.S_addr;
        src_port = si_other.sin_port;
        break;
      }
      success = (si_other.sin_addr.S_un.S_addr == src_addr);
      success = success && (si_other.sin_port != src_port);
    } while (!success);
  }

  s.close();
  return 0;
}

