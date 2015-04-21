#include <stdint.h>
/* #pragma message "Using AVX2 version shuffle/unshuffle" */

#include <immintrin.h>

/* Routine optimized for shuffling a buffer for a type size of 2 bytes. */
void
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
      _mm256_store_si256((__m256i*)(dest+j*32), d[0]);
      c[0] = _mm256_blend_epi32(c[0], c[1], 0x0F);
      d[1] = _mm256_permute4x64_epi64( c[0], 0x4E);
      _mm256_store_si256((__m256i*)(dest+j*32+(size>>1)), d[1]);
    }
    dest += 128;
  }
}


/* Routine optimized for shuffling a buffer for a type size of 4 bytes. */
void
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


/* Routine optimized for shuffling a buffer for a type size of 8 bytes. */
void
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


/* Routine optimized for shuffling a buffer for a type size of 16 bytes. */
void
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

    for (k = 0; k < 16; k++) {
      ymm0[k] = _mm256_permute4x64_epi64( ymm0[k], 0xD8);
      ymm0[k] = _mm256_shuffle_epi8( ymm0[k], shmask);
    }

    /* Store the result vectors */
    for (k = 0; k < 16; k++) {
      ((__m256i *)dest)[k*numof16belem+i] = ymm0[k];
    }
  }
}


/* Routine optimized for unshuffling a buffer for a type size of 2 bytes. */
void
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


/* Routine optimized for unshuffling a buffer for a type size of 4 bytes. */
void
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


/* Routine optimized for unshuffling a buffer for a type size of 8 bytes. */
void
unshuffle8_AVX2(uint8_t* dest, const uint8_t* orig, size_t size)
{
  size_t i, j, k;
  size_t neblock, numof16belem;
  __m256i ymm0[8], ymm1[8];

  neblock = size / 8;
  numof16belem = neblock/32;
  for (i = 0, k = 0; i < numof16belem; i++, k += 8) {
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


/* Routine optimized for unshuffling a buffer for a type size of 16 bytes. */
void
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

