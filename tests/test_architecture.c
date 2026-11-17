/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#include "../blosc/blosc-common.h"

#if defined(_M_ARM64EC)
  #if defined(__SSE2__) || defined(__AVX2__)
    #error ARM64EC must not select x86 SIMD intrinsics.
  #endif
  #if defined(SHUFFLE_SSE2_ENABLED) || defined(SHUFFLE_AVX2_ENABLED)
    #error ARM64EC must use the generic shuffle implementations.
  #endif
#elif defined(_MSC_VER) && (defined(_M_X64) || \
    (defined(_M_IX86) && _M_IX86_FP >= 2))
  #if !defined(__SSE2__)
    #error SSE2 must remain enabled for supported native x86 targets.
  #endif
#endif

int main(void)
{
  return 0;
}
