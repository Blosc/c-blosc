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
#include <emmintrin.h>

#define BLOSC_VERSION 1    //  Should be 1-byte long, but can be increased if necessary

#define MB 1024*1024

// Block sizes
//#define CL (2*1024)  /* 2K */
//#define CL (4*1024)  /* 4K */  /* Page size.  Optimal for P4. */
#define CL (8*1024)  /* 8K */  /* Seems optimal for Core2 and P4. */
//#define CL (16*1024) /* 16K */  /* Seems optimal for Core2. */
//#define CL (32*1024) /* 32K */

// Datatype size
#define ELSIZE 2

// Chunksize
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
//#define NITER1 1

#define CLK_NITER CLOCKS_PER_SEC*NITER

// Macro to type-cast an array address to a vector address:
#define ToVectorAddress(x) ((__m128i*)&(x))

// The next is useful for debugging purposes
void printxmm(__m128i xmm0) {
  unsigned char buf[16];

  ((__m128i *)buf)[0] = xmm0;
  printf("%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
          buf[0], buf[1], buf[2], buf[3],
          buf[4], buf[5], buf[6], buf[7],
          buf[8], buf[9], buf[10], buf[11],
          buf[12], buf[13], buf[14], buf[15]);
}


// Routine optimized for shuffling a buffer for a type size of 2 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-20
static void
shuffle2(unsigned char* dest, unsigned char* src, size_t size)
{
  size_t i, j, k;
  size_t numof16belem;
  __m128i xmm0[2], xmm1[2];

  numof16belem = size / (16*2);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*2) {
    // Fetch and transpose bytes, words and double words in groups of 32 bytes
    for (k = 0; k < 2; k++) {
      xmm0[k] = _mm_loadu_si128((__m128i*)(src+j+k*16));
      xmm0[k] = _mm_shufflelo_epi16(xmm0[k], 0xd8);
      xmm0[k] = _mm_shufflehi_epi16(xmm0[k], 0xd8);
      xmm0[k] = _mm_shuffle_epi32(xmm0[k], 0xd8);
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0x4e);
      xmm0[k] = _mm_unpacklo_epi8(xmm0[k], xmm1[k]);
      xmm0[k] = _mm_shuffle_epi32(xmm0[k], 0xd8);
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0x4e);
      xmm0[k] = _mm_unpacklo_epi16(xmm0[k], xmm1[k]);
      xmm0[k] = _mm_shuffle_epi32(xmm0[k], 0xd8);
    }
    // Transpose quad words
    for (k = 0; k < 1; k++) {
      xmm1[k*2] = _mm_unpacklo_epi64(xmm0[k], xmm0[k+1]);
      xmm1[k*2+1] = _mm_unpackhi_epi64(xmm0[k], xmm0[k+1]);
    }
    // Store the result vectors
    for (k = 0; k < 2; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm1[k];
    }
  }
}


// Routine optimized for shuffling a buffer for a type size of 4 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-20
static void
shuffle4(unsigned char* dest, unsigned char* src, size_t size)
{
  size_t i, j, k;
  size_t numof16belem;
  __m128i xmm0[4], xmm1[4];

  numof16belem = size / (16*4);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*4) {
    // Fetch and transpose bytes and words in groups of 64 bytes
    for (k = 0; k < 4; k++) {
      xmm0[k] = _mm_loadu_si128((__m128i*)(src+j+k*16));
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0xd8);
      xmm0[k] = _mm_shuffle_epi32(xmm0[k], 0x8d);
      xmm0[k] = _mm_unpacklo_epi8(xmm1[k], xmm0[k]);
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0x04e);
      xmm0[k] = _mm_unpacklo_epi16(xmm0[k], xmm1[k]);
    }
    // Transpose double words
    for (k = 0; k < 2; k++) {
      xmm1[k*2] = _mm_unpacklo_epi32(xmm0[k*2], xmm0[k*2+1]);
      xmm1[k*2+1] = _mm_unpackhi_epi32(xmm0[k*2], xmm0[k*2+1]);
    }
    // Transpose quad words
    for (k = 0; k < 2; k++) {
      xmm0[k*2] = _mm_unpacklo_epi64(xmm1[k], xmm1[k+2]);
      xmm0[k*2+1] = _mm_unpackhi_epi64(xmm1[k], xmm1[k+2]);
    }
    // Store the result vectors
    for (k = 0; k < 4; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm0[k];
    }
  }
}


// Routine optimized for shuffling a buffer for a type size of 8 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-20
static void
shuffle8(unsigned char* dest, unsigned char* src, size_t size)
{
  size_t i, j, k, l;
  size_t numof16belem;
  __m128i xmm0[8], xmm1[8];

  numof16belem = size / (16*8);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*8) {
    // Fetch and transpose bytes in groups of 128 bytes
    for (k = 0; k < 8; k++) {
      xmm0[k] = _mm_loadu_si128((__m128i*)(src+j+k*16));
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0x4e);
      xmm1[k] = _mm_unpacklo_epi8(xmm0[k], xmm1[k]);
    }
    // Transpose words
    for (k = 0, l = 0; k < 4; k++, l +=2) {
      xmm0[k*2] = _mm_unpacklo_epi16(xmm1[l], xmm1[l+1]);
      xmm0[k*2+1] = _mm_unpackhi_epi16(xmm1[l], xmm1[l+1]);
    }
    // Transpose double words
    for (k = 0, l = 0; k < 4; k++, l++) {
      if (k == 2) l += 2;
      xmm1[k*2] = _mm_unpacklo_epi32(xmm0[l], xmm0[l+2]);
      xmm1[k*2+1] = _mm_unpackhi_epi32(xmm0[l], xmm0[l+2]);
    }
    // Transpose quad words
    for (k = 0; k < 4; k++) {
      xmm0[k*2] = _mm_unpacklo_epi64(xmm1[k], xmm1[k+4]);
      xmm0[k*2+1] = _mm_unpackhi_epi64(xmm1[k], xmm1[k+4]);
    }
    // Store the result vectors
    for (k = 0; k < 8; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm0[k];
    }
  }
}


// Routine optimized for shuffling a buffer for a type size of 16 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-20
static void
shuffle16(unsigned char* dest, unsigned char* src, size_t size)
{
  size_t i, j, k, l;
  size_t numof16belem;
  __m128i xmm0[16], xmm1[16];

  numof16belem = size / (16*16);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*16) {
    // Fetch elements in groups of 256 bytes
    for (k = 0; k < 16; k++) {
      xmm0[k] = _mm_loadu_si128((__m128i*)(src+j+k*16));
    }
    // Transpose bytes
    for (k = 0, l = 0; k < 8; k++, l +=2) {
      xmm1[k*2] = _mm_unpacklo_epi8(xmm0[l], xmm0[l+1]);
      xmm1[k*2+1] = _mm_unpackhi_epi8(xmm0[l], xmm0[l+1]);
    }
    // Transpose words
    for (k = 0, l = -2; k < 8; k++, l++) {
      if ((k%2) == 0) l += 2;
      xmm0[k*2] = _mm_unpacklo_epi16(xmm1[l], xmm1[l+2]);
      xmm0[k*2+1] = _mm_unpackhi_epi16(xmm1[l], xmm1[l+2]);
    }
    // Transpose double words
    for (k = 0, l = -4; k < 8; k++, l++) {
      if ((k%4) == 0) l += 4;
      xmm1[k*2] = _mm_unpacklo_epi32(xmm0[l], xmm0[l+4]);
      xmm1[k*2+1] = _mm_unpackhi_epi32(xmm0[l], xmm0[l+4]);
    }
    // Transpose quad words
    for (k = 0; k < 8; k++) {
      xmm0[k*2] = _mm_unpacklo_epi64(xmm1[k], xmm1[k+8]);
      xmm0[k*2+1] = _mm_unpackhi_epi64(xmm1[k], xmm1[k+8]);
    }
    // Store the result vectors
    for (k = 0; k < 16; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm0[k];
    }
  }
}


unsigned int
blosc_compress(size_t bytesoftype, size_t nbytes, void *orig, void *dest)
{
    unsigned char *_src=NULL;   /* Alias for source buffer */
    unsigned char *_dest=NULL;  /* Alias for destination buffer */
    unsigned char *flags;
    size_t numofCL;             /* Number of complete CL in buffer */
    size_t numofelementsCL;     /* Number of elements in CL */
    size_t i, j, k, l;          /* Local index variables */
    size_t leftover;            /* Extra bytes at end of buffer */
    size_t val, val2, eqval;
    unsigned int cbytes, cebytes, ctbytes;
    __m128i value, value2, cmpeq, andreg;
    const char ones[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    const char cmpresult[16];
    // Temporary buffer for data block
    unsigned char tmp[CL] __attribute__((aligned(64)));

    numofCL = nbytes / CL;
    numofelementsCL = CL / bytesoftype;
    leftover = nbytes % CL;
    _src = (unsigned char *)(orig);
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
    for (i = 16; i < nbytes; i += 16) {
      value2 = _mm_loadu_si128(ToVectorAddress(_src[i]));
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
    ((unsigned int *)_dest)[0] = bytesoftype;       // The type size
    ((unsigned int *)_dest)[1] = CL;                // The block size
    _dest += 8;
    ctbytes += 8;
    for (k = 0; k < numofCL; k++) {
      /* shuffle */
      // tmp <-- _src
      if (bytesoftype == 4) {
        shuffle4(tmp, _src, CL);
      }
      else if (bytesoftype == 8) {
        shuffle8(tmp, _src, CL);
      }
      else if (bytesoftype == 16) {
        shuffle16(tmp, _src, CL);
      }
      else if (bytesoftype == 2) {
        shuffle2(tmp, _src, CL);
      }
      else {
        for (j = 0; j < bytesoftype; j++) {
          for (i = 0; i < numofelementsCL; i++) {
            tmp[j*numofelementsCL+i] = _src[i*bytesoftype+j];
          }
        }
      }
      _src += CL;
      for (j = 0; j < bytesoftype; j++) {
        _dest += sizeof(int);
        cbytes = blosclz_compress(tmp+j*numofelementsCL, numofelementsCL, _dest);
        if (cbytes == -1) {
          /* The compressor has been unable to compress data significantly */
          memcpy(_dest, tmp+j*numofelementsCL, numofelementsCL);
          cbytes = numofelementsCL;
        }
        ((unsigned int *)(_dest))[-1] = cbytes;
        _dest += cbytes;
        ctbytes += cbytes + sizeof(int);
        // TODO:  Perform a better check so as to avoid a dest buffer overrun
        if (ctbytes > nbytes) {
          //printf("uncompressible data!\n");
          return 0;    // Uncompressible data
        }
      }  // Close j < bytesoftype
    }  // Close k < numofCL

    if(leftover > 0) {
      printf("leftover!\n");
      memcpy((void*)tmp, (void*)_src, leftover);
      _dest += sizeof(int);
      cbytes = blosclz_compress(tmp, leftover, _dest);
      ((unsigned int *)(_dest))[-1] = cbytes;
      _dest += cbytes;
      ctbytes += cbytes + sizeof(int);
    }

    return ctbytes;
}


// Routine optimized for unshuffling a buffer for a type size of 2 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-13
static void
unshuffle2(unsigned char* dest, unsigned char* orig, size_t size)
{
  size_t i, k;
  size_t numofelementsCL, numof16belem;
  __m128i xmm1[2], xmm2[2];

  numofelementsCL = size / 2;
  numof16belem = numofelementsCL / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 2) {
    // Load the first 32 bytes in 2 XMM registrers
    xmm1[0] = ((__m128i *)orig)[0*numof16belem+i];
    xmm1[1] = ((__m128i *)orig)[1*numof16belem+i];
    // Shuffle bytes
    // Compute the low 32 bytes
    xmm2[0] = _mm_unpacklo_epi8(xmm1[0], xmm1[1]);
    // Compute the hi 32 bytes
    xmm2[1] = _mm_unpackhi_epi8(xmm1[0], xmm1[1]);
    // Store the result vectors in proper order
    ((__m128i *)dest)[k+0] = xmm2[0];
    ((__m128i *)dest)[k+1] = xmm2[1];
  }
}


// Routine optimized for unshuffling a buffer for a type size of 4 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-13
static void
unshuffle4(unsigned char* dest, unsigned char* orig, size_t size)
{
  size_t i, j, k;
  size_t numofelementsCL, numof16belem;
  __m128i xmm0[4], xmm1[4];

  numofelementsCL = size / 4;
  numof16belem = numofelementsCL / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 4) {
    // Load the first 64 bytes in 4 XMM registrers
    for (j = 0; j < 4; j++) {
      xmm0[j] = ((__m128i *)orig)[j*numof16belem+i];
    }
    // Shuffle bytes
    for (j = 0; j < 2; j++) {
      // Compute the low 32 bytes
      xmm1[j] = _mm_unpacklo_epi8(xmm0[j*2], xmm0[j*2+1]);
      // Compute the hi 32 bytes
      xmm1[2+j] = _mm_unpackhi_epi8(xmm0[j*2], xmm0[j*2+1]);
    }
    // Shuffle 2-byte words
    for (j = 0; j < 2; j++) {
      // Compute the low 32 bytes
      xmm0[j] = _mm_unpacklo_epi16(xmm1[j*2], xmm1[j*2+1]);
      // Compute the hi 32 bytes
      xmm0[2+j] = _mm_unpackhi_epi16(xmm1[j*2], xmm1[j*2+1]);
    }
    // Store the result vectors in proper order
    ((__m128i *)dest)[k+0] = xmm0[0];
    ((__m128i *)dest)[k+1] = xmm0[2];
    ((__m128i *)dest)[k+2] = xmm0[1];
    ((__m128i *)dest)[k+3] = xmm0[3];
  }
}


// Routine optimized for unshuffling a buffer for a type size of 8 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-13
static void
unshuffle8(unsigned char* dest, unsigned char* orig, size_t size)
{
  size_t i, j, k;
  size_t numofelementsCL, numof16belem;
  __m128i xmm0[8], xmm1[8];

  numofelementsCL = size / 8;
  numof16belem = numofelementsCL / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 8) {
    // Load the first 64 bytes in 8 XMM registrers
    for (j = 0; j < 8; j++) {
      xmm0[j] = ((__m128i *)orig)[j*numof16belem+i];
    }
    // Shuffle bytes
    for (j = 0; j < 4; j++) {
      // Compute the low 32 bytes
      xmm1[j] = _mm_unpacklo_epi8(xmm0[j*2], xmm0[j*2+1]);
      // Compute the hi 32 bytes
      xmm1[4+j] = _mm_unpackhi_epi8(xmm0[j*2], xmm0[j*2+1]);
    }
    // Shuffle 2-byte words
    for (j = 0; j < 4; j++) {
      // Compute the low 32 bytes
      xmm0[j] = _mm_unpacklo_epi16(xmm1[j*2], xmm1[j*2+1]);
      // Compute the hi 32 bytes
      xmm0[4+j] = _mm_unpackhi_epi16(xmm1[j*2], xmm1[j*2+1]);
    }
    // Shuffle 4-byte dwords
    for (j = 0; j < 4; j++) {
      // Compute the low 32 bytes
      xmm1[j] = _mm_unpacklo_epi32(xmm0[j*2], xmm0[j*2+1]);
      // Compute the hi 32 bytes
      xmm1[4+j] = _mm_unpackhi_epi32(xmm0[j*2], xmm0[j*2+1]);
    }
    // Store the result vectors in proper order
    ((__m128i *)dest)[k+0] = xmm1[0];
    ((__m128i *)dest)[k+1] = xmm1[4];
    ((__m128i *)dest)[k+2] = xmm1[2];
    ((__m128i *)dest)[k+3] = xmm1[6];
    ((__m128i *)dest)[k+4] = xmm1[1];
    ((__m128i *)dest)[k+5] = xmm1[5];
    ((__m128i *)dest)[k+6] = xmm1[3];
    ((__m128i *)dest)[k+7] = xmm1[7];
  }
}


// Routine optimized for unshuffling a buffer for a type size of 16 bytes.
// The buffer should be aligned on a 16 bytes boundary and be of a power of 2 size.
// F. Alted 2009-05-13
static void
unshuffle16(unsigned char* dest, unsigned char* orig, size_t size)
{
  size_t i, j, k;
  size_t numofelementsCL, numof16belem;
  __m128i xmm1[16], xmm2[16];

  numofelementsCL = size / 16;
  numof16belem = numofelementsCL / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 16) {
    // Load the first 128 bytes in 16 XMM registrers
    for (j = 0; j < 16; j++) {
      xmm1[j] = ((__m128i *)orig)[j*numof16belem+i];
    }
    // Shuffle bytes
    for (j = 0; j < 8; j++) {
      // Compute the low 32 bytes
      xmm2[j] = _mm_unpacklo_epi8(xmm1[j*2], xmm1[j*2+1]);
      // Compute the hi 32 bytes
      xmm2[8+j] = _mm_unpackhi_epi8(xmm1[j*2], xmm1[j*2+1]);
    }
    // Shuffle 2-byte words
    for (j = 0; j < 8; j++) {
      // Compute the low 32 bytes
      xmm1[j] = _mm_unpacklo_epi16(xmm2[j*2], xmm2[j*2+1]);
      // Compute the hi 32 bytes
      xmm1[8+j] = _mm_unpackhi_epi16(xmm2[j*2], xmm2[j*2+1]);
    }
    // Shuffle 4-byte dwords
    for (j = 0; j < 8; j++) {
      // Compute the low 32 bytes
      xmm2[j] = _mm_unpacklo_epi32(xmm1[j*2], xmm1[j*2+1]);
      // Compute the hi 32 bytes
      xmm2[8+j] = _mm_unpackhi_epi32(xmm1[j*2], xmm1[j*2+1]);
    }
    // Shuffle 8-byte qwords
    for (j = 0; j < 8; j++) {
      // Compute the low 32 bytes
      xmm1[j] = _mm_unpacklo_epi64(xmm2[j*2], xmm2[j*2+1]);
      // Compute the hi 32 bytes
      xmm1[8+j] = _mm_unpackhi_epi64(xmm2[j*2], xmm2[j*2+1]);
    }
    // Store the result vectors in proper order
    ((__m128i *)dest)[k+0] = xmm1[0];
    ((__m128i *)dest)[k+1] = xmm1[8];
    ((__m128i *)dest)[k+2] = xmm1[4];
    ((__m128i *)dest)[k+3] = xmm1[12];
    ((__m128i *)dest)[k+4] = xmm1[2];
    ((__m128i *)dest)[k+5] = xmm1[10];
    ((__m128i *)dest)[k+6] = xmm1[6];
    ((__m128i *)dest)[k+7] = xmm1[14];
    ((__m128i *)dest)[k+8] = xmm1[1];
    ((__m128i *)dest)[k+9] = xmm1[9];
    ((__m128i *)dest)[k+10] = xmm1[5];
    ((__m128i *)dest)[k+11] = xmm1[13];
    ((__m128i *)dest)[k+12] = xmm1[3];
    ((__m128i *)dest)[k+13] = xmm1[11];
    ((__m128i *)dest)[k+14] = xmm1[7];
    ((__m128i *)dest)[k+15] = xmm1[15];
  }
}


static void *
_blosc_d(size_t bytesoftype, size_t numofelementsCL,
         unsigned char* _src, unsigned char* _dest, unsigned char *tmp)
{
  size_t i, j, k, l;                   /* Local index variables */
  size_t nbytes, cbytes;
  unsigned char* _tmp;

  _tmp = tmp;
  for (j = 0; j < bytesoftype; j++) {
    cbytes = ((unsigned int *)(_src))[0];       // The number of compressed bytes
    //printf("cbytes: %d,%d\n", cbytes, bytesoftype);
    _src += sizeof(int);
    /* uncompress */
    if (cbytes == 1) {
      memset(_tmp, *_src, numofelementsCL);
    }
    else if (cbytes == numofelementsCL) {
      memcpy(_tmp, _src, numofelementsCL);
    }
    else {
      nbytes = blosclz_decompress(_src, cbytes, _tmp, numofelementsCL);
      assert (nbytes == numofelementsCL);
    }
    _src += cbytes;
    _tmp += numofelementsCL;
  }

  if (bytesoftype == 2) {
    unshuffle2(_dest, tmp, CL);
  }
  else if (bytesoftype == 4) {
    unshuffle4(_dest, tmp, CL);
  }
  else if (bytesoftype == 8) {
    unshuffle8(_dest, tmp, CL);
  }
  else if (bytesoftype == 16) {
    unshuffle16(_dest, tmp, CL);
  }
  else {
  /* unshuffle */
    for (i = 0; i < numofelementsCL; i++) {
      for (j = 0; j < bytesoftype; j++) {
        _dest[i*bytesoftype+j] = tmp[j*numofelementsCL+i];
      }
    }
  }
  return(_src);
}


unsigned int
blosc_decompress(size_t bytesoftype, size_t cbbytes, void *orig, void *dest)
{
  unsigned char *_src=NULL;   /* Alias for source buffer */
  unsigned char *_dest=NULL;  /* Alias for destination buffer */
  unsigned char version, flags;
  unsigned char rep, value;
  size_t leftover;            /* Extra bytes at end of buffer */
  size_t numofelementsCL;     /* Number of elements in CL */
  size_t numofCL;             /* Number of complete CL in buffer */
  size_t k;
  size_t nbytes, dbytes, cbytes, ntbytes = 0;
  __m128i xmm0;
  unsigned char *tmp;

  _src = (unsigned char *)(orig);
  _dest = (unsigned char *)(dest);

  // Read the header block
  version = _src[0];                        // The blosc version
  flags = _src[1];                          // The flags for this block
  _src += 2;
  nbytes = ((unsigned int *)_src)[0];       // The size of the chunk
  _src += sizeof(int);

  //printf("nbytes-->%d, %d\n", nbytes, flags);
  // Check for the trivial repeat pattern
  if (flags == 1) {
    rep = _src[0];                          // The number of bytes repeated
    if (rep == 1) {
      // Copy values in blocks of 16 bytes
      value = _src[1];
      xmm0 = _mm_set1_epi8(value);        // The repeated value
    }
    else if (rep == 16) {
      xmm0 = _mm_loadu_si128(ToVectorAddress(_src[1]));
    }

    // Copy value into destination
    for (k = 0; k < nbytes/16; k++) {
      ((__m128i *)dest)[k] = xmm0;
    }

    if (rep == 1) {
      // Copy the remainding values
      _dest = (unsigned char *)dest + k*16;
      for (k = 0; k < nbytes%16; k++) {
        _dest[k] = value;
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
  numofCL = nbytes / blocksize;
  leftover = nbytes % blocksize;
  numofelementsCL = blocksize / bytesoftype;

  // Create temporary area
  posix_memalign((void **)&tmp, 64, blocksize);

  for (k = 0; k < numofCL; k++) {
    _src = _blosc_d(bytesoftype, numofelementsCL, _src, _dest, tmp);
    _dest += CL;
    ntbytes += CL;
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
    void *orig, *dest, *origcpy;
    //unsigned char orig[SIZE], dest[SIZE], origcpy[SIZE] __attribute__((aligned(64)));
    unsigned char *__orig, *__origcpy;
    size_t i;
    long long l;
    clock_t last, current;
    float tmemcpy, tcompr, tshuf, tdecompr, tunshuf;

    orig = malloc(SIZE);  origcpy = malloc(SIZE);
    //posix_memalign((void **)&orig, 64, SIZE);
    //posix_memalign((void **)&origcpy, 64, SIZE);
    posix_memalign((void **)&dest, 64, SIZE);   // Must be aligned to 16 bytes at least!

    srand(1);

    // Initialize the original buffer
    int* _orig = (int *)orig;
    int* _origcpy = (int *)origcpy;
    //float* _orig = (float *)orig;
    //float* _origcpy = (float *)origcpy;
    //for(l = 0; l < SIZE/sizeof(long long); ++l){
      //((long long *)_orig)[l] = l;
      //((long long *)_orig)[l] = rand() >> 24;
      //((long long *)_orig)[l] = 1;
    for(i = 0; i < SIZE/sizeof(int); ++i) {
      //_orig[i] = 1;
      //_orig[i] = 0x01010101;
      //_orig[i] = 0x01020304;
      //_orig[i] = i * 1/.3;
      _orig[i] = i;
      //_orig[i] = rand() >> 24;
      //_orig[i] = rand() >> 22;
      //_orig[i] = rand() >> 13;
      //_orig[i] = rand() >> 9;
      //_orig[i] = rand() >> 6;
      //_orig[i] = rand() >> 30;
    }

    memcpy(origcpy, orig, SIZE);

    last = clock();
    for (i = 0; i < NITER; i++) {
        memcpy(dest, orig, SIZE);
    }
    current = clock();
    tmemcpy = (current-last)/((float)CLK_NITER);
    printf("memcpy:\t\t %fs, %.1f MB/s\n", tmemcpy, SIZE/(tmemcpy*MB));

    last = clock();
    for (i = 0; i < NITER; i++)
        cbytes = blosc_compress(ELSIZE, SIZE, orig, dest);
    current = clock();
    tshuf = (current-last)/((float)CLK_NITER);
    printf("blosc_compress:\t %fs, %.1f MB/s\t", tshuf, SIZE/(tshuf*MB));
    printf("Orig bytes: %d  Final bytes: %d\n", SIZE, cbytes);

    last = clock();
    for (i = 0; i < NITER; i++)
      nbytes = blosc_decompress(ELSIZE, cbytes, dest, orig);
    current = clock();
    tunshuf = (current-last)/((float)CLK_NITER);
    printf("blosc_d:\t %fs, %.1f MB/s\t", tunshuf, nbytes/(tunshuf*MB));
    printf("Orig bytes: %d  Final bytes: %d\n", cbytes, nbytes);

    // Check that data has done a good roundtrip
    _orig = (int *)orig;
    _origcpy = (int *)origcpy;
    for(i = 0; i < SIZE/sizeof(int); ++i){
       if (_orig[i] != _origcpy[i]) {
           printf("Error: original data and round-trip do not match in pos %d\n", (int)i);
           printf("Orig--> %x, Copy--> %x\n", _orig[i], _origcpy[i]);
           exit(1);
       }
    }

    free(orig); free(origcpy);  free(dest);
    return 0;
}

