/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

/* SSE2-accelerated shuffle/unshuffle routines. */
   
#ifndef SHUFFLE_SSE2_H
#define SHUFFLE_SSE2_H

/* Make sure SSE2 is available for the compilation target and compiler. */
#if !defined(__SSE2__)
  #error SSE2 is not supported by the target architecture/platform and/or this compiler.
#endif

#include "shuffle-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  SSE2-accelerated shuffle routine.
*/
DLL_EXPORT void shuffle_sse2(const size_t bytesoftype, const size_t blocksize,
             const uint8_t* const _src, uint8_t* const _dest);

/**
  SSE2-accelerated unshuffle routine.
*/
DLL_EXPORT void unshuffle_sse2(const size_t bytesoftype, const size_t blocksize,
               const uint8_t* const _src, uint8_t* const _dest);

#ifdef __cplusplus
}
#endif

#endif /* SHUFFLE_SSE2_H */
