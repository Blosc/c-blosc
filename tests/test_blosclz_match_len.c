/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Unit tests checking that a blosclz match whose length extension
  bytes would overflow an int32 is rejected on decompression.

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#include "test_common.h"

int tests_run = 0;

/* Global vars */
#define BLOCKSIZE 4096
#define NFF       (INT32_MAX / 255 + 1)  /* 0xff bytes to wrap the length */
#define SPLIT     (1 + 8 + 1 + NFF + 3)
#define CBYTES    (BLOSC_MAX_OVERHEAD + 4 + 4 + SPLIT)

uint8_t *src;
uint8_t dest[BLOCKSIZE];


static void write_le32(uint8_t *p, int32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}


/* Build a single blosclz block holding an 8 byte literal followed by a
   long match whose length extension is a run of NFF 0xff bytes. */
static void build_buffer(void) {
  uint8_t *p;

  src[0] = BLOSC_VERSION_FORMAT;
  src[1] = BLOSC_BLOSCLZ_VERSION_FORMAT;
  src[2] = 0x10;                        /* blosclz, no shuffle, dont_split */
  src[3] = 1;
  write_le32(src + 4, BLOCKSIZE);
  write_le32(src + 8, BLOCKSIZE);
  write_le32(src + 12, CBYTES);
  write_le32(src + BLOSC_MAX_OVERHEAD, BLOSC_MAX_OVERHEAD + 4);
  write_le32(src + BLOSC_MAX_OVERHEAD + 4, SPLIT);
  p = src + BLOSC_MAX_OVERHEAD + 8;
  *p++ = 7;                             /* 8 byte literal */
  memset(p, 'A', 8);
  p += 8;
  *p++ = 0xe0;                          /* match with extended length */
  memset(p, 0xff, NFF);
  p += NFF;
  *p++ = 0;                             /* last length byte */
  *p++ = 1;                             /* distance */
  *p++ = 0;                             /* next control byte */
}


static const char *test_decompress(void) {
  int r = blosc_decompress(src, dest, sizeof(dest));
  mu_assert("ERROR: blosc_decompress accepted an overflowing match length",
            r < 0);
  return 0;
}


static const char *test_getitem(void) {
  int r = blosc_getitem(src, 0, BLOCKSIZE, dest);
  mu_assert("ERROR: blosc_getitem accepted an overflowing match length",
            r < 0);
  return 0;
}


static const char *all_tests(void) {
  mu_run_test(test_decompress);
  mu_run_test(test_getitem);

  return 0;
}


int main(int argc, char **argv) {
  const char *result;

  printf("STARTING TESTS for %s", argv[0]);

  blosc_init();
  blosc_set_nthreads(1);

  src = (uint8_t *)calloc(1, CBYTES);
  build_buffer();

  /* Run all the suite */
  result = all_tests();
  if (result != 0) {
    printf(" (%s)\n", result);
  }
  else {
    printf(" ALL TESTS PASSED");
  }
  printf("\tTests run: %d\n", tests_run);

  free(src);
  blosc_destroy();

  return result != 0;
}
