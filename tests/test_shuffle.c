/*********************************************************************
 * 	Test to compare AVX2 and SSE2 shuffle and unshuffle functions
 * 	compile:
 * 		icc -m64 -O3 -march=core-avx2 test_shuffle.c -o test_shuffle
 * 		or  gcc -m64 -O3 -march=core-avx2 test_shuffle.c -o test_shuffle (gcc>=4.7)
 * 	Run:
 * 		./test_shuffle [-b bytesoftype] [-i numberiterations] [-s sizeofshuffleblock]
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>


double getseconds(struct timeval last, struct timeval current);
void dump_data(uint8_t *buf, int len);


#include <immintrin.h>

static void
shuffle2_AVX2(uint8_t* dest, const uint8_t* src, size_t size)
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

static void
unshuffle2_AVX2(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j;
  size_t nitem;
  __m256i a[2], b[2], c[2];
  
  nitem = size/64;
  for( i=0, j=0;i<nitem;i++,j+=2 ) {
    a[0] = ((__m256i *)src)[0*nitem+i];
    a[1] = ((__m256i *)src)[1*nitem+i];
    a[0] = _mm256_permute4x64_epi64(a[0], 0xD8);
    a[1] = _mm256_permute4x64_epi64(a[1], 0xD8);
    b[0] = _mm256_unpacklo_epi8(a[0], a[1]);
    b[1] = _mm256_unpackhi_epi8(a[0], a[1]);
    ((__m256i *)dest)[j+0] = b[0];
    ((__m256i *)dest)[j+1] = b[1]; 
  }
}

/* Routine optimized for shuffling a buffer for a type size of 4 bytes. */
static void
shuffle4_AVX2(uint8_t* dest, const uint8_t* src, size_t size)
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
      ((__m256i *)dest)[k*numof16belem+i] = ymm0[k];
    }
  }
}

/* Routine optimized for unshuffling a buffer for a type size of 4 bytes. */
static void
unshuffle4_AVX2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m256i ymm0[4], ymm1[4];

  neblock = size / 4;
  numof16belem = neblock / 32;
  for (i = 0, k = 0; i < numof16belem; i++, k += 4) {
    /* Load the first 64 bytes in 4 XMM registrers */
    for (j = 0; j < 4; j++) {
      ymm0[j] = ((__m256i *)orig)[j*numof16belem+i];
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
    ((__m256i *)dest)[k+0] = ymm1[0];
    ((__m256i *)dest)[k+1] = ymm1[1];
    ((__m256i *)dest)[k+2] = ymm1[2];
    ((__m256i *)dest)[k+3] = ymm1[3];
  }
}



/* Routine optimized for shuffling a buffer for a type size of 8 bytes. */
static void
shuffle8_AVX2(uint8_t* dest, const uint8_t* src, size_t size)
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
        ((__m256i *)dest)[k*numof16belem+i] = ymm0[k];
    }
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 8 bytes. */
static void
unshuffle8_AVX2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m256i ymm0[8], ymm1[8];

  neblock = size / 8;
  numof16belem = neblock/32;
  for (i = 0, k = 0; i < numof16belem; i++, k += 8) 
  {
    /* Load the first 64 bytes in 8 XMM registrers */
    for (j = 0; j < 8; j++) {
      ymm0[j] = ((__m256i *)orig)[j*numof16belem+i];
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
      ((__m256i *)dest)[k+0] = ymm1[0];
      ((__m256i *)dest)[k+1] = ymm1[2];
      ((__m256i *)dest)[k+2] = ymm1[1];
      ((__m256i *)dest)[k+3] = ymm1[3];
      ((__m256i *)dest)[k+4] = ymm1[4];
      ((__m256i *)dest)[k+5] = ymm1[6];
      ((__m256i *)dest)[k+6] = ymm1[5]; 
      ((__m256i *)dest)[k+7] = ymm1[7];
  }
}

/* Routine optimized for shuffling a buffer for a type size of 16 bytes. */
static void
shuffle16_AVX2(uint8_t* dest, const uint8_t* src, size_t size)
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

    for (k=0;k<16;k++) {
      ymm0[k] = _mm256_permute4x64_epi64( ymm0[k], 0xD8);
      ymm0[k] = _mm256_shuffle_epi8( ymm0[k], shmask);
    }

    /* Store the result vectors */
    for (k = 0; k < 16; k++) {
      ((__m256i *)dest)[k*numof16belem+i] = ymm0[k];
    }
  }
}

/* Routine optimized for unshuffling a buffer for a type size of 16 bytes. */
static void
unshuffle16_AVX2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m256i ymm1[16], ymm2[16];

  neblock = size / 16;
  numof16belem = neblock / 32;
  for (i = 0, k = 0; i < numof16belem; i++, k += 16) {
    /* Load the first 128 bytes in 16 XMM registrers */
    for (j = 0; j < 16; j++) {
      ymm1[j] = ((__m256i *)orig)[j*numof16belem+i];
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
    ((__m256i *)dest)[k+0] = ymm2[0];
    ((__m256i *)dest)[k+1] = ymm2[4];
    ((__m256i *)dest)[k+2] = ymm2[2];
    ((__m256i *)dest)[k+3] = ymm2[6];
    ((__m256i *)dest)[k+4] = ymm2[1];
    ((__m256i *)dest)[k+5] = ymm2[5];
    ((__m256i *)dest)[k+6] = ymm2[3];
    ((__m256i *)dest)[k+7] = ymm2[7];
    ((__m256i *)dest)[k+8] = ymm2[8];
    ((__m256i *)dest)[k+9] = ymm2[12];
    ((__m256i *)dest)[k+10] = ymm2[10];
    ((__m256i *)dest)[k+11] = ymm2[14];
    ((__m256i *)dest)[k+12] = ymm2[9];
    ((__m256i *)dest)[k+13] = ymm2[13];
    ((__m256i *)dest)[k+14] = ymm2[11];
    ((__m256i *)dest)[k+15] = ymm2[15];
  }
}


/* The SSE2 versions of shuffle and unshuffle */

#include <emmintrin.h>

/* The next is useful for debugging purposes */
#if 0
static void printxmm(__m128i xmm0)
{
  uint8_t buf[16];

  ((__m128i *)buf)[0] = xmm0;
  printf("%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
          buf[0], buf[1], buf[2], buf[3],
          buf[4], buf[5], buf[6], buf[7],
          buf[8], buf[9], buf[10], buf[11],
          buf[12], buf[13], buf[14], buf[15]);
}
#endif


/* Routine optimized for shuffling a buffer for a type size of 2 bytes. */
static void
shuffle2(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k;
  size_t numof16belem;
  __m128i xmm0[2], xmm1[2];

  numof16belem = size / (16*2);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*2) {
    /* Fetch and transpose bytes, words and double words in groups of
       32 bytes */
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
    /* Transpose quad words */
    for (k = 0; k < 1; k++) {
      xmm1[k*2] = _mm_unpacklo_epi64(xmm0[k], xmm0[k+1]);
      xmm1[k*2+1] = _mm_unpackhi_epi64(xmm0[k], xmm0[k+1]);
    }
    /* Store the result vectors */
    for (k = 0; k < 2; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm1[k];
    }
  }
}


/* Routine optimized for shuffling a buffer for a type size of 4 bytes. */
static void
shuffle4(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k;
  size_t numof16belem;
  __m128i xmm0[4], xmm1[4];

  numof16belem = size / (16*4);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*4) {
    /* Fetch and transpose bytes and words in groups of 64 bytes */
    for (k = 0; k < 4; k++) {
      xmm0[k] = _mm_loadu_si128((__m128i*)(src+j+k*16));
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0xd8);
      xmm0[k] = _mm_shuffle_epi32(xmm0[k], 0x8d);
      xmm0[k] = _mm_unpacklo_epi8(xmm1[k], xmm0[k]);
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0x04e);
      xmm0[k] = _mm_unpacklo_epi16(xmm0[k], xmm1[k]);
    }
    /* Transpose double words */
    for (k = 0; k < 2; k++) {
      xmm1[k*2] = _mm_unpacklo_epi32(xmm0[k*2], xmm0[k*2+1]);
      xmm1[k*2+1] = _mm_unpackhi_epi32(xmm0[k*2], xmm0[k*2+1]);
    }
    /* Transpose quad words */
    for (k = 0; k < 2; k++) {
      xmm0[k*2] = _mm_unpacklo_epi64(xmm1[k], xmm1[k+2]);
      xmm0[k*2+1] = _mm_unpackhi_epi64(xmm1[k], xmm1[k+2]);
    }
    /* Store the result vectors */
    for (k = 0; k < 4; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm0[k];
    }
  }
}


/* Routine optimized for shuffling a buffer for a type size of 8 bytes. */
static void
shuffle8(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k, l;
  size_t numof16belem;
  __m128i xmm0[8], xmm1[8];

  numof16belem = size / (16*8);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*8) {
    /* Fetch and transpose bytes in groups of 128 bytes */
    for (k = 0; k < 8; k++) {
      xmm0[k] = _mm_loadu_si128((__m128i*)(src+j+k*16));
      xmm1[k] = _mm_shuffle_epi32(xmm0[k], 0x4e);
      xmm1[k] = _mm_unpacklo_epi8(xmm0[k], xmm1[k]);
    }
    /* Transpose words */
    for (k = 0, l = 0; k < 4; k++, l +=2) {
      xmm0[k*2] = _mm_unpacklo_epi16(xmm1[l], xmm1[l+1]);
      xmm0[k*2+1] = _mm_unpackhi_epi16(xmm1[l], xmm1[l+1]);
    }
    /* Transpose double words */
    for (k = 0, l = 0; k < 4; k++, l++) {
      if (k == 2) l += 2;
      xmm1[k*2] = _mm_unpacklo_epi32(xmm0[l], xmm0[l+2]);
      xmm1[k*2+1] = _mm_unpackhi_epi32(xmm0[l], xmm0[l+2]);
    }
    /* Transpose quad words */
    for (k = 0; k < 4; k++) {
      xmm0[k*2] = _mm_unpacklo_epi64(xmm1[k], xmm1[k+4]);
      xmm0[k*2+1] = _mm_unpackhi_epi64(xmm1[k], xmm1[k+4]);
    }
    /* Store the result vectors */
    for (k = 0; k < 8; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm0[k];
    }
  }
}


/* Routine optimized for shuffling a buffer for a type size of 16 bytes. */
static void
shuffle16(uint8_t* dest, const uint8_t* src, size_t size)
{
  size_t i, j, k, l;
  size_t numof16belem;
  __m128i xmm0[16], xmm1[16];

  numof16belem = size / (16*16);
  for (i = 0, j = 0; i < numof16belem; i++, j += 16*16) {
    /* Fetch elements in groups of 256 bytes */
    for (k = 0; k < 16; k++) {
      xmm0[k] = _mm_loadu_si128((__m128i*)(src+j+k*16));
    }
    /* Transpose bytes */
    for (k = 0, l = 0; k < 8; k++, l +=2) {
      xmm1[k*2] = _mm_unpacklo_epi8(xmm0[l], xmm0[l+1]);
      xmm1[k*2+1] = _mm_unpackhi_epi8(xmm0[l], xmm0[l+1]);
    }
    /* Transpose words */
    for (k = 0, l = -2; k < 8; k++, l++) {
      if ((k%2) == 0) l += 2;
      xmm0[k*2] = _mm_unpacklo_epi16(xmm1[l], xmm1[l+2]);
      xmm0[k*2+1] = _mm_unpackhi_epi16(xmm1[l], xmm1[l+2]);
    }
    /* Transpose double words */
    for (k = 0, l = -4; k < 8; k++, l++) {
      if ((k%4) == 0) l += 4;
      xmm1[k*2] = _mm_unpacklo_epi32(xmm0[l], xmm0[l+4]);
      xmm1[k*2+1] = _mm_unpackhi_epi32(xmm0[l], xmm0[l+4]);
    }
    /* Transpose quad words */
    for (k = 0; k < 8; k++) {
      xmm0[k*2] = _mm_unpacklo_epi64(xmm1[k], xmm1[k+8]);
      xmm0[k*2+1] = _mm_unpackhi_epi64(xmm1[k], xmm1[k+8]);
    }
    /* Store the result vectors */
    for (k = 0; k < 16; k++) {
      ((__m128i *)dest)[k*numof16belem+i] = xmm0[k];
    }
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 2 bytes. */
static void
unshuffle2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, k;
  size_t neblock, numof16belem;
  __m128i xmm1[2], xmm2[2];

  neblock = size / 2;
  numof16belem = neblock / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 2) {
    /* Load the first 32 bytes in 2 XMM registrers */
    xmm1[0] = ((__m128i *)orig)[0*numof16belem+i];
    xmm1[1] = ((__m128i *)orig)[1*numof16belem+i];
    /* Shuffle bytes */
    /* Compute the low 32 bytes */
    xmm2[0] = _mm_unpacklo_epi8(xmm1[0], xmm1[1]);
    /* Compute the hi 32 bytes */
    xmm2[1] = _mm_unpackhi_epi8(xmm1[0], xmm1[1]);
    /* Store the result vectors in proper order */
    ((__m128i *)dest)[k+0] = xmm2[0];
    ((__m128i *)dest)[k+1] = xmm2[1];
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 4 bytes. */
static void
unshuffle4(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m128i xmm0[4], xmm1[4];

  neblock = size / 4;
  numof16belem = neblock / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 4) {
    /* Load the first 64 bytes in 4 XMM registrers */
    for (j = 0; j < 4; j++) {
      xmm0[j] = ((__m128i *)orig)[j*numof16belem+i];
    }
    /* Shuffle bytes */
    for (j = 0; j < 2; j++) {
      /* Compute the low 32 bytes */
      xmm1[j] = _mm_unpacklo_epi8(xmm0[j*2], xmm0[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm1[2+j] = _mm_unpackhi_epi8(xmm0[j*2], xmm0[j*2+1]);
    }
    /* Shuffle 2-byte words */
    for (j = 0; j < 2; j++) {
      /* Compute the low 32 bytes */
      xmm0[j] = _mm_unpacklo_epi16(xmm1[j*2], xmm1[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm0[2+j] = _mm_unpackhi_epi16(xmm1[j*2], xmm1[j*2+1]);
    }
    /* Store the result vectors in proper order */
    ((__m128i *)dest)[k+0] = xmm0[0];
    ((__m128i *)dest)[k+1] = xmm0[2];
    ((__m128i *)dest)[k+2] = xmm0[1];
    ((__m128i *)dest)[k+3] = xmm0[3];
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 8 bytes. */
static void
unshuffle8(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m128i xmm0[8], xmm1[8];

  neblock = size / 8;
  numof16belem = neblock / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 8) {
    /* Load the first 64 bytes in 8 XMM registrers */
    for (j = 0; j < 8; j++) {
      xmm0[j] = ((__m128i *)orig)[j*numof16belem+i];
    }
    /* Shuffle bytes */
    for (j = 0; j < 4; j++) {
      /* Compute the low 32 bytes */
      xmm1[j] = _mm_unpacklo_epi8(xmm0[j*2], xmm0[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm1[4+j] = _mm_unpackhi_epi8(xmm0[j*2], xmm0[j*2+1]);
    }
    /* Shuffle 2-byte words */
    for (j = 0; j < 4; j++) {
      /* Compute the low 32 bytes */
      xmm0[j] = _mm_unpacklo_epi16(xmm1[j*2], xmm1[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm0[4+j] = _mm_unpackhi_epi16(xmm1[j*2], xmm1[j*2+1]);
    }
    /* Shuffle 4-byte dwords */
    for (j = 0; j < 4; j++) {
      /* Compute the low 32 bytes */
      xmm1[j] = _mm_unpacklo_epi32(xmm0[j*2], xmm0[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm1[4+j] = _mm_unpackhi_epi32(xmm0[j*2], xmm0[j*2+1]);
    }
    /* Store the result vectors in proper order */
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


/* Routine optimized for unshuffling a buffer for a type size of 16 bytes. */
static void
unshuffle16(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m128i xmm1[16], xmm2[16];

  neblock = size / 16;
  numof16belem = neblock / 16;
  for (i = 0, k = 0; i < numof16belem; i++, k += 16) {
    /* Load the first 128 bytes in 16 XMM registrers */
    for (j = 0; j < 16; j++) {
      xmm1[j] = ((__m128i *)orig)[j*numof16belem+i];
    }
    /* Shuffle bytes */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      xmm2[j] = _mm_unpacklo_epi8(xmm1[j*2], xmm1[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm2[8+j] = _mm_unpackhi_epi8(xmm1[j*2], xmm1[j*2+1]);
    }
    /* Shuffle 2-byte words */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      xmm1[j] = _mm_unpacklo_epi16(xmm2[j*2], xmm2[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm1[8+j] = _mm_unpackhi_epi16(xmm2[j*2], xmm2[j*2+1]);
    }
    /* Shuffle 4-byte dwords */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      xmm2[j] = _mm_unpacklo_epi32(xmm1[j*2], xmm1[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm2[8+j] = _mm_unpackhi_epi32(xmm1[j*2], xmm1[j*2+1]);
    }
    /* Shuffle 8-byte qwords */
    for (j = 0; j < 8; j++) {
      /* Compute the low 32 bytes */
      xmm1[j] = _mm_unpacklo_epi64(xmm2[j*2], xmm2[j*2+1]);
      /* Compute the hi 32 bytes */
      xmm1[8+j] = _mm_unpackhi_epi64(xmm2[j*2], xmm2[j*2+1]);
    }
    /* Store the result vectors in proper order */
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




static void print_reg_256i(__m256i *ymm)
{
#ifndef DBG
	return ;
#endif
  uint8_t *buf;
  int i=0;
  //((__m256i *)buf)[0] = ymm;
  buf = (uint8_t*)ymm;
  for(i=0;i<32;i++ )
  {
	printf("%2d ", buf[i]);
  }
  printf("\n");
}


int NUM = 256*4*8;
int ITER  = 900000;


void dump_data(uint8_t *buf, int len)
{
	int i = 0;
	for(i=0;i<NUM;i++)
	{
		printf("%2d ", buf[i] );
	}
	printf("\n");
}

double getseconds( struct timeval last, struct timeval current) {
  int sec, usec;

  sec = current.tv_sec - last.tv_sec;
  usec = current.tv_usec - last.tv_usec;
  return ( (double)sec + usec*((double)(1e-6)) );
}

typedef void (*SHFUNC)(uint8_t*, const uint8_t*, size_t );

int main(int argc, char*argv[])
{
  SHFUNC sse2_sh_funcs[] = {
    shuffle2,
    shuffle4,
    shuffle8,
    shuffle16
  };
  SHFUNC sse2_unsh_funcs[] = {
    unshuffle2,
    unshuffle4,
    unshuffle8,
    unshuffle16
  };
  SHFUNC avx2_sh_funcs[] = {
    shuffle2_AVX2,
    shuffle4_AVX2,
    shuffle8_AVX2,
    shuffle16_AVX2
  };
  SHFUNC avx2_unsh_funcs[] = {
    unshuffle2_AVX2,
    unshuffle4_AVX2,
    unshuffle8_AVX2,
    unshuffle16_AVX2
  };

  SHFUNC SF, SF2, UF, UF2;
  srand (time(NULL));
  int i;
  double tm, pretm;
  struct timeval stm, etm;
  int index = 2, funcidx;
  int unalign = 0;
  uint8_t sum = 0;
  uint8_t *x, *y, *y2;

  int opt ;
  while ((opt = getopt(argc, argv, "b:i:s:a:")) != -1) {
    switch(opt)
    {
      case 'b':
        index = atoi(optarg);
        if( index!=2 && index!=4 && index!=8 && index!=16)
        {
          printf("Bytesoftype must be one of 2,4,8,16, not %d \n", index);
          return 0;
        }
        break;
      case 'i':
        ITER = atoi(optarg);
        if( ITER<=0 )
        {
          printf("Number of iterations should be larger than 0\n");
          return 0;
        }
        break;
      case 's':
        NUM = atoi(optarg);
        if( NUM<256 || (NUM & (NUM-1))!=0 )
        {
          printf("Block size should be a power of 2 and larger or equal than 256 bytes. \n");
          return 0;
        }
        break;
      case 'a':
        unalign = atoi(optarg);
        if( unalign<0 || unalign>=64 )
        {
          printf("Unalign should be smaller than 64\n");
          return 0;
        }
        break;
      default:
         printf("Unkown option -%c \n", opt);
         printf("Usage: %s [-b bytesoftype] [-i iterations] [-s blocksize] \n", argv[0]);
         return 0;
    }
  }

  switch( index)
  {
    case 2:
      funcidx = 0;
      break;
    case 4:
      funcidx = 1;
      break;
    case 8:
      funcidx = 2;
      break;
    case 16:
      funcidx = 3;
      break;
    default:
      printf("Bytesoftype must be one of 2,4,8,16 \n");
      return 0;
  }

  printf("Execution with bytesoftype=%d iterations=%d sizeofblock=%d unalign=%d \n", index, ITER, NUM, unalign);



  y = (uint8_t*)_mm_malloc(NUM+64, 64) + unalign;
  x = (uint8_t*)_mm_malloc(NUM+64, 64) + unalign;
  y2 =(uint8_t*)_mm_malloc(NUM+64, 64) + unalign;
  
  SF = sse2_sh_funcs[funcidx];
  SF2 = avx2_sh_funcs[funcidx];
  UF = sse2_unsh_funcs[funcidx];
  UF2 = avx2_unsh_funcs[funcidx];

  for(i=0;i<NUM;i++)
  {
    x[i] = (uint8_t)(rand()%256);//i/16;//i%16;//i+1;
    y[i] = 0;
    y2[i] = 0;
  }

  gettimeofday(&stm,NULL);
  for( i=0;i<ITER;i++)
  {
    SF(y,x,NUM); 
  }
  gettimeofday(&etm, NULL);
  tm = getseconds(stm, etm);
  pretm = tm;
  printf("SSE2 time=%0.3f \n", tm);
  
  gettimeofday(&stm,NULL);
  for( i=0;i<ITER;i++)
  {
    SF2(y2, x, NUM);
  }
  gettimeofday(&etm, NULL);
  tm = getseconds(stm, etm);
  printf("AVX2 time=%0.3f ratio=%0.3f \n", tm, pretm/tm);

#if 1
  for( i=0;i<NUM;i++ )
  {
    if( y[i]!=y2[i] )
    {
      printf("Function error!\n");
      dump_data(y, NUM);
      dump_data(y2, NUM);
      break;
    }
    if( i==NUM-1 )
    {
      printf("Functional OK! \n");
    }
  }
#endif
  
#if 1
  gettimeofday(&stm,NULL);
  for( i=0;i<ITER;i++)
  {
    UF(x,y,NUM);
  }
  gettimeofday(&etm, NULL);
  tm = getseconds(stm, etm);
  pretm = tm;
  printf("SSE2 unshuffle time=%0.3f \n", tm);
  
  gettimeofday(&stm,NULL);
  for( i=0;i<ITER;i++)
  {
    UF2(y2, y, NUM);
  }
  gettimeofday(&etm, NULL);
  tm = getseconds(stm, etm);
  printf("AVX2 unshuffle time=%0.3f ratio=%0.3f \n",  tm, pretm/tm);
  
#if 1
  for( i=0;i<NUM;i++ )
  {
    if( x[i]!=y2[i] )
    {
      printf("Function error!\n");
      dump_data(x, NUM);
      dump_data(y2, NUM);
      break;
    }
    if( i==NUM-1 )
    {
      printf("Functional OK! \n");
    }
  }
#endif
#endif

  return 0;

}
