/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Unit tests for basic features in Blosc.

  Creation date: 2010-06-07
  Author: Francesc Alted <francesc@blosc.org>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include "test_common.h"
#include "../blosc/shuffle.h"
#include "../blosc/shuffle-generic.h"

/* Include accelerated shuffles if supported by this compiler.
   TODO: Need to also do run-time CPU feature support here. */
#if defined(SHUFFLE_SSE2_ENABLED)
  #include "../blosc/shuffle-sse2.h"
#else
  #if defined(_MSC_VER)
  #pragma message("SSE2 shuffle tests not enabled.")
  #else
  #warning SSE2 shuffle tests not enabled.
  #endif
#endif  /* defined(SHUFFLE_SSE2_ENABLED) */

#if defined(SHUFFLE_AVX2_ENABLED)
  #include "../blosc/shuffle-avx2.h"
#else
  #if defined(_MSC_VER)
  #pragma message("AVX2 shuffle tests not enabled.")
  #else
  #warning AVX2 shuffle tests not enabled.
  #endif
#endif  /* defined(SHUFFLE_AVX2_ENABLED) */


int tests_run = 0;

/* Global vars */
void *src, *srccpy, *dest, *dest2;
size_t nbytes, cbytes;
int clevel = 1;
int doshuffle = 0;
size_t typesize = 4;
size_t size = 1000;             /* must be divisible by 4 */

static char* malloc_cleaned(size_t size)
{
  const int clean_value = 0x99;
  char* buf = malloc(size);
  memset(buf, clean_value, size);
  return buf;
}


/* Check maxout with maxout < size */
static char *test_maxout_less() {

  /* Get a compressed buffer */
  cbytes = blosc_compress(clevel, doshuffle, typesize, size, src,
                          dest, size+15);
  mu_assert("ERROR: cbytes is not 0", cbytes == 0);

  return 0;
}

/* Check maxout with maxout == size */
static char *test_maxout_equal() {

  /* Get a compressed buffer */
  cbytes = blosc_compress(clevel, doshuffle, typesize, size, src,
                          dest, size+16);
  mu_assert("ERROR: cbytes is not correct", cbytes == size+16);

  /* Decompress the buffer */
  nbytes = blosc_decompress(dest, dest2, size);
  mu_assert("ERROR: nbytes incorrect(1)", nbytes == size);

  return 0;
}


/* Check maxout with maxout > size */
static char *test_maxout_great() {
  /* Get a compressed buffer */
  cbytes = blosc_compress(clevel, doshuffle, typesize, size, src,
                          dest, size+17);
  mu_assert("ERROR: cbytes is not 0", cbytes == size+16);

  /* Decompress the buffer */
  nbytes = blosc_decompress(dest, dest2, size);
  mu_assert("ERROR: nbytes incorrect(1)", nbytes == size);

  return 0;
}

static char * test_shuffle()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size + BLOSC_MAX_OVERHEAD);
      for (k = 0; k < buffer_size; k++) {
        d[k] = rand();
      }
      blosc_compress(5, 1, type_size, buffer_size, d, o, buffer_size + BLOSC_MAX_OVERHEAD);
      blosc_decompress(o, d2, buffer_size);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle test failed", ok);
    }
  }

  return 0;
}

static char * test_noshuffle()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size + BLOSC_MAX_OVERHEAD);
      for (k = 0; k < buffer_size; k++) {
        d[k] = rand();
      }
      blosc_compress(5, 0, type_size, buffer_size, d, o, buffer_size + BLOSC_MAX_OVERHEAD);
      blosc_decompress(o, d2, buffer_size);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: noshuffle test failed", ok);
    }
  }

  return 0;
}

static char * test_shuffle_generic_then_unshuffle_generic()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Generic shuffle, then generic unshuffle. */
      shuffle_generic(type_size, buffer_size, d, o);
      unshuffle_generic(type_size, buffer_size, o, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle_generic+unshuffle_generic test failed", ok);
    }
  }

  return 0;
}

#if defined(SHUFFLE_SSE2_ENABLED)

static char * test_shuffle_sse2_vs_shuffle_generic()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      char * o_generic = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Shuffle with accelerated and generic shuffles. */
      shuffle_sse2(type_size, buffer_size, d, o);
      shuffle_generic(type_size, buffer_size, d, o_generic);
      ok = (memcmp(o, o_generic, buffer_size) == 0);
      free(d);
      free(o);
      free(o_generic);
      mu_assert("ERROR: shuffle_sse2 vs. shuffle_generic test failed", ok);
    }
  }

  return 0;
}

static char * test_shuffle_generic_then_unshuffle_sse2()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Generic shuffle, then accelerated unshuffle. */
      shuffle_generic(type_size, buffer_size, d, o);
      unshuffle_sse2(type_size, buffer_size, o, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle_generic+unshuffle_sse2 test failed", ok);
    }
  }

  return 0;
}

static char * test_shuffle_sse2_then_unshuffle_generic()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Accelerated shuffle then generic unshuffle. */
      shuffle_sse2(type_size, buffer_size, d, o);
      unshuffle_generic(type_size, buffer_size, o, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle_sse2+unshuffle_generic test failed", ok);
    }
  }

  return 0;
}

static char * test_shuffle_sse2_then_unshuffle_sse2()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Accelerated shuffle then accelerated unshuffle. */
      shuffle_sse2(type_size, buffer_size, d, o);
      unshuffle_sse2(type_size, buffer_size, o, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle_sse2+unshuffle_sse2 test failed", ok);
    }
  }

  return 0;
}

#endif /* defined(SHUFFLE_SSE2_ENABLED) */

#if defined(SHUFFLE_AVX2_ENABLED)

static char * test_shuffle_avx2_vs_shuffle_generic()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      char * o_generic = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Shuffle with accelerated and generic shuffles. */
      shuffle_avx2(type_size, buffer_size, d, o);
      shuffle_generic(type_size, buffer_size, d, o_generic);
      ok = (memcmp(o, o_generic, buffer_size) == 0);
      free(d);
      free(o);
      free(o_generic);
      mu_assert("ERROR: shuffle_avx2 vs. shuffle_generic test failed", ok);
    }
  }

  return 0;
}

static char * test_shuffle_generic_then_unshuffle_avx2()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Generic shuffle, then accelerated unshuffle. */
      shuffle_generic(type_size, buffer_size, d, o);
      unshuffle_avx2(type_size, buffer_size, o, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle_generic+unshuffle_avx2 test failed", ok);
    }
  }

  return 0;
}

static char * test_shuffle_avx2_then_unshuffle_generic()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Accelerated shuffle then generic unshuffle. */
      shuffle_avx2(type_size, buffer_size, d, o);
      unshuffle_generic(type_size, buffer_size, o, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle_avx2+unshuffle_generic test failed", ok);
    }
  }

  return 0;
}

static char * test_shuffle_avx2_then_unshuffle_avx2()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size);
      for (k = 0; k < buffer_size; k++) {
        d[k] = (char)k;
      }
      /* Accelerated shuffle then accelerated unshuffle. */
      shuffle_avx2(type_size, buffer_size, d, o);
      unshuffle_avx2(type_size, buffer_size, o, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: shuffle_avx2+unshuffle_avx2 test failed", ok);
    }
  }

  return 0;
}

#endif /* defined(SHUFFLE_AVX2_ENABLED) */

static char * test_getitem()
{
  size_t lengths[] = { 7, 64 * 3, 7 * 256, 500, 8000, 100000, 702713 };
  size_t types[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 22, 30, 32, 42, 48, 52, 53, 64, 80 };
  int i, j;
  size_t k;
  int ok;
  for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
      size_t num_elements = lengths[i];
      size_t type_size = types[j];
      size_t buffer_size = num_elements * type_size;
      char * d = malloc_cleaned(buffer_size);
      char * d2 = malloc_cleaned(buffer_size);
      char * o = malloc_cleaned(buffer_size + BLOSC_MAX_OVERHEAD);
      for (k = 0; k < buffer_size; k++) {
        d[k] = rand();
      }
      blosc_compress(5, 1, type_size, buffer_size, d, o, buffer_size + BLOSC_MAX_OVERHEAD);
      blosc_getitem(o, 0, num_elements, d2);
      ok = (memcmp(d, d2, buffer_size) == 0);
      free(d);
      free(d2);
      free(o);
      mu_assert("ERROR: getitem test failed", ok);
    }
  }

  return 0;
}

static char *all_tests() {
  mu_run_test(test_maxout_less);
  mu_run_test(test_maxout_equal);
  mu_run_test(test_maxout_great);

  mu_run_test(test_getitem);

  /* Basic shuffle/unshuffle tests with compression */
  mu_run_test(test_shuffle);
  mu_run_test(test_noshuffle);

  /* Roundtrip test with generic shuffle/unshuffle routines. */
  mu_run_test(test_shuffle_generic_then_unshuffle_generic);
  
#if defined(SHUFFLE_SSE2_ENABLED)
  /* One-way comparison between generic and SSE shuffles. */
  mu_run_test(test_shuffle_sse2_vs_shuffle_generic);
  
  /* Roundtrip tests between generic and SSE2 shuffle/unshuffle routines. */
  mu_run_test(test_shuffle_generic_then_unshuffle_sse2);
  mu_run_test(test_shuffle_sse2_then_unshuffle_generic);
  mu_run_test(test_shuffle_sse2_then_unshuffle_sse2);
#endif /* defined(SHUFFLE_SSE2_ENABLED) */

#if defined(SHUFFLE_AVX2_ENABLED)
  /* One-way comparison between generic and SSE shuffles. */
  mu_run_test(test_shuffle_sse2_vs_shuffle_generic);

  /* Roundtrip tests between generic and SSE2 shuffle/unshuffle routines. */
  mu_run_test(test_shuffle_generic_then_unshuffle_sse2);
  mu_run_test(test_shuffle_sse2_then_unshuffle_generic);
  mu_run_test(test_shuffle_sse2_then_unshuffle_sse2);
#endif /* defined(SHUFFLE_AVX2_ENABLED) */

  return 0;
}

int main(int argc, char **argv) {
  size_t i;
  int32_t *_src;
  char *result;

  printf("STARTING TESTS for %s", argv[0]);

  blosc_init();
  blosc_set_nthreads(1);

  /* Initialize buffers */
  src = malloc_cleaned(size);
  srccpy = malloc_cleaned(size);
  dest = malloc_cleaned(size + 16);
  dest2 = malloc_cleaned(size);
  _src = (int32_t *)src;
  for (i=0; i < (size/4); i++) {
    _src[i] = i;
  }
  memcpy(srccpy, src, size);

  /* Run all the suite */
  result = all_tests();
  if (result != 0) {
    printf(" (%s)\n", result);
  }
  else {
    printf(" ALL TESTS PASSED");
  }
  printf("\tTests run: %d\n", tests_run);

  free(src); free(srccpy); free(dest); free(dest2);
  blosc_destroy();

  return result != 0;
}
