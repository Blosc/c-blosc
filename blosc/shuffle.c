/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>
  Creation date: 2009-05-20

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include "shuffle.h"
#include "shuffle-generic.h"

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
  /* Function pointer to the shuffle routine for this implementation. */
  shuffle_func shuffle;
  /* Function pointer to the unshuffle routine for this implementation. */
  unshuffle_func unshuffle;
} shuffle_implementation_t;


/*  Detect hardware and set function pointers to the best shuffle/unshuffle
    implementations supported by the host processor. */
#if defined(__AVX2__) || defined(__SSE2__)    /* Intel/i686 */

static shuffle_implementation_t
get_shuffle_implementation() {
  /* TODO : Fix this so it supports Clang and MSVC and works correctly in virtualized envs. */
#if !defined(__clang__) && defined(__GNUC__) && defined(__GNUC_MINOR__) && \
    __GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#if defined(__AVX2__)
  if (__builtin_cpu_supports("avx2")) {
    shuffle_implementation_t impl_avx2;
    impl_avx2.shuffle = (shuffle_func)shuffle_avx2;
    impl_avx2.unshuffle = (unshuffle_func)unshuffle_avx2;
    return impl_avx2;
  }
#endif  /* defined(__AVX2__) */
#if defined(__SSE2__)
  if (__builtin_cpu_supports("sse2")) {
    shuffle_implementation_t impl_sse2;
    impl_sse2.shuffle = (shuffle_func)shuffle_sse2;
    impl_sse2.unshuffle = (unshuffle_func)unshuffle_sse2;
    return impl_sse2;
  }
#endif  /* defined(__SSE2__) */
#endif  /* GCC run-time CPU feature support */
  
  /*  Processor doesn't support any of the hardware-accelerated implementations,
      so use the generic implementation. */
  shuffle_implementation_t impl_generic;
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

/* TODO : Maybe re-implement the initialization stuff below using the
          pthread_init_once() function. */

/*  Flag indicating whether the implementation has been initialized.
    Zero means it hasn't been initialized, non-zero means it has. */
static int32_t implementation_initialized;

/*  The dynamically-chosen shuffle/unshuffle implementation.
    This is only safe to use once `implementation_initialized` is set. */
static shuffle_implementation_t host_implementation;

/*  Initialize the shuffle implementation, if necessary. */
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
