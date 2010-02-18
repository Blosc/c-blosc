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

#define MB    (1024*1024)

#define NITER  (20*1000)               /* Number of iterations */
#define CLK_NITER  (CLOCKS_PER_SEC*NITER/1e6)



int main(void) {
  int nbytes, cbytes;
  void *src, *dest, *srccpy;
  size_t i;
  clock_t last, current;
  float tmemcpy, tshuf, tunshuf;
  int *_src;
  int *_srccpy;
  int rshift = 22;           /* For random data */
  int clevel;
  int doshuffle = 1;
  int fd;
  int rbytes;
  char *filename = "128KB-block-4B-typesize.data";
  int size = 128*1024;                       /* Buffer size */
  unsigned int elsize = 4;                   /* Datatype size */

  src = malloc(size);
  srccpy = malloc(size);
  dest = malloc(size);

  srand(1);

  /* Initialize the original buffer */
  /* _src = (int *)src; */
  /* _srccpy = (int *)srccpy; */
  /* elsize = sizeof(int); */
  /* for(i = 0; i < size/elsize; ++i) { */
  /*   /\* Choose one below *\/ */
  /*   /\* _src[i] = 1; *\/ */
  /*   /\* _src[i] = 0x01010101; *\/ */
  /*   /\* _src[i] = 0x01020304; *\/ */
  /*   /\* _src[i] = i * 1/.3; *\/ */
  /*   /\* _src[i] = i; *\/ */
  /*   _src[i] = rand() >> rshift; */
  /* } */

  /* For data coming from a file */
  fd = open(filename, 0);
  rbytes = read(fd, src, size);
  size = rbytes;
  if (rbytes == -1) {
    perror(NULL);
  }
  close(fd);

  printf("********************** Setup info *****************************\n");
  printf("Blosc version: %s (%s)\n", BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);
/*  printf("Using random data with %d significant bits (out of 32)\n", 32-rshift); */
  printf("Using data coming from file: %s\n", filename);
  printf("Dataset size: %d bytes\t Type size: %d bytes\n", size, elsize);
  printf("Shuffle active?  %s\n", doshuffle ? "Yes" : "No");
  printf("********************** Running benchmarks *********************\n");

  memcpy(srccpy, src, size);

  last = clock();
  for (i = 0; i < NITER; i++) {
    memcpy(dest, src, size);
  }
  current = clock();
  tmemcpy = (current-last)/CLK_NITER;
  printf("memcpy:\t\t %6.1f us, %.1f MB/s\n", tmemcpy, size/(tmemcpy*MB/1e6));

  for (clevel=1; clevel<10; clevel++) {

    printf("Compression level: %d\n", clevel);

    last = clock();
    for (i = 0; i < NITER; i++) {
      cbytes = blosc_compress(clevel, doshuffle, elsize, size, src, dest);
    }
    current = clock();
    tshuf = (current-last)/CLK_NITER;
    printf("compression:\t %6.1f us, %.1f MB/s\t  ",
           tshuf, size/(tshuf*MB/1e6));
    printf("Final bytes: %d  Compr ratio: %3.2f\n",
           cbytes, size/(float)cbytes);

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
    tunshuf = (current-last)/CLK_NITER;
    printf("decompression:\t %6.1f us, %.1f MB/s\t  ",
           tunshuf, nbytes/(tunshuf*MB/1e6));
    if (nbytes < 0) {
      printf("FAIL.  Error code: %d\n", nbytes);
    }
    /* printf("Orig bytes: %d\tFinal bytes: %d\n", cbytes, nbytes); */

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

    printf("OK\n");

  } /* End clevel loop */

 out:
  free(src); free(srccpy);  free(dest);
  return 0;
}
