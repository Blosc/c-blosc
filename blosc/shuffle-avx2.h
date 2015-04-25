/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

/* AVX2-accelerated shuffle/unshuffle routines. */

#ifndef SHUFFLE_AVX2_H
#define SHUFFLE_AVX2_H

/* Make sure AVX2 is available for the compilation target and compiler. */
#if !defined(__AVX2__)
  #error AVX2 is not supported by the target architecture/platform and/or this compiler.
#endif

#include "shuffle-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  AVX2-accelerated shuffle routine.
*/
DLL_EXPORT void shuffle_avx2(const size_t bytesoftype, const size_t blocksize,
             const uint8_t* const _src, uint8_t* const _dest);

/**
  AVX2-accelerated unshuffle routine.
*/
DLL_EXPORT void unshuffle_avx2(const size_t bytesoftype, const size_t blocksize,
               const uint8_t* const _src, uint8_t* const _dest);

#ifdef __cplusplus
}
#endif

#endif /* SHUFFLE_AVX2_H */
