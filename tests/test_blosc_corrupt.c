/*
    Copyright (C) 2016  Francesc Alted
    http://blosc.org
    License: MIT (see LICENSE.txt)

    Example program showing a roundtrip error.

    To compile this program:

    $ gcc blosc_error.c -o blosc_erro -lblosc


*/

#include <stdio.h>
#include <blosc.h>

int main(){
  FILE *fp;
  fp = fopen("/tmp/blosc_corrupt.data", "r");
  fseek(fp, 0L, SEEK_END);
  int size = (int)ftell(fp);
  rewind(fp);

  int64_t *data = malloc(size);
  int64_t *data_out = malloc(size + BLOSC_MIN_HEADER_LENGTH);
  int64_t *data_dest = malloc(size);

  fread(data, size, 1, fp);

  printf("Blosc version info: %s (%s)\n", BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);

  /* From 1.9 on, we don't need to initialize the Blosc compressor anymore */
  /* blosc_init(); */

  /* Compress with clevel=5 and shuffle active  */
  int isize = size;
  int osize = size * BLOSC_MIN_HEADER_LENGTH;
  int csize = blosc_compress(5, 1, sizeof(float), isize, data, data_out, osize);
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
  int dsize = blosc_decompress(data_out, data_dest, isize);
  if (dsize < 0) {
    printf("Decompression error.  Error code: %d\n", dsize);
    return dsize;
  }

  printf("Decompression succesful!\n");

  for(int i = 0; i < size; i++){
    if(data[i] != data_dest[i]) {
      printf("Decompressed data differs from original!\n");
      return -1;
    }
  }

  printf("Succesful roundtrip!\n");
  return 0;
}
