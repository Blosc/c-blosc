/*********************************************************************
  Small benchmark for testing basic capabilities of Blosc.

  You can select different degrees of 'randomness' in input buffer, as
  well as external datafiles (uncomment the lines after "For data
  coming from a file" comment).

  To compile using GCC:

    gcc -O3 -msse2 -o bench bench.c blosc.c blosclz.c shuffle.c

  I'm collecting speeds for different machines, so the output of your
  benchmarks and your processor specifications are welcome!

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "blosc.h"


/* Constants for benchmarking purposes */
#define MB    1024*1024
#define NITER 4*4000            /* Number of iterations */
#define CLK_NITER CLOCKS_PER_SEC*NITER

int main(void) {
  unsigned int nbytes, cbytes;
  void *src, *dest, *srccpy;
  size_t i;
  clock_t last, current;
  float tmemcpy, tshuf, tunshuf;
  int size = 32*1024;         /* Buffer size */
  unsigned int elsize;                 /* Datatype size */
  int *_src;
  int *_srccpy;

  src = malloc(size+6);
  srccpy = malloc(size+6);
  dest = malloc(size+6);

  srand(1);

  /* Initialize the original buffer */
  _src = (int *)src;
  _srccpy = (int *)srccpy;
  elsize = sizeof(int);
  for(i = 0; i < size/elsize; ++i) {
    /* Choose one below */
    /* _src[i] = 1; */
    /* _src[i] = 0x01010101; */
    /* _src[i] = 0x01020304; */
    /* _src[i] = i * 1/.3; */
    /* _src[i] = i; */
    _src[i] = rand() >> 22;
  }

  /* For data coming from a file */
  /* int fd; */
  /* ssize_t rbytes; */
  /* elsize = 8; */
  /* fd = open("16KB-block-8B-typesize.data", 0); */
  /* off_t seek = lseek(fd, 0, SEEK_SET); */
  /* rbytes = read(fd, src, size); */
  /* /\* rbytes = read(fd, src, 8192); *\/ */
  /* size = rbytes; */
  /* printf("Read %zd bytes with %zd seek\n", rbytes, seek); */
  /* if (rbytes == -1) { */
  /*   perror(NULL); */
  /* } */
  /* close(fd); */

  set_complevel(1);
  set_shuffle(1);

  memcpy(srccpy, src, size);

  last = clock();
  for (i = 0; i < NITER; i++) {
    memcpy(dest, src, size);
  }
  current = clock();
  tmemcpy = (current-last)/((float)CLK_NITER);
  printf("memcpy:\t\t %fs, %.1f MB/s\n", tmemcpy, size/(tmemcpy*MB));

  last = clock();
  for (i = 0; i < NITER; i++) {
    cbytes = blosc_compress(elsize, size, src, dest);
  }
  current = clock();
  tshuf = (current-last)/((float)CLK_NITER);
  printf("blosc_compress:\t %fs, %.1f MB/s\t", tshuf, size/(tshuf*MB));
  printf("Orig bytes: %d  Final bytes: %d\n", size, cbytes);

  last = clock();
  for (i = 0; i < NITER; i++)
    if (cbytes == 0) {
      memcpy(dest, src, size);
      nbytes = size;
    }
    else {
      nbytes = blosc_decompress(dest, src, size);
    }
  current = clock();
  tunshuf = (current-last)/((float)CLK_NITER);
  printf("blosc_d:\t %fs, %.1f MB/s\t", tunshuf, nbytes/(tunshuf*MB));
  printf("Orig bytes: %d  Final bytes: %d\n", cbytes, nbytes);

  /* Check if data has had a good roundtrip */
  _src = (int *)src;
  _srccpy = (int *)srccpy;
  for(i = 0; i < size/sizeof(int); ++i){
    if (_src[i] != _srccpy[i]) {
      printf("Error: original data and round-trip do not match in pos %d\n",
             (int)i);
      printf("Orig--> %x, Copy--> %x\n", _src[i], _srccpy[i]);
      goto out;
    }
  }

 out:
  free(src); free(srccpy);  free(dest);
  return 0;
}
