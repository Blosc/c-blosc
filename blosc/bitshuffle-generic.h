/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

/* Generic (non-hardware-accelerated) shuffle/unshuffle routines.
   These are used when hardware-accelerated functions aren't available
   for a particular platform; they are also used by the hardware-
   accelerated functions to handle any remaining elements in a block
   which isn't a multiple of the hardware's vector size. */

#ifndef BITSHUFFLE_GENERIC_H
#define BITSHUFFLE_GENERIC_H

#include "shuffle-common.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The next is useful for debugging purposes */
#if 0
#include <stdio.h>
#include <string.h>

static void printymm(__m256i ymm0)
{
  uint8_t buf[32];

  ((__m256i *)buf)[0] = ymm0;
  printf("%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
          buf[0], buf[1], buf[2], buf[3],
          buf[4], buf[5], buf[6], buf[7],
          buf[8], buf[9], buf[10], buf[11],
          buf[12], buf[13], buf[14], buf[15],
          buf[16], buf[17], buf[18], buf[19],
          buf[20], buf[21], buf[22], buf[23],
          buf[24], buf[25], buf[26], buf[27],
          buf[28], buf[29], buf[30], buf[31]);
}
#endif



/*  Macros. */
#define CHECK_MULT_EIGHT(n) if (n % 8) return -80;
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define CHECK_ERR(count) if (count < 0) { return count; }


/* ---- Worker code not requiring special instruction sets. ----
 *
 * The following code does not use any x86 specific vectorized instructions
 * and should compile on any machine
 *
 */

/* Transpose 8x8 bit array packed into a single quadword *x*.
 * *t* is workspace. */
#define TRANS_BIT_8X8(x, t) {                                               \
        t = (x ^ (x >> 7)) & 0x00AA00AA00AA00AALL;                          \
        x = x ^ t ^ (t << 7);                                               \
        t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCLL;                         \
        x = x ^ t ^ (t << 14);                                              \
        t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0LL;                         \
        x = x ^ t ^ (t << 28);                                              \
    }


/* Transpose of an array of arbitrarily typed elements. */
#define TRANS_ELEM_TYPE(in, out, lda, ldb, type_t) {                        \
        type_t* in_type = (type_t*) in;                                     \
        type_t* out_type = (type_t*) out;                                   \
        size_t ii, jj, kk;                                                  \
        for (ii = 0; ii + 7 < lda; ii += 8) {                               \
            for (jj = 0; jj < ldb; jj++) {                                  \
                for (kk = 0; kk < 8; kk++) {                                \
                    out_type[jj*lda + ii + kk] =                            \
                        in_type[ii*ldb + kk * ldb + jj];                    \
                }                                                           \
            }                                                               \
        }                                                                   \
        for (ii = lda - lda % 8; ii < lda; ii ++) {                         \
            for (jj = 0; jj < ldb; jj++) {                                  \
                out_type[jj*lda + ii] = in_type[ii*ldb + jj];               \
            }                                                               \
        }                                                                   \
    }


/* Bitshuffle the data.
 *
 * Transpose the bits within elements.
 *
 * Parameters
 * ----------
 *  in : input buffer, must be of size * elem_size bytes
 *  out : output buffer, must be of size * elem_size bytes
 *  size : number of elements in input
 *  elem_size : element size of typed data
 *  tmp_buffer : temporary buffer with the same `size` than `in` and `out`
 *
 * Returns
 * -------
 *  nothing -- this cannot fail
 *
 */

int64_t bshuf_trans_bit_elem_scal(void* in, void* out, const size_t size,
                                  const size_t elem_size, void* tmp_buf);

/* Unshuffle bitshuffled data.
 *
 * Untranspose the bits within elements.
 *
 * To properly unshuffle bitshuffled data, *size* and *elem_size* must
 * match the parameters used to shuffle the data.
 *
 * Parameters
 * ----------
 *  in : input buffer, must be of size * elem_size bytes
 *  out : output buffer, must be of size * elem_size bytes
 *  size : number of elements in input
 *  elem_size : element size of typed data
 *  tmp_buffer : temporary buffer with the same `size` than `in` and `out`
 *
 * Returns
 * -------
 *  nothing -- this cannot fail
 *
 */

int64_t bshuf_untrans_bit_elem_scal(void* in, void* out, const size_t size,
                                    const size_t elem_size, void* tmp_buf);


#ifdef __cplusplus
}
#endif

#endif /* BITSHUFFLE_GENERIC_H */
