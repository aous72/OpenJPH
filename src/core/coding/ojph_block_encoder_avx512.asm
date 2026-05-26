;***************************************************************************/
; This software is released under the 2-Clause BSD license, included
; below.
;
; Copyright (c) 2019, Aous Naman
; Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
; Copyright (c) 2019, The University of New South Wales, Australia
; Copyright (c) 2026, Osamu Watanabe
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are
; met:
;
; 1. Redistributions of source code must retain the above copyright
; notice, this list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above copyright
; notice, this list of conditions and the following disclaimer in the
; documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
; IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
; TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
; PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
; HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
; TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;***************************************************************************/
;
; ojph_block_encoder_avx512.asm
; NASM x86-64 implementation of MagSgn batch encoding for the HTJ2K encoder.
;
; Replaces the scalar inner loop of proc_ms_encode.  Keeps the bit
; accumulator state register-resident across all 64 codewords,
; eliminating unnecessary memory stores caused by pointer aliasing
; in the C++ version.  See docs/nasm_design.md for rationale.
;
; Requires BMI1 (tzcnt) and BMI2 (shlx, shrx, bzhi).

section .note.GNU-stack noalloc noexec nowrite progbits

section .text

; ms_struct field offsets (verified with offsetof, see C++ definition)
%define MS_BUF        0
%define MS_POS        8
%define MS_USED_BITS  16
%define MS_TMP        24
%define MS_LAST_FF    32

; Stack frame layout (32 bytes, allocated with sub rsp):
;   [rsp +  0] = SWAR constant 0x0101010101010101
;   [rsp +  8] = SWAR constant 0x8080808080808080
;   [rsp + 16] = saved cwd_len  (used in overflow path)
;   [rsp + 20] = saved avail    (used in overflow path)
;
; Inside .drain (called via call, return address on stack):
;   [rsp +  8] = 0x0101...
;   [rsp + 16] = 0x8080...
%define STK_0101      0
%define STK_8080      8
%define STK_CWDLEN    16
%define STK_AVAIL     20
%define STK_0101_D    8        ; offset inside .drain (call pushes 8)
%define STK_8080_D    16

; Register allocation:
;   r15  = ms_struct pointer (for final writeback)
;   r14  = tmp (64-bit bit accumulator)
;   r13  = used_bits in bits 0-6, last_was_ff in bit 8
;   r12  = buf base pointer
;   ebp  = pos
;   rsi  = cwd array cursor   (advances +16 per group)
;   rdx  = cwd_len array cursor
;   rdi  = end pointer (cwd + 256)
;   rbx  = scratch, preserved across drain calls
;   rax, rcx, r8-r11 = scratch

global ojph_ms_encode_batch

ojph_ms_encode_batch:
    push   rbx
    push   rbp
    push   r12
    push   r13
    push   r14
    push   r15
    sub    rsp, 32

    mov    rax, 0x0101010101010101
    mov    [rsp + STK_0101], rax
    mov    rax, 0x8080808080808080
    mov    [rsp + STK_8080], rax

    ; Load ms_struct state into registers
    mov    r15, rdi
    mov    r12, [rdi + MS_BUF]
    mov    ebp, [rdi + MS_POS]
    mov    r14, [rdi + MS_TMP]
    mov    eax, [rdi + MS_USED_BITS]
    movzx  r13d, byte [rdi + MS_LAST_FF]
    shl    r13d, 8
    or     r13d, eax

    lea    rdi, [rsi + 256]

; =================================================================
; Main loop: 16 groups of 4 codewords
;
; Prefix-sum the shift amounts so the three shlx instructions can
; issue as soon as their respective prefix sum is ready, instead of
; waiting for the serial add chain.
; =================================================================
.group_loop:
    ; --- Load all 4 cwd values ---
    mov    eax, [rsi]           ; cwd[0]
    mov    r8d, [rsi + 4]       ; cwd[1]
    mov    r9d, [rsi + 8]       ; cwd[2]
    mov    r10d, [rsi + 12]     ; cwd[3]

    ; --- Prefix sums of cwd_len: a, ab, abc, abcd ---
    mov    ecx, [rdx]           ; a   = cwd_len[0]
    shlx   r8, r8, rcx          ; cwd[1] << a  (issue as soon as a is loaded)
    mov    r11d, [rdx + 4]      ; b   = cwd_len[1]
    add    r11d, ecx            ; ab  = a + b
    or     rax, r8              ; _cwd |= cwd[1] << a
    mov    ebx, [rdx + 8]       ; c   = cwd_len[2]
    add    ebx, r11d            ; abc = ab + c
    mov    ecx, [rdx + 12]      ; d   = cwd_len[3]
    add    ecx, ebx             ; abcd = abc + d = total

    ; --- Check if all 4 fit ---
    cmp    ecx, 64
    jg     .split

    ; --- Fast path: combine remaining two (shifts use precomputed sums) ---
    shlx   r9, r9, r11          ; cwd[2] << ab
    or     rax, r9
    shlx   r10, r10, rbx        ; cwd[3] << abc
    or     rax, r10

; -----------------------------------------------------------------
; ms_encode_nodefer (inline) — main entry
; Input: rax = cwd, ecx = cwd_len
; -----------------------------------------------------------------
.encode:
    movzx  r8d, r13b
    mov    r9d, 64
    sub    r9d, r8d
    cmp    ecx, r9d
    jg     .encode_overflow
    shlx   rax, rax, r8
    or     r14, rax
    add    r13b, cl

.next_group:
    add    rsi, 16
    add    rdx, 16
    cmp    rsi, rdi
    jb     .group_loop
    jmp    .final_drain

.encode_overflow:
    bzhi   r10, rax, r9
    shlx   r10, r10, r8
    or     r14, r10
    mov    r13b, 64
    mov    rbx, rax
    mov    [rsp + STK_CWDLEN], ecx
    mov    [rsp + STK_AVAIL], r9d
    call   ojph_ms_drain_internal
    mov    rax, rbx
    mov    ecx, [rsp + STK_CWDLEN]
    mov    r9d, [rsp + STK_AVAIL]
    shrx   rax, rax, r9
    sub    ecx, r9d
    jmp    .encode

; -----------------------------------------------------------------
; Split path: encode pair 0 and pair 1 separately.
; At entry from group_loop: rax = cwd[0]|(cwd[1]<<a), r11d = ab.
; -----------------------------------------------------------------
.split:
    mov    ecx, r11d            ; pair0 cwd_len = ab
.split_encode_p0:
    movzx  r8d, r13b
    mov    r9d, 64
    sub    r9d, r8d
    cmp    ecx, r9d
    jg     .split_p0_overflow
    shlx   rax, rax, r8
    or     r14, rax
    add    r13b, cl
    jmp    .split_pair1

.split_p0_overflow:
    bzhi   r10, rax, r9
    shlx   r10, r10, r8
    or     r14, r10
    mov    r13b, 64
    mov    rbx, rax
    mov    [rsp + STK_CWDLEN], ecx
    mov    [rsp + STK_AVAIL], r9d
    call   ojph_ms_drain_internal
    mov    rax, rbx
    mov    ecx, [rsp + STK_CWDLEN]
    mov    r9d, [rsp + STK_AVAIL]
    shrx   rax, rax, r9
    sub    ecx, r9d
    jmp    .split_encode_p0

.split_pair1:
    mov    eax, [rsi + 8]
    mov    ecx, [rdx + 8]
    mov    r8d, [rsi + 12]
    shlx   r8, r8, rcx
    or     rax, r8
    add    ecx, [rdx + 12]
    jmp    .encode

; =================================================================
; Final drain + epilogue
; =================================================================
.final_drain:
    call   ojph_ms_drain_internal

    ; Write state back to ms_struct
    mov    [r15 + MS_TMP], r14
    movzx  eax, r13b
    mov    [r15 + MS_USED_BITS], eax
    mov    [r15 + MS_POS], ebp
    bt     r13d, 8
    setc   al
    mov    [r15 + MS_LAST_FF], al

    add    rsp, 32
    pop    r15
    pop    r14
    pop    r13
    pop    r12
    pop    rbp
    pop    rbx
    ret

; =================================================================
; ms_drain: drain complete bytes from the accumulator to the buffer.
;
; Shared by ojph_ms_encode_batch and ojph_ms_encode_pairs.
; Both callers use the same register convention and stack layout.
;
; Modifies: r14 (tmp), r13 (used_bits|flags), ebp (pos)
; Clobbers: rax, rcx, r8, r9, r10, r11
; Preserves: rbx, rsi, rdx, rdi, r12, r15
;
; Stack constants (shifted +8 by the call):
;   [rsp + STK_0101_D] = 0x0101010101010101
;   [rsp + STK_8080_D] = 0x8080808080808080
; =================================================================
ojph_ms_drain_internal:
.drain:
    test   r13d, 0x100
    jnz    .drain_after_ff

.drain_loop:
    cmp    r13b, 8
    jb     .drain_ret

    movzx  eax, r13b
    shr    eax, 3
    cmp    eax, 8
    jle    .drain_n_ok
    mov    eax, 8
.drain_n_ok:

    ; SWAR 0xFF detection: detect bytes equal to 0xFF in tmp.
    ; Formula: ff = (~tmp - 0x0101...) & tmp & 0x8080...
    mov    r9, r14
    not    r9
    mov    r8, r9
    sub    r8, [rsp + STK_0101_D]
    and    r8, r14
    and    r8, [rsp + STK_8080_D]

    ; Apply valid_mask for n_bytes < 8
    cmp    eax, 8
    je     .drain_check_ff
    mov    ecx, eax
    shl    ecx, 3
    or     r9, -1
    bzhi   r9, r9, rcx
    and    r8, r9

.drain_check_ff:
    test   r8, r8
    jnz    .drain_has_ff

    ; --- No 0xFF: bulk store ---
    mov    [r12 + rbp], r14
    add    ebp, eax
    mov    ecx, eax
    shl    ecx, 3
    cmp    ecx, 64
    jge    .drain_clear
    shrx   r14, r14, rcx
    sub    r13b, cl
    jmp    .drain_loop

.drain_clear:
    xor    r14d, r14d
    sub    r13b, cl
    jmp    .drain_loop

    ; --- 0xFF found: write bytes up to and including the 0xFF ---
.drain_has_ff:
    tzcnt  r8, r8
    shr    r8d, 3
    lea    ecx, [r8d + 1]
    mov    [r12 + rbp], r14
    add    ebp, ecx
    shl    ecx, 3
    cmp    ecx, 64
    jge    .drain_ff_clear
    shrx   r14, r14, rcx
    sub    r13b, cl
    jmp    .drain_post_ff

.drain_ff_clear:
    xor    r14d, r14d
    sub    r13b, cl

    ; After 0xFF byte, next byte must be 7 bits (JPEG 2000 byte stuffing)
.drain_post_ff:
    cmp    r13b, 7
    jb     .drain_set_ff
    mov    al, r14b
    and    al, 0x7F
    mov    [r12 + rbp], al
    inc    ebp
    shr    r14, 7
    sub    r13b, 7
    and    r13d, 0xFF
    jmp    .drain_loop

.drain_set_ff:
    or     r13d, 0x100

.drain_ret:
    ret

    ; Entry when last byte written was 0xFF
.drain_after_ff:
    cmp    r13b, 7
    jb     .drain_ret
    mov    al, r14b
    and    al, 0x7F
    mov    [r12 + rbp], al
    inc    ebp
    shr    r14, 7
    sub    r13b, 7
    and    r13d, 0xFF
    jmp    .drain_loop

; =================================================================
; MagSgn pair encoder
;
; Processes 32 pre-combined pairs (ui64 cwd, int cwd_len) produced
; by SIMD pair-wise combining in proc_ms_encode.  Processes 16
; groups of 2 pairs, combining each group with one shift+OR.
;
; void ojph_ms_encode_pairs(void *msp,
;     const ui64 *cwd, const int *cwd_len);
;
; System V ABI: rdi=msp, rsi=cwd (32 x ui64), rdx=cwd_len (32 x int)
;
; Register allocation: same as ojph_ms_encode_batch
; =================================================================

global ojph_ms_encode_pairs

ojph_ms_encode_pairs:
    push   rbx
    push   rbp
    push   r12
    push   r13
    push   r14
    push   r15
    sub    rsp, 32

    mov    rax, 0x0101010101010101
    mov    [rsp + STK_0101], rax
    mov    rax, 0x8080808080808080
    mov    [rsp + STK_8080], rax

    ; Load ms_struct state into registers
    mov    r15, rdi
    mov    r12, [rdi + MS_BUF]
    mov    ebp, [rdi + MS_POS]
    mov    r14, [rdi + MS_TMP]
    mov    eax, [rdi + MS_USED_BITS]
    movzx  r13d, byte [rdi + MS_LAST_FF]
    shl    r13d, 8
    or     r13d, eax

    lea    rdi, [rsi + 256]          ; end = cwd + 32 (32 x 8 = 256 bytes)

; =================================================================
; Main loop: 16 groups of 2 pre-combined pairs
; =================================================================
.p_group_loop:
    ; Load 2 pre-combined pairs (64-bit each)
    mov    rax, [rsi]                ; cwd[0] (ui64)
    mov    r8, [rsi + 8]             ; cwd[1] (ui64)

    ; Load lengths and compute total
    mov    ecx, [rdx]                ; cwd_len[0]
    mov    r11d, [rdx + 4]           ; cwd_len[1]
    add    r11d, ecx                 ; total = cwd_len[0] + cwd_len[1]

    ; Try to combine both pairs
    cmp    r11d, 64
    jg     .p_split

    ; Fast path: combine and encode
    shlx   r8, r8, rcx
    or     rax, r8
    mov    ecx, r11d

.p_encode:
    movzx  r8d, r13b
    mov    r9d, 64
    sub    r9d, r8d
    cmp    ecx, r9d
    jg     .p_encode_overflow
    shlx   rax, rax, r8
    or     r14, rax
    add    r13b, cl

.p_next_group:
    add    rsi, 16
    add    rdx, 8
    cmp    rsi, rdi
    jb     .p_group_loop
    jmp    .p_final_drain

.p_encode_overflow:
    bzhi   r10, rax, r9
    shlx   r10, r10, r8
    or     r14, r10
    mov    r13b, 64
    mov    rbx, rax
    mov    [rsp + STK_CWDLEN], ecx
    mov    [rsp + STK_AVAIL], r9d
    call   ojph_ms_drain_internal
    mov    rax, rbx
    mov    ecx, [rsp + STK_CWDLEN]
    mov    r9d, [rsp + STK_AVAIL]
    shrx   rax, rax, r9
    sub    ecx, r9d
    jmp    .p_encode

; Split path: encode each pair separately
.p_split:
    mov    ecx, [rdx]                ; cwd_len[0]
.p_split_encode_p0:
    movzx  r8d, r13b
    mov    r9d, 64
    sub    r9d, r8d
    cmp    ecx, r9d
    jg     .p_split_p0_overflow
    shlx   rax, rax, r8
    or     r14, rax
    add    r13b, cl
    jmp    .p_split_pair1

.p_split_p0_overflow:
    bzhi   r10, rax, r9
    shlx   r10, r10, r8
    or     r14, r10
    mov    r13b, 64
    mov    rbx, rax
    mov    [rsp + STK_CWDLEN], ecx
    mov    [rsp + STK_AVAIL], r9d
    call   ojph_ms_drain_internal
    mov    rax, rbx
    mov    ecx, [rsp + STK_CWDLEN]
    mov    r9d, [rsp + STK_AVAIL]
    shrx   rax, rax, r9
    sub    ecx, r9d
    jmp    .p_split_encode_p0

.p_split_pair1:
    mov    rax, [rsi + 8]
    mov    ecx, [rdx + 4]
    jmp    .p_encode

.p_final_drain:
    call   ojph_ms_drain_internal

    ; Write state back to ms_struct
    mov    [r15 + MS_TMP], r14
    movzx  eax, r13b
    mov    [r15 + MS_USED_BITS], eax
    mov    [r15 + MS_POS], ebp
    bt     r13d, 8
    setc   al
    mov    [r15 + MS_LAST_FF], al

    add    rsp, 32
    pop    r15
    pop    r14
    pop    r13
    pop    r12
    pop    rbp
    pop    rbx
    ret
