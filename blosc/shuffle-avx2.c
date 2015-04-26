/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include "shuffle-generic.h"
#include "shuffle-avx2.h"

/* Make sure AVX2 is available for the compilation target and compiler. */
#if !defined(__AVX2__)
  #error AVX2 is not supported by the target architecture/platform and/or this compiler.
#endif

#include <immintrin.h>


/* The next is useful for debugging purposes */
#if 0
#include <stdio.h>
#include <string.h>

static void printymm(__m256i ymm0)
{
  uint8_t buf[32];

  ((__m256i *)buf)[0] = ymm0;
  printf("%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
          buf[0], buf[1], buf[2], buf[3],
          buf[4], buf[5], buf[6], buf[7],
          buf[8], buf[9], buf[10], buf[11],
          buf[12], buf[13], buf[14], buf[15],
          buf[16], buf[17], buf[18], buf[19],
          buf[20], buf[21], buf[22], buf[23],
          buf[24], buf[25], buf[26], buf[27],
          buf[28], buf[29], buf[30], buf[31]);
}
#endif


/* Routine optimized for shuffling a buffer for a type size of 2 bytes. */
static void
shuffle2_avx2(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k;
  size_t nitem;
  __m256i a[2], b[2], c[2], d[2], shmask;
  static uint8_t b_mask[] = {0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
          0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F,
          0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
          0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F };

  nitem = size/256;
  shmask = _mm256_loadu_si256( (__m256i*)(b_mask));
  for( i=0;i<nitem;i++ ) {
    for( j=0;j<4;j++ )  {
      for(k=0;k<2;k++) {
        a[k] = _mm256_loadu_si256( (__m256i*)(src + i*256 +j*64+k*32));
        b[k] = _mm256_shuffle_epi8( a[k], shmask );
      }
      c[0] = _mm256_permute4x64_epi64( b[0], 0xD8);
      c[1] = _mm256_permute4x64_epi64( b[1], 0x8D);

      d[0] = _mm256_blend_epi32(c[0], c[1], 0xF0);
      _mm256_storeu_si256((__m256i*)(dest+j*32), d[0]);
      c[0] = _mm256_blend_epi32(c[0], c[1], 0x0F);
      d[1] = _mm256_permute4x64_epi64( c[0], 0x4E);
      _mm256_storeu_si256((__m256i*)(dest+j*32+(size>>1)), d[1]);
    }
    dest += 128;
  }
}


/* Routine optimized for shuffling a buffer for a type size of 4 bytes. */
static void
shuffle4_avx2(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k;
  size_t numof16belem;
  __m256i ymm0[4], ymm1[4], mask;
  static uint8_t b_mask[] = {
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00
  };

  numof16belem = size / (32*4);
  mask = _mm256_loadu_si256((__m256i*)(b_mask));
  for (i = 0, j = 0; i < numof16belem; i++, j += 32*4) {
    /* Fetch and transpose bytes and words in groups of 64 bytes */
    for (k = 0; k < 4; k++) {
      ymm0[k] = _mm256_loadu_si256((__m256i*)(src+j+k*32));
      ymm1[k] = _mm256_shuffle_epi32(ymm0[k], 0xd8);
      ymm0[k] = _mm256_shuffle_epi32(ymm0[k], 0x8d);
      ymm0[k] = _mm256_unpacklo_epi8(ymm1[k], ymm0[k]);
      ymm1[k] = _mm256_shuffle_epi32(ymm0[k], 0x04e);
      ymm0[k] = _mm256_unpacklo_epi16(ymm0[k], ymm1[k]);
    }
    /* Transpose double words */
    for (k = 0; k < 2; k++) {
      ymm1[k*2] = _mm256_unpacklo_epi32(ymm0[k*2], ymm0[k*2+1]);
      ymm1[k*2+1] = _mm256_unpackhi_epi32(ymm0[k*2], ymm0[k*2+1]);
    }
    /* Transpose quad words */
    for (k = 0; k < 2; k++) {
      ymm0[k*2] = _mm256_unpacklo_epi64(ymm1[k], ymm1[k+2]);
      ymm0[k*2+1] = _mm256_unpackhi_epi64(ymm1[k], ymm1[k+2]);
    }

    for (k = 0; k < 4; k++) {
      ymm0[k] = _mm256_permutevar8x32_epi32(ymm0[k], mask);
    }

    /* Store the result vectors */
    for (k = 0; k < 4; k++) {
      _mm256_storeu_si256((__m256i*)(dest) + k*numof16belem+i, ymm0[k]);
    }
  }
}


/* Routine optimized for shuffling a buffer for a type size of 8 bytes. */
static void
shuffle8_avx2(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k, l;
  size_t numof16belem;
  __m256i ymm0[8], ymm1[8];
  numof16belem = size / (16*8*2);

  for (i = 0, j = 0; i < numof16belem; i++, j += 16*8*2) {
    /* Fetch and transpose bytes in groups of 128 bytes */
    for (k = 0; k < 8; k++) {
      ymm0[k] = _mm256_loadu_si256((__m256i*)(src+j+k*32));
      ymm1[k] = _mm256_shuffle_epi32(ymm0[k], 0x4e);
      ymm1[k] = _mm256_unpacklo_epi8(ymm0[k], ymm1[k]);
    }

    /* Transpose words */
    for (k = 0, l = 0; k < 4; k++, l +=2) {
      ymm0[k*2] = _mm256_unpacklo_epi16(ymm1[l], ymm1[l+1]);
      ymm0[k*2+1] = _mm256_unpackhi_epi16(ymm1[l], ymm1[l+1]);
    }
    /* Transpose double words */
    for (k = 0, l = 0; k < 4; k++, l++) {
      if (k == 2) l += 2;
      ymm1[k*2] = _mm256_unpacklo_epi32(ymm0[l], ymm0[l+2]);
      ymm1[k*2+1] = _mm256_unpackhi_epi32(ymm0[l], ymm0[l+2]);
    }
    /* Transpose quad words */
    for (k = 0; k < 4; k++) {
      ymm0[k*2] = _mm256_unpacklo_epi64(ymm1[k], ymm1[k+4]);
      ymm0[k*2+1] = _mm256_unpackhi_epi64(ymm1[k], ymm1[k+4]);
    }
    for( k=0;k<8;k++)
    {
      ymm1[k] = _mm256_permute4x64_epi64(ymm0[k], 0x72);
      ymm0[k] = _mm256_permute4x64_epi64(ymm0[k], 0xD8);
      ymm0[k] = _mm256_unpacklo_epi16(ymm0[k], ymm1[k]);
    }
    /* Store the result vectors */
    for (k = 0; k < 8; k++) {
      _mm256_storeu_si256((__m256i*)(dest) + k*numof16belem+i, ymm0[k]);
    }
  }
}


/* Routine optimized for shuffling a buffer for a type size of 16 bytes. */
static void
shuffle16_avx2(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k, l;
  size_t numof16belem;
  __m256i ymm0[16], ymm1[16], shmask;
  static uint8_t b_mask[] = { 0x00, 0x08, 0x01, 0x09, 0x02, 0x0A, 0x03, 0x0B,
          0x04, 0x0C, 0x05, 0x0D, 0x06, 0x0E, 0x07, 0x0F,
          0x00, 0x08, 0x01, 0x09, 0x02, 0x0A, 0x03, 0x0B,
          0x04, 0x0C, 0x05, 0x0D, 0x06, 0x0E, 0x07, 0x0F
  };

  numof16belem = size / (16*32);
  shmask = _mm256_loadu_si256((__m256i*)b_mask);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*32) {
    /* Fetch elements in groups of 256 bytes */
    for (k = 0; k < 16; k++) {
      ymm0[k] = _mm256_loadu_si256((__m256i*)(src+j+k*32));
    }

    /* Transpose bytes */
    for (k = 0, l = 0; k < 8; k++, l +=2) {
      ymm1[k*2] = _mm256_unpacklo_epi8(ymm0[l], ymm0[l+1]);
      ymm1[k*2+1] = _mm256_unpackhi_epi8(ymm0[l], ymm0[l+1]);
    }

    /* Transpose words */
    for (k = 0, l = -2; k < 8; k++, l++) {
      if ((k%2) == 0) l += 2;
      ymm0[k*2] = _mm256_unpacklo_epi16(ymm1[l], ymm1[l+2]);
      ymm0[k*2+1] = _mm256_unpackhi_epi16(ymm1[l], ymm1[l+2]);
    }

    /* Transpose double words */
    for (k = 0, l = -4; k < 8; k++, l++) {
      if ((k%4) == 0) l += 4;
      ymm1[k*2] = _mm256_unpacklo_epi32(ymm0[l], ymm0[l+4]);
      ymm1[k*2+1] = _mm256_unpackhi_epi32(ymm0[l], ymm0[l+4]);
    }

    /* Transpose quad words */
    for (k = 0; k < 8; k++) {
      ymm0[k*2] = _mm256_unpacklo_epi64(ymm1[k], ymm1[k+8]);
      ymm0[k*2+1] = _mm256_unpackhi_epi64(ymm1[k], ymm1[k+8]);
    }

    for (k = 0; k < 16; k++) {
      ymm0[k] = _mm256_permute4x64_epi64( ymm0[k], 0xD8);
      ymm0[k] = _mm256_shuffle_epi8( ymm0[k], shmask);
    }

    /* Store the result vectors */
    for (k = 0; k < 16; k++) {
      _mm256_storeu_si256((__m256i*)(dest) + k*numof16belem+i, ymm0[k]);
    }
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 2 bytes. */
static void
unshuffle2_avx2(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k;
  size_t nitem;
  __m256i a[2], b[2], c[2];

  nitem = size/64;
  for( i=0, j=0;i<nitem;i++,j+=2 ) {
    a[0] = _mm256_loadu_si256(((__m256i*)src) + 0*nitem+i);
    a[1] = _mm256_loadu_si256(((__m256i*)src) + 1*nitem+i);
    a[0] = _mm256_permute4x64_epi64(a[0], 0xD8);
    a[1] = _mm256_permute4x64_epi64(a[1], 0xD8);
    b[0] = _mm256_unpacklo_epi8(a[0], a[1]);
    b[1] = _mm256_unpackhi_epi8(a[0], a[1]);
    for (k = 0; k < 2; k++) {
      _mm256_storeu_si256((__m256i*)(dest) + j+k, b[k]);
    }
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 4 bytes. */
static void
unshuffle4_avx2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k, l;
  size_t neblock, numof16belem;
  __m256i ymm0[4], ymm1[4];

  neblock = size / 4;
  numof16belem = neblock / 32;
  for (i = 0, k = 0; i < numof16belem; i++, k += 4) {
    /* Load the first 64 bytes in 4 XMM registrers */
    for (j = 0; j < 4; j++) {
      ymm0[j] = _mm256_loadu_si256(((__m256i *)orig) + j*numof16belem+i);
    }
    /* Shuffle bytes */
    for (j = 0; j < 2; j++) {
      /* Compute the low 32 bytes */
      ymm1[j] = _mm256_unpacklo_epi8(ymm0[j*2], ymm0[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm1[2+j] = _mm256_unpackhi_epi8(ymm0[j*2], ymm0[j*2+1]);
    }

    /* Shuffle 2-byte words */
    for (j = 0; j < 2; j++) {
      /* Compute the low 32 bytes */
      ymm0[j] = _mm256_unpacklo_epi16(ymm1[j*2], ymm1[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm0[2+j] = _mm256_unpackhi_epi16(ymm1[j*2], ymm1[j*2+1]);
    }

    ymm1[0] = _mm256_permute2x128_si256(ymm0[0], ymm0[2], 0x20);
    ymm1[1] = _mm256_permute2x128_si256(ymm0[1], ymm0[3], 0x20);
    ymm1[2] = _mm256_permute2x128_si256(ymm0[0], ymm0[2], 0x31);
    ymm1[3] = _mm256_permute2x128_si256(ymm0[1], ymm0[3], 0x31);

    /* Store the result vectors in proper order */
    for (l = 0; l < 4; l++) {
      _mm256_storeu_si256(((__m256i *)dest) + k+l, ymm1[l]);
    }
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 8 bytes. */
static void
unshuffle8_avx2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m256i ymm0[8], ymm1[8];

  neblock = size / 8;
  numof16belem = neblock/32;
  for (i = 0, k = 0; i < numof16belem; i++, k += 8) {
    /* Load the first 64 bytes in 8 XMM registrers */
    for (j = 0; j < 8; j++) {
      ymm0[j] = _mm256_loadu_si256(((__m256i *)orig) + j*numof16belem+i);
    }
    /* Shuffle bytes */
    for (j = 0; j < 4; j++) {
      /* Compute the low 32 bytes */
      ymm1[j] = _mm256_unpacklo_epi8(ymm0[j*2], ymm0[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm1[4+j] = _mm256_unpackhi_epi8(ymm0[j*2], ymm0[j*2+1]);
    }
    /* Shuffle 2-byte words */
    for (j = 0; j < 4; j++) {
      /* Compute the low 32 bytes */
      ymm0[j] = _mm256_unpacklo_epi16(ymm1[j*2], ymm1[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm0[4+j] = _mm256_unpackhi_epi16(ymm1[j*2], ymm1[j*2+1]);
    }

     for( j=0;j<8;j++)
      {
        ymm0[j] = _mm256_permute4x64_epi64(ymm0[j], 0xD8);
      }

    /* Shuffle 4-byte dwords */
    for (j = 0; j < 4; j++) {
      /* Compute the low 32 bytes */
      ymm1[j] = _mm256_unpacklo_epi32(ymm0[j*2], ymm0[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm1[4+j] = _mm256_unpackhi_epi32(ymm0[j*2], ymm0[j*2+1]);
    }

    /* Store the result vectors in proper order */
    _mm256_storeu_si256(((__m256i *)dest) + k+0, ymm1[0]);
    _mm256_storeu_si256(((__m256i *)dest) + k+1, ymm1[2]);
    _mm256_storeu_si256(((__m256i *)dest) + k+2, ymm1[1]);
    _mm256_storeu_si256(((__m256i *)dest) + k+3, ymm1[3]);
    _mm256_storeu_si256(((__m256i *)dest) + k+4, ymm1[4]);
    _mm256_storeu_si256(((__m256i *)dest) + k+5, ymm1[6]);
    _mm256_storeu_si256(((__m256i *)dest) + k+6, ymm1[5]);
    _mm256_storeu_si256(((__m256i *)dest) + k+7, ymm1[7]);
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 16 bytes. */
static void
unshuffle16_avx2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m256i ymm1[16], ymm2[16];

  neblock = size / 16;
  numof16belem = neblock / 32;
  for (i = 0, k = 0; i < numof16belem; i++, k += 16) {
    /* Load the first 128 bytes in 16 XMM registrers */
    for (j = 0; j < 16; j++) {
      ymm1[j] = _mm256_loadu_si256(((__m256i *)orig) + j*numof16belem+i);
    }
    /* Shuffle bytes */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      ymm2[j] = _mm256_unpacklo_epi8(ymm1[j*2], ymm1[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm2[8+j] = _mm256_unpackhi_epi8(ymm1[j*2], ymm1[j*2+1]);
    }
    /* Shuffle 2-byte words */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      ymm1[j] = _mm256_unpacklo_epi16(ymm2[j*2], ymm2[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm1[8+j] = _mm256_unpackhi_epi16(ymm2[j*2], ymm2[j*2+1]);
    }
    /* Shuffle 4-byte dwords */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      ymm2[j] = _mm256_unpacklo_epi32(ymm1[j*2], ymm1[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm2[8+j] = _mm256_unpackhi_epi32(ymm1[j*2], ymm1[j*2+1]);
    }

    /* Shuffle 8-byte qwords */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      ymm1[j] = _mm256_unpacklo_epi64(ymm2[j*2], ymm2[j*2+1]);
      /* Compute the hi 32 bytes */
      ymm1[8+j] = _mm256_unpackhi_epi64(ymm2[j*2], ymm2[j*2+1]);
    }

    for( j=0;j<8;j++) {
      ymm2[j] = _mm256_permute2x128_si256(ymm1[j], ymm1[j+8], 0x20);
      ymm2[j+8] = _mm256_permute2x128_si256(ymm1[j], ymm1[j+8], 0x31);
    }

    /* Store the result vectors in proper order */
    _mm256_storeu_si256(((__m256i *)dest) + k+ 0, ymm2[ 0]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 1, ymm2[ 4]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 2, ymm2[ 2]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 3, ymm2[ 6]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 4, ymm2[ 1]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 5, ymm2[ 5]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 6, ymm2[ 3]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 7, ymm2[ 7]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 8, ymm2[ 8]);
    _mm256_storeu_si256(((__m256i *)dest) + k+ 9, ymm2[12]);
    _mm256_storeu_si256(((__m256i *)dest) + k+10, ymm2[10]);
    _mm256_storeu_si256(((__m256i *)dest) + k+11, ymm2[14]);
    _mm256_storeu_si256(((__m256i *)dest) + k+12, ymm2[ 9]);
    _mm256_storeu_si256(((__m256i *)dest) + k+13, ymm2[13]);
    _mm256_storeu_si256(((__m256i *)dest) + k+14, ymm2[11]);
    _mm256_storeu_si256(((__m256i *)dest) + k+15, ymm2[15]);
  }
}

/* Shuffle a block.  This can never fail. */
void
shuffle_avx2(const size_t bytesoftype, const size_t blocksize,
             const uint8_t* const _src, uint8_t* const _dest) {
  int unaligned_dest = (int)((uintptr_t)_dest % 16);
  int multiple_of_block = (blocksize % (32 * bytesoftype)) == 0;
  int too_small = (blocksize < 256);

  if (unaligned_dest || !multiple_of_block || too_small) {
    /* _dest buffer is not aligned, not multiple of the vectorization size
     * or is too small.  Call the generic routine. */
    shuffle_generic(bytesoftype, blocksize, _src, _dest);
    return;
  }

  /* Optimized shuffle */
  /* The buffer must be aligned on a 16 bytes boundary, have a power */
  /* of 2 size and be larger or equal than 256 bytes. */
  if (bytesoftype == 4) {
    shuffle4_avx2(_dest, _src, blocksize);
  }
  else if (bytesoftype == 8) {
    shuffle8_avx2(_dest, _src, blocksize);
  }
  else if (bytesoftype == 16) {
    shuffle16_avx2(_dest, _src, blocksize);
  }
  else if (bytesoftype == 2) {
    shuffle2_avx2(_dest, _src, blocksize);
  }
  else {
    /* Non-optimized shuffle */
    shuffle_generic(bytesoftype, blocksize, _src, _dest);
  }
}

/* Unshuffle a block.  This can never fail. */
void
unshuffle_avx2(const size_t bytesoftype, const size_t blocksize,
               const uint8_t* const _src, uint8_t* const _dest) {
  int unaligned_src = (int)((uintptr_t)_src % 16);
  int unaligned_dest = (int)((uintptr_t)_dest % 16);
  int multiple_of_block = (blocksize % (32 * bytesoftype)) == 0;
  int too_small = (blocksize < 256);

  if (unaligned_src || unaligned_dest || !multiple_of_block || too_small) {
    /* _src or _dest buffer is not aligned, not multiple of the vectorization
     * size or is not too small.  Call the generic routine. */
    unshuffle_generic(bytesoftype, blocksize, _src, _dest);
    return;
  }

  /* Optimized unshuffle */
  /* The buffers must be aligned on a 16 bytes boundary, have a power */
  /* of 2 size and be larger or equal than 256 bytes. */
  if (bytesoftype == 4) {
    unshuffle4_avx2(_dest, _src, blocksize);
  }
  else if (bytesoftype == 8) {
    unshuffle8_avx2(_dest, _src, blocksize);
  }
  else if (bytesoftype == 16) {
    unshuffle16_avx2(_dest, _src, blocksize);
  }
  else if (bytesoftype == 2) {
    unshuffle2_avx2(_dest, _src, blocksize);
  }
  else {
    /* Non-optimized unshuffle */
    unshuffle_generic(bytesoftype, blocksize, _src, _dest);
  }
}
