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
// File: threaded_frame_processors.h
// Author: Aous Naman
// Date: 23 April 2024
//***************************************************************************/

#ifndef THREADED_FRAME_PROCESSOR_H
#define THREADED_FRAME_PROCESSOR_H

#include "ojph_threads.h"
#include "stream_expand_support.h"

namespace ojph
{
  namespace thds 
  { class thread_pool; }

namespace stex
{

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** @brief Saves a j2k frame to disk without decoding.
 * 
 */
struct j2k_frame_storer : public thds::worker_thread_base
{
public:  
  /**
   * @brief default construction
   */
  j2k_frame_storer() {
    file = NULL;
    name_template = NULL;
  }
  /**
   * @brief default destructor doing nothing
   */
  ~j2k_frame_storer() override {}

public:  
  /**
   *  @brief call this function to initialize its members
   * 
   *  @param file is a stex_file holding the j2k codestream with other
   *         variables.
   *  @param name_template holds the a filename template
   */
  void init(stex_file* file, const char* name_template)
  {
    this->file = file;
    this->name_template = name_template;
  }

  /**
   * @brief A thread from the thread_pool call this function to execute 
   *        the task
   */
  void execute() override;

private:
  stex_file* file;            //!<a j2k codestream file with other variables
  const char* name_template;  //!<a template for the target file name
};

} // !stex namespace
} // !ojph namespace

#endif // !THREADED_FRAME_PROCESSOR_H