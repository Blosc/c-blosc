/*
    Copyright (C) 2014  Francesc Alted
    http://blosc.org
    License: MIT (see LICENSE.txt)

    Example program demonstrating use of the Blosc filter from C code.

    To compile this program:

    gcc simple.c -o simple -lblosc -lpthread

    or, if you don't have the blosc library installed:

    gcc -O3 -msse2 simple.c ../blosc/*.c -I../blosc -o simple -lpthread

    Using MSVC on Windows:

    cl /Ox /Fesimple.exe /Iblosc simple.c blosc\*.c

    To run:

    $ ./simple
    Blosc version info: 1.4.2.dev ($Date:: 2014-07-08 #$)
    Compression: 4000000 -> 158494 (25.2x)
    Decompression succesful!
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
  int i;

  for(i=0; i<SIZE; i++){
    data[i] = i;
  }

  /* Register the filter with the library */
  printf("Blosc version info: %s (%s)\n",
	 BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);

  /* Compress with clevel=5, shuffle active, blosclz compressor, and single-threaded  */
  csize = blosc_compress_ctx(5, 1, sizeof(float), isize, data, data_out, osize,
            BLOSC_BLOSCLZ_COMPNAME, 0, 1);
  if (csize == 0) {
    printf("Buffer is uncompressible.  Giving up.\n");
    return 1;
  }
  else if (csize < 0) {
    printf("Compression error.  Error code: %d\n", csize);
    return csize;
  }

  printf("Compression: %d -> %d (%.1fx)\n", isize, csize, (1.*isize) / csize);

  /* Decompress  */
  dsize = blosc_decompress_ctx(data_out, data_dest, dsize, 1);
  if (dsize < 0) {
    printf("Decompression error.  Error code: %d\n", dsize);
    return dsize;
  }

  printf("Decompression succesful!\n");

  for(i=0;i<SIZE;i++){
    if(data[i] != data_dest[i]) {
      printf("Decompressed data differs from original!\n");
      return -1;
    }
  }

  printf("Succesful roundtrip!\n");
  return 0;
}
