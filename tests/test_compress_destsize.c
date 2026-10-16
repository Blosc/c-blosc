/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Unit tests checking that blosc_compress() keeps the memory-safety
  guarantee (never write past `destsize`) when `destsize` is large
  enough for the header but too small for the per-block pointer array.

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#include "test_common.h"

int tests_run = 0;

/* Global vars */
void *src, *dest;
size_t typesize = 4;
size_t size = 4000;             /* must be divisible by typesize */


/* A `destsize` that only fits the 16-byte header but not the bstarts
   array must be rejected.  Several small blocks make the bstarts array
   span many bytes, so the old code wrote them past `destsize`.  `dest`
   is sized to exactly `destsize` so the stray write lands out of the
   allocation and is caught under AddressSanitizer. */
static const char *test_destsize_header_only(void) {
  size_t destsize = BLOSC_MAX_OVERHEAD + 4;
  void *tight = malloc(destsize);
  int cbytes;
  blosc_set_blocksize(256);    /* force many blocks -> large bstarts */
  cbytes = blosc_compress(5, BLOSC_SHUFFLE, typesize, size, src, tight,
                          destsize);
  mu_assert("ERROR: oversized output not rejected", cbytes == 0);
  blosc_set_blocksize(0);
  free(tight);
  return 0;
}


/* Same check on the threaded path. */
static const char *test_destsize_header_only_threaded(void) {
  size_t destsize = BLOSC_MAX_OVERHEAD + 4;
  void *tight = malloc(destsize);
  int cbytes;
  blosc_set_nthreads(4);
  blosc_set_blocksize(256);
  cbytes = blosc_compress(5, BLOSC_SHUFFLE, typesize, size, src, tight,
                          destsize);
  mu_assert("ERROR: oversized output not rejected (threaded)", cbytes == 0);
  blosc_set_blocksize(0);
  blosc_set_nthreads(1);
  free(tight);
  return 0;
}


/* A `destsize` of (size + BLOSC_MAX_OVERHEAD) must still succeed. */
static const char *test_destsize_enough(void) {
  int cbytes, nbytes;
  void *dest2 = malloc(size);
  cbytes = blosc_compress(5, BLOSC_SHUFFLE, typesize, size, src, dest,
                          size + BLOSC_MAX_OVERHEAD);
  mu_assert("ERROR: compression failed with enough room", cbytes > 0);
  nbytes = blosc_decompress(dest, dest2, size);
  mu_assert("ERROR: roundtrip size mismatch", nbytes == (int)size);
  mu_assert("ERROR: roundtrip data mismatch", memcmp(src, dest2, size) == 0);
  free(dest2);
  return 0;
}


static const char *all_tests(void) {
  mu_run_test(test_destsize_header_only);
  mu_run_test(test_destsize_header_only_threaded);
  mu_run_test(test_destsize_enough);
  return 0;
}

int main(int argc, char **argv) {
  const char *result;
  size_t i;
  uint8_t *_src;

  printf("STARTING TESTS for %s", argv[0]);

  blosc_init();
  blosc_set_nthreads(1);

  src = malloc(size);
  dest = malloc(size + BLOSC_MAX_OVERHEAD);
  _src = (uint8_t *)src;
  for (i = 0; i < size; i++) {
    _src[i] = (uint8_t)(i * 7);
  }

  result = all_tests();
  if (result != 0) {
    printf(" (%s)\n", result);
  }
  else {
    printf(" ALL TESTS PASSED");
  }
  printf("\tTests run: %d\n", tests_run);

  free(src);
  free(dest);

  blosc_destroy();

  return result != 0;
}
