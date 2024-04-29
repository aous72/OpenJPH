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
// File: ojph_threads.h
// Author: Aous Naman
// Date: 22 April 2024
//***************************************************************************/

#ifndef OJPH_THREADS_H
#define OJPH_THREADS_H

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>

namespace ojph
{
namespace thds
{

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** @brief A base object for queuing tasks in the thread_pool
 *  
 *  Tasks run in the thread_pool must derive from this function and define
 *  \"execute\".  Derived objects can include their own member variables.
 * 
 */
class worker_thread_base
{
public:
  /**
   *  @brief virtual construction is a necessity to deconstruct derived 
   *  objects.
   */
  virtual ~worker_thread_base() { }

  /**
   *  @brief Derived functions must define this function to execute its work
   */
  virtual void execute() = 0;
};


///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/** 
 *  @brief Implements a pool of threads, and can queue tasks.
 *  
 */
class thread_pool
{
public:
  /**
   *  @brief default constructor
   */
  thread_pool() { stop.store(false, std::memory_order_relaxed); }
  /**
   *  @brief default destructor
   */
  ~thread_pool();

public:
  /**
   *  @brief Initializes the thread pool
   * 
   *  @param num_threads the number of threads the thread pool holds
   */
  void init(size_t num_threads);

  /**
   *  @brief Adds a task to the thread pool
   *
   *  @param task the task to added, must be derived from worker_thread_base
   */
  void add_task(worker_thread_base* task);

  /**
   *  @brief Returns the number of threads in the thread pool
   *
   *  @retuen number of threads in the thread pool
   */
  size_t get_num_threads() { return threads.size(); }

private:
  /**
   *  @brief A static function to start a thread
   *
   *  @param tp a pointer to the thread pool
   */
  static void start_thread(thread_pool* tp);

private:
  std::vector<std::thread> threads;
  std::deque<worker_thread_base*> tasks;
  std::mutex mutex;
  std::condition_variable condition;
  std::atomic_bool stop;
};

} // !thds namespace 
} // !ojph namespace






#endif // !OJPH_THREADS_H