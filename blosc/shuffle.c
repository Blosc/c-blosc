/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>
  Creation date: 2009-05-20

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include "shuffle.h"
#include "shuffle-common.h"
#include "shuffle-generic.h"
#include <stdbool.h>

/* If C11 is supported use it's built-in atomic support for initialization. */
#if __STDC_VERSION__ >= 201112L
  #include <stdatomic.h>
#endif

/*  Include hardware-accelerated shuffle/unshuffle routines based on
    the target architecture. Note that a target architecture may support
    more than one type of acceleration!*/
#if defined(__AVX2__)
  #include "shuffle-avx2.h"
#endif  /* defined(__AVX2__) */

#if defined(__SSE2__)
  #include "shuffle-sse2.h"
#endif  /* defined(__SSE2__) */


/*  Define function pointer types for shuffle/unshuffle routines. */
typedef void(*shuffle_func)(size_t, size_t, const uint8_t* const, uint8_t* const);
typedef void(*unshuffle_func)(size_t, size_t, const uint8_t* const, uint8_t* const);

/* An implementation of shuffle/unshuffle routines. */
typedef struct shuffle_implementation {
  /* Name of this implementation. */
  const char* name;
  /* Function pointer to the shuffle routine for this implementation. */
  shuffle_func shuffle;
  /* Function pointer to the unshuffle routine for this implementation. */
  unshuffle_func unshuffle;
} shuffle_implementation_t;


/*  Detect hardware and set function pointers to the best shuffle/unshuffle
    implementations supported by the host processor. */
#if defined(__AVX2__) || defined(__SSE2__)    /* Intel/i686 */

#include <immintrin.h>  /* Needed for _xgetbv */
#if defined(_MSC_VER)
  #include <intrin.h>
#else

/*  Implement the __cpuid and __cpuidex intrinsics for GCC, Clang,
    and others using inline assembly. */
__attribute__((always_inline))
static inline void
__cpuidex(int32_t cpuInfo[4], int32_t function_id, uint32_t subfunction_id) {
  __asm__ __volatile__ (
# if defined(__i386__) && defined (__PIC__)
  /*  Can't clobber ebx with PIC running under 32-bit, so it needs to be manually restored.
      https://software.intel.com/en-us/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family
  */
     "movl %%ebx, %%edi\n\t"
     "cpuid\n\t"
     "xchgl %%ebx, %%edi":
     "=D" (cpuInfo[1]),
#else
        "cpuid":
        "=b" (cpuInfo[1]),
#endif  /* defined(__i386) && defined(__PIC__) */
        "=a" (cpuInfo[0]),
        "=c" (cpuInfo[2]),
        "=d" (cpuInfo[3]) :
        "a" (function_id), "c" (subfunction_id)
    );
}

#define __cpuid(cpuInfo, function_id) __cpuidex(cpuInfo, function_id, 0)

#endif /* defined(_MSC_VER) */

static shuffle_implementation_t
get_shuffle_implementation() {
  /* Holds the values of eax, ebx, ecx, edx set by the `cpuid` instruction */
  int32_t cpu_info[4];

  /* Get the number of basic functions available. */
  __cpuid(cpu_info, 0);
  int32_t max_basic_function_id = cpu_info[0];

  /* Check for SSE-based features and required OS support */
  __cpuid(cpu_info, 1);
  const bool sse2_available = (cpu_info[3] & (1 << 26)) != 0;
  const bool sse3_available = (cpu_info[2] & (1 << 0)) != 0;
  const bool ssse3_available = (cpu_info[2] & (1 << 9)) != 0;
  const bool sse41_available = (cpu_info[2] & (1 << 19)) != 0;
  const bool sse42_available = (cpu_info[2] & (1 << 20)) != 0;

  const bool xsave_available = (cpu_info[2] & (1 << 26)) != 0;
  const bool xsave_enabled_by_os = (cpu_info[2] & (1 << 27)) != 0;

  /* Check for AVX-based features, if the processor supports extended features. */
  bool avx2_available = false;
  bool avx512bw_available = false;
  if (max_basic_function_id >= 7) {
    __cpuid(cpu_info, 7);
    avx2_available = (cpu_info[1] & (1 << 5)) != 0;
    avx512bw_available = (cpu_info[1] & (1 << 30)) != 0;
  }

  /*  Even if certain features are supported by the CPU, they may not be supported
      by the OS (in which case using them would crash the process or system).
      If xsave is available and enabled by the OS, check the contents of the
      extended control register XCR0 to see if the CPU features are enabled. */
  bool xmm_state_enabled = false;
  bool ymm_state_enabled = false;
  bool zmm_state_enabled = false;

#if defined(_XCR_XFEATURE_ENABLED_MASK)
  if (xsave_available && xsave_enabled_by_os && (
      sse2_available || sse3_available || ssse3_available
      || sse41_available || sse42_available
      || avx2_available || avx512bw_available)) {
    /* Determine which register states can be restored by the OS. */
    uint64_t xcr0_contents = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);

    xmm_state_enabled = (xcr0_contents & (1UL << 1)) != 0;
    ymm_state_enabled = (xcr0_contents & (1UL << 2)) != 0;

    /*  Require support for both the upper 256-bits of zmm0-zmm15 to be
        restored as well as all of zmm16-zmm31 and the opmask registers. */
    zmm_state_enabled = (xcr0_contents & 0x70) == 0x70;
  }
#endif /* defined(_XCR_XFEATURE_ENABLED_MASK) */

  /* Using the gathered CPU information, determine which implementation to use. */
#if defined(__AVX2__)
  if (ymm_state_enabled && avx2_available) {
    shuffle_implementation_t impl_avx2;
    impl_avx2.name = "avx2";
    impl_avx2.shuffle = (shuffle_func)shuffle_avx2;
    impl_avx2.unshuffle = (unshuffle_func)unshuffle_avx2;
    return impl_avx2;
  }
#endif  /* defined(__AVX2__) */

#if defined(__SSE2__)
  if (xmm_state_enabled && sse2_available) {
    shuffle_implementation_t impl_sse2;
    impl_sse2.name = "sse2";
    impl_sse2.shuffle = (shuffle_func)shuffle_sse2;
    impl_sse2.unshuffle = (unshuffle_func)unshuffle_sse2;
    return impl_sse2;
  }
#endif  /* defined(__SSE2__) */

  /*  Processor doesn't support any of the hardware-accelerated implementations,
      so use the generic implementation. */
  shuffle_implementation_t impl_generic;
  impl_generic.name = "generic";
  impl_generic.shuffle = (shuffle_func)shuffle_generic;
  impl_generic.unshuffle = (unshuffle_func)unshuffle_generic;
  return impl_generic;
}

#else   /* No hardware acceleration supported for the target architecture. */
  #if defined(_MSC_VER)
  #pragma message("Hardware-acceleration detection not implemented for the target architecture. Only the generic shuffle/unshuffle routines will be available.")
  #else
  #warning Hardware-acceleration detection not implemented for the target architecture. Only the generic shuffle/unshuffle routines will be available.
  #endif

static shuffle_implementation_t
get_shuffle_implementation() {
  /* No hardware detection available, so just use the generic implementation. */
  shuffle_implementation_t impl;
  impl.shuffle = (shuffle_func)shuffle_generic;
  impl.unshuffle = (unshuffle_func)unshuffle_generic;
  return impl;
}

#endif


/*  Flag indicating whether the implementation has been initialized.
    Zero means it hasn't been initialized, non-zero means it has. */
static int32_t implementation_initialized;

/*  The dynamically-chosen shuffle/unshuffle implementation.
    This is only safe to use once `implementation_initialized` is set. */
static shuffle_implementation_t host_implementation;

/*  Initialize the shuffle implementation, if necessary. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((always_inline))
#endif
static
#if defined(_MSC_VER)
__forceinline
#else
inline
#endif
void init_shuffle_implementation() {
  /* Initialization could (in rare cases) take place concurrently on multiple threads,
     but it shouldn't matter because the initialization should return the same result
     on each thread (so the implementation will be the same). Since that's the case
     we can avoid complicated synchronization here and get a small performance benefit
     because we don't need to perform a volatile load on the initialization variable
     each time this function is called. */
#if defined(__GNUC__) || defined(__clang__)
  if (__builtin_likely(!implementation_initialized, 0)) {
#else
  if (!implementation_initialized) {
#endif
    /* Initialize the implementation. */
    host_implementation = get_shuffle_implementation();

    /*  Set the flag indicating the implementation has been initialized.
        Use an atomic exchange if supported; it's not strictly necessary
        but helps avoid unnecessary re-initialization. */
#if __STDC_VERSION__ >= 201112L
    atomic_exchange(&implementation_initialized, 1);
#elif defined(_MSC_VER)
    _InterlockedExchange(&implementation_initialized, 1);
#else
    implementation_initialized = 1;
#endif
  }
}

/*  Shuffle a block by dynamically dispatching to the appropriate
    hardware-accelerated routine at run-time. */
void
shuffle(const size_t bytesoftype, const size_t blocksize,
         const uint8_t* const _src, uint8_t* const _dest) {
  /* Initialize the shuffle implementation if necessary. */
  init_shuffle_implementation();

  /*  The implementation is initialized.
      Dispatch to it's shuffle routine. */
  (host_implementation.shuffle)(bytesoftype, blocksize, _src, _dest);
}

/*  Unshuffle a block by dynamically dispatching to the appropriate
    hardware-accelerated routine at run-time. */
void
unshuffle(const size_t bytesoftype, const size_t blocksize,
               const uint8_t* const _src, uint8_t* const _dest) {
  /* Initialize the shuffle implementation if necessary. */
  init_shuffle_implementation();

  /*  The implementation is initialized.
      Dispatch to it's unshuffle routine. */
  (host_implementation.unshuffle)(bytesoftype, blocksize, _src, _dest);
}
