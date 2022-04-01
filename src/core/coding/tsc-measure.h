/* Copyright (c) 2013, Scott Tsai
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TSC_MEASURE_H
#define TSC_MEASURE_H

#include <stdint.h>

/* See: How to Benmark Code Execution Times on Intel IA32 and IA-64
 * Instruction Set Architectures,
 * http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
 *
 * */

#if !defined(__i386__) && !defined(__x86_64__)
#error tsc-measure.h only supports x86 and x86-64
#endif

/* -> # of cycles */
__attribute__((gnu_inline, always_inline))
static uint64_t __inline__ tsc_measure_start(void)
{
	uint32_t tl, th;
	__asm__ volatile ("cpuid;"
			"rdtsc;"
			"movl %%edx, %[th];"
			"movl %%eax, %[tl];"
			: /* outputs */
			[th] "=r" (th),
			[tl] "=r" (tl)
			: /* inputs */
			: /* clobbers */
			"rax", "rbx", "rcx", "rdx"
			);
	return (((uint64_t)(th)) << 32) | tl;
}

/* -> # of cycles */
__attribute__((gnu_inline, always_inline))
static uint64_t __inline__ tsc_measure_stop(void)
{
	uint32_t tl, th;
	__asm__ volatile ("rdtscp;"
			"movl %%edx, %[th];"
			"movl %%eax, %[tl];"
			"cpuid;"
			: /* outputs */
			[th] "=r" (th),
			[tl] "=r" (tl)
			: /* inputs */
			: /* clobbers */
			"rax", "rbx", "rcx", "rdx"
			);
	return (((uint64_t)th) << 32) | tl;
}

#endif
