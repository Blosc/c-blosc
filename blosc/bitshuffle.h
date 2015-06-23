/*
 * Bitshuffle - Filter for improving compression of typed binary data.
 *
 * Author: Kiyoshi Masui <kiyo@physics.ubc.ca>
 * Website: http://www.github.com/kiyo-masui/bitshuffle
 * Created: 2014
 *
 * See LICENSES/BITSHUFFLE.txt file for details about copyright and
 * rights to use.
 *
 * Note: Adapted for c-blosc by Francesc Alted.
 *
 * Header File
 *
 * Worker routines return an int64_t which is the number of bytes processed
 * if positive or an error code if negative.
 *
 * Error codes:
 *      -1    : Failed to allocate memory.
 *      -11   : Missing SSE.
 *      -12   : Missing AVX.
 *      -80   : Input size not a multiple of 8.
 *      -81   : block_size not multiple of 8.
 *      -91   : Decompression error, wrong number of bytes processed.
 *      -1YYY : Error internal to compression routine with error code -YYY.
 */


#ifndef BITSHUFFLE_H
#define BITSHUFFLE_H

#include "shuffle-common.h"


/*  Keep in sync with upstream */
#ifndef BSHUF_VERSION_MAJOR
#define BSHUF_VERSION_MAJOR 0
#define BSHUF_VERSION_MINOR 2
#define BSHUF_VERSION_POINT 1
#endif

#ifdef __cplusplus
extern "C" {
#endif


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
 *
 * Returns
 * -------
 *  nothing -- this cannot fail
 *
 */
BLOSC_NO_EXPORT int64_t bshuf_trans_bit_elem(void* in, void* out,
                                             const size_t size,
                                             const size_t elem_size,
                                             void *tmp_buf);


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
 *
 * Returns
 * -------
 *  nothing -- this cannot fail
 *
 */
BLOSC_NO_EXPORT int64_t bshuf_untrans_bit_elem(void* in, void* out,
                                               const size_t size,
                                               const size_t elem_size,
                                               void *tmp_buf);


#ifdef __cplusplus
} /*  extern "C" */
#endif

#endif  /*  BITSHUFFLE_H */
