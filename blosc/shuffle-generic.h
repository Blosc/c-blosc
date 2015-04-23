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

#ifndef SHUFFLE_GENERIC_H
#define SHUFFLE_GENERIC_H

#include "shuffle-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  Generic (non-hardware-accelerated) shuffle routine.
*/
BLOSC_DLL_EXPORT void shuffle_generic(const size_t bytesoftype, const size_t blocksize,
                                      const uint8_t* const _src, uint8_t* const _dest);

/**
  Generic (non-hardware-accelerated) unshuffle routine.
*/
BLOSC_DLL_EXPORT void unshuffle_generic(const size_t bytesoftype, const size_t blocksize,
                                        const uint8_t* const _src, uint8_t* const _dest);

#ifdef __cplusplus
}
#endif

#endif /* SHUFFLE_GENERIC_H */
