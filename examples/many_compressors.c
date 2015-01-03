/*
    Copyright (C) 2014  Francesc Alted
    http://blosc.org
    License: MIT (see LICENSE.txt)

    Example program demonstrating use of the Blosc filter from C code.

    To compile this program:

    gcc many_compressors.c -o many_compressors -lblosc -lpthread
    
    or, if you don't have the blosc library installed:

    gcc -O3 -msse2 many_compressors.c ../blosc/*.c -I../blosc \
        -o many_compressors -lpthread \
        -DHAVE_ZLIB -lz -DHAVE_LZ4 -llz4 -DHAVE_SNAPPY -lsnappy

    Using MSVC on Windows:

    cl /Ox /Femany_compressors.exe /Iblosc many_compressors.c blosc\*.c
    
    To run:

    ./many_compressors
    Blosc version info: 1.4.2.dev ($Date:: 2014-07-08 #$)
    Using 4 threads (previously using 1)
    Using blosclz compressor
    Compression: 4000000 -> 158494 (25.2x)
    Succesful roundtrip!
    Using lz4 compressor
    Compression: 4000000 -> 234238 (17.1x)
    Succesful roundtrip!
    Using lz4hc compressor
    Compression: 4000000 -> 38314 (104.4x)
    Succesful roundtrip!
    Using snappy compressor
    Compression: 4000000 -> 311617 (12.8x)
    Succesful roundtrip!
    Using zlib compressor
    Compression: 4000000 -> 22103 (181.0x)
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
  static const int nthreads = 4;    /* Number of threads to use */
  static const int complevel = 5;   /* Compression level */
  int i;
  const char* const compressors[] = {"blosclz", "lz4", "lz4hc", "snappy", "zlib"};
  const int ncompressors = sizeof(compressors) / sizeof(char*);
  
  int ccode, rcode;

  for(i=0; i<SIZE; i++){
    data[i] = i;
  }

  /* Register the filter with the library */
  printf("Blosc version info: %s (%s)\n",
	 BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);

  printf("Using %d threads\n", nthreads);

  /* Tell Blosc to use some number of threads */
  for (ccode=0; ccode < ncompressors; ccode++) {

    rcode = blosc_compname_to_compcode(compressors[ccode]);
    if (rcode < 0) {
      printf("Error setting %s compressor.  It really exists?",
	     compressors[ccode]);
      return rcode;
    }
    printf("Using %s compressor\n", compressors[ccode]);

    /* Compress with clevel=5 and shuffle active  */
    csize = blosc_compress_ctx(5, 1, sizeof(float), isize, data, data_out, osize,
      compressors[ccode], 0, nthreads);
    if (csize < 0) {
      printf("Compression error.  Error code: %d\n", csize);
      return csize;
    }

    printf("Compression: %d -> %d (%.1fx)\n", isize, csize, (1.*isize) / csize);

    /* Decompress  */
    dsize = blosc_decompress_ctx(data_out, data_dest, dsize, nthreads);
    if (dsize < 0) {
        printf("Decompression error.  Error code: %d\n", dsize);
        return dsize;
    }

    for(i=0;i<SIZE;i++){
      if(data[i] != data_dest[i]) {
	      printf("Decompressed data differs from original!\n");
	      return -1;
      }
    }

    printf("Succesful roundtrip!\n");
  }

  return 0;
}
