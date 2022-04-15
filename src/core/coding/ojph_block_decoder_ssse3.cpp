//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman 
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
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
// File: ojph_block_decoder_ssse3.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

//***************************************************************************/
/** @file ojph_block_decoder_ssse3.cpp
 *  @brief implements a faster HTJ2K block decoder using ssse3
 */

#include <cassert>
#include <cstring>
#include "ojph_block_decoder.h"
#include "ojph_arch.h"
#include "ojph_message.h"

#ifdef OJPH_COMPILER_MSVC
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include "tsc-measure.h"

namespace ojph {
  namespace local_ssse3 {

    struct _counter {
      _counter() { count = 0; num_samples = 0; }
      ~_counter() { printf("%ld, %ld, %f\n", count, num_samples, (float)count / (float)num_samples); }
      uint64_t count;
      uint64_t tmp;
      uint64_t num_samples;
    } counter;

    //************************************************************************/
    /** @defgroup vlc_decoding_tables_grp VLC decoding tables
     *  @{
     *  VLC decoding tables used in decoding VLC codewords to these fields:  \n
     *  \li \c cwd_len : 3bits -> the codeword length of the VLC codeword;    
     *                   the VLC cwd is in the LSB of bitstream              \n
     *  \li \c u_off   : 1bit  -> u_offset, which is 1 if u value is not 0   \n
     *  \li \c rho     : 4bits -> signficant samples within a quad           \n
     *  \li \c e_1     : 4bits -> EMB e_1                                    \n
     *  \li \c e_k     : 4bits -> EMB e_k                                    \n
     *                                                                       \n
     *  The table index is 10 bits and composed of two parts:                \n
     *  The 7 LSBs contain a codeword which might be shorter than 7 bits;    
     *  this word is the next decoable bits in the bitstream.                \n
     *  The 3 MSB is the context of for the codeword.                        \n
     */

    /// @brief vlc_tbl0 contains decoding information for initial row of quads
    static ui16 vlc_tbl0[1024] = { 0 };
    /// @brief vlc_tbl1 contains decoding information for non-initial row of 
    ///        quads
    static ui16 vlc_tbl1[1024] = { 0 };
    /// @}

    //************************************************************************/
    /** @defgroup uvlc_decoding_tables_grp VLC decoding tables
     *  @{
     *  UVLC decoding tables used to partiallu decode u values from UVLC     
     *  codewords.                                                           \n
     *  The table index is 8 (or 9)  bits and composed of two parts:         \n
     *  The 6 LSBs carries the head of the VLC to be decoded. Up to 6 bits to 
     *  be used; these are uvlc prefix code for quad 0 and 1                 \n
     *  The 2 (or 3) MSBs contain u_off of quad 0 + 2 * o_off quad 1
     *  + 4 * mel event for initial row of quads when needed                 \n
     *                                                                       \n
     *  Each entry contains, starting from the LSB                           \n
     *  \li \c total prefix length for quads 0 and 1 (3 bits)                \n
     *  \li \c total suffix length for quads 0 and 1 (4 bits)                \n
     *  \li \c suffix length for quad 0 (3 bits)                             \n
     *  \li \c prefix for quad 0 (3 bits)                                    \n
     *  \li \c prefix for quad 1 (3 bits)                                    \n
     */

    /// @brief uvlc_tbl0 contains decoding information for initial row of quads
    static ui16 uvlc_tbl0[256+64] = { 0 };
    /// @brief uvlc_tbl1 contains decoding information for non-initial row of 
    ///        quads
    static ui16 uvlc_tbl1[256] = { 0 };
    /// @}

    //************************************************************************/
    /** @brief MEL state structure for reading and decoding the MEL bitstream
     *
     *  A number of events is decoded from the MEL bitstream ahead of time
     *  and stored in run/num_runs.
     *  Each run represents the number of zero events before a one event.
     */ 
    struct dec_mel_st {
      dec_mel_st() : data(NULL), tmp(0), bits(0), size(0), unstuff(false),
        k(0), num_runs(0), runs(0)
      {}
      // data decoding machinary
      ui8* data;    //!<the address of data (or bitstream)
      ui64 tmp;     //!<temporary buffer for read data
      int bits;     //!<number of bits stored in tmp
      int size;     //!<number of bytes in MEL code
      bool unstuff; //!<true if the next bit needs to be unstuffed
      int k;        //!<state of MEL decoder

      // queue of decoded runs
      int num_runs; //!<number of decoded runs left in runs (maximum 8)
      ui64 runs;    //!<runs of decoded MEL codewords (7 bits/run)
    };

    //************************************************************************/
    /** @brief Reads and unstuffs the MEL bitstream
     * 
     *  This design needs more bytes in the codeblock buffer than the length
     *  of the cleanup pass by up to 2 bytes.
     *
     *  Unstuffing removes the MSB of the byte following a byte whose
     *  value is 0xFF; this prevents sequences larger than 0xFF7F in value
     *  from appearing the bitstream.
     *
     *  @param [in]  melp is a pointer to dec_mel_st structure
     */
    static inline
    void mel_read(dec_mel_st *melp)
    {
      if (melp->bits > 32)  //there are enough bits in the tmp variable
        return;             // return without reading new data
      ui32 val = 0xFFFFFFFF;
      //the next line (the if statement) needs to be tested first
      //if (melp->size > 0)          // if there is data in the MEL segment
        val = *(ui32*)melp->data;  // read 32 bits from MEL data
      
      // next we unstuff them before adding them to the buffer
      int bits = 32 - melp->unstuff; // number of bits in val, subtract 1 if
                                     // the previously read byte requires 
                                     // unstuffing

      // data is unstuffed and accumulated in t
      // bits has the number of bits in t
      ui32 t = (melp->size > 0) ? (val & 0xFF) : 0xFF; // feed 0xFF if the 
                                      // MEL bitstream has been exhausted
      if (melp->size == 1) t |= 0xF;  // if this is 1 byte before the last
                                      // in MEL+VLC segments (remember they
                                      // can overlap)
      melp->data += melp->size-- > 0; // advance data by 1 byte if we have not
                                      // reached the end of the MEL segment
      bool unstuff = ((val & 0xFF) == 0xFF); // true if the byte
                                             // needs unstuffing

      bits -= unstuff; // there is one less bit in t if unstuffing is needed
      t = t << (8 - unstuff); // move up to make room for the next byte

      //this is a repeat of the above
      t |= (melp->size > 0) ? ((val>>8) & 0xFF) : 0xFF;
      if (melp->size == 1) t |= 0xF;
      melp->data += melp->size-- > 0;
      unstuff = (((val >> 8) & 0xFF) == 0xFF);

      bits -= unstuff;
      t = t << (8 - unstuff);

      t |= (melp->size > 0) ? ((val>>16) & 0xFF) : 0xFF;
      if (melp->size == 1) t |= 0xF;
      melp->data += melp->size-- > 0;
      unstuff = (((val >> 16) & 0xFF) == 0xFF);

      bits -= unstuff;
      t = t << (8 - unstuff);

      t |= (melp->size > 0) ? ((val>>24) & 0xFF) : 0xFF;
      if (melp->size == 1) t |= 0xF;
      melp->data += melp->size-- > 0;
      melp->unstuff = (((val >> 24) & 0xFF) == 0xFF);

      // move t to tmp, and push the result all the way up, so we read from
      // the MSB
      melp->tmp |= ((ui64)t) << (64 - bits - melp->bits);
      melp->bits += bits; //increment the number of bits in tmp
    }

    //************************************************************************/
    /** @brief Decodes unstuffed MEL segment bits stored in tmp to runs
     * 
     *  Runs are stored in "runs" and the number of runs in "num_runs".
     *  Each run represents a number of zero events that may or may not 
     *  terminate in a 1 event.
     *  Each run is stored in 7 bits.  The LSB is 1 if the run terminates in
     *  a 1 event, 0 otherwise.  The next 6 bits, for the case terminating 
     *  with 1, contain the number of consecutive 0 zero events * 2; for the 
     *  case terminating with 0, they store (number of consecutive 0 zero 
     *  events - 1) * 2.
     *  A total of 6 bits (made up of 1 + 5) should have been enough.
     *
     *  @param [in]  melp is a pointer to dec_mel_st structure
     */
    static inline
    void mel_decode(dec_mel_st *melp)
    {
      static const int mel_exp[13] = { //MEL exponents
        0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5
      };

      if (melp->bits < 6) // if there are less than 6 bits in tmp
        mel_read(melp);   // then read from the MEL bitstream
                          // 6 bits is the largest decodable MEL cwd

      //repeat so long that there is enough decodable bits in tmp,
      // and the runs store is not full (num_runs < 8)
      while (melp->bits >= 6 && melp->num_runs < 8)
      {
        int eval = mel_exp[melp->k]; // number of bits associated with state
        int run = 0;
        if (melp->tmp & (1ull<<63)) //The next bit to decode (stored in MSB)
        { //one is found
          run = 1 << eval;  
          run--; // consecutive runs of 0 events - 1
          melp->k = melp->k + 1 < 12 ? melp->k + 1 : 12;//increment, max is 12
          melp->tmp <<= 1; // consume one bit from tmp
          melp->bits -= 1;
          run = run << 1; // a stretch of zeros not terminating in one
        }
        else
        { //0 is found
          run = (int)(melp->tmp >> (63 - eval)) & ((1 << eval) - 1);
          melp->k = melp->k - 1 > 0 ? melp->k - 1 : 0; //decrement, min is 0
          melp->tmp <<= eval + 1; //consume eval + 1 bits (max is 6)
          melp->bits -= eval + 1;
          run = (run << 1) + 1; // a stretch of zeros terminating with one
        }
        eval = melp->num_runs * 7;           // 7 bits per run
        melp->runs &= ~((ui64)0x3F << eval); // 6 bits are sufficient
        melp->runs |= ((ui64)run) << eval;   // store the value in runs
        melp->num_runs++;                    // increment count  
      }
    }

    //************************************************************************/
    /** @brief Initiates a dec_mel_st structure for MEL decoding and reads
     *         some bytes in order to get the read address to a multiple
     *         of 4 
     *
     *  @param [in]  melp is a pointer to dec_mel_st structure
     *  @param [in]  bbuf is a pointer to byte buffer
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  scup is the length of MEL+VLC segments
     */
    static inline
    void mel_init(dec_mel_st *melp, ui8* bbuf, int lcup, int scup)
    {
      melp->data = bbuf + lcup - scup; // move the pointer to the start of MEL
      melp->bits = 0;                  // 0 bits in tmp
      melp->tmp = 0;                   //
      melp->unstuff = false;           // no unstuffing
      melp->size = scup - 1;           // size is the length of MEL+VLC-1
      melp->k = 0;                     // 0 for state 
      melp->num_runs = 0;              // num_runs is 0
      melp->runs = 0;                  //

      //This code is borrowed; original is for a different architecture
      //These few lines take care of the case where data is not at a multiple
      // of 4 boundary.  It reads 1,2,3 up to 4 bytes from the MEL segment
      int num = 4 - (int)(intptr_t(melp->data) & 0x3);
      for (int i = 0; i < num; ++i) { // this code is similar to mel_read
        assert(melp->unstuff == false || melp->data[0] <= 0x8F);
        ui64 d = (melp->size > 0) ? *melp->data : 0xFF;//if buffer is consumed
                                                       //set data to 0xFF
        if (melp->size == 1) d |= 0xF; //if this is MEL+VLC-1, set LSBs to 0xF
                                       // see the standard
        melp->data += melp->size-- > 0; //increment if the end is not reached
        int d_bits = 8 - melp->unstuff; //if unstuffing is needed, reduce by 1
        melp->tmp = (melp->tmp << d_bits) | d; //store bits in tmp
        melp->bits += d_bits;  //increment tmp by number of bits
        melp->unstuff = ((d & 0xFF) == 0xFF); //true of next byte needs 
                                              //unstuffing
      }
      melp->tmp <<= (64 - melp->bits); //push all the way up so the first bit
                                       // is the MSB
    }

    //************************************************************************/
    /** @brief Retrieves one run from dec_mel_st; if there are no runs stored
     *         MEL segment is decoded
     *
     * @param [in]  melp is a pointer to dec_mel_st structure
     */    
    static inline
    int mel_get_run(dec_mel_st *melp)
    {
      if (melp->num_runs == 0)  //if no runs, decode more bit from MEL segment
        mel_decode(melp);

      int t = melp->runs & 0x7F; //retrieve one run
      melp->runs >>= 7;  // remove the retrieved run
      melp->num_runs--;
      return t; // return run
    }

    //************************************************************************/
    /** @brief A structure for reading and unstuffing a segment that grows
     *         backward, such as VLC and MRP
     */ 
    struct rev_struct {
      rev_struct() : data(NULL), tmp(0), bits(0), size(0), unstuff(false)
      {}
      //storage
      ui8* data;     //!<pointer to where to read data
      ui64 tmp;	     //!<temporary buffer of read data
      ui32 bits;     //!<number of bits stored in tmp
      int size;      //!<number of bytes left
      bool unstuff;  //!<true if the last byte is more than 0x8F
                     //!<then the current byte is unstuffed if it is 0x7F
    };

    //************************************************************************/
    /** @brief Read and unstuff data from a backwardly-growing segment
     *
     *  This reader can read up to 8 bytes from before the VLC segment.
     *  Care must be taken not read from unreadable memory, causing a 
     *  segmentation fault.
     * 
     *  Note that there is another subroutine rev_read_mrp that is slightly
     *  different.  The other one fills zeros when the buffer is exhausted.
     *  This one basically does not care if the bytes are consumed, because
     *  any extra data should not be used in the actual decoding.
     *
     *  Unstuffing is needed to prevent sequences more than 0xFF8F from 
     *  appearing in the bits stream; since we are reading backward, we keep
     *  watch when a value larger than 0x8F appears in the bitstream. 
     *  If the byte following this is 0x7F, we unstuff this byte (ignore the 
     *  MSB of that byte, which should be 0).
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     */
    inline void rev_read(rev_struct *vlcp)
    {
      //process 4 bytes at a time
      if (vlcp->bits > 32)  // if there are more than 32 bits in tmp, then 
        return;             // reading 32 bits can overflow vlcp->tmp
      ui32 val = 0;
      //the next line (the if statement) needs to be tested first
      if (vlcp->size > 0)  // if there are bytes left in the VLC segment
      {
        // We pad the data by 8 bytes at the beginning of the code stream 
        // buffer
        val = *(ui32*)vlcp->data; // then read 32 bits
        vlcp->data -= 4;          // move data pointer back by 4
        vlcp->size -= 4;          // reduce available byte by 4
        
        //because we read ahead of time, we might in fact exceed vlc size,
        // but data should not be used if the codeblock is properly generated
        //The mel code can in fact occupy zero length, if it has a small number
        // of bits and these bits overlap with the VLC code
        ////if test is alright, remove this
        //// We pad the data by 8 bytes at the beginning of the code stream 
        //// buffer
        //if (vlcp->size < -4) 
        //  OJPH_ERROR(0x00010001, "Error in reading VLC data: vlcp size %d "
        //             "less than -4 before rev_read", vlcp->size);
      }

      //accumulate in tmp, number of bits in tmp are stored in bits
      ui32 tmp = val >> 24;  //start with the MSB byte
      ui32 bits;

      // test unstuff (previous byte is >0x8F), and this byte is 0x7F
      bits = 8 - ((vlcp->unstuff && (((val >> 24) & 0x7F) == 0x7F)) ? 1 : 0);
      bool unstuff = (val >> 24) > 0x8F; //this is for the next byte

      tmp |= ((val >> 16) & 0xFF) << bits; //process the next byte
      bits += 8 - ((unstuff && (((val >> 16) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 16) & 0xFF) > 0x8F;

      tmp |= ((val >> 8) & 0xFF) << bits;
      bits += 8 - ((unstuff && (((val >> 8) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 8) & 0xFF) > 0x8F;

      tmp |= (val & 0xFF) << bits;
      bits += 8 - ((unstuff && ((val & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = (val & 0xFF) > 0x8F;

      // now move the read and unstuffed bits into vlcp->tmp
      vlcp->tmp |= (ui64)tmp << vlcp->bits;
      vlcp->bits += bits;
      vlcp->unstuff = unstuff; // this for the next read
    }

    //************************************************************************/
    /** @brief Initiates the rev_struct structure and reads a few bytes to 
     *         move the read address to multiple of 4
     *
     *  There is another similar rev_init_mrp subroutine.  The difference is
     *  that this one, rev_init, discards the first 12 bits (they have the
     *  sum of the lengths of VLC and MEL segments), and first unstuff depends
     *  on first 4 bits.
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     *  @param [in]  data is a pointer to byte at the start of the cleanup pass
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  scup is the length of MEL+VLC segments
     */
    inline void rev_init(rev_struct *vlcp, ui8* data, int lcup, int scup)
    {
      //first byte has only the upper 4 bits
      vlcp->data = data + lcup - 2;

      //size can not be larger than this, in fact it should be smaller
      vlcp->size = scup - 2;

      ui32 d = *vlcp->data--; // read one byte (this is a half byte)
      vlcp->tmp = d >> 4;    // both initialize and set
      vlcp->bits = 4 - ((vlcp->tmp & 7) == 7); //check standard
      vlcp->unstuff = (d | 0xF) > 0x8F; //this is useful for the next byte

      //This code is designed for an architecture that read address should
      // align to the read size (address multiple of 4 if read size is 4)
      //These few lines take care of the case where data is not at a multiple
      // of 4 boundary. It reads 1,2,3 up to 4 bytes from the VLC bitstream
      int num = 1 + (int)(intptr_t(vlcp->data) & 0x3);
      int tnum = num < vlcp->size ? num : vlcp->size;
      for (int i = 0; i < tnum; ++i) {
        ui64 d;
        d = *vlcp->data--;  // read one byte and move read pointer
        //check if the last byte was >0x8F (unstuff == true) and this is 0x7F
        ui32 d_bits = 8 - ((vlcp->unstuff && ((d & 0x7F) == 0x7F)) ? 1 : 0);
        vlcp->tmp |= d << vlcp->bits; // move data to vlcp->tmp
        vlcp->bits += d_bits;
        vlcp->unstuff = d > 0x8F; // for next byte
      }
      vlcp->size -= tnum;
      vlcp->data -= 3; // make ready to read 32 bits (address multiple of 4)
      rev_read(vlcp);  // read another 32 buts
    }

    //************************************************************************/
    /** @brief Retrieves 32 bits from the head of a rev_struct structure 
     *
     *  By the end of this call, vlcp->tmp must have no less than 33 bits
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     */
    inline ui32 rev_fetch(rev_struct *vlcp)
    {
      if (vlcp->bits < 32)  // if there are less then 32 bits, read more
      {
        rev_read(vlcp);     // read 32 bits, but unstuffing might reduce this
        if (vlcp->bits < 32)// if there is still space in vlcp->tmp for 32 bits
          rev_read(vlcp);   // read another 32
      }
      return (ui32)vlcp->tmp; // return the head (bottom-most) of vlcp->tmp
    }

    //************************************************************************/
    /** @brief Consumes num_bits from a rev_struct structure
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     *  @param [in]  num_bits is the number of bits to be removed
     */
    inline ui32 rev_advance(rev_struct *vlcp, ui32 num_bits)
    {
      assert(num_bits <= vlcp->bits); // vlcp->tmp must have more than num_bits
      vlcp->tmp >>= num_bits;         // remove bits
      vlcp->bits -= num_bits;         // decrement the number of bits
      return (ui32)vlcp->tmp;
    }

    //************************************************************************/
    /** @brief Reads and unstuffs from rev_struct
     *
     *  This is different than rev_read in that this fills in zeros when the
     *  the available data is consumed.  The other does not care about the
     *  values when all data is consumed.
     *
     *  See rev_read for more information about unstuffing
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     */
    inline void rev_read_mrp(rev_struct *mrp)
    {
      //process 4 bytes at a time
      if (mrp->bits > 32)
        return;
      ui32 val = 0;
      //the next line (the if statement) needs to be tested first
      //notice that second line can be simplified to mrp->data -= 4
      // if (mrp->size > 0)
      {
        val = *(ui32*)mrp->data;            // read 32 bits
        mrp->data -= mrp->size > 0 ? 4 : 0; // move back read pointer only if 
                                            // there is data
      }

      //accumulate in tmp, and keep count in bits
      ui32 tmp = (mrp->size-- > 0) ? (val >> 24) : 0; // fill zeros if all 
      ui32 bits;                                       // bytes are used
      //test if the last byte > 0x8F (unstuff must be true) and this is 0x7F
      bits = 8 - ((mrp->unstuff && (((val >> 24) & 0x7F) == 0x7F)) ? 1 : 0);
      bool unstuff = (val >> 24) > 0x8F;

      //process the next byte
      tmp |= (mrp->size-- > 0) ? (((val >> 16) & 0xFF) << bits) : 0;
      bits += 8 - ((unstuff && (((val >> 16) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 16) & 0xFF) > 0x8F;

      tmp |= (mrp->size-- > 0) ? (((val >> 8) & 0xFF) << bits) : 0;
      bits += 8 - ((unstuff && (((val >> 8) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 8) & 0xFF) > 0x8F;

      tmp |= (mrp->size-- > 0) ? ((val & 0xFF) << bits) : 0;
      bits += 8 - ((unstuff && ((val & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = (val & 0xFF) > 0x8F;

      mrp->tmp |= (ui64)tmp << mrp->bits; // move data to mrp pointer
      mrp->bits += bits;
      mrp->unstuff = unstuff;             // next byte
    }

    //************************************************************************/
    /** @brief Initialized rev_struct structure for MRP segment, and reads
     *         a number of bytes such that the next 32 bits read are from
     *         an address that is a multiple of 4. Note this is designed for
     *         an architecture that read size must be compatible with the
     *         alignment of the read address
     *
     *  There is another simiar subroutine rev_init.  This subroutine does 
     *  NOT skip the first 12 bits, and starts with unstuff set to true.
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     *  @param [in]  data is a pointer to byte at the start of the cleanup pass
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  len2 is the length of SPP+MRP segments
     */
    inline void rev_init_mrp(rev_struct *mrp, ui8* data, int lcup, int len2)
    {
      mrp->data = data + lcup + len2 - 1;
      mrp->size = len2;
      mrp->unstuff = true;
      mrp->bits = 0;
      mrp->tmp = 0;

      //This code is designed for an architecture that read address should
      // align to the read size (address multiple of 4 if read size is 4)
      //These few lines take care of the case where data is not at a multiple
      // of 4 boundary.  It reads 1,2,3 up to 4 bytes from the MRP stream
      int num = 1 + (int)(intptr_t(mrp->data) & 0x3);
      for (int i = 0; i < num; ++i) {
        ui64 d;
        //read a byte, 0 if no more data
        d = (mrp->size-- > 0) ? *mrp->data-- : 0; 
        //check if unstuffing is needed
        ui32 d_bits = 8 - ((mrp->unstuff && ((d & 0x7F) == 0x7F)) ? 1 : 0);
        mrp->tmp |= d << mrp->bits; // move data to vlcp->tmp
        mrp->bits += d_bits;
        mrp->unstuff = d > 0x8F; // for next byte
      }
      mrp->data -= 3; //make ready to read a 32 bits
      rev_read_mrp(mrp);
    }

    //************************************************************************/
    /** @brief Retrieves 32 bits from the head of a rev_struct structure 
     *
     *  By the end of this call, mrp->tmp must have no less than 33 bits
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     */
    inline ui32 rev_fetch_mrp(rev_struct *mrp)
    {
      if (mrp->bits < 32) // if there are less than 32 bits in mrp->tmp
      {
        rev_read_mrp(mrp);    // read 30-32 bits from mrp
        if (mrp->bits < 32)   // if there is a space of 32 bits
          rev_read_mrp(mrp);  // read more
      }
      return (ui32)mrp->tmp;  // return the head of mrp->tmp
    }

    //************************************************************************/
    /** @brief Consumes num_bits from a rev_struct structure
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     *  @param [in]  num_bits is the number of bits to be removed
     */
    inline ui32 rev_advance_mrp(rev_struct *mrp, ui32 num_bits)
    {
      assert(num_bits <= mrp->bits); // we must not consume more than mrp->bits
      mrp->tmp >>= num_bits;  // discard the lowest num_bits bits
      mrp->bits -= num_bits;
      return (ui32)mrp->tmp;  // return data after consumption
    }

    //************************************************************************/
    /** @ingroup vlc_decoding_tables_grp
     *  @brief Initializes vlc_tbl0 and vlc_tbl1 tables, from table0.h and
     *         table1.h
     */
    static bool vlc_init_tables()
    {
      const bool debug = false; //useful for checking 

      //Data in the table is arranged in this format (taken from the standard)
      // c_q is the context for a quad
      // rho is the signficance pattern for a quad
      // u_off indicate if u value is 0 (u_off is 0), or communicated
      // e_k, e_1 EMB patterns
      // cwd VLC codeword
      // cwd VLC codeword length
      struct vlc_src_table { int c_q, rho, u_off, e_k, e_1, cwd, cwd_len; };
      // initial quad rows
      vlc_src_table tbl0[] = {
    #include "table0.h"
      };
      // number of entries in the table
      size_t tbl0_size = sizeof(tbl0) / sizeof(vlc_src_table); 

      // nono-initial quad rows
      vlc_src_table tbl1[] = {
    #include "table1.h"
      };
      // number of entries in the table
      size_t tbl1_size = sizeof(tbl1) / sizeof(vlc_src_table);

      if (debug) memset(vlc_tbl0, 0, sizeof(vlc_tbl0)); //unnecessary

      // this is to convert table entries into values for decoder look up
      // There can be at most 1024 possibilites, not all of them are valid.
      // 
      for (int i = 0; i < 1024; ++i)
      {
        int cwd = i & 0x7F; // from i extract codeword
        int c_q = i >> 7;   // from i extract context
        // See if this case exist in the table, if so then set the entry in
        // vlc_tbl0
        for (size_t j = 0; j < tbl0_size; ++j) 
          if (tbl0[j].c_q == c_q) // this is an and operation
            if (tbl0[j].cwd == (cwd & ((1 << tbl0[j].cwd_len) - 1)))
            {
              if (debug) assert(vlc_tbl0[i] == 0);
              // Put this entry into the table
              vlc_tbl0[i] = (ui16)((tbl0[j].rho << 4) | (tbl0[j].u_off << 3)
                | (tbl0[j].e_k << 12) | (tbl0[j].e_1 << 8) | tbl0[j].cwd_len);
            }
      }

      if (debug) memset(vlc_tbl1, 0, sizeof(vlc_tbl1)); //unnecessary

      // this the same as above but for non-initial rows
      for (int i = 0; i < 1024; ++i)
      {
        int cwd = i & 0x7F; //7 bits
        int c_q = i >> 7;
        for (size_t j = 0; j < tbl1_size; ++j)
          if (tbl1[j].c_q == c_q) // this is an and operation
            if (tbl1[j].cwd == (cwd & ((1 << tbl1[j].cwd_len) - 1)))
            {
              if (debug) assert(vlc_tbl1[i] == 0);
              vlc_tbl1[i] = (ui16)((tbl1[j].rho << 4) | (tbl1[j].u_off << 3)
                | (tbl1[j].e_k << 12) | (tbl1[j].e_1 << 8) | tbl1[j].cwd_len);
            }
      }

      return true;
    }

    //************************************************************************/
    /** @ingroup uvlc_decoding_tables_grp
     *  @brief Initializes uvlc_tbl0 and uvlc_tbl1 tables
     */
    static bool uvlc_init_tables()
    {
      // table stores possible decoding three bits from vlc
      // there are 8 entries for xx1, x10, 100, 000, where x means do not
      // care table value is made up of
      // 2 bits in the LSB for prefix length 
      // 3 bits for suffix length
      // 3 bits in the MSB for prefix value (u_pfx in Table 3 of ITU T.814)
      static const ui8 dec[8] = { // the index is the prefix codeword
        3 | (5 << 2) | (5 << 5), //000 == 000, prefix codeword "000"
        1 | (0 << 2) | (1 << 5), //001 == xx1, prefix codeword "1"
        2 | (0 << 2) | (2 << 5), //010 == x10, prefix codeword "01"
        1 | (0 << 2) | (1 << 5), //011 == xx1, prefix codeword "1"
        3 | (1 << 2) | (3 << 5), //100 == 100, prefix codeword "001"
        1 | (0 << 2) | (1 << 5), //101 == xx1, prefix codeword "1"
        2 | (0 << 2) | (2 << 5), //110 == x10, prefix codeword "01"
        1 | (0 << 2) | (1 << 5)  //111 == xx1, prefix codeword "1"
      };

      for (ui32 i = 0; i < 256 + 64; ++i)
      { 
        ui32 mode = i >> 6;
        ui32 vlc = i & 0x3F;

        if (mode == 0)      // both u_off are 0
          uvlc_tbl0[i] = 0;
        else if (mode <= 2) // u_off are either 01 or 10
        {
          ui32 d = dec[vlc & 0x7];   //look at the least significant 3 bits

          ui32 total_prefix = d & 0x3;
          ui32 total_suffix = (d >> 2) & 0x7;
          ui32 u0_suffix_len = (mode == 1) ? total_suffix : 0;
          ui32 u0 = (mode == 1) ? (d >> 5) : 0;
          ui32 u1 = (mode == 1) ? 0 : (d >> 5);

          uvlc_tbl0[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
          
        }
        else if (mode == 3) // both u_off are 1, and MEL event is 0
        {
          ui32 d0 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword
          vlc >>= d0 & 0x3;          // Consume bits
          ui32 d1 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword

          ui32 total_prefix, u0_suffix_len, total_suffix, u0, u1;
          if ((d0 & 0x3) == 3)
          {
            total_prefix = (d0 & 0x3) + 1;
            u0_suffix_len = (d0 >> 2) & 0x7;
            total_suffix = u0_suffix_len;
            u0 = d0 >> 5;
            u1 = (vlc & 1) + 1;
          }
          else
          {
            total_prefix = (d0 & 0x3) + (d1 & 0x3);
            u0_suffix_len = (d0 >> 2) & 0x7;
            total_suffix = u0_suffix_len + ((d1 >> 2) & 0x7);
            u0 = d0 >> 5;
            u1 = d1 >> 5;
          }

          uvlc_tbl0[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
        }
        else if (mode == 4) // both u_off are 1, and MEL event is 1
        {
          ui32 d0 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword
          vlc >>= d0 & 0x3;          // Consume bits
          ui32 d1 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword

          ui32 total_prefix = (d0 & 0x3) + (d1 & 0x3);
          ui32 u0_suffix_len = (d0 >> 2) & 0x7;
          ui32 total_suffix = u0_suffix_len + ((d1 >> 2) & 0x7);
          ui32 u0 = (d0 >> 5) + 2;
          ui32 u1 = (d1 >> 5) + 2;

          uvlc_tbl0[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
        }
      }

      for (ui32 i = 0; i < 256; ++i)
      {
        ui32 mode = i >> 6;
        ui32 vlc = i & 0x3F;

        if (mode == 0)       // both u_off are 0
          uvlc_tbl1[i] = 0;
        else if (mode <= 2)  // u_off are either 01 or 10
        {
          ui32 d = dec[vlc & 0x7];   // look at the 3 LSB bits

          ui32 total_prefix = d & 0x3;
          ui32 total_suffix = (d >> 2) & 0x7;
          ui32 u0_suffix_len = (mode == 1) ? total_suffix : 0;
          ui32 u0 = (mode == 1) ? (d >> 5) : 0;
          ui32 u1 = (mode == 1) ? 0 : (d >> 5);

          uvlc_tbl1[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
        }
        else if (mode == 3) // both u_off are 1
        {
          ui32 d0 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword
          vlc >>= d0 & 0x3;          // Consume bits
          ui32 d1 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword

          ui32 total_prefix = (d0 & 0x3) + (d1 & 0x3);
          ui32 u0_suffix_len = (d0 >> 2) & 0x7;
          ui32 total_suffix = u0_suffix_len + ((d1 >> 2) & 0x7);
          ui32 u0 = d0 >> 5;
          ui32 u1 = d1 >> 5;

          uvlc_tbl1[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
        }
      }
      return true;
    }

    //************************************************************************/
    /** @ingroup vlc_decoding_tables_grp
     *  @brief Initializes VLC tables vlc_tbl0 and vlc_tbl1
     */
    static bool vlc_tables_initialized = vlc_init_tables();

    //************************************************************************/
    /** @ingroup uvlc_decoding_tables_grp
     *  @brief Initializes UVLC tables uvlc_tbl0 and uvlc_tbl1
     */
    static bool uvlc_tables_initialized = uvlc_init_tables();

    //************************************************************************/
    /** @brief State structure for reading and unstuffing of forward-growing 
     *         bitstreams; these are: MagSgn and SPP bitstreams
     */
    struct frwd_struct {
      const ui8* data;  //!<pointer to bitstream
      ui8 tmp[48];      //!<temporary buffer of read data + 16 extra
      ui32 bits;        //!<number of bits stored in tmp
      ui32 unstuff;     //!<1 if a bit needs to be unstuffed from next byte
      int size;         //!<size of data
    };

    //************************************************************************/
    /** @brief Read and unstuffs 32 bits from forward-growing bitstream
     *  
     *  A template is used to accommodate a different requirement for
     *  MagSgn and SPP bitstreams; in particular, when MagSgn bitstream is
     *  consumed, 0xFF's are fed, while when SPP is exhausted 0's are fed in.
     *  X controls this value.
     *
     *  Unstuffing prevent sequences that are more than 0xFF7F from appearing
     *  in the conpressed sequence.  So whenever a value of 0xFF is coded, the
     *  MSB of the next byte is set 0 and must be ignored during decoding.
     *
     *  Reading can go beyond the end of buffer by up to 16 bytes.
     *
     *  @tparam       X is the value fed in when the bitstream is exhausted
     *  @param  [in]  msp is a pointer to frwd_struct structure
     *
     */
    template<int X>
    void frwd_read(frwd_struct *msp)
    {
      assert(msp->bits <= 128);

      __m128i offset, val, validity, all_xff;
      val = _mm_loadu_si128((__m128i*)msp->data);
      int bytes = msp->size >= 16 ? 16 : msp->size;
      validity = _mm_set1_epi8((char)bytes);
      msp->data += bytes;
      msp->size -= bytes;
      int bits = 128;
      offset = _mm_set_epi64x(0x0F0E0D0C0B0A0908,0x0706050403020100);
      validity = _mm_cmpgt_epi8(validity, offset);
      all_xff = _mm_set1_epi8(-1);
      if (X == 0xFF) // the compiler should remove this if statement
      {
        __m128i t = _mm_xor_si128(validity, all_xff); // complement
        val = _mm_or_si128(t, val); // fill with 0xFF
      }
      else if (X == 0)
        val = _mm_and_si128(validity, val); // fill with zeros 
      else
        assert(0);

      __m128i ff_bytes;
      ff_bytes = _mm_cmpeq_epi8(val, all_xff);
      ff_bytes = _mm_and_si128(ff_bytes, validity);
      ui32 flags = (ui32)_mm_movemask_epi8(ff_bytes); 
      flags <<= 1; // unstuff following byte
      ui32 next_unstuff = flags >> 16;
      flags |= msp->unstuff;
      flags &= 0xFFFF;
      while (flags) 
      { // bit unstuffing occurs on average once every 256 bytes
        // therefore it is not an issue if it is a bit slow
        // here we process 16 bytes
        --bits; // consuming one stuffing bit

        ui32 loc = 31 - count_leading_zeros(flags);
        flags ^= 1 << loc;

        __m128i m, t, c;
        t = _mm_set1_epi8((char)loc);
        m = _mm_cmpgt_epi8(offset, t);

        t = _mm_and_si128(m, val);  // keep bits at locations larger than loc
        c = _mm_srli_epi64(t, 1);   // 1 bits left
        t = _mm_srli_si128(t, 8);   // 8 bytes left
        t = _mm_slli_epi64(t, 63);  // keep the MSB only
        t = _mm_or_si128(t, c);     // combine the above 3 steps
                                    
        val = _mm_or_si128(t, _mm_andnot_si128(m, val));
      }

      // combine with earlier data
      assert(msp->bits >= 0 && msp->bits <= 128);
      int cur_bytes = msp->bits >> 3;
      int cur_bits = msp->bits & 7;
      __m128i b1, b2;
      b1 = _mm_sll_epi64(val, _mm_set1_epi64x(cur_bits));
      b2 = _mm_slli_si128(val, 8);  // 8 bytes right
      b2 = _mm_srl_epi64(b2, _mm_set1_epi64x(64-cur_bits));
      b1 = _mm_or_si128(b1, b2);
      b2 = _mm_loadu_si128((__m128i*)(msp->tmp + cur_bytes));
      b2 = _mm_or_si128(b1, b2);
      _mm_storeu_si128((__m128i*)(msp->tmp + cur_bytes), b2);

      int consumed_bits = bits < 128 - cur_bits ? bits : 128 - cur_bits;
      cur_bytes = (msp->bits + (ui32)consumed_bits + 7) >> 3; // round up
      int upper = _mm_extract_epi16(val, 7);
      upper >>= consumed_bits - 128 + 16;
      msp->tmp[cur_bytes] = (ui8)upper; // copy byte

      msp->bits += (ui32)bits;
      msp->unstuff = next_unstuff;   // next unstuff
      assert(msp->unstuff == 0 || msp->unstuff == 1);
    }

    //************************************************************************/
    /** @brief Initialize frwd_struct struct and reads some bytes
     *  
     *  @tparam      X is the value fed in when the bitstream is exhausted.
     *               See frwd_read regarding the template
     *  @param [in]  msp is a pointer to frwd_struct
     *  @param [in]  data is a pointer to the start of data
     *  @param [in]  size is the number of byte in the bitstream
     */
    template<int X>
    void frwd_init(frwd_struct *msp, const ui8* data, int size)
    {
      msp->data = data;
      _mm_storeu_si128((__m128i *)msp->tmp, _mm_setzero_si128());
      _mm_storeu_si128((__m128i *)msp->tmp + 1, _mm_setzero_si128());
      _mm_storeu_si128((__m128i *)msp->tmp + 2, _mm_setzero_si128());

      msp->bits = 0;
      msp->unstuff = 0;
      msp->size = size;

      frwd_read<X>(msp); // read 128 bits more
    }

    //************************************************************************/
    /** @brief Consume num_bits bits from the bitstream of frwd_struct
     *
     *  @param [in]  msp is a pointer to frwd_struct
     *  @param [in]  num_bits is the number of bit to consume
     */
    inline void frwd_advance(frwd_struct *msp, ui32 num_bits)
    {
      assert(num_bits > 0 && num_bits <= msp->bits && num_bits < 128);
      msp->bits -= num_bits;

      __m128i *p = (__m128i*)(msp->tmp + ((num_bits >> 3) & 0x18));
      num_bits &= 63;

      __m128i v0, v1, c0, c1, t;
      v0 = _mm_loadu_si128(p);
      v1 = _mm_loadu_si128(p + 1);

      // shift right by num_bits
      c0 = _mm_srl_epi64(v0, _mm_set1_epi64x(num_bits));
      t = _mm_srli_si128(v0, 8);
      t = _mm_sll_epi64(t, _mm_set1_epi64x(64 - num_bits));
      c0 = _mm_or_si128(c0, t);
      t = _mm_slli_si128(v1, 8);
      t = _mm_sll_epi64(t, _mm_set1_epi64x(64 - num_bits));
      c0 = _mm_or_si128(c0, t);

      _mm_storeu_si128((__m128i*)msp->tmp, c0);

      c1 = _mm_srl_epi64(v1, _mm_set1_epi64x(num_bits));
      t = _mm_srli_si128(v1, 8);
      t = _mm_sll_epi64(t, _mm_set1_epi64x(64 - num_bits));
      c1 = _mm_or_si128(c1, t);

      _mm_storeu_si128((__m128i*)msp->tmp + 1, c1);
    }

    //************************************************************************/
    /** @brief Fetches 32 bits from the frwd_struct bitstream
     *
     *  @tparam      X is the value fed in when the bitstream is exhausted.
     *               See frwd_read regarding the template
     *  @param [in]  msp is a pointer to frwd_struct
     */
    template<int X>
    __m128i frwd_fetch(frwd_struct *msp)
    {
      if (msp->bits <= 128)
      {
        frwd_read<X>(msp);
        if (msp->bits <= 128) //need to test
          frwd_read<X>(msp);
      }
      __m128i t = _mm_loadu_si128((__m128i*)msp->tmp);
      return t;
    }


    template <int N>
    static inline 
    __m128i one_quad_decode32(const __m128i inf_u_q, __m128i U_q, 
                              frwd_struct* magsgn, ui32 p, __m128i& vn)
    {
      __m128i w0;    // workers
      __m128i insig; // lanes hold FF's if samples are insignificant
      __m128i flags; // lanes hold e_k, e_1, and rho
      __m128i row;   // decoded row

      row = _mm_setzero_si128();
      w0 = _mm_shuffle_epi32(inf_u_q, _MM_SHUFFLE(N, N, N, N));
      // we keeps e_k, e_1, and rho in w2
      flags = _mm_and_si128(w0, _mm_set_epi32(0x8880, 0x4440, 0x2220, 0x1110));
      insig = _mm_cmpeq_epi32(flags, _mm_setzero_si128());
      if (_mm_movemask_epi8(insig) != 0xFFFF) //are all insignificant?
      {
        U_q = _mm_shuffle_epi32(U_q, _MM_SHUFFLE(N, N, N, N));
        flags = _mm_mullo_epi16(flags, _mm_set_epi16(1,1,2,2,4,4,8,8));
        __m128i ms_vec = frwd_fetch<0xFF>(magsgn); 

        // U_q holds U_q for this quad
        // flags has e_k, e_1, and rho such that e_k is sitting in the
        // 0x8000, e_1 in 0x800, and rho in 0x80

        // next e_k and m_n
        __m128i m_n;
        w0 = _mm_srli_epi32(flags, 15); // e_k
        m_n = _mm_sub_epi32(U_q, w0);
        m_n = _mm_andnot_si128(insig, m_n);

        // find cumulative sums
        // to find at which bit in ms_vec the sample starts
        __m128i inc_sum = m_n; // inclusive scane
        inc_sum = _mm_add_epi32(inc_sum, _mm_bslli_si128(inc_sum, 4));
        inc_sum = _mm_add_epi32(inc_sum, _mm_bslli_si128(inc_sum, 8));
        int total_mn = _mm_extract_epi16(inc_sum, 6);
        __m128i ex_sum = _mm_bslli_si128(inc_sum, 4); // exclusive scane

        // find the starting byte and starting bit
        __m128i byte_idx = _mm_srli_epi32(ex_sum, 3);
        __m128i bit_idx = _mm_and_si128(ex_sum, _mm_set1_epi32(7));
        byte_idx = _mm_shuffle_epi8(byte_idx, 
          _mm_set_epi32(0x0C0C0C0C, 0x08080808, 0x04040404, 0x00000000));
        byte_idx = _mm_add_epi32(byte_idx, _mm_set1_epi32(0x03020100));
        __m128i d0 = _mm_shuffle_epi8(ms_vec, byte_idx);
        byte_idx = _mm_add_epi32(byte_idx, _mm_set1_epi32(0x01010101));
        __m128i d1 = _mm_shuffle_epi8(ms_vec, byte_idx);

        // shift samples values to correct location
        bit_idx = _mm_or_si128(bit_idx, _mm_slli_epi32(bit_idx, 16));
        __m128i bit_shift = _mm_shuffle_epi8(
          _mm_set_epi8(1u, 3u, 7u, 15u, 31u, 63u, 127u, 255u,
                       1u, 3u, 7u, 15u, 31u, 63u, 127u, 255u), bit_idx);
        bit_shift = _mm_add_epi16(bit_shift, _mm_set1_epi16(0x0101));
        d0 = _mm_mullo_epi16(d0, bit_shift);
        d0 = _mm_srli_epi16(d0, 8); // we should have 8 bits in the LSB
        d1 = _mm_mullo_epi16(d1, bit_shift);
        d1 = _mm_and_si128(d1, _mm_set1_epi32(0xFF00FF00)); // 8 in MSB
        d0 = _mm_or_si128(d0, d1);

        // ui32 v0 = _mm_extract_epi32(d, 0);
        // ui32 v1 = _mm_extract_epi32(d, 1);
        // ui32 v2 = _mm_extract_epi32(d, 2);
        // ui32 v3 = _mm_extract_epi32(d, 3);
        
        // find location of e_k and mask
        __m128i shift;
        __m128i ones = _mm_set1_epi32(1);
        __m128i twos = _mm_set1_epi32(2);
        __m128i U_q_m1 = _mm_sub_epi32(U_q, ones);
        U_q_m1 = _mm_and_si128(U_q_m1, _mm_set_epi32(0,0,0,0x1F));
        w0 = _mm_sub_epi32(twos, w0);        
        shift = _mm_sll_epi32(w0, U_q_m1); // U_q_m1 must be no more than 31
        ms_vec = _mm_and_si128(d0, _mm_sub_epi32(shift, ones));

        // next e_1
        w0 = _mm_and_si128(flags, _mm_set1_epi32(0x800));
        w0 = _mm_cmpeq_epi32(w0, _mm_setzero_si128());
        w0 = _mm_andnot_si128(w0, shift);  // e_1 in correct position
        ms_vec = _mm_or_si128(ms_vec, w0); // e_1
        w0 = _mm_slli_epi32(ms_vec, 31);   // sign
        ms_vec = _mm_or_si128(ms_vec, ones); // bin center
        __m128i tvn = ms_vec;
        ms_vec = _mm_add_epi32(ms_vec, twos);// + 2
        ms_vec = _mm_slli_epi32(ms_vec, p - 1);
        ms_vec = _mm_or_si128(ms_vec, w0); // sign
        row = _mm_andnot_si128(insig, ms_vec); // significant only

        ms_vec = _mm_andnot_si128(insig, tvn); // significant only
        tvn = _mm_or_si128(ms_vec, twos); // twos for easy emax calculation
        if (N == 0) // the compiler should remove one
          tvn = _mm_shuffle_epi8(tvn, 
            _mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0x0F0E0D0C, 0x07060504));
        else if (N == 1)
          tvn = _mm_shuffle_epi8(tvn, 
            _mm_set_epi32(0xFFFFFFFF, 0x0F0E0D0C, 0x07060504, 0xFFFFFFFF));
        else
          assert(0);
        vn = _mm_or_si128(vn, tvn);

        // if (N == 1)
        //   printf("%04x %04x %04x", _mm_extract_epi32(vn, 0), _mm_extract_epi32(vn, 1),
        //           _mm_extract_epi32(vn, 2));        

        // e0 = _mm_extract_epi32(ms_vec, 1);
        // e1 = _mm_extract_epi32(ms_vec, 3);
        // e0 = _mm_extract_epi16(ms_vec, 3);
        // e0 <<= 16;
        // e0 |= _mm_extract_epi16(ms_vec, 2);
        // e1 = _mm_extract_epi16(ms_vec, 7);
        // e1 <<= 16;
        // e1 |= _mm_extract_epi16(ms_vec, 6);

        // ui32 r0 = _mm_extract_epi32(row, 0);
        // ui32 r1 = _mm_extract_epi32(row, 1);
        // ui32 r2 = _mm_extract_epi32(row, 2);
        // ui32 r3 = _mm_extract_epi32(row, 3);

        // printf("%08x %08x ", v0, r0);
        // printf("%08x %08x ", v1, r1);
        // printf("%08x %08x ", v2, r2);
        // printf("%08x %08x ", v3, r3);

        if (total_mn)
          frwd_advance(magsgn, (ui32)total_mn);
      }
      return row;
    }

    static inline 
    __m128i one_quad_decode16(const __m128i inf_u_q, __m128i U_q, 
                              frwd_struct* magsgn, ui32 p, __m128i& vn)
    {
      __m128i w0;     // workers
      __m128i insig;  // lanes hold FF's if samples are insignificant
      __m128i flags;  // lanes hold e_k, e_1, and rho
      __m128i row;    // decoded row

      row = _mm_setzero_si128();
      w0 = _mm_shuffle_epi8(inf_u_q, 
        _mm_set_epi16(0x0504u, 0x0504u, 0x0504u, 0x0504u,
                      0x0100u, 0x0100u, 0x0100u, 0x0100u));
      // we keeps e_k, e_1, and rho in w2
      flags = _mm_and_si128(w0, 
        _mm_set_epi16(0x8880u, 0x4440u, 0x2220u, 0x1110u,
                      0x8880u, 0x4440u, 0x2220u, 0x1110u));
      insig = _mm_cmpeq_epi16(flags, _mm_setzero_si128());
      if (_mm_movemask_epi8(insig) != 0xFFFF) //are all insignificant?
      {
        U_q = _mm_shuffle_epi8(U_q, 
          _mm_set_epi16(0x0504u, 0x0504u, 0x0504u, 0x0504u,
                        0x0100u, 0x0100u, 0x0100u, 0x0100u));
        flags = _mm_mullo_epi16(flags, _mm_set_epi16(1,2,4,8,1,2,4,8));
        __m128i ms_vec = frwd_fetch<0xFF>(magsgn); 

        // U_q holds U_q for this quad
        // flags has e_k, e_1, and rho such that e_k is sitting in the
        // 0x8000, e_1 in 0x800, and rho in 0x80

        // next e_k and m_n
        __m128i m_n;
        w0 = _mm_srli_epi16(flags, 15); // e_k
        m_n = _mm_sub_epi16(U_q, w0);
        m_n = _mm_andnot_si128(insig, m_n);

        // find cumulative sums
        // to find at which bit in ms_vec the sample starts
        __m128i inc_sum = m_n; // inclusive scane
        inc_sum = _mm_add_epi16(inc_sum, _mm_bslli_si128(inc_sum, 2));
        inc_sum = _mm_add_epi16(inc_sum, _mm_bslli_si128(inc_sum, 4));
        inc_sum = _mm_add_epi16(inc_sum, _mm_bslli_si128(inc_sum, 8));
        int total_mn = _mm_extract_epi16(inc_sum, 7);
        __m128i ex_sum = _mm_bslli_si128(inc_sum, 2); // exclusive scane

        // find the starting byte and starting bit
        __m128i byte_idx = _mm_srli_epi16(ex_sum, 3);
        __m128i bit_idx = _mm_and_si128(ex_sum, _mm_set1_epi16(7));
        byte_idx = _mm_shuffle_epi8(byte_idx, 
          _mm_set_epi16(0x0E0E, 0x0C0C, 0x0A0A, 0x0808, 
                        0x0606, 0x0404, 0x0202, 0x0000));
        byte_idx = _mm_add_epi16(byte_idx, _mm_set1_epi16(0x0100));
        __m128i d0 = _mm_shuffle_epi8(ms_vec, byte_idx);
        byte_idx = _mm_add_epi16(byte_idx, _mm_set1_epi16(0x0101));
        __m128i d1 = _mm_shuffle_epi8(ms_vec, byte_idx);

        // shift samples values to correct location
        __m128i bit_shift = _mm_shuffle_epi8(
          _mm_set_epi8(1u, 3u, 7u, 15u, 31u, 63u, 127u, 255u,
                       1u, 3u, 7u, 15u, 31u, 63u, 127u, 255u), bit_idx);
        bit_shift = _mm_add_epi16(bit_shift, _mm_set1_epi16(0x0101u));
        d0 = _mm_mullo_epi16(d0, bit_shift);
        d0 = _mm_srli_epi16(d0, 8); // we should have 8 bits in the LSB
        d1 = _mm_mullo_epi16(d1, bit_shift);
        d1 = _mm_and_si128(d1, _mm_set1_epi16(0xFF00u)); // 8 in MSB
        d0 = _mm_or_si128(d0, d1);

        // ui32 v0 = _mm_extract_epi16(d0, 0);
        // ui32 v1 = _mm_extract_epi16(d0, 1);
        // ui32 v2 = _mm_extract_epi16(d0, 2);
        // ui32 v3 = _mm_extract_epi16(d0, 3);
        // ui32 v4 = _mm_extract_epi16(d0, 4);
        // ui32 v5 = _mm_extract_epi16(d0, 5);
        // ui32 v6 = _mm_extract_epi16(d0, 6);
        // ui32 v7 = _mm_extract_epi16(d0, 7);
        
        // find location of e_k and mask
        __m128i shift, t0, t1, Uq0, Uq1;
        __m128i ones = _mm_set1_epi16(1);
        __m128i twos = _mm_set1_epi16(2);
        __m128i U_q_m1 = _mm_sub_epi32(U_q, ones);
        Uq0 = _mm_and_si128(U_q_m1, _mm_set_epi32(0,0,0,0x1F));
        Uq1 = _mm_bsrli_si128(U_q_m1, 14);
        w0 = _mm_sub_epi16(twos, w0);
        t0 = _mm_and_si128(w0, _mm_set_epi64x(0, -1));
        t1 = _mm_and_si128(w0, _mm_set_epi64x(-1, 0));
        t0 = _mm_sll_epi16(t0, Uq0);
        t1 = _mm_sll_epi16(t1, Uq1);
        shift = _mm_or_si128(t0, t1);
        ms_vec = _mm_and_si128(d0, _mm_sub_epi16(shift, ones));

        // next e_1
        w0 = _mm_and_si128(flags, _mm_set1_epi16(0x800));
        w0 = _mm_cmpeq_epi16(w0, _mm_setzero_si128());
        w0 = _mm_andnot_si128(w0, shift);  // e_1 in correct position
        ms_vec = _mm_or_si128(ms_vec, w0); // e_1
        w0 = _mm_slli_epi16(ms_vec, 15);   // sign
        ms_vec = _mm_or_si128(ms_vec, ones); // bin center
        __m128i tvn = ms_vec;
        ms_vec = _mm_add_epi16(ms_vec, twos);// + 2
        ms_vec = _mm_slli_epi16(ms_vec, p - 1);
        ms_vec = _mm_or_si128(ms_vec, w0); // sign
        row = _mm_andnot_si128(insig, ms_vec); // significant only

        ms_vec = _mm_andnot_si128(insig, tvn); // significant only
        tvn = _mm_or_si128(ms_vec, twos); // twos for easy emax calculation
        w0 = _mm_shuffle_epi8(tvn, 
              _mm_set_epi16(0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu, 
                             0xFFFFu, 0xFFFFu, 0x0706u, 0x0302u));
        vn = _mm_or_si128(vn, w0);
        w0 = _mm_shuffle_epi8(tvn, 
              _mm_set_epi16(0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu, 
                             0xFFFFu, 0x0F0Eu, 0x0B0Au, 0xFFFFu));
        vn = _mm_or_si128(vn, w0);

        // printf("%04x %04x %04x", _mm_extract_epi16(vn, 0), _mm_extract_epi16(vn, 1),
        //         _mm_extract_epi16(vn, 2));

        // ui32 r0 = _mm_extract_epi16(row, 0);
        // ui32 r1 = _mm_extract_epi16(row, 1);
        // ui32 r2 = _mm_extract_epi16(row, 2);
        // ui32 r3 = _mm_extract_epi16(row, 3);
        // ui32 r4 = _mm_extract_epi16(row, 4);
        // ui32 r5 = _mm_extract_epi16(row, 5);
        // ui32 r6 = _mm_extract_epi16(row, 6);
        // ui32 r7 = _mm_extract_epi16(row, 7);

        // printf("%08x %08x ", v0, r0);
        // printf("%08x %08x ", v1, r1);
        // printf("%08x %08x ", v2, r2);
        // printf("%08x %08x ", v3, r3);
        // printf("%08x %08x ", v4, r4);
        // printf("%08x %08x ", v5, r5);
        // printf("%08x %08x ", v6, r6);
        // printf("%08x %08x ", v7, r7);

        if (total_mn)
          frwd_advance(magsgn, (ui32)total_mn);
      }
      return row;
    }


    //************************************************************************/
    /** @brief Decodes one codeblock, processing the cleanup, siginificance
     *         propagation, and magnitude refinement pass
     *
     *  @param [in]   coded_data is a pointer to bitstream
     *  @param [in]   decoded_data is a pointer to decoded codeblock data buf.
     *  @param [in]   missing_msbs is the number of missing MSBs
     *  @param [in]   num_passes is the number of passes: 1 if CUP only,
     *                2 for CUP+SPP, and 3 for CUP+SPP+MRP
     *  @param [in]   lengths1 is the length of cleanup pass
     *  @param [in]   lengths2 is the length of refinement passes (either SPP
     *                only or SPP+MRP)
     *  @param [in]   width is the decoded codeblock width 
     *  @param [in]   height is the decoded codeblock height
     *  @param [in]   stride is the decoded codeblock buffer stride 
     */
    bool ojph_decode_codeblock_ssse3(ui8* coded_data, ui32* decoded_data,
                                     ui32 missing_msbs, ui32 num_passes,
                                     ui32 lengths1, ui32 lengths2,
                                     ui32 width, ui32 height, ui32 stride)
    {
      // The cleanup pass is decoded in two steps; in step one,
      // the VLC and MEL segments are decoded, generating a record that 
      // has 2 bytes per quad. The 2 bytes contain, u, rho, e^1 & e^k.
      // This information should be sufficient for the next step.
      // In step 2, we decode the MagSgn segment.

      if (num_passes > 1 && lengths2 == 0)
      {
        OJPH_WARN(0x00010001, "A malformed codeblock that has more than "
                              "one coding pass, but zero length for "
                              "2nd and potential 3rd pass.\n");
        num_passes = 1;
      }
      if (num_passes > 3)
      {
        OJPH_ERROR(0x00010002, "We do not support more than 3 coding passes; "
                               "This codeblocks has %d passes.\n",
                               num_passes);
        return false;
      }

      if (missing_msbs > 29) // p < 1
        return false;        // 32 bits are not enough to decode this
      else if (missing_msbs == 29) // if p is 1, then num_passes must be 1
        num_passes = 1;
      ui32 p = 30 - missing_msbs; // The least significant bitplane for CUP
      // There is a way to handle the case of p == 0, but a different path
      // is required
      ui32 mmsbp2 = missing_msbs + 2;

      // read scup and fix the bytes there
      int lcup, scup;
      lcup = (int)lengths1;  // length of CUP
      //scup is the length of MEL + VLC
      scup = (((int)coded_data[lcup-1]) << 4) + (coded_data[lcup-2] & 0xF);
      if (scup < 2 || scup > lcup || scup > 4079) //something is wrong
        return false;

      // The temporary storage scratch holds two types of data in an 
      // interleaved fashion. The interleaving allows us to use one
      // memory pointer.
      // We have one entry for a decoded VLC code, and one entry for UVLC.
      // Entries are 16 bits each, corresponding to one quad, 
      // but since we want to use XMM registers of the SSE family 
      // of SIMD; we allocated 16 bytes or more per quad row; that is,
      // the width is no smaller than 16 bytes (or 8 entries), and the
      // height is 512 quads
      // Each VLC entry contains, in the following order, starting 
      // from MSB
      // e_k (4bits), e_1 (4bits), rho (4bits), useless for step 2 (4bits)
      // Each entry in UVLC contains u_q
      // We added an extra row, should we need a few more entries.
      ui16 scratch[8 * 513] = {0};          // 8+ kB

      si32 sstr = ((width + 2) + 7) & ~7; // multiples of 8

      assert((stride & 0x3) == 0);

      counter.tmp = tsc_measure_start();

      // step 1 decoding VLC and MEL segments
      {
        // init structures
        dec_mel_st mel;
        mel_init(&mel, coded_data, lcup, scup);
        rev_struct vlc;
        rev_init(&vlc, coded_data, lcup, scup);

        int run = mel_get_run(&mel); // decode runs of events from MEL bitstrm
                                     // data represented as runs of 0 events
                                     // See mel_decode description

        ui32 vlc_val;
        ui32 c_q = 0;
        ui16 *sp = scratch;
        //initial quad row
        for (ui32 x = 0; x < width; sp += 4)
        {
          // decode VLC
          /////////////

          // first quad
          vlc_val = rev_fetch(&vlc);

          //decode VLC using the context c_q and the head of VLC bitstream
          ui16 t0 = vlc_tbl0[ c_q + (vlc_val & 0x7F) ];

          // if context is zero, use one MEL event
          if (c_q == 0) //zero context
          {
            run -= 2; //subtract 2, since events number if multiplied by 2

            // Is the run terminated in 1? if so, use decoded VLC code, 
            // otherwise, discard decoded data, since we will decoded again 
            // using a different context
            t0 = (run == -1) ? t0 : 0;

            // is run -1 or -2? this means a run has been consumed
            if (run < 0) 
              run = mel_get_run(&mel);  // get another run
          }
          //run -= (c_q == 0) ? 2 : 0;
          //t0 = (c_q != 0 || run == -1) ? t0 : 0;
          //if (run < 0)
          //  run = mel_get_run(&mel);  // get another run
          sp[0] = t0; 
          x += 2;

          // prepare context for the next quad; eqn. 1 in ITU T.814
          c_q = ((t0 & 0x10U) << 3) | ((t0 & 0xE0U) << 2);

          //remove data from vlc stream (0 bits are removed if vlc is not used)
          vlc_val = rev_advance(&vlc, t0 & 0x7);

          //second quad
          ui16 t1 = 0;

          //decode VLC using the context c_q and the head of VLC bitstream
          t1 = vlc_tbl0[c_q + (vlc_val & 0x7F)]; 

          // if context is zero, use one MEL event
          if (c_q == 0 && x < width) //zero context
          {
            run -= 2; //subtract 2, since events number if multiplied by 2

            // if event is 0, discard decoded t1
            t1 = (run == -1) ? t1 : 0;

            if (run < 0) // have we consumed all events in a run
              run = mel_get_run(&mel); // if yes, then get another run
          }
          t1 = x < width ? t1 : 0;
          //run -= (c_q == 0 && x < width) ? 2 : 0;
          //t1 = (c_q != 0 || run == -1) ? t1 : 0;
          //if (run < 0)
          //  run = mel_get_run(&mel);  // get another run
          sp[2] = t1;
          x += 2;

          //prepare context for the next quad, eqn. 1 in ITU T.814
          c_q = ((t1 & 0x10U) << 3) | ((t1 & 0xE0U) << 2);

          //remove data from vlc stream, if qinf is not used, cwdlen is 0
          vlc_val = rev_advance(&vlc, t1 & 0x7);
          
          // decode u
          /////////////
          // uvlc_mode is made up of u_offset bits from the quad pair
          ui32 uvlc_mode = ((t0 & 0x8U) << 3) | ((t1 & 0x8U) << 4);
          if (uvlc_mode == 0xc0)// if both u_offset are set, get an event from
          {                     // the MEL run of events
            run -= 2; //subtract 2, since events number if multiplied by 2

            uvlc_mode += (run == -1) ? 0x40 : 0; // increment uvlc_mode by
                                                 // is 0x40

            if (run < 0)//if run is consumed (run is -1 or -2), get another run
              run = mel_get_run(&mel);
          }
          //run -= (uvlc_mode == 0xc0) ? 2 : 0;
          //uvlc_mode += (uvlc_mode == 0xc0 && run == -1) ? 0x40 : 0;
          //if (run < 0)
          //  run = mel_get_run(&mel);  // get another run

          //decode uvlc_mode to get u for both quads
          ui32 uvlc_entry = uvlc_tbl0[uvlc_mode + (vlc_val & 0x3F)];
          //remove total prefix length
          vlc_val = rev_advance(&vlc, uvlc_entry & 0x7); 
          uvlc_entry >>= 3; 
          //extract suffixes for quad 0 and 1
          ui32 len = uvlc_entry & 0xF;           //suffix length for 2 quads
          ui32 tmp = vlc_val & ((1 << len) - 1); //suffix value for 2 quads
          vlc_val = rev_advance(&vlc, len);
          uvlc_entry >>= 4;
          // quad 0 length
          len = uvlc_entry & 0x7; // quad 0 suffix length
          uvlc_entry >>= 3;
          ui16 u_q = (ui16)(1 + (uvlc_entry&7) + (tmp&~(0xFFU<<len))); //kap. 1
          sp[1] = u_q; 
          u_q = (ui16)(1 + (uvlc_entry >> 3) + (tmp >> len));  //kappa == 1
          sp[3] = u_q; 
        }
        sp[0] = sp[1] = 0;

        //non initial quad rows
        for (ui32 y = 2; y < height; y += 2)
        {
          c_q = 0;                                // context
          ui16 *sp = scratch + (y >> 1) * sstr;   // this row of quads

          for (ui32 x = 0; x < width; sp += 4)
          {
            // decode VLC
            /////////////

            // sigma_q (n, ne, nf)
            c_q |= ((sp[0 - sstr] & 0xA0U) << 2);
            c_q |= ((sp[2 - sstr] & 0x20U) << 4);

            // first quad
            vlc_val = rev_fetch(&vlc);

            //decode VLC using the context c_q and the head of VLC bitstream
            ui16 t0 = vlc_tbl1[ c_q + (vlc_val & 0x7F) ];

            // if context is zero, use one MEL event
            if (c_q == 0) //zero context
            {
              run -= 2; //subtract 2, since events number is multiplied by 2

              // Is the run terminated in 1? if so, use decoded VLC code, 
              // otherwise, discard decoded data, since we will decoded again 
              // using a different context
              t0 = (run == -1) ? t0 : 0;

              // is run -1 or -2? this means a run has been consumed
              if (run < 0) 
                run = mel_get_run(&mel);  // get another run
            }
            //run -= (c_q == 0) ? 2 : 0;
            //t0 = (c_q != 0 || run == -1) ? t0 : 0;
            //if (run < 0)
            //  run = mel_get_run(&mel);  // get another run
            sp[0] = t0;
            x += 2;

            // prepare context for the next quad; eqn. 2 in ITU T.814
            // sigma_q (w, sw)
            c_q = ((t0 & 0x40U) << 2) | ((t0 & 0x80U) << 1);
            // sigma_q (nw)
            c_q |= sp[0 - sstr] & 0x80;
            // sigma_q (n, ne, nf)
            c_q |= ((sp[2 - sstr] & 0xA0U) << 2);
            c_q |= ((sp[4 - sstr] & 0x20U) << 4);

            //remove data from vlc stream (0 bits are removed if vlc is unused)
            vlc_val = rev_advance(&vlc, t0 & 0x7);

            //second quad
            ui16 t1 = 0;

            //decode VLC using the context c_q and the head of VLC bitstream
            t1 = vlc_tbl1[ c_q + (vlc_val & 0x7F)]; 

            // if context is zero, use one MEL event
            if (c_q == 0 && x < width) //zero context
            {
              run -= 2; //subtract 2, since events number if multiplied by 2

              // if event is 0, discard decoded t1
              t1 = (run == -1) ? t1 : 0;

              if (run < 0) // have we consumed all events in a run
                run = mel_get_run(&mel); // if yes, then get another run
            }
            t1 = x < width ? t1 : 0;
            //run -= (c_q == 0 && x < width) ? 2 : 0;
            //t1 = (c_q != 0 || run == -1) ? t1 : 0;
            //if (run < 0)
            //  run = mel_get_run(&mel);  // get another run
            sp[2] = t1; 
            x += 2;

            // partial c_q, will be completed when we process the next quad
            // sigma_q (w, sw)
            c_q = ((t1 & 0x40U) << 2) | ((t1 & 0x80U) << 1);
            // sigma_q (nw)
            c_q |= sp[2 - sstr] & 0x80;

            //remove data from vlc stream, if qinf is not used, cwdlen is 0
            vlc_val = rev_advance(&vlc, t1 & 0x7);
          
            // decode u
            /////////////
            // uvlc_mode is made up of u_offset bits from the quad pair
            ui32 uvlc_mode = ((t0 & 0x8U) << 3) | ((t1 & 0x8U) << 4);
            ui32 uvlc_entry = uvlc_tbl1[uvlc_mode + (vlc_val & 0x3F)];
            //remove total prefix length
            vlc_val = rev_advance(&vlc, uvlc_entry & 0x7);
            uvlc_entry >>= 3;
            //extract suffixes for quad 0 and 1
            ui32 len = uvlc_entry & 0xF;           //suffix length for 2 quads
            ui32 tmp = vlc_val & ((1 << len) - 1); //suffix value for 2 quads
            vlc_val = rev_advance(&vlc, len);
            uvlc_entry >>= 4;
            // quad 0 length
            len = uvlc_entry & 0x7; // quad 0 suffix length
            uvlc_entry >>= 3;
            ui16 u_q = (ui16)((uvlc_entry & 7) + (tmp & ~(0xFU << len))); //u_q
            sp[1] = u_q;
            u_q = (ui16)((uvlc_entry >> 3) + (tmp >> len)); // u_q
            sp[3] = u_q;
          }
          sp[0] = sp[1] = 0;
        }
      }

      // step2 we decode magsgn
      if (mmsbp2 >= 16)
      {
        // We allocate a scratch row for storing v_n values.
        // We allocated enough 512 entries for 512 qauds.
        // The 8 leaves 16 bytes before the buffer and 16 after 
        // This is enough for SSE family of SIMD
        const int v_n_size = 512 + 8;
        ui32 v_n_scratch[2 * v_n_size] = {0}; // 4+ kB

        frwd_struct magsgn;
        frwd_init<0xFF>(&magsgn, coded_data, lcup - scup);

        {
          ui16 *sp = scratch;
          ui32 *vp = v_n_scratch;
          ui32 *dp = decoded_data;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //here we process two quads
            __m128i w0, w1; // workers
            __m128i inf_u_q, U_q;
            // determine U_q
            {
              inf_u_q = _mm_loadu_si128((__m128i*)sp);
              U_q = _mm_srli_epi32(inf_u_q, 16);

              w0 = _mm_cmpgt_epi32(U_q, _mm_set1_epi32((int)mmsbp2));
              int i = _mm_movemask_epi8(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }
            // printf("%2d  0 ", x);

            __m128i vn = _mm_set1_epi32(2);
            __m128i row0 = one_quad_decode32<0>(inf_u_q, U_q, &magsgn, p, vn);
            __m128i row1 = one_quad_decode32<1>(inf_u_q, U_q, &magsgn, p, vn);
            w0 = _mm_loadu_si128((__m128i*)vp);
            w0 = _mm_and_si128(w0, _mm_set_epi32(0,0,0,0xFFFFFFFF));
            w0 = _mm_or_si128(w0, vn);
            _mm_storeu_si128((__m128i*)vp, w0);            

            // printf("\n");

            //interleave in ssse3 style 
            w0 = _mm_unpacklo_epi32(row0, row1);
            w1 = _mm_unpackhi_epi32(row0, row1);
            row0 = _mm_unpacklo_epi32(w0, w1);
            row1 = _mm_unpackhi_epi32(w0, w1);
            _mm_store_si128((__m128i*)dp, row0);
            _mm_store_si128((__m128i*)(dp + stride), row1);
          }
        }

        for (ui32 y = 2; y < height; y += 2)
        {
          {
            // perform 31 - count_leading_zeros(*vp) here
            ui32 *vp = v_n_scratch;
            const __m128i lut_lo = _mm_set_epi8(
              4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 31
            );
            const __m128i lut_hi = _mm_set_epi8(
              0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 31
            );
            const __m128i nibble_mask = _mm_set1_epi8(0x0F);
            const __m128i byte_offset8 = _mm_set1_epi16(8);
            const __m128i byte_offset16 = _mm_set1_epi16(16);
            const __m128i cc = _mm_set1_epi32(31);
            for (ui32 x = 0; x <= width; x += 8, vp += 4)
            {
              __m128i v, t; // workers
              v = _mm_loadu_si128((__m128i*)vp);

              t = _mm_and_si128(nibble_mask, v);
              v = _mm_and_si128(_mm_srli_epi16(v, 4), nibble_mask);
              t = _mm_shuffle_epi8(lut_lo, t);
              v = _mm_shuffle_epi8(lut_hi, v);
              v = _mm_min_epu8(v, t);

              t = _mm_srli_epi16(v, 8);
              v = _mm_or_si128(v, byte_offset8);
              v = _mm_min_epu8(v, t);

              t = _mm_srli_epi32(v, 16);
              v = _mm_or_si128(v, byte_offset16);
              v = _mm_min_epu8(v, t);

              v = _mm_sub_epi16(cc, v);
              _mm_storeu_si128((__m128i*)(vp + v_n_size), v);
            }
          }

          ui32 *vp = v_n_scratch;
          ui16 *sp = scratch + (y >> 1) * sstr;
          ui32 *dp = decoded_data + y * stride;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //process two quads
            __m128i w0, w1; // workers
            __m128i inf_u_q, U_q;
            // determine U_q
            {
              __m128i gamma, emax, kappa, u_q; // needed locally

              inf_u_q = _mm_loadu_si128((__m128i*)sp);
              gamma = _mm_and_si128(inf_u_q, _mm_set1_epi32(0xF0));
              w0 = _mm_sub_epi32(gamma, _mm_set1_epi32(1));
              gamma = _mm_and_si128(gamma, w0);
              gamma = _mm_cmpeq_epi32(gamma, _mm_setzero_si128());

              emax = _mm_loadu_si128((__m128i*)(vp + v_n_size)); 
              w0 = _mm_bsrli_si128(emax, 4);
              emax = _mm_max_epi16(w0, emax); // no max_epi32 in ssse3              
              emax = _mm_andnot_si128(gamma, emax);

              kappa = _mm_set1_epi32(1);
              kappa = _mm_max_epi16(emax, kappa); // no max_epi32 in ssse3

              u_q = _mm_srli_epi32(inf_u_q, 16);
              U_q = _mm_add_epi32(u_q, kappa);

              w0 = _mm_cmpgt_epi32(U_q, _mm_set1_epi32((int)mmsbp2));
              int i = _mm_movemask_epi8(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }
            // printf("%2d %2d ", x, y);

            __m128i vn = _mm_set1_epi32(2);
            __m128i row0 = one_quad_decode32<0>(inf_u_q, U_q, &magsgn, p, vn);
            __m128i row1 = one_quad_decode32<1>(inf_u_q, U_q, &magsgn, p, vn);
            w0 = _mm_loadu_si128((__m128i*)vp);
            w0 = _mm_and_si128(w0, _mm_set_epi32(0,0,0,0xFFFFFFFF));
            w0 = _mm_or_si128(w0, vn);
            _mm_storeu_si128((__m128i*)vp, w0);  

            // printf("\n");

            //interleave in ssse3 style
            w0 = _mm_unpacklo_epi32(row0, row1);
            w1 = _mm_unpackhi_epi32(row0, row1);
            row0 = _mm_unpacklo_epi32(w0, w1);
            row1 = _mm_unpackhi_epi32(w0, w1);
            _mm_store_si128((__m128i*)dp, row0);
            _mm_store_si128((__m128i*)(dp + stride), row1);
          }
        }
      }
      else 
      {
        // reduce bitplane by 16 because we now have 16 bits instead of 32
        p -= 16;

        // We allocate a scratch row for storing v_n values.
        // We allocated enough 512 entries for 512 qauds.
        // The 8 leaves 16 bytes before the buffer and 16 after 
        // This is enough for SSE family of SIMD
        const int v_n_size = 512 + 16;
        ui16 v_n_scratch[2 * v_n_size] = {0}; // 2+ kB

        frwd_struct magsgn;
        frwd_init<0xFF>(&magsgn, coded_data, lcup - scup);

        {
          ui16 *sp = scratch;
          ui16 *vp = v_n_scratch;
          ui32 *dp = decoded_data;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //here we process two quads
            __m128i w0, w1; // workers
            __m128i inf_u_q, U_q;
            // determine U_q
            {
              inf_u_q = _mm_loadu_si128((__m128i*)sp);
              U_q = _mm_srli_epi32(inf_u_q, 16);

              w0 = _mm_cmpgt_epi32(U_q, _mm_set1_epi32((int)mmsbp2));
              int i = _mm_movemask_epi8(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }
            //printf("%2d  0 ", x);

            __m128i vn = _mm_set1_epi16(2);
            __m128i row = one_quad_decode16(inf_u_q, U_q, &magsgn, p, vn);
            w0 = _mm_loadu_si128((__m128i*)vp);
            w0 = _mm_and_si128(w0, _mm_set_epi16(0,0,0,0,0,0,0,0xFFFFu));
            w0 = _mm_or_si128(w0, vn);
            _mm_storeu_si128((__m128i*)vp, w0);  

            //printf("\n");

            //interleave in ssse3 style 
            w0 = _mm_shuffle_epi8(row, 
              _mm_set_epi16(0x0D0Cu, 0x8080u, 0x0908u, 0x8080u,
                            0x0504u, 0x8080u, 0x0100u, 0x8080u));
            _mm_store_si128((__m128i*)dp, w0);
            w1 = _mm_shuffle_epi8(row, 
              _mm_set_epi16(0x0F0Eu, 0x8080u, 0x0B0Au, 0x8080u,
                            0x0706u, 0x8080u, 0x0302u, 0x8080u));
            _mm_store_si128((__m128i*)(dp + stride), w1);
          }
        }

        for (ui32 y = 2; y < height; y += 2)
        {
          {
            // perform 31 - count_leading_zeros(*vp) here
            ui16 *vp = v_n_scratch;
            // for (ui32 x = 0; x <= width; x += 2, ++vp)
            // {
            //   vp[v_n_size] = 31 - count_leading_zeros(*vp | 2);
            // }
            const __m128i lut_lo = _mm_set_epi8(
              4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 15
            );
            const __m128i lut_hi = _mm_set_epi8(
              0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 15
            );
            const __m128i nibble_mask = _mm_set1_epi8(0x0F);
            const __m128i byte_offset8 = _mm_set1_epi16(8);
            const __m128i cc = _mm_set1_epi16(15);
            for (ui32 x = 0; x <= width; x += 16, vp += 8)
            {
              __m128i v, t; // workers
              v = _mm_loadu_si128((__m128i*)vp);

              t = _mm_and_si128(nibble_mask, v);
              v = _mm_and_si128(_mm_srli_epi16(v, 4), nibble_mask);
              t = _mm_shuffle_epi8(lut_lo, t);
              v = _mm_shuffle_epi8(lut_hi, v);
              v = _mm_min_epu8(v, t);

              t = _mm_srli_epi16(v, 8);
              v = _mm_or_si128(v, byte_offset8);
              v = _mm_min_epu8(v, t);

              v = _mm_sub_epi16(cc, v);
              _mm_storeu_si128((__m128i*)(vp + v_n_size), v);
            }
          }

          ui16 *vp = v_n_scratch;
          ui16 *sp = scratch + (y >> 1) * sstr;
          ui32 *dp = decoded_data + y * stride;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //process two quads
            __m128i w0, w1; // workers
            __m128i inf_u_q, U_q;
            // determine U_q
            {
              __m128i gamma, emax, kappa, u_q; // needed locally

              inf_u_q = _mm_loadu_si128((__m128i*)sp);
              gamma = _mm_and_si128(inf_u_q, _mm_set1_epi32(0xF0));
              w0 = _mm_sub_epi32(gamma, _mm_set1_epi32(1));
              gamma = _mm_and_si128(gamma, w0);
              gamma = _mm_cmpeq_epi32(gamma, _mm_setzero_si128());

              emax = _mm_loadu_si128((__m128i*)(vp + v_n_size)); 
              w0 = _mm_bsrli_si128(emax, 2);
              emax = _mm_max_epi16(w0, emax); // no max_epi32 in ssse3
              emax = _mm_shuffle_epi8(emax, 
                _mm_set_epi16(0x8080u, 0x0706u, 0x8080u, 0x0504u, 
                              0x8080u, 0x0302u, 0x8080u, 0x0100u));
              emax = _mm_andnot_si128(gamma, emax);

              kappa = _mm_set1_epi32(1);
              kappa = _mm_max_epi16(emax, kappa); // no max_epi32 in ssse3

              u_q = _mm_srli_epi32(inf_u_q, 16);
              U_q = _mm_add_epi32(u_q, kappa);

              w0 = _mm_cmpgt_epi32(U_q, _mm_set1_epi32((int)mmsbp2));
              int i = _mm_movemask_epi8(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }
            //printf("%2d %2d ", x, y);

            __m128i vn = _mm_set1_epi16(2);
            __m128i row = one_quad_decode16(inf_u_q, U_q, &magsgn, p, vn);
            w0 = _mm_loadu_si128((__m128i*)vp);
            w0 = _mm_and_si128(w0, _mm_set_epi16(0,0,0,0,0,0,0,0xFFFFu));
            w0 = _mm_or_si128(w0, vn);
            _mm_storeu_si128((__m128i*)vp, w0);  

            //printf("\n");

            w0 = _mm_shuffle_epi8(row, 
              _mm_set_epi16(0x0D0Cu, 0x8080u, 0x0908u, 0x8080u,
                            0x0504u, 0x8080u, 0x0100u, 0x8080u));
            _mm_store_si128((__m128i*)dp, w0);
            w1 = _mm_shuffle_epi8(row, 
              _mm_set_epi16(0x0F0Eu, 0x8080u, 0x0B0Au, 0x8080u,
                            0x0706u, 0x8080u, 0x0302u, 0x8080u));
            _mm_store_si128((__m128i*)(dp + stride), w1);
          }
        }
      }

      counter.count += tsc_measure_stop() - counter.tmp;
      counter.num_samples += width * height;
      return true;
    }
  }
}
