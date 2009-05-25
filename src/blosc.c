/*
  blosc - Blocked Suffling and Compression Library

  See LICENSE.txt for details about copyright and rights to use.
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "blosclz.h"
#include "shuffle.h"
#include <emmintrin.h>

#define BLOSC_VERSION 1    //  Should be 1-byte long

#define MB 1024*1024

// Block sizes for optimal use of the first-level cache
//#define BLOCKSIZE (2*1024)  /* 2K */
//#define BLOCKSIZE (4*1024)  /* 4K */  /* Page size.  Optimal for P4. */
#define BLOCKSIZE (8*1024)  /* 8K */  /* Seems optimal for Core2 and P4. */
//#define BLOCKSIZE (16*1024) /* 16K */  /* Seems optimal for Core2. */
//#define BLOCKSIZE (32*1024) /* 32K */

// Datatype size
#define ELSIZE 4

// Chunksize (for benchmark purposes)
//#define SIZE 8*1024  // 8 KB
//#define SIZE 16*1024  // 16 KB
//#define SIZE 32*1024  // 32 KB
//#define SIZE 64*1024  // 64 KB
#define SIZE 128*1024  // 128 KB
//#define SIZE 256*1024  // 256 KB
//#define SIZE 512*1024  // 512 KB
//#define SIZE 1024*1024  // 1024 KB
//#define SIZE 16*1024*1024  // 16 MB

// Number of iterations
#define NITER 4*4000
//#define NITER 1
#define NITER1 1

#define CLK_NITER CLOCKS_PER_SEC*NITER


// Macro to type-cast an array address to a vector address:
#define ToVectorAddress(x) ((__m128i*)&(x))


// Shuffle & Compress a single block
static size_t
_blosc_c(size_t typesize, size_t blocksize,
         unsigned char* _src, unsigned char* _dest, unsigned char *tmp) {
  size_t j;
  size_t neblock = blocksize / typesize;
  unsigned int cbytes, ctbytes = 0;

  // Shuffle this block
  shuffle(typesize, blocksize, _src, tmp);

  // Compress each shuffled byte for this block
  for (j = 0; j < typesize; j++) {
    _dest += sizeof(int);
    cbytes = blosclz_compress(tmp+j*neblock, neblock, _dest);
    if (cbytes == -1) {
      // The compressor has been unable to compress data significantly
      memcpy(_dest, tmp+j*neblock, neblock);
      cbytes = neblock;
    }
    ((unsigned int *)(_dest))[-1] = cbytes;
    _dest += cbytes;
    ctbytes += cbytes + sizeof(int);
  }  // Close j < typesize

  return ctbytes;
}


unsigned int
blosc_compress(size_t typesize, size_t nbytes, void *src, void *dest)
{
  unsigned char *_src=NULL;   // Alias for source buffer
  unsigned char *_dest=NULL;  // Alias for destination buffer
  unsigned char *flags;       // Flags for header
  size_t nblocks;             // Number of complete blocks in buffer
  size_t neblock;             // Number of elements in block
  size_t j;                   // Local index variables
  size_t leftover;            // Extra bytes at end of buffer
  size_t eqval;
  unsigned int cbytes, ctbytes;
  __m128i value, value2, cmpeq, andreg;
  const char ones[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  const char cmpresult[16];
  // Temporary buffer for data block
  unsigned char tmp[BLOCKSIZE] __attribute__((aligned(64)));

  nblocks = nbytes / BLOCKSIZE;
  neblock = BLOCKSIZE / typesize;
  leftover = nbytes % BLOCKSIZE;
  _src = (unsigned char *)(src);
  _dest = (unsigned char *)(dest);

  // Write header for this block
  *_dest++ = BLOSC_VERSION;                   // The blosc version
  flags = _dest++;                            // Flags (to be filled later on)
  ctbytes = 2;
  ((unsigned int *)(_dest))[0] = nbytes;      // The size of the chunk
  _dest += sizeof(int);
  ctbytes += sizeof(int);

  // First, look for a trivial repetition pattern
  // Note that all the loads and stores have to be unaligned as we cannot
  // guarantee that the source data is aligned to 16-bytes.
  value = _mm_loadu_si128(ToVectorAddress(_src[0]));
  // Initially all values are equal indeed :)
  andreg = _mm_loadu_si128(ToVectorAddress(ones));
  for (j = 16; j < nbytes; j += 16) {
    value2 = _mm_loadu_si128(ToVectorAddress(_src[j]));
    // Compare with value vector byte-to-byte
    cmpeq = _mm_cmpeq_epi8(value, value2);
    // Do an and with the previous comparison
    andreg = _mm_and_si128(andreg, cmpeq);
  }
  // Store the cummulative 'and' register in memory
  _mm_storeu_si128(ToVectorAddress(cmpresult), andreg);
  // Are all values equal?
  eqval = strncmp(cmpresult, ones, 16);
  if (eqval == 0) {
    // Trivial repetition pattern found
    *flags = 1;           // bit 0 set to one, all the rest to 0
    *_dest++ = 16;        // 16 repeating byte
    _mm_storeu_si128(ToVectorAddress(_dest[0]), value);    // The repeated bytes
    ctbytes += 1 + 16;
    return ctbytes;
  }

  // Start the shuffle way
  *flags = 2;           // bit 1 set to one, all the rest to 0
  // First, write the shuffle header
  ((unsigned int *)_dest)[0] = typesize;          // The type size
  ((unsigned int *)_dest)[1] = BLOCKSIZE;         // The block size
  _dest += 8;
  ctbytes += 8;
  for (j = 0; j < nblocks; j++) {
    cbytes = _blosc_c(typesize, BLOCKSIZE, _src, _dest, tmp);
    _dest += cbytes;
    _src += BLOCKSIZE;
    ctbytes += cbytes;
    // TODO:  Perform a better check so as to avoid a dest buffer overrun
    if (ctbytes > nbytes) {
      return 0;    // Uncompressible data
    }
  }  // Close k < nblocks

  if(leftover > 0) {
    memcpy((void*)tmp, (void*)_src, leftover);
    _dest += sizeof(int);
    cbytes = blosclz_compress(tmp, leftover, _dest);
    ((unsigned int *)(_dest))[-1] = cbytes;
    _dest += cbytes;
    ctbytes += cbytes + sizeof(int);
  }

  return ctbytes;
}


// Decompress & unshuffle a single block
static size_t
_blosc_d(size_t typesize, size_t blocksize,
         unsigned char* _src, unsigned char* _dest, unsigned char *tmp)
{
  size_t j;
  size_t nbytes, cbytes, ctbytes = 0;
  size_t neblock = blocksize / typesize;        // Number of elements on a block
  unsigned char* _tmp;

  _tmp = tmp;
  for (j = 0; j < typesize; j++) {
    cbytes = ((unsigned int *)(_src))[0];       // The number of compressed bytes
    _src += sizeof(int);
    /* uncompress */
    if (cbytes == 1) {
      memset(_tmp, *_src, neblock);
    }
    else if (cbytes == neblock) {
      memcpy(_tmp, _src, neblock);
    }
    else {
      nbytes = blosclz_decompress(_src, cbytes, _tmp, neblock);
      assert (nbytes == neblock);
    }
    _src += cbytes;
    ctbytes += cbytes + sizeof(int);
    _tmp += neblock;
  }

  unshuffle(typesize, blocksize, tmp, _dest);
  return ctbytes;
}


unsigned int
blosc_decompress(void *src, void *dest, size_t dest_size)
{
  unsigned char *_src=NULL;         // Alias for source buffer
  unsigned char *_dest=NULL;        // Alias for destination buffer
  unsigned char version, flags;     // Info in compressed header
  size_t leftover;                  // Extra bytes at end of buffer
  size_t nblocks;                   // Number of complete blocks in buffer
  size_t j;
  size_t nbytes, dbytes, cbytes, ntbytes = 0;
  unsigned char *tmp;

  _src = (unsigned char *)(src);
  _dest = (unsigned char *)(dest);

  // Read the header block
  version = _src[0];                        // The blosc version
  flags = _src[1];                          // The flags for this block
  _src += 2;
  nbytes = ((unsigned int *)_src)[0];       // The size of the chunk
  // TODO: Devise a schema for meaningful return codes
  if (nbytes > dest_size) {
    return -1;
  }
  _src += sizeof(int);

  // Check for the trivial repeat pattern
  if (flags == 1) {
    unsigned char rep = _src[0];            // The number of repeated bytes
    __m128i xmm0;
    if (rep == 1) {
      // Copy values in blocks of 16 bytes
      xmm0 = _mm_set1_epi8(_src[1]);        // The repeated value
    }
    else {                                  // rep == 16
      xmm0 = _mm_loadu_si128(ToVectorAddress(_src[1]));
    }

    // Copy value into destination
    for (j = 0; j < nbytes/16; j++) {
      ((__m128i *)dest)[j] = xmm0;
    }

    if (rep == 1) {
      // Copy the remainding values
      _dest = (unsigned char *)dest + j*16;
      for (j = 0; j < nbytes%16; j++) {
        _dest[j] = _src[1];                 // Copy the repeated value
      }
    }
    return nbytes;
  }

  // Shuffle way
  // Read header info
  unsigned int typesize = ((unsigned int *)_src)[0];
  unsigned int blocksize = ((unsigned int *)_src)[1];
  _src += 8;
  // Compute some params
  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;

  // Create temporary area
  posix_memalign((void **)&tmp, 64, blocksize);

  for (j = 0; j < nblocks; j++) {
    cbytes = _blosc_d(typesize, blocksize, _src, _dest, tmp);
    _src += cbytes;
    _dest += blocksize;
    ntbytes += blocksize;
  }

  if(leftover > 0) {
    cbytes = ((unsigned int *)(_src))[0];
    _src += sizeof(int);
    dbytes = blosclz_decompress(_src, cbytes, tmp, leftover);
    ntbytes += dbytes;
    memcpy((void*)_dest, (void*)tmp, leftover);
  }

  free(tmp);
  return ntbytes;
}


int main() {
    unsigned int nbytes, cbytes;
    void *src, *dest, *srccpy;
    //unsigned char src[SIZE], dest[SIZE], srccpy[SIZE] __attribute__((aligned(64)));
    size_t i;
    clock_t last, current;
    float tmemcpy, tshuf, tunshuf;

    src = malloc(SIZE);  srccpy = malloc(SIZE);
    //posix_memalign((void **)&src, 64, SIZE);
    //posix_memalign((void **)&srccpy, 64, SIZE);
    posix_memalign((void **)&dest, 64, SIZE);   // Must be aligned to 16 bytes at least!

    srand(1);

    // Initialize the original buffer
    int* _src = (int *)src;
    int* _srccpy = (int *)srccpy;
    //float* _src = (float *)src;
    //float* _srccpy = (float *)srccpy;
    //for(long long l = 0; l < SIZE/sizeof(long long); ++l){
      //((long long *)_src)[l] = l;
      //((long long *)_src)[l] = rand() >> 24;
      //((long long *)_src)[l] = 1;
    for(i = 0; i < SIZE/sizeof(int); ++i) {
      //_src[i] = 1;
      //_src[i] = 0x01010101;
      _src[i] = 0x01020304;
      //_src[i] = i * 1/.3;
      //_src[i] = i;
      //_src[i] = rand() >> 24;
      //_src[i] = rand() >> 22;
      //_src[i] = rand() >> 13;
      //_src[i] = rand() >> 9;
      //_src[i] = rand() >> 6;
      //_src[i] = rand() >> 30;
    }

    memcpy(srccpy, src, SIZE);

    last = clock();
    for (i = 0; i < NITER; i++) {
        memcpy(dest, src, SIZE);
    }
    current = clock();
    tmemcpy = (current-last)/((float)CLK_NITER);
    printf("memcpy:\t\t %fs, %.1f MB/s\n", tmemcpy, SIZE/(tmemcpy*MB));

    last = clock();
    for (i = 0; i < NITER; i++)
        cbytes = blosc_compress(ELSIZE, SIZE, src, dest);
    current = clock();
    tshuf = (current-last)/((float)CLK_NITER);
    printf("blosc_compress:\t %fs, %.1f MB/s\t", tshuf, SIZE/(tshuf*MB));
    printf("Orig bytes: %d  Final bytes: %d\n", SIZE, cbytes);

    last = clock();
    for (i = 0; i < NITER; i++)
      nbytes = blosc_decompress(dest, src, SIZE);
    current = clock();
    tunshuf = (current-last)/((float)CLK_NITER);
    printf("blosc_d:\t %fs, %.1f MB/s\t", tunshuf, nbytes/(tunshuf*MB));
    printf("Orig bytes: %d  Final bytes: %d\n", cbytes, nbytes);

    // Check that data has done a good roundtrip
    _src = (int *)src;
    _srccpy = (int *)srccpy;
    for(i = 0; i < SIZE/sizeof(int); ++i){
       if (_src[i] != _srccpy[i]) {
           printf("Error: original data and round-trip do not match in pos %d\n", (int)i);
           printf("Orig--> %x, Copy--> %x\n", _src[i], _srccpy[i]);
           exit(1);
       }
    }

    free(src); free(srccpy);  free(dest);
    return 0;
}

