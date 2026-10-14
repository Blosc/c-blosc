/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Unit tests checking that blosc_getitem() rejects out-of-range
  `start`/`nitems` arguments instead of reading or writing out of
  bounds.

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#include "test_common.h"

int tests_run = 0;

/* Global vars */
void *original;                 /* uncompressed reference data */
void *src;                      /* compressed buffer */
void *dest;                     /* output buffer */
size_t typesize = 8;
size_t nelem = 1024;
size_t size;                    /* typesize * nelem */


/* A negative `nitems` must be rejected: it makes the per-block copy
   length negative, which wraps when passed to the byte copy as size_t. */
static const char *test_negative_nitems(void) {
  int r = blosc_getitem(src, 10, -5, dest);
  mu_assert("ERROR: negative nitems not rejected", r < 0);
  return 0;
}


/* A `start` far past the end must be rejected; the range check must not
   overflow while computing start * typesize. */
static const char *test_start_out_of_range(void) {
  int r = blosc_getitem(src, 20000000, 1, dest);
  mu_assert("ERROR: out-of-range start not rejected", r < 0);
  return 0;
}


/* start + nitems past the end must be rejected. */
static const char *test_stop_out_of_range(void) {
  int r = blosc_getitem(src, (int)nelem - 1, 5, dest);
  mu_assert("ERROR: out-of-range stop not rejected", r < 0);
  return 0;
}


/* A valid in-range request must still succeed and round-trip. */
static const char *test_valid_request(void) {
  int start = 100, nitems = 50;
  int r = blosc_getitem(src, start, nitems, dest);
  mu_assert("ERROR: valid getitem failed", r == nitems * (int)typesize);
  mu_assert("ERROR: valid getitem data mismatch",
            memcmp(dest, (uint8_t *)original + (size_t)start * typesize,
                   (size_t)nitems * typesize) == 0);
  return 0;
}


static const char *all_tests(void) {
  mu_run_test(test_negative_nitems);
  mu_run_test(test_start_out_of_range);
  mu_run_test(test_stop_out_of_range);
  mu_run_test(test_valid_request);
  return 0;
}

#define BUFFER_ALIGN_SIZE   32

int main(int argc, char **argv) {
  const char *result;
  size_t i;
  uint8_t *o;

  printf("STARTING TESTS for %s", argv[0]);

  blosc_init();
  blosc_set_nthreads(1);

  size = typesize * nelem;
  original = blosc_test_malloc(BUFFER_ALIGN_SIZE, size);
  dest = blosc_test_malloc(BUFFER_ALIGN_SIZE, size);
  src = blosc_test_malloc(BUFFER_ALIGN_SIZE, size + BLOSC_MAX_OVERHEAD);

  o = (uint8_t *)original;
  for (i = 0; i < size; i++) {
    o[i] = (uint8_t)(i * 7 + 3);
  }

  /* Compress into `src` so blosc_getitem has a valid buffer to read. */
  blosc_compress(5, 1, typesize, size, original, src,
                 size + BLOSC_MAX_OVERHEAD);

  result = all_tests();
  if (result != 0) {
    printf(" (%s)\n", result);
  }
  else {
    printf(" ALL TESTS PASSED");
  }
  printf("\tTests run: %d\n", tests_run);

  blosc_test_free(original);
  blosc_test_free(dest);
  blosc_test_free(src);

  blosc_destroy();

  return result != 0;
}
