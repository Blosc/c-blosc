/*********************************************************************
Blosc - Blocked Suffling and Compression Library

Author: Francesc Alted <francesc@blosc.org>
Creation date: 2009-05-20

See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include <stdio.h>
#include <string.h>
#include "shuffle.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#include "win32/stdint-windows.h"
#define __SSE2__          /* Windows does not define this by default */
#else
#include <stdint.h>
#include <inttypes.h>
#endif  /* _WIN32 */


/* The non-SSE2 versions of shuffle and unshuffle */

/* Shuffle a block.  This can never fail. */
static void _shuffle(size_t bytesoftype, size_t blocksize,
	const uint8_t* _src, uint8_t* _dest)
{
	size_t i, j, neblock, leftover;

	/* Non-optimized shuffle */
	neblock = blocksize / bytesoftype;  /* Number of elements in a block */
	for (j = 0; j < bytesoftype; j++) {
		for (i = 0; i < neblock; i++) {
			_dest[j*neblock + i] = _src[i*bytesoftype + j];
		}
	}
	leftover = blocksize % bytesoftype;
	memcpy(_dest + neblock*bytesoftype, _src + neblock*bytesoftype, leftover);
}

/* Unshuffle a block.  This can never fail. */
static void _unshuffle(size_t bytesoftype, size_t blocksize,
	const uint8_t* _src, uint8_t* _dest)
{
	size_t i, j, neblock, leftover;

	/* Non-optimized unshuffle */
	neblock = blocksize / bytesoftype;  /* Number of elements in a block */
	for (i = 0; i < neblock; i++) {
		for (j = 0; j < bytesoftype; j++) {
			_dest[i*bytesoftype + j] = _src[j*neblock + i];
		}
	}
	leftover = blocksize % bytesoftype;
	memcpy(_dest + neblock*bytesoftype, _src + neblock*bytesoftype, leftover);
}


#ifdef __SSE2__

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
	size_t i, j;
	const size_t numof16belem = size / (2 * sizeof(__m128i));


	for (i = 0, j = 0; i < numof16belem; i++, j += 2) {
		__m128i xmm0[2], xmm1[2];

		/* Fetch and transpose bytes, words and double words in groups of
		32 bytes */
		xmm0[0] = _mm_loadu_si128((__m128i*)(src + (j + 0) * sizeof(__m128i)));
		xmm0[0] = _mm_shufflelo_epi16(xmm0[0], 0xd8);
		xmm0[0] = _mm_shufflehi_epi16(xmm0[0], 0xd8);
		xmm0[0] = _mm_shuffle_epi32(xmm0[0], 0xd8);
		xmm1[0] = _mm_shuffle_epi32(xmm0[0], 0x4e);
		xmm0[0] = _mm_unpacklo_epi8(xmm0[0], xmm1[0]);
		xmm0[0] = _mm_shuffle_epi32(xmm0[0], 0xd8);
		xmm1[0] = _mm_shuffle_epi32(xmm0[0], 0x4e);
		xmm0[0] = _mm_unpacklo_epi16(xmm0[0], xmm1[0]);
		xmm0[0] = _mm_shuffle_epi32(xmm0[0], 0xd8);

		xmm0[1] = _mm_loadu_si128((__m128i*)(src + (j + 1) * sizeof(__m128i)));
		xmm0[1] = _mm_shufflelo_epi16(xmm0[1], 0xd8);
		xmm0[1] = _mm_shufflehi_epi16(xmm0[1], 0xd8);
		xmm0[1] = _mm_shuffle_epi32(xmm0[1], 0xd8);
		xmm1[1] = _mm_shuffle_epi32(xmm0[1], 0x4e);
		xmm0[1] = _mm_unpacklo_epi8(xmm0[1], xmm1[1]);
		xmm0[1] = _mm_shuffle_epi32(xmm0[1], 0xd8);
		xmm1[1] = _mm_shuffle_epi32(xmm0[1], 0x4e);
		xmm0[1] = _mm_unpacklo_epi16(xmm0[1], xmm1[1]);
		xmm0[1] = _mm_shuffle_epi32(xmm0[1], 0xd8);

		/* Transpose quad words */
		xmm1[0] = _mm_unpacklo_epi64(xmm0[0], xmm0[1]);
		xmm1[1] = _mm_unpackhi_epi64(xmm0[0], xmm0[1]);

		/* Store the result vectors */
		((__m128i *)dest)[0 * numof16belem + i] = xmm1[0];
		((__m128i *)dest)[1 * numof16belem + i] = xmm1[1];
	}
}


/* Routine optimized for shuffling a buffer for a type size of 4 bytes. */
static void
shuffle4(uint8_t* dest, const uint8_t* src, size_t size)
{
	size_t i, j;
	const size_t numof16belem = size / (4 * sizeof(__m128i));

	for (i = 0, j = 0; i < numof16belem; i++, j += 4) {
		__m128i xmm0[4], xmm1[4];

		/* Fetch and transpose bytes and words in groups of 64 bytes */
		xmm0[0] = _mm_loadu_si128((__m128i*)(src + (j + 0) * sizeof(__m128i)));
		xmm1[0] = _mm_shuffle_epi32(xmm0[0], 0xd8);
		xmm0[0] = _mm_shuffle_epi32(xmm0[0], 0x8d);
		xmm0[0] = _mm_unpacklo_epi8(xmm1[0], xmm0[0]);
		xmm1[0] = _mm_shuffle_epi32(xmm0[0], 0x04e);
		xmm0[0] = _mm_unpacklo_epi16(xmm0[0], xmm1[0]);

		xmm0[1] = _mm_loadu_si128((__m128i*)(src + (j + 1) * sizeof(__m128i)));
		xmm1[1] = _mm_shuffle_epi32(xmm0[1], 0xd8);
		xmm0[1] = _mm_shuffle_epi32(xmm0[1], 0x8d);
		xmm0[1] = _mm_unpacklo_epi8(xmm1[1], xmm0[1]);
		xmm1[1] = _mm_shuffle_epi32(xmm0[1], 0x04e);
		xmm0[1] = _mm_unpacklo_epi16(xmm0[1], xmm1[1]);

		xmm0[2] = _mm_loadu_si128((__m128i*)(src + (j + 2) * sizeof(__m128i)));
		xmm1[2] = _mm_shuffle_epi32(xmm0[2], 0xd8);
		xmm0[2] = _mm_shuffle_epi32(xmm0[2], 0x8d);
		xmm0[2] = _mm_unpacklo_epi8(xmm1[2], xmm0[2]);
		xmm1[2] = _mm_shuffle_epi32(xmm0[2], 0x04e);
		xmm0[2] = _mm_unpacklo_epi16(xmm0[2], xmm1[2]);

		xmm0[3] = _mm_loadu_si128((__m128i*)(src + (j + 3) * sizeof(__m128i)));
		xmm1[3] = _mm_shuffle_epi32(xmm0[3], 0xd8);
		xmm0[3] = _mm_shuffle_epi32(xmm0[3], 0x8d);
		xmm0[3] = _mm_unpacklo_epi8(xmm1[3], xmm0[3]);
		xmm1[3] = _mm_shuffle_epi32(xmm0[3], 0x04e);
		xmm0[3] = _mm_unpacklo_epi16(xmm0[3], xmm1[3]);

		/* Transpose double words */
		xmm1[0] = _mm_unpacklo_epi32(xmm0[0], xmm0[1]);
		xmm1[1] = _mm_unpackhi_epi32(xmm0[0], xmm0[1]);

		xmm1[2] = _mm_unpacklo_epi32(xmm0[2], xmm0[3]);
		xmm1[3] = _mm_unpackhi_epi32(xmm0[2], xmm0[3]);

		/* Transpose quad words */
		xmm0[0] = _mm_unpacklo_epi64(xmm1[0], xmm1[2]);
		xmm0[1] = _mm_unpackhi_epi64(xmm1[0], xmm1[2]);

		xmm0[2] = _mm_unpacklo_epi64(xmm1[1], xmm1[3]);
		xmm0[3] = _mm_unpackhi_epi64(xmm1[1], xmm1[3]);

		/* Store the result vectors */
		((__m128i *)dest)[0 * numof16belem + i] = xmm0[0];
		((__m128i *)dest)[1 * numof16belem + i] = xmm0[1];
		((__m128i *)dest)[2 * numof16belem + i] = xmm0[2];
		((__m128i *)dest)[3 * numof16belem + i] = xmm0[3];
	}
}


/* Routine optimized for shuffling a buffer for a type size of 8 bytes. */
static void
shuffle8(uint8_t* dest, const uint8_t* src, size_t size)
{
	size_t i, j;
	const size_t numof16belem = size / (8 * sizeof(__m128i));

	for (i = 0, j = 0; i < numof16belem; i++, j += 8) {
		__m128i xmm0[8], xmm1[8];

		/* Fetch and transpose bytes in groups of 128 bytes */
		xmm0[0] = _mm_loadu_si128((__m128i*)(src + (j + 0) * sizeof(__m128i)));
		xmm1[0] = _mm_shuffle_epi32(xmm0[0], 0x4e);
		xmm1[0] = _mm_unpacklo_epi8(xmm0[0], xmm1[0]);

		xmm0[1] = _mm_loadu_si128((__m128i*)(src + (j + 1) * sizeof(__m128i)));
		xmm1[1] = _mm_shuffle_epi32(xmm0[1], 0x4e);
		xmm1[1] = _mm_unpacklo_epi8(xmm0[1], xmm1[1]);

		xmm0[2] = _mm_loadu_si128((__m128i*)(src + (j + 2) * sizeof(__m128i)));
		xmm1[2] = _mm_shuffle_epi32(xmm0[2], 0x4e);
		xmm1[2] = _mm_unpacklo_epi8(xmm0[2], xmm1[2]);

		xmm0[3] = _mm_loadu_si128((__m128i*)(src + (j + 3) * sizeof(__m128i)));
		xmm1[3] = _mm_shuffle_epi32(xmm0[3], 0x4e);
		xmm1[3] = _mm_unpacklo_epi8(xmm0[3], xmm1[3]);

		xmm0[4] = _mm_loadu_si128((__m128i*)(src + (j + 4) * sizeof(__m128i)));
		xmm1[4] = _mm_shuffle_epi32(xmm0[4], 0x4e);
		xmm1[4] = _mm_unpacklo_epi8(xmm0[4], xmm1[4]);

		xmm0[5] = _mm_loadu_si128((__m128i*)(src + (j + 5) * sizeof(__m128i)));
		xmm1[5] = _mm_shuffle_epi32(xmm0[5], 0x4e);
		xmm1[5] = _mm_unpacklo_epi8(xmm0[5], xmm1[5]);

		xmm0[6] = _mm_loadu_si128((__m128i*)(src + (j + 6) * sizeof(__m128i)));
		xmm1[6] = _mm_shuffle_epi32(xmm0[6], 0x4e);
		xmm1[6] = _mm_unpacklo_epi8(xmm0[6], xmm1[6]);

		xmm0[7] = _mm_loadu_si128((__m128i*)(src + (j + 7) * sizeof(__m128i)));
		xmm1[7] = _mm_shuffle_epi32(xmm0[7], 0x4e);
		xmm1[7] = _mm_unpacklo_epi8(xmm0[7], xmm1[7]);

		/* Transpose words */
		xmm0[0] = _mm_unpacklo_epi16(xmm1[0], xmm1[1]);
		xmm0[1] = _mm_unpackhi_epi16(xmm1[0], xmm1[1]);

		xmm0[2] = _mm_unpacklo_epi16(xmm1[2], xmm1[3]);
		xmm0[3] = _mm_unpackhi_epi16(xmm1[2], xmm1[3]);

		xmm0[4] = _mm_unpacklo_epi16(xmm1[4], xmm1[5]);
		xmm0[5] = _mm_unpackhi_epi16(xmm1[4], xmm1[5]);

		xmm0[6] = _mm_unpacklo_epi16(xmm1[6], xmm1[7]);
		xmm0[7] = _mm_unpackhi_epi16(xmm1[6], xmm1[7]);

		/* Transpose double words */
		xmm1[0] = _mm_unpacklo_epi32(xmm0[0], xmm0[2]);
		xmm1[1] = _mm_unpackhi_epi32(xmm0[0], xmm0[2]);

		xmm1[2] = _mm_unpacklo_epi32(xmm0[1], xmm0[3]);
		xmm1[3] = _mm_unpackhi_epi32(xmm0[1], xmm0[3]);

		xmm1[4] = _mm_unpacklo_epi32(xmm0[4], xmm0[6]);
		xmm1[5] = _mm_unpackhi_epi32(xmm0[4], xmm0[6]);

		xmm1[6] = _mm_unpacklo_epi32(xmm0[5], xmm0[7]);
		xmm1[7] = _mm_unpackhi_epi32(xmm0[5], xmm0[7]);

		/* Transpose quad words */
		xmm0[0] = _mm_unpacklo_epi64(xmm1[0], xmm1[4]);
		xmm0[1] = _mm_unpackhi_epi64(xmm1[0], xmm1[4]);

		xmm0[2] = _mm_unpacklo_epi64(xmm1[1], xmm1[5]);
		xmm0[3] = _mm_unpackhi_epi64(xmm1[1], xmm1[5]);

		xmm0[4] = _mm_unpacklo_epi64(xmm1[2], xmm1[6]);
		xmm0[5] = _mm_unpackhi_epi64(xmm1[2], xmm1[6]);

		xmm0[6] = _mm_unpacklo_epi64(xmm1[3], xmm1[7]);
		xmm0[7] = _mm_unpackhi_epi64(xmm1[3], xmm1[7]);

		/* Store the result vectors */
		((__m128i *)dest)[0 * numof16belem + i] = xmm0[0];
		((__m128i *)dest)[1 * numof16belem + i] = xmm0[1];
		((__m128i *)dest)[2 * numof16belem + i] = xmm0[2];
		((__m128i *)dest)[3 * numof16belem + i] = xmm0[3];
		((__m128i *)dest)[4 * numof16belem + i] = xmm0[4];
		((__m128i *)dest)[5 * numof16belem + i] = xmm0[5];
		((__m128i *)dest)[6 * numof16belem + i] = xmm0[6];
		((__m128i *)dest)[7 * numof16belem + i] = xmm0[7];
	}
}


/* Routine optimized for shuffling a buffer for a type size of 16 bytes. */
static void
shuffle16(uint8_t* dest, const uint8_t* src, size_t size)
{
	size_t i, j;
	const size_t numof16belem = size / (16 * sizeof(__m128i));

	for (i = 0, j = 0; i < numof16belem; i++, j += 16) {
		__m128i xmm0[16], xmm1[16];

		/* Fetch elements in groups of 256 bytes */
		xmm0[0] = _mm_loadu_si128(((__m128i*)src) + (j + 0));
		xmm0[1] = _mm_loadu_si128(((__m128i*)src) + (j + 1));
		xmm0[2] = _mm_loadu_si128(((__m128i*)src) + (j + 2));
		xmm0[3] = _mm_loadu_si128(((__m128i*)src) + (j + 3));
		xmm0[4] = _mm_loadu_si128(((__m128i*)src) + (j + 4));
		xmm0[5] = _mm_loadu_si128(((__m128i*)src) + (j + 5));
		xmm0[6] = _mm_loadu_si128(((__m128i*)src) + (j + 6));
		xmm0[7] = _mm_loadu_si128(((__m128i*)src) + (j + 7));
		xmm0[8] = _mm_loadu_si128(((__m128i*)src) + (j + 8));
		xmm0[9] = _mm_loadu_si128(((__m128i*)src) + (j + 9));
		xmm0[10] = _mm_loadu_si128(((__m128i*)src) + (j + 10));
		xmm0[11] = _mm_loadu_si128(((__m128i*)src) + (j + 11));
		xmm0[12] = _mm_loadu_si128(((__m128i*)src) + (j + 12));
		xmm0[13] = _mm_loadu_si128(((__m128i*)src) + (j + 13));
		xmm0[14] = _mm_loadu_si128(((__m128i*)src) + (j + 14));
		xmm0[15] = _mm_loadu_si128(((__m128i*)src) + (j + 15));

		/* Transpose bytes */
		xmm1[0] = _mm_unpacklo_epi8(xmm0[0], xmm0[1]);
		xmm1[1] = _mm_unpackhi_epi8(xmm0[0], xmm0[1]);

		xmm1[2] = _mm_unpacklo_epi8(xmm0[2], xmm0[3]);
		xmm1[3] = _mm_unpackhi_epi8(xmm0[2], xmm0[3]);

		xmm1[4] = _mm_unpacklo_epi8(xmm0[4], xmm0[5]);
		xmm1[5] = _mm_unpackhi_epi8(xmm0[4], xmm0[5]);

		xmm1[6] = _mm_unpacklo_epi8(xmm0[6], xmm0[7]);
		xmm1[7] = _mm_unpackhi_epi8(xmm0[6], xmm0[7]);

		xmm1[8] = _mm_unpacklo_epi8(xmm0[8], xmm0[9]);
		xmm1[9] = _mm_unpackhi_epi8(xmm0[8], xmm0[9]);

		xmm1[10] = _mm_unpacklo_epi8(xmm0[10], xmm0[11]);
		xmm1[11] = _mm_unpackhi_epi8(xmm0[10], xmm0[11]);

		xmm1[12] = _mm_unpacklo_epi8(xmm0[12], xmm0[13]);
		xmm1[13] = _mm_unpackhi_epi8(xmm0[12], xmm0[13]);

		xmm1[14] = _mm_unpacklo_epi8(xmm0[14], xmm0[15]);
		xmm1[15] = _mm_unpackhi_epi8(xmm0[14], xmm0[15]);

		/* Transpose words */
		xmm0[0] = _mm_unpacklo_epi16(xmm1[0], xmm1[2]);
		xmm0[1] = _mm_unpackhi_epi16(xmm1[0], xmm1[2]);

		xmm0[2] = _mm_unpacklo_epi16(xmm1[1], xmm1[3]);
		xmm0[3] = _mm_unpackhi_epi16(xmm1[1], xmm1[3]);

		xmm0[4] = _mm_unpacklo_epi16(xmm1[4], xmm1[6]);
		xmm0[5] = _mm_unpackhi_epi16(xmm1[4], xmm1[6]);

		xmm0[6] = _mm_unpacklo_epi16(xmm1[5], xmm1[7]);
		xmm0[7] = _mm_unpackhi_epi16(xmm1[5], xmm1[7]);

		xmm0[8] = _mm_unpacklo_epi16(xmm1[8], xmm1[10]);
		xmm0[9] = _mm_unpackhi_epi16(xmm1[8], xmm1[10]);

		xmm0[10] = _mm_unpacklo_epi16(xmm1[9], xmm1[11]);
		xmm0[11] = _mm_unpackhi_epi16(xmm1[9], xmm1[11]);

		xmm0[12] = _mm_unpacklo_epi16(xmm1[12], xmm1[14]);
		xmm0[13] = _mm_unpackhi_epi16(xmm1[12], xmm1[14]);

		xmm0[14] = _mm_unpacklo_epi16(xmm1[13], xmm1[15]);
		xmm0[15] = _mm_unpackhi_epi16(xmm1[13], xmm1[15]);

		/* Transpose double words */
		xmm1[0] = _mm_unpacklo_epi32(xmm0[0], xmm0[4]);
		xmm1[1] = _mm_unpackhi_epi32(xmm0[0], xmm0[4]);

		xmm1[2] = _mm_unpacklo_epi32(xmm0[1], xmm0[5]);
		xmm1[3] = _mm_unpackhi_epi32(xmm0[1], xmm0[5]);

		xmm1[4] = _mm_unpacklo_epi32(xmm0[2], xmm0[6]);
		xmm1[5] = _mm_unpackhi_epi32(xmm0[2], xmm0[6]);

		xmm1[6] = _mm_unpacklo_epi32(xmm0[3], xmm0[7]);
		xmm1[7] = _mm_unpackhi_epi32(xmm0[3], xmm0[7]);

		xmm1[8] = _mm_unpacklo_epi32(xmm0[8], xmm0[12]);
		xmm1[9] = _mm_unpackhi_epi32(xmm0[8], xmm0[12]);

		xmm1[10] = _mm_unpacklo_epi32(xmm0[9], xmm0[13]);
		xmm1[11] = _mm_unpackhi_epi32(xmm0[9], xmm0[13]);

		xmm1[12] = _mm_unpacklo_epi32(xmm0[10], xmm0[14]);
		xmm1[13] = _mm_unpackhi_epi32(xmm0[10], xmm0[14]);

		xmm1[14] = _mm_unpacklo_epi32(xmm0[11], xmm0[15]);
		xmm1[15] = _mm_unpackhi_epi32(xmm0[11], xmm0[15]);

		/* Transpose quad words */
		xmm0[0] = _mm_unpacklo_epi64(xmm1[0], xmm1[8]);
		xmm0[1] = _mm_unpackhi_epi64(xmm1[0], xmm1[8]);

		xmm0[2] = _mm_unpacklo_epi64(xmm1[1], xmm1[9]);
		xmm0[3] = _mm_unpackhi_epi64(xmm1[1], xmm1[9]);

		xmm0[4] = _mm_unpacklo_epi64(xmm1[2], xmm1[10]);
		xmm0[5] = _mm_unpackhi_epi64(xmm1[2], xmm1[10]);

		xmm0[6] = _mm_unpacklo_epi64(xmm1[3], xmm1[11]);
		xmm0[7] = _mm_unpackhi_epi64(xmm1[3], xmm1[11]);

		xmm0[8] = _mm_unpacklo_epi64(xmm1[4], xmm1[12]);
		xmm0[9] = _mm_unpackhi_epi64(xmm1[4], xmm1[12]);

		xmm0[10] = _mm_unpacklo_epi64(xmm1[5], xmm1[13]);
		xmm0[11] = _mm_unpackhi_epi64(xmm1[5], xmm1[13]);

		xmm0[12] = _mm_unpacklo_epi64(xmm1[6], xmm1[14]);
		xmm0[13] = _mm_unpackhi_epi64(xmm1[6], xmm1[14]);

		xmm0[14] = _mm_unpacklo_epi64(xmm1[7], xmm1[15]);
		xmm0[15] = _mm_unpackhi_epi64(xmm1[7], xmm1[15]);

		/* Store the result vectors */
		_mm_storeu_si128(((__m128i*)dest) + (0 * numof16belem + i), xmm0[0]);
		_mm_storeu_si128(((__m128i*)dest) + (1 * numof16belem + i), xmm0[1]);
		_mm_storeu_si128(((__m128i*)dest) + (2 * numof16belem + i), xmm0[2]);
		_mm_storeu_si128(((__m128i*)dest) + (3 * numof16belem + i), xmm0[3]);
		_mm_storeu_si128(((__m128i*)dest) + (4 * numof16belem + i), xmm0[4]);
		_mm_storeu_si128(((__m128i*)dest) + (5 * numof16belem + i), xmm0[5]);
		_mm_storeu_si128(((__m128i*)dest) + (6 * numof16belem + i), xmm0[6]);
		_mm_storeu_si128(((__m128i*)dest) + (7 * numof16belem + i), xmm0[7]);
		_mm_storeu_si128(((__m128i*)dest) + (8 * numof16belem + i), xmm0[8]);
		_mm_storeu_si128(((__m128i*)dest) + (9 * numof16belem + i), xmm0[9]);
		_mm_storeu_si128(((__m128i*)dest) + (10 * numof16belem + i), xmm0[10]);
		_mm_storeu_si128(((__m128i*)dest) + (11 * numof16belem + i), xmm0[11]);
		_mm_storeu_si128(((__m128i*)dest) + (12 * numof16belem + i), xmm0[12]);
		_mm_storeu_si128(((__m128i*)dest) + (13 * numof16belem + i), xmm0[13]);
		_mm_storeu_si128(((__m128i*)dest) + (14 * numof16belem + i), xmm0[14]);
		_mm_storeu_si128(((__m128i*)dest) + (15 * numof16belem + i), xmm0[15]);
	}
}


/* Routine optimized for shuffling a buffer for a type size larger than 16 bytes. */
static void
shuffle_multipart(uint8_t* dest, const uint8_t* src, size_t size, size_t bytesoftype)
{
	size_t j;
	const size_t num_elements = size / bytesoftype;
	const lldiv_t vecs_per_el = lldiv(bytesoftype, sizeof(__m128i));

	for (j = 0; j < num_elements; j += sizeof(__m128i)) {
		/* Advance the offset into the type by the vector size (in bytes), unless this is
		the initial iteration and the type size is not a multiple of the vector size.
		In that case, only advance by the number of bytes necessary so that the number
		of remaining bytes in the type will be a multiple of the vector size. */
		size_t offset_into_type;
		for (offset_into_type = 0; offset_into_type < bytesoftype;
			offset_into_type += (offset_into_type == 0 && vecs_per_el.rem > 0 ? vecs_per_el.rem : sizeof(__m128i))) {
			__m128i xmm0[16], xmm1[16];

			/* Fetch elements in groups of 256 bytes */
			const uint8_t* const src_with_offset = src + offset_into_type;
			xmm0[0] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 0) * bytesoftype));
			xmm0[1] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 1) * bytesoftype));
			xmm0[2] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 2) * bytesoftype));
			xmm0[3] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 3) * bytesoftype));
			xmm0[4] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 4) * bytesoftype));
			xmm0[5] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 5) * bytesoftype));
			xmm0[6] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 6) * bytesoftype));
			xmm0[7] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 7) * bytesoftype));
			xmm0[8] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 8) * bytesoftype));
			xmm0[9] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 9) * bytesoftype));
			xmm0[10] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 10) * bytesoftype));
			xmm0[11] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 11) * bytesoftype));
			xmm0[12] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 12) * bytesoftype));
			xmm0[13] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 13) * bytesoftype));
			xmm0[14] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 14) * bytesoftype));
			xmm0[15] = _mm_loadu_si128((__m128i*)(src_with_offset + (j + 15) * bytesoftype));

			/* Transpose bytes */
			xmm1[0] = _mm_unpacklo_epi8(xmm0[0], xmm0[1]);
			xmm1[1] = _mm_unpackhi_epi8(xmm0[0], xmm0[1]);

			xmm1[2] = _mm_unpacklo_epi8(xmm0[2], xmm0[3]);
			xmm1[3] = _mm_unpackhi_epi8(xmm0[2], xmm0[3]);

			xmm1[4] = _mm_unpacklo_epi8(xmm0[4], xmm0[5]);
			xmm1[5] = _mm_unpackhi_epi8(xmm0[4], xmm0[5]);

			xmm1[6] = _mm_unpacklo_epi8(xmm0[6], xmm0[7]);
			xmm1[7] = _mm_unpackhi_epi8(xmm0[6], xmm0[7]);

			xmm1[8] = _mm_unpacklo_epi8(xmm0[8], xmm0[9]);
			xmm1[9] = _mm_unpackhi_epi8(xmm0[8], xmm0[9]);

			xmm1[10] = _mm_unpacklo_epi8(xmm0[10], xmm0[11]);
			xmm1[11] = _mm_unpackhi_epi8(xmm0[10], xmm0[11]);

			xmm1[12] = _mm_unpacklo_epi8(xmm0[12], xmm0[13]);
			xmm1[13] = _mm_unpackhi_epi8(xmm0[12], xmm0[13]);

			xmm1[14] = _mm_unpacklo_epi8(xmm0[14], xmm0[15]);
			xmm1[15] = _mm_unpackhi_epi8(xmm0[14], xmm0[15]);

			/* Transpose words */
			xmm0[0] = _mm_unpacklo_epi16(xmm1[0], xmm1[2]);
			xmm0[1] = _mm_unpackhi_epi16(xmm1[0], xmm1[2]);

			xmm0[2] = _mm_unpacklo_epi16(xmm1[1], xmm1[3]);
			xmm0[3] = _mm_unpackhi_epi16(xmm1[1], xmm1[3]);

			xmm0[4] = _mm_unpacklo_epi16(xmm1[4], xmm1[6]);
			xmm0[5] = _mm_unpackhi_epi16(xmm1[4], xmm1[6]);

			xmm0[6] = _mm_unpacklo_epi16(xmm1[5], xmm1[7]);
			xmm0[7] = _mm_unpackhi_epi16(xmm1[5], xmm1[7]);

			xmm0[8] = _mm_unpacklo_epi16(xmm1[8], xmm1[10]);
			xmm0[9] = _mm_unpackhi_epi16(xmm1[8], xmm1[10]);

			xmm0[10] = _mm_unpacklo_epi16(xmm1[9], xmm1[11]);
			xmm0[11] = _mm_unpackhi_epi16(xmm1[9], xmm1[11]);

			xmm0[12] = _mm_unpacklo_epi16(xmm1[12], xmm1[14]);
			xmm0[13] = _mm_unpackhi_epi16(xmm1[12], xmm1[14]);

			xmm0[14] = _mm_unpacklo_epi16(xmm1[13], xmm1[15]);
			xmm0[15] = _mm_unpackhi_epi16(xmm1[13], xmm1[15]);

			/* Transpose double words */
			xmm1[0] = _mm_unpacklo_epi32(xmm0[0], xmm0[4]);
			xmm1[1] = _mm_unpackhi_epi32(xmm0[0], xmm0[4]);

			xmm1[2] = _mm_unpacklo_epi32(xmm0[1], xmm0[5]);
			xmm1[3] = _mm_unpackhi_epi32(xmm0[1], xmm0[5]);

			xmm1[4] = _mm_unpacklo_epi32(xmm0[2], xmm0[6]);
			xmm1[5] = _mm_unpackhi_epi32(xmm0[2], xmm0[6]);

			xmm1[6] = _mm_unpacklo_epi32(xmm0[3], xmm0[7]);
			xmm1[7] = _mm_unpackhi_epi32(xmm0[3], xmm0[7]);

			xmm1[8] = _mm_unpacklo_epi32(xmm0[8], xmm0[12]);
			xmm1[9] = _mm_unpackhi_epi32(xmm0[8], xmm0[12]);

			xmm1[10] = _mm_unpacklo_epi32(xmm0[9], xmm0[13]);
			xmm1[11] = _mm_unpackhi_epi32(xmm0[9], xmm0[13]);

			xmm1[12] = _mm_unpacklo_epi32(xmm0[10], xmm0[14]);
			xmm1[13] = _mm_unpackhi_epi32(xmm0[10], xmm0[14]);

			xmm1[14] = _mm_unpacklo_epi32(xmm0[11], xmm0[15]);
			xmm1[15] = _mm_unpackhi_epi32(xmm0[11], xmm0[15]);

			/* Transpose quad words */
			xmm0[0] = _mm_unpacklo_epi64(xmm1[0], xmm1[8]);
			xmm0[1] = _mm_unpackhi_epi64(xmm1[0], xmm1[8]);

			xmm0[2] = _mm_unpacklo_epi64(xmm1[1], xmm1[9]);
			xmm0[3] = _mm_unpackhi_epi64(xmm1[1], xmm1[9]);

			xmm0[4] = _mm_unpacklo_epi64(xmm1[2], xmm1[10]);
			xmm0[5] = _mm_unpackhi_epi64(xmm1[2], xmm1[10]);

			xmm0[6] = _mm_unpacklo_epi64(xmm1[3], xmm1[11]);
			xmm0[7] = _mm_unpackhi_epi64(xmm1[3], xmm1[11]);

			xmm0[8] = _mm_unpacklo_epi64(xmm1[4], xmm1[12]);
			xmm0[9] = _mm_unpackhi_epi64(xmm1[4], xmm1[12]);

			xmm0[10] = _mm_unpacklo_epi64(xmm1[5], xmm1[13]);
			xmm0[11] = _mm_unpackhi_epi64(xmm1[5], xmm1[13]);

			xmm0[12] = _mm_unpacklo_epi64(xmm1[6], xmm1[14]);
			xmm0[13] = _mm_unpackhi_epi64(xmm1[6], xmm1[14]);

			xmm0[14] = _mm_unpacklo_epi64(xmm1[7], xmm1[15]);
			xmm0[15] = _mm_unpackhi_epi64(xmm1[7], xmm1[15]);

			/* Store the result vectors */
			uint8_t* const dest_for_jth_element = dest + j;
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 0))), xmm0[0]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 1))), xmm0[1]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 2))), xmm0[2]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 3))), xmm0[3]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 4))), xmm0[4]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 5))), xmm0[5]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 6))), xmm0[6]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 7))), xmm0[7]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 8))), xmm0[8]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 9))), xmm0[9]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 10))), xmm0[10]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 11))), xmm0[11]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 12))), xmm0[12]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 13))), xmm0[13]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 14))), xmm0[14]);
			_mm_storeu_si128((__m128i*)(dest_for_jth_element + (num_elements * (offset_into_type + 15))), xmm0[15]);
		}
	}
}


/* Shuffle a block.  This can never fail. */
void shuffle(size_t bytesoftype, size_t blocksize,
	const uint8_t* _src, uint8_t* _dest) {
	int unaligned_dest = (int)((uintptr_t)_dest % sizeof(__m128i));
	int multiple_of_block = (blocksize % (sizeof(__m128i) * bytesoftype)) == 0;
	int too_small = (blocksize < 256);

	if (unaligned_dest || !multiple_of_block || too_small) {
		/* _dest buffer is not aligned, not multiple of the vectorization size
		* or is too small.  Call the non-sse2 version. */
		_shuffle(bytesoftype, blocksize, _src, _dest);
		return;
	}

	/* Optimized shuffle */
	/* The buffer must be aligned on a 16 bytes boundary, have a power */
	/* of 2 size and be larger or equal than 256 bytes. */
	switch (bytesoftype)
	{
	case 2:
		shuffle2(_dest, _src, blocksize);
		break;
	case 4:
		shuffle4(_dest, _src, blocksize);
		break;
	case 8:
		shuffle8(_dest, _src, blocksize);
		break;
	case 16:
		shuffle16(_dest, _src, blocksize);
		break;
	default:
		if (bytesoftype > sizeof(__m128i)) {
			shuffle_multipart(_dest, _src, blocksize, bytesoftype);
		}
		else {
			/* Non-optimized shuffle */
			_shuffle(bytesoftype, blocksize, _src, _dest);
		}
		break;
	}
}


/* Routine optimized for unshuffling a buffer for a type size of 2 bytes. */
static void
unshuffle2(uint8_t* dest, const uint8_t* orig, size_t size)
{
	size_t i, j;
	const size_t numof16belem = size / (2 * sizeof(__m128i));

	for (i = 0, j = 0; i < numof16belem; i++, j += 2) {
		__m128i xmm1[2], xmm2[2];

		/* Load the first 32 bytes in 2 XMM registers */
		xmm1[0] = ((__m128i *)orig)[0 * numof16belem + i];
		xmm1[1] = ((__m128i *)orig)[1 * numof16belem + i];
		/* Unshuffle bytes */
		/* Compute the low 32 bytes */
		xmm2[0] = _mm_unpacklo_epi8(xmm1[0], xmm1[1]);
		/* Compute the hi 32 bytes */
		xmm2[1] = _mm_unpackhi_epi8(xmm1[0], xmm1[1]);
		/* Store the result vectors in proper order */
		((__m128i *)dest)[j + 0] = xmm2[0];
		((__m128i *)dest)[j + 1] = xmm2[1];
	}
}


/* Routine optimized for unshuffling a buffer for a type size of 4 bytes. */
static void
unshuffle4(uint8_t* dest, const uint8_t* orig, size_t size)
{
	size_t i, j;
	const size_t numof16belem = size / (4 * sizeof(__m128i));

	for (i = 0, j = 0; i < numof16belem; i++, j += 4) {
		__m128i xmm0[4], xmm1[4];

		/* Load the first 64 bytes in 4 XMM registers */
		xmm0[0] = ((__m128i *)orig)[0 * numof16belem + i];
		xmm0[1] = ((__m128i *)orig)[1 * numof16belem + i];
		xmm0[2] = ((__m128i *)orig)[2 * numof16belem + i];
		xmm0[3] = ((__m128i *)orig)[3 * numof16belem + i];

		/* Unshuffle bytes */
		xmm1[0] = _mm_unpacklo_epi8(xmm0[0], xmm0[1]);
		xmm1[2] = _mm_unpackhi_epi8(xmm0[0], xmm0[1]);

		xmm1[1] = _mm_unpacklo_epi8(xmm0[2], xmm0[3]);
		xmm1[3] = _mm_unpackhi_epi8(xmm0[2], xmm0[3]);

		/* Unshuffle 2-byte words */
		xmm0[0] = _mm_unpacklo_epi16(xmm1[0], xmm1[1]);
		xmm0[2] = _mm_unpackhi_epi16(xmm1[0], xmm1[1]);

		xmm0[1] = _mm_unpacklo_epi16(xmm1[2], xmm1[3]);
		xmm0[3] = _mm_unpackhi_epi16(xmm1[2], xmm1[3]);

		/* Store the result vectors in proper order */
		((__m128i *)dest)[j + 0] = xmm0[0];
		((__m128i *)dest)[j + 1] = xmm0[2];
		((__m128i *)dest)[j + 2] = xmm0[1];
		((__m128i *)dest)[j + 3] = xmm0[3];
	}
}


/* Routine optimized for unshuffling a buffer for a type size of 8 bytes. */
static void
unshuffle8(uint8_t* dest, const uint8_t* orig, size_t size)
{
	size_t i, j;
	const size_t numof16belem = size / (8 * sizeof(__m128i));

	for (i = 0, j = 0; i < numof16belem; i++, j += 8) {
		__m128i xmm0[8], xmm1[8];

		/* Load the first 64 bytes in 8 XMM registers */
		xmm0[0] = ((__m128i *)orig)[0 * numof16belem + i];
		xmm0[1] = ((__m128i *)orig)[1 * numof16belem + i];
		xmm0[2] = ((__m128i *)orig)[2 * numof16belem + i];
		xmm0[3] = ((__m128i *)orig)[3 * numof16belem + i];
		xmm0[4] = ((__m128i *)orig)[4 * numof16belem + i];
		xmm0[5] = ((__m128i *)orig)[5 * numof16belem + i];
		xmm0[6] = ((__m128i *)orig)[6 * numof16belem + i];
		xmm0[7] = ((__m128i *)orig)[7 * numof16belem + i];

		/* Unshuffle bytes */
		xmm1[0] = _mm_unpacklo_epi8(xmm0[0], xmm0[1]);
		xmm1[4] = _mm_unpackhi_epi8(xmm0[0], xmm0[1]);

		xmm1[1] = _mm_unpacklo_epi8(xmm0[2], xmm0[3]);
		xmm1[5] = _mm_unpackhi_epi8(xmm0[2], xmm0[3]);

		xmm1[2] = _mm_unpacklo_epi8(xmm0[4], xmm0[5]);
		xmm1[6] = _mm_unpackhi_epi8(xmm0[4], xmm0[5]);

		xmm1[3] = _mm_unpacklo_epi8(xmm0[6], xmm0[7]);
		xmm1[7] = _mm_unpackhi_epi8(xmm0[6], xmm0[7]);

		/* Unshuffle 2-byte words */
		xmm0[0] = _mm_unpacklo_epi16(xmm1[0], xmm1[1]);
		xmm0[4] = _mm_unpackhi_epi16(xmm1[0], xmm1[1]);

		xmm0[1] = _mm_unpacklo_epi16(xmm1[2], xmm1[3]);
		xmm0[5] = _mm_unpackhi_epi16(xmm1[2], xmm1[3]);

		xmm0[2] = _mm_unpacklo_epi16(xmm1[4], xmm1[5]);
		xmm0[6] = _mm_unpackhi_epi16(xmm1[4], xmm1[5]);

		xmm0[3] = _mm_unpacklo_epi16(xmm1[6], xmm1[7]);
		xmm0[7] = _mm_unpackhi_epi16(xmm1[6], xmm1[7]);

		/* Unshuffle 4-byte dwords */
		xmm1[0] = _mm_unpacklo_epi32(xmm0[0], xmm0[1]);
		xmm1[4] = _mm_unpackhi_epi32(xmm0[0], xmm0[1]);

		xmm1[1] = _mm_unpacklo_epi32(xmm0[2], xmm0[3]);
		xmm1[5] = _mm_unpackhi_epi32(xmm0[2], xmm0[3]);

		xmm1[2] = _mm_unpacklo_epi32(xmm0[4], xmm0[5]);
		xmm1[6] = _mm_unpackhi_epi32(xmm0[4], xmm0[5]);

		xmm1[3] = _mm_unpacklo_epi32(xmm0[6], xmm0[7]);
		xmm1[7] = _mm_unpackhi_epi32(xmm0[6], xmm0[7]);

		/* Store the result vectors in proper order */
		((__m128i *)dest)[j + 0] = xmm1[0];
		((__m128i *)dest)[j + 1] = xmm1[4];
		((__m128i *)dest)[j + 2] = xmm1[2];
		((__m128i *)dest)[j + 3] = xmm1[6];
		((__m128i *)dest)[j + 4] = xmm1[1];
		((__m128i *)dest)[j + 5] = xmm1[5];
		((__m128i *)dest)[j + 6] = xmm1[3];
		((__m128i *)dest)[j + 7] = xmm1[7];
	}
}


/* Routine optimized for unshuffling a buffer for a type size of 16 bytes. */
static void
unshuffle16(uint8_t* dest, const uint8_t* orig, size_t size)
{
	size_t i, j;
	const size_t numof16belem = size / (16 * sizeof(__m128i));

	for (i = 0, j = 0; i < numof16belem; i++, j += 16) {
		__m128i xmm1[16], xmm2[16];

		/* Load the first 128 bytes in 16 XMM registers */
		xmm1[0] = ((__m128i *)orig)[0 * numof16belem + i];
		xmm1[1] = ((__m128i *)orig)[1 * numof16belem + i];
		xmm1[2] = ((__m128i *)orig)[2 * numof16belem + i];
		xmm1[3] = ((__m128i *)orig)[3 * numof16belem + i];
		xmm1[4] = ((__m128i *)orig)[4 * numof16belem + i];
		xmm1[5] = ((__m128i *)orig)[5 * numof16belem + i];
		xmm1[6] = ((__m128i *)orig)[6 * numof16belem + i];
		xmm1[7] = ((__m128i *)orig)[7 * numof16belem + i];
		xmm1[8] = ((__m128i *)orig)[8 * numof16belem + i];
		xmm1[9] = ((__m128i *)orig)[9 * numof16belem + i];
		xmm1[10] = ((__m128i *)orig)[10 * numof16belem + i];
		xmm1[11] = ((__m128i *)orig)[11 * numof16belem + i];
		xmm1[12] = ((__m128i *)orig)[12 * numof16belem + i];
		xmm1[13] = ((__m128i *)orig)[13 * numof16belem + i];
		xmm1[14] = ((__m128i *)orig)[14 * numof16belem + i];
		xmm1[15] = ((__m128i *)orig)[15 * numof16belem + i];

		/* Unshuffle bytes */
		xmm2[0] = _mm_unpacklo_epi8(xmm1[0], xmm1[1]);
		xmm2[8] = _mm_unpackhi_epi8(xmm1[0], xmm1[1]);

		xmm2[1] = _mm_unpacklo_epi8(xmm1[2], xmm1[3]);
		xmm2[9] = _mm_unpackhi_epi8(xmm1[2], xmm1[3]);

		xmm2[2] = _mm_unpacklo_epi8(xmm1[4], xmm1[5]);
		xmm2[10] = _mm_unpackhi_epi8(xmm1[4], xmm1[5]);

		xmm2[3] = _mm_unpacklo_epi8(xmm1[6], xmm1[7]);
		xmm2[11] = _mm_unpackhi_epi8(xmm1[6], xmm1[7]);

		xmm2[4] = _mm_unpacklo_epi8(xmm1[8], xmm1[9]);
		xmm2[12] = _mm_unpackhi_epi8(xmm1[8], xmm1[9]);

		xmm2[5] = _mm_unpacklo_epi8(xmm1[10], xmm1[11]);
		xmm2[13] = _mm_unpackhi_epi8(xmm1[10], xmm1[11]);

		xmm2[6] = _mm_unpacklo_epi8(xmm1[12], xmm1[13]);
		xmm2[14] = _mm_unpackhi_epi8(xmm1[12], xmm1[13]);

		xmm2[7] = _mm_unpacklo_epi8(xmm1[14], xmm1[15]);
		xmm2[15] = _mm_unpackhi_epi8(xmm1[14], xmm1[15]);

		/* Unshuffle 2-byte words */
		xmm1[0] = _mm_unpacklo_epi16(xmm2[0], xmm2[1]);
		xmm1[8] = _mm_unpackhi_epi16(xmm2[0], xmm2[1]);

		xmm1[1] = _mm_unpacklo_epi16(xmm2[2], xmm2[3]);
		xmm1[9] = _mm_unpackhi_epi16(xmm2[2], xmm2[3]);

		xmm1[2] = _mm_unpacklo_epi16(xmm2[4], xmm2[5]);
		xmm1[10] = _mm_unpackhi_epi16(xmm2[4], xmm2[5]);

		xmm1[3] = _mm_unpacklo_epi16(xmm2[6], xmm2[7]);
		xmm1[11] = _mm_unpackhi_epi16(xmm2[6], xmm2[7]);

		xmm1[4] = _mm_unpacklo_epi16(xmm2[8], xmm2[9]);
		xmm1[12] = _mm_unpackhi_epi16(xmm2[8], xmm2[9]);

		xmm1[5] = _mm_unpacklo_epi16(xmm2[10], xmm2[11]);
		xmm1[13] = _mm_unpackhi_epi16(xmm2[10], xmm2[11]);

		xmm1[6] = _mm_unpacklo_epi16(xmm2[12], xmm2[13]);
		xmm1[14] = _mm_unpackhi_epi16(xmm2[12], xmm2[13]);

		xmm1[7] = _mm_unpacklo_epi16(xmm2[14], xmm2[15]);
		xmm1[15] = _mm_unpackhi_epi16(xmm2[14], xmm2[15]);

		/* Unshuffle 4-byte dwords */
		xmm2[0] = _mm_unpacklo_epi32(xmm1[0], xmm1[1]);
		xmm2[8] = _mm_unpackhi_epi32(xmm1[0], xmm1[1]);

		xmm2[1] = _mm_unpacklo_epi32(xmm1[2], xmm1[3]);
		xmm2[9] = _mm_unpackhi_epi32(xmm1[2], xmm1[3]);

		xmm2[2] = _mm_unpacklo_epi32(xmm1[4], xmm1[5]);
		xmm2[10] = _mm_unpackhi_epi32(xmm1[4], xmm1[5]);

		xmm2[3] = _mm_unpacklo_epi32(xmm1[6], xmm1[7]);
		xmm2[11] = _mm_unpackhi_epi32(xmm1[6], xmm1[7]);

		xmm2[4] = _mm_unpacklo_epi32(xmm1[8], xmm1[9]);
		xmm2[12] = _mm_unpackhi_epi32(xmm1[8], xmm1[9]);

		xmm2[5] = _mm_unpacklo_epi32(xmm1[10], xmm1[11]);
		xmm2[13] = _mm_unpackhi_epi32(xmm1[10], xmm1[11]);

		xmm2[6] = _mm_unpacklo_epi32(xmm1[12], xmm1[13]);
		xmm2[14] = _mm_unpackhi_epi32(xmm1[12], xmm1[13]);

		xmm2[7] = _mm_unpacklo_epi32(xmm1[14], xmm1[15]);
		xmm2[15] = _mm_unpackhi_epi32(xmm1[14], xmm1[15]);

		/* Unshuffle 8-byte qwords */
		xmm1[0] = _mm_unpacklo_epi64(xmm2[0], xmm2[1]);
		xmm1[8] = _mm_unpackhi_epi64(xmm2[0], xmm2[1]);

		xmm1[1] = _mm_unpacklo_epi64(xmm2[2], xmm2[3]);
		xmm1[9] = _mm_unpackhi_epi64(xmm2[2], xmm2[3]);

		xmm1[2] = _mm_unpacklo_epi64(xmm2[4], xmm2[5]);
		xmm1[10] = _mm_unpackhi_epi64(xmm2[4], xmm2[5]);

		xmm1[3] = _mm_unpacklo_epi64(xmm2[6], xmm2[7]);
		xmm1[11] = _mm_unpackhi_epi64(xmm2[6], xmm2[7]);

		xmm1[4] = _mm_unpacklo_epi64(xmm2[8], xmm2[9]);
		xmm1[12] = _mm_unpackhi_epi64(xmm2[8], xmm2[9]);

		xmm1[5] = _mm_unpacklo_epi64(xmm2[10], xmm2[11]);
		xmm1[13] = _mm_unpackhi_epi64(xmm2[10], xmm2[11]);

		xmm1[6] = _mm_unpacklo_epi64(xmm2[12], xmm2[13]);
		xmm1[14] = _mm_unpackhi_epi64(xmm2[12], xmm2[13]);

		xmm1[7] = _mm_unpacklo_epi64(xmm2[14], xmm2[15]);
		xmm1[15] = _mm_unpackhi_epi64(xmm2[14], xmm2[15]);

		/* Store the result vectors in proper order */
		((__m128i *)dest)[j + 0] = xmm1[0];
		((__m128i *)dest)[j + 1] = xmm1[8];
		((__m128i *)dest)[j + 2] = xmm1[4];
		((__m128i *)dest)[j + 3] = xmm1[12];
		((__m128i *)dest)[j + 4] = xmm1[2];
		((__m128i *)dest)[j + 5] = xmm1[10];
		((__m128i *)dest)[j + 6] = xmm1[6];
		((__m128i *)dest)[j + 7] = xmm1[14];
		((__m128i *)dest)[j + 8] = xmm1[1];
		((__m128i *)dest)[j + 9] = xmm1[9];
		((__m128i *)dest)[j + 10] = xmm1[5];
		((__m128i *)dest)[j + 11] = xmm1[13];
		((__m128i *)dest)[j + 12] = xmm1[3];
		((__m128i *)dest)[j + 13] = xmm1[11];
		((__m128i *)dest)[j + 14] = xmm1[7];
		((__m128i *)dest)[j + 15] = xmm1[15];
	}
}


/* Routine optimized for unshuffling a buffer for a type size larger than 16 bytes. */
static void
unshuffle_multipart(uint8_t* dest, const uint8_t* orig, size_t size, size_t bytesoftype)
{
	const size_t num_elements = size / bytesoftype;
	const lldiv_t vecs_per_el = lldiv(bytesoftype, sizeof(__m128i));

	/* The unshuffle loops are inverted (compared to shuffle_multipart)
		 to optimize cache utilization. */
	size_t offset_into_type;
	for (offset_into_type = 0; offset_into_type < bytesoftype;
		offset_into_type += (offset_into_type == 0 && vecs_per_el.rem > 0 ? vecs_per_el.rem : sizeof(__m128i))) {
		size_t j;
		for (j = 0; j < num_elements; j += sizeof(__m128i)) {
			__m128i xmm1[16], xmm2[16];

			/* Load the first 128 bytes in 16 XMM registers */
			uint8_t* const src_for_jth_element = orig + j;
			xmm1[0] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 0))));
			xmm1[1] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 1))));
			xmm1[2] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 2))));
			xmm1[3] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 3))));
			xmm1[4] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 4))));
			xmm1[5] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 5))));
			xmm1[6] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 6))));
			xmm1[7] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 7))));
			xmm1[8] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 8))));
			xmm1[9] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 9))));
			xmm1[10] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 10))));
			xmm1[11] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 11))));
			xmm1[12] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 12))));
			xmm1[13] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 13))));
			xmm1[14] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 14))));
			xmm1[15] = _mm_loadu_si128((__m128i*)(src_for_jth_element + (num_elements * (offset_into_type + 15))));

			/* Unshuffle bytes */
			xmm2[0] = _mm_unpacklo_epi8(xmm1[0], xmm1[1]);
			xmm2[8] = _mm_unpackhi_epi8(xmm1[0], xmm1[1]);

			xmm2[1] = _mm_unpacklo_epi8(xmm1[2], xmm1[3]);
			xmm2[9] = _mm_unpackhi_epi8(xmm1[2], xmm1[3]);

			xmm2[2] = _mm_unpacklo_epi8(xmm1[4], xmm1[5]);
			xmm2[10] = _mm_unpackhi_epi8(xmm1[4], xmm1[5]);

			xmm2[3] = _mm_unpacklo_epi8(xmm1[6], xmm1[7]);
			xmm2[11] = _mm_unpackhi_epi8(xmm1[6], xmm1[7]);

			xmm2[4] = _mm_unpacklo_epi8(xmm1[8], xmm1[9]);
			xmm2[12] = _mm_unpackhi_epi8(xmm1[8], xmm1[9]);

			xmm2[5] = _mm_unpacklo_epi8(xmm1[10], xmm1[11]);
			xmm2[13] = _mm_unpackhi_epi8(xmm1[10], xmm1[11]);

			xmm2[6] = _mm_unpacklo_epi8(xmm1[12], xmm1[13]);
			xmm2[14] = _mm_unpackhi_epi8(xmm1[12], xmm1[13]);

			xmm2[7] = _mm_unpacklo_epi8(xmm1[14], xmm1[15]);
			xmm2[15] = _mm_unpackhi_epi8(xmm1[14], xmm1[15]);

			/* Unshuffle 2-byte words */
			xmm1[0] = _mm_unpacklo_epi16(xmm2[0], xmm2[1]);
			xmm1[8] = _mm_unpackhi_epi16(xmm2[0], xmm2[1]);

			xmm1[1] = _mm_unpacklo_epi16(xmm2[2], xmm2[3]);
			xmm1[9] = _mm_unpackhi_epi16(xmm2[2], xmm2[3]);

			xmm1[2] = _mm_unpacklo_epi16(xmm2[4], xmm2[5]);
			xmm1[10] = _mm_unpackhi_epi16(xmm2[4], xmm2[5]);

			xmm1[3] = _mm_unpacklo_epi16(xmm2[6], xmm2[7]);
			xmm1[11] = _mm_unpackhi_epi16(xmm2[6], xmm2[7]);

			xmm1[4] = _mm_unpacklo_epi16(xmm2[8], xmm2[9]);
			xmm1[12] = _mm_unpackhi_epi16(xmm2[8], xmm2[9]);

			xmm1[5] = _mm_unpacklo_epi16(xmm2[10], xmm2[11]);
			xmm1[13] = _mm_unpackhi_epi16(xmm2[10], xmm2[11]);

			xmm1[6] = _mm_unpacklo_epi16(xmm2[12], xmm2[13]);
			xmm1[14] = _mm_unpackhi_epi16(xmm2[12], xmm2[13]);

			xmm1[7] = _mm_unpacklo_epi16(xmm2[14], xmm2[15]);
			xmm1[15] = _mm_unpackhi_epi16(xmm2[14], xmm2[15]);

			/* Unshuffle 4-byte dwords */
			xmm2[0] = _mm_unpacklo_epi32(xmm1[0], xmm1[1]);
			xmm2[8] = _mm_unpackhi_epi32(xmm1[0], xmm1[1]);

			xmm2[1] = _mm_unpacklo_epi32(xmm1[2], xmm1[3]);
			xmm2[9] = _mm_unpackhi_epi32(xmm1[2], xmm1[3]);

			xmm2[2] = _mm_unpacklo_epi32(xmm1[4], xmm1[5]);
			xmm2[10] = _mm_unpackhi_epi32(xmm1[4], xmm1[5]);

			xmm2[3] = _mm_unpacklo_epi32(xmm1[6], xmm1[7]);
			xmm2[11] = _mm_unpackhi_epi32(xmm1[6], xmm1[7]);

			xmm2[4] = _mm_unpacklo_epi32(xmm1[8], xmm1[9]);
			xmm2[12] = _mm_unpackhi_epi32(xmm1[8], xmm1[9]);

			xmm2[5] = _mm_unpacklo_epi32(xmm1[10], xmm1[11]);
			xmm2[13] = _mm_unpackhi_epi32(xmm1[10], xmm1[11]);

			xmm2[6] = _mm_unpacklo_epi32(xmm1[12], xmm1[13]);
			xmm2[14] = _mm_unpackhi_epi32(xmm1[12], xmm1[13]);

			xmm2[7] = _mm_unpacklo_epi32(xmm1[14], xmm1[15]);
			xmm2[15] = _mm_unpackhi_epi32(xmm1[14], xmm1[15]);

			/* Unshuffle 8-byte qwords */
			xmm1[0] = _mm_unpacklo_epi64(xmm2[0], xmm2[1]);
			xmm1[8] = _mm_unpackhi_epi64(xmm2[0], xmm2[1]);

			xmm1[1] = _mm_unpacklo_epi64(xmm2[2], xmm2[3]);
			xmm1[9] = _mm_unpackhi_epi64(xmm2[2], xmm2[3]);

			xmm1[2] = _mm_unpacklo_epi64(xmm2[4], xmm2[5]);
			xmm1[10] = _mm_unpackhi_epi64(xmm2[4], xmm2[5]);

			xmm1[3] = _mm_unpacklo_epi64(xmm2[6], xmm2[7]);
			xmm1[11] = _mm_unpackhi_epi64(xmm2[6], xmm2[7]);

			xmm1[4] = _mm_unpacklo_epi64(xmm2[8], xmm2[9]);
			xmm1[12] = _mm_unpackhi_epi64(xmm2[8], xmm2[9]);

			xmm1[5] = _mm_unpacklo_epi64(xmm2[10], xmm2[11]);
			xmm1[13] = _mm_unpackhi_epi64(xmm2[10], xmm2[11]);

			xmm1[6] = _mm_unpacklo_epi64(xmm2[12], xmm2[13]);
			xmm1[14] = _mm_unpackhi_epi64(xmm2[12], xmm2[13]);

			xmm1[7] = _mm_unpacklo_epi64(xmm2[14], xmm2[15]);
			xmm1[15] = _mm_unpackhi_epi64(xmm2[14], xmm2[15]);

			/* Store the result vectors in proper order */
			const uint8_t* const dest_with_offset = dest + offset_into_type;
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 0) * bytesoftype), xmm1[0]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 1) * bytesoftype), xmm1[8]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 2) * bytesoftype), xmm1[4]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 3) * bytesoftype), xmm1[12]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 4) * bytesoftype), xmm1[2]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 5) * bytesoftype), xmm1[10]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 6) * bytesoftype), xmm1[6]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 7) * bytesoftype), xmm1[14]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 8) * bytesoftype), xmm1[1]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 9) * bytesoftype), xmm1[9]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 10) * bytesoftype), xmm1[5]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 11) * bytesoftype), xmm1[13]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 12) * bytesoftype), xmm1[3]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 13) * bytesoftype), xmm1[11]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 14) * bytesoftype), xmm1[7]);
			_mm_storeu_si128((__m128i*)(dest_with_offset + (j + 15) * bytesoftype), xmm1[15]);
		}
	}
}

/* Unshuffle a block.  This can never fail. */
void unshuffle(size_t bytesoftype, size_t blocksize,
	const uint8_t* _src, uint8_t* _dest) {
	int unaligned_src = (int)((uintptr_t)_src % sizeof(__m128i));
	int unaligned_dest = (int)((uintptr_t)_dest % sizeof(__m128i));
	int multiple_of_block = (blocksize % (sizeof(__m128i) * bytesoftype)) == 0;
	int too_small = (blocksize < 256);

	if (unaligned_src || unaligned_dest || !multiple_of_block || too_small) {
		/* _src or _dest buffer is not aligned, not multiple of the vectorization
		* size or is not too small.  Call the non-sse2 version. */
		_unshuffle(bytesoftype, blocksize, _src, _dest);
		return;
	}

	/* Optimized unshuffle */
	/* The buffers must be aligned on a 16 bytes boundary, have a power */
	/* of 2 size and be larger or equal than 256 bytes. */
	switch (bytesoftype)
	{
	case 2:
		unshuffle2(_dest, _src, blocksize);
		break;
	case 4:
		unshuffle4(_dest, _src, blocksize);
		break;
	case 8:
		unshuffle8(_dest, _src, blocksize);
		break;
	case 16:
		unshuffle16(_dest, _src, blocksize);
		break;
	default:
		if (bytesoftype > sizeof(__m128i)) {
			unshuffle_multipart(_dest, _src, blocksize, bytesoftype);
		}
		else {
			/* Non-optimized unshuffle */
			_unshuffle(bytesoftype, blocksize, _src, _dest);
		}
		break;
	}
}

#else   /* no __SSE2__ available */

void shuffle(size_t bytesoftype, size_t blocksize,
	const uint8_t* _src, uint8_t* _dest) {
	_shuffle(bytesoftype, blocksize, _src, _dest);
}

void unshuffle(size_t bytesoftype, size_t blocksize,
	const uint8_t* _src, uint8_t* _dest) {
	_unshuffle(bytesoftype, blocksize, _src, _dest);
}

#endif  /* __SSE2__ */
