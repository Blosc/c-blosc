/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Unit tests for Blosc API.

  Creation date: 2010-06-07
  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_WIN32) && !defined(__MINGW32__)
  #include <time.h>
#else
  #include <unistd.h>
  #include <sys/time.h>
#endif
#include <math.h>
#include "../src/blosc.h"


/* This is MinUnit in action (http://www.jera.com/techinfo/jtns/jtn002.html) */
#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do \
    { char *message = test(); tests_run++;                          \
      if (message) { printf("%c", 'F'); return message;}            \
      else printf("%c", '.'); } while (0)

int tests_run = 0;

#define KB  1024
#define MB  (1024*KB)
#define GB  (1024*MB)

/* Global vars */
void *src, *srccpy, *dest, *dest2;
size_t nbytes, cbytes;
int clevel = 3;
int doshuffle = 1;
size_t typesize = 4;
size_t size = 1*MB;



static char *test_cbuffer_sizes() {
  size_t nbytes_, cbytes_;

  blosc_cbuffer_sizes(dest, &nbytes_, &cbytes_);
  mu_assert("ERROR: nbytes incorrect(1)", nbytes == size);
  mu_assert("ERROR: nbytes incorrect(2)", nbytes_ == nbytes);
  mu_assert("ERROR: cbytes incorrect", cbytes == cbytes_);
  return 0;
}

static char *test_cbuffer_metainfo() {
  size_t typesize_;
  int doshuffle_;

  blosc_cbuffer_metainfo(dest, &typesize_, &doshuffle_);
  mu_assert("ERROR: typesize incorrect", typesize_ == typesize);
  mu_assert("ERROR: shuffle incorrect", doshuffle_ == doshuffle);
  return 0;
}


static char *test_cbuffer_versions() {
  int version_;
  int versionlz_;

  blosc_cbuffer_versions(dest, &version_, &versionlz_);
  mu_assert("ERROR: version incorrect", version_ == BLOSC_VERSION_FORMAT);
  mu_assert("ERROR: versionlz incorrect", versionlz_ == BLOSCLZ_VERSION_FORMAT);
  return 0;
}


static char *all_tests() {
  mu_run_test(test_cbuffer_sizes);
  mu_run_test(test_cbuffer_metainfo);
  mu_run_test(test_cbuffer_versions);
  return 0;
}

int main(int argc, char **argv) {

  blosc_set_nthreads(1);

  /* Initialize buffers */
  src = malloc(size);
  srccpy = malloc(size);
  dest = malloc(size);
  dest2 = malloc(size);
  memset(src, 0, size);
  memcpy(srccpy, src, size);

  /* Get a compressed buffer */
  cbytes = blosc_compress(clevel, doshuffle, typesize, size, src, dest);

  /* Get a decompressed buffer */
  nbytes = blosc_decompress(dest, dest2, size);

  /* Run all the suite */
  char *result = all_tests();
  if (result != 0) {
    printf(" (%s)\n", result);
  }
  else {
    printf("\nALL TESTS PASSED\n");
  }
  printf("\nTests run: %d\n", tests_run);

  return result != 0;
}

