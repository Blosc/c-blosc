/*
    Copyright (C) 2014  Francesc Alted
    http://blosc.org
    License: MIT (see LICENSE.txt)

    Example program demonstrating use of the Blosc filter from C code.

    To compile this program using gcc or clang:

    gcc/clang multithread.c -o multithread -lblosc -lpthread

    or, if you don't have the blosc library installed:

    gcc -O3 -msse2 multithread.c ../blosc/*.c  -I../blosc -o multithread -lpthread

    Using MSVC on Windows:

    cl /Ox /Femultithread.exe /Iblosc multithread.c blosc\*.c
    
    To run:

    $ ./multithread
    Blosc version info: 1.4.2.dev ($Date:: 2014-07-08 #$)
    Using 1 threads (previously using 1)
    Compression: 4000000 -> 158494 (25.2x)
    Succesful roundtrip!
    Using 2 threads (previously using 1)
    Compression: 4000000 -> 158494 (25.2x)
    Succesful roundtrip!
    Using 3 threads (previously using 2)
    Compression: 4000000 -> 158494 (25.2x)
    Succesful roundtrip!
    Using 4 threads (previously using 3)
    Compression: 4000000 -> 158494 (25.2x)
    Succesful roundtrip!

*/

#include <stdio.h>
#include <blosc.h>

#define SIZE 100*100*100
#define SHAPE {100,100,100}
#define CHUNKSHAPE {1,100,100}

int main(){
  static float data[SIZE];
  static float data_out[SIZE];
  static float data_dest[SIZE];
  const size_t isize = SIZE*sizeof(float), osize = SIZE*sizeof(float);
  const size_t dsize = SIZE*sizeof(float), csize;
  static const int clevel = 5;      /* Compression level */
  static const int maxthreads = 4;  /* Maximum number of threads to use */
  int nthreads;
  int i;

  for(i=0; i<SIZE; i++){
    data[i] = i;
  }

  /* Register the filter with the library */
  printf("Blosc version info: %s (%s)\n",
	 BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);

  /* Tell Blosc to use some number of threads */
  for (nthreads=1; nthreads <= maxthreads; nthreads++) {
    printf("Using %d threads\n", nthreads);

    /* Compress with specified clevel and shuffle active  */
    csize = blosc_compress_ctx(clevel, 1, sizeof(float), isize, data, data_out, osize,
              BLOSC_BLOSCLZ_COMPNAME, 0, nthreads);
    if (csize < 0) {
      fprintf(stderr, "Compression error.  Error code: %d\n", csize);
      return csize;
    }

    printf("Compression: %d -> %d (%.1fx)\n", isize, csize, (1.*isize) / csize);

    /* Decompress  */
    dsize = blosc_decompress_ctx(data_out, data_dest, dsize, nthreads);
    if (dsize < 0) {
        fprintf(stderr, "Decompression error.  Error code: %d\n", dsize);
        return dsize;
    }

    for(i=0;i<SIZE;i++){
      if(data[i] != data_dest[i]) {
        fprintf(stderr, "Decompressed data differs from original!\n");
        return -1;
      }
    }

    printf("Succesful roundtrip!\n");
  }

  return 0;
}
