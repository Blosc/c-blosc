/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Unit tests for basic features in Blosc.

  Creation date: 2010-06-07
  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include "test_common.h"

int tests_run = 0;

/* Global vars */
void *src, *srccpy, *dest, *dest2;
size_t nbytes, cbytes;
int clevel = 1;
int doshuffle = 0;
size_t typesize = 4;
size_t size = 1000;             /* must be divisible by 4 */


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


static char *all_tests() {
  mu_run_test(test_maxout_less);
  mu_run_test(test_maxout_equal);
  mu_run_test(test_maxout_great);
  return 0;
}

int main(int argc, char **argv) {
  int i;
  int32_t *_src;

  printf("STARTING TESTS for %s", argv[0]);

  blosc_set_nthreads(1);

  /* Initialize buffers */
  src = malloc(size);
  srccpy = malloc(size);
  dest = malloc(size+16);
  dest2 = malloc(size);
  _src = (int32_t *)src;
  for (i=0; i < (size/4); i++) {
    _src[i] = i;
  }
  memcpy(srccpy, src, size);

  /* Run all the suite */
  char *result = all_tests();
  if (result != 0) {
    printf(" (%s)\n", result);
  }
  else {
    printf(" ALL TESTS PASSED");
  }
  printf("\tTests run: %d\n", tests_run);

  free(src); free(srccpy); free(dest); free(dest2);
  return result != 0;
}
