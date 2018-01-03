/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>
  Creation date: 2018-01-03

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

/*********************************************************************
  The code in this file is heavily based on memcopy.h, from the
  zlib-ng compression library.  See LICENSES/ZLIB.txt for details.
  See also: https://github.com/Dead2/zlib-ng/blob/develop/zlib.h

  New implementations by Francesc Alted:
    * fast_copy() and safe_copy() functions
    * Support for SSE2/AVX2 copy instructions for these routines
**********************************************************************/


#ifndef MEMCOPY_H_
#define MEMCOPY_H_

#include <assert.h>
#include "blosc-common.h"


static inline unsigned char *copy_1_bytes(unsigned char *out, const unsigned char *from) {
  *out++ = *from;
  return out;
}

static inline unsigned char *copy_2_bytes(unsigned char *out, unsigned char *from) {
  uint16_t chunk;
  unsigned sz = sizeof(chunk);
  memcpy(&chunk, from, sz);
  memcpy(out, &chunk, sz);
  return out + sz;
}

static inline unsigned char *copy_3_bytes(unsigned char *out, unsigned char *from) {
  out = copy_1_bytes(out, from);
  return copy_2_bytes(out, from + 1);
}

static inline unsigned char *copy_4_bytes(unsigned char *out, unsigned char *from) {
  uint32_t chunk;
  unsigned sz = sizeof(chunk);
  memcpy(&chunk, from, sz);
  memcpy(out, &chunk, sz);
  return out + sz;
}

static inline unsigned char *copy_5_bytes(unsigned char *out, unsigned char *from) {
  out = copy_1_bytes(out, from);
  return copy_4_bytes(out, from + 1);
}

static inline unsigned char *copy_6_bytes(unsigned char *out, unsigned char *from) {
  out = copy_2_bytes(out, from);
  return copy_4_bytes(out, from + 2);
}

static inline unsigned char *copy_7_bytes(unsigned char *out, unsigned char *from) {
  out = copy_3_bytes(out, from);
  return copy_4_bytes(out, from + 3);
}

static inline unsigned char *copy_8_bytes(unsigned char *out, const unsigned char *from) {
  uint64_t chunk;
  memcpy(&chunk, from, 8);
  memcpy(out, &chunk, 8);
  return out + 8;
}

#if defined(__SSE2__)
static inline unsigned char *copy_16_bytes(unsigned char *out, const unsigned char *from) {
  __m128i chunk;
  chunk = _mm_loadu_si128((__m128i*)from);
  _mm_storeu_si128((__m128i*)out, chunk);
  return out + 16;
}
#endif  // __SSE2__

#if defined(__AVX2__)
static inline unsigned char *copy_32_bytes(unsigned char *out, const unsigned char *from) {
  __m256i chunk;
  chunk = _mm256_loadu_si256((__m256i*)from);
  _mm256_storeu_si256((__m256i*)out, chunk);
  return out + 32;
}

static inline unsigned char *copy_32_bytes_aligned(unsigned char *out, const unsigned char *from) {
  __m256i chunk;
  chunk = _mm256_load_si256((__m256i*)from);
  _mm256_storeu_si256((__m256i*)out, chunk);
  return out + 32;
}
#endif  // __AVX2__

/* Copy LEN bytes (7 or fewer) from FROM into OUT. Return OUT + LEN. */
static inline unsigned char *copy_bytes(unsigned char *out, const unsigned char *from, unsigned len) {
  assert(len < 8);

#ifndef UNALIGNED_OK
  while (len--) {
    *out++ = *from++;
  }
  return out;
#else
  switch (len) {
    case 7:
        return copy_7_bytes(out, from);
    case 6:
        return copy_6_bytes(out, from);
    case 5:
        return copy_5_bytes(out, from);
    case 4:
        return copy_4_bytes(out, from);
    case 3:
        return copy_3_bytes(out, from);
    case 2:
        return copy_2_bytes(out, from);
    case 1:
        return copy_1_bytes(out, from);
    case 0:
        return out;
    default:
        assert(0);
    }

    return out;
#endif /* UNALIGNED_OK */
}

/* Byte by byte semantics: copy LEN bytes from FROM and write them to OUT. Return OUT + LEN. */
static inline unsigned char *chunk_memcpy(unsigned char *out, const unsigned char *from, unsigned len) {
  unsigned sz = sizeof(uint64_t);
  unsigned rem = len % sz;
  unsigned by8;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
  copy_8_bytes(out, from);

  len /= sz;
  out += rem;
  from += rem;

  by8 = len % 8;
  len -= by8;
  switch (by8) {
    case 7:
      out = copy_8_bytes(out, from);
      from += sz;
    case 6:
      out = copy_8_bytes(out, from);
      from += sz;
    case 5:
      out = copy_8_bytes(out, from);
      from += sz;
    case 4:
      out = copy_8_bytes(out, from);
      from += sz;
    case 3:
      out = copy_8_bytes(out, from);
      from += sz;
    case 2:
      out = copy_8_bytes(out, from);
      from += sz;
    case 1:
      out = copy_8_bytes(out, from);
      from += sz;
    default:
      break;
  }

  while (len) {
    out = copy_8_bytes(out, from);
    from += sz;
    out = copy_8_bytes(out, from);
    from += sz;
    out = copy_8_bytes(out, from);
    from += sz;
    out = copy_8_bytes(out, from);
    from += sz;
    out = copy_8_bytes(out, from);
    from += sz;
    out = copy_8_bytes(out, from);
    from += sz;
    out = copy_8_bytes(out, from);
    from += sz;
    out = copy_8_bytes(out, from);
    from += sz;

    len -= 8;
  }

  return out;
}

/* SSE2 version of chunk_memcpy() */
#if defined(__SSE2__)
static inline unsigned char *chunk_memcpy_16(unsigned char *out, const unsigned char *from, unsigned len) {
  unsigned sz = sizeof(__m128i);
  unsigned rem = len % sz;
  unsigned ilen;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
  copy_16_bytes(out, from);

  len /= sz;
  out += rem;
  from += rem;

  for (ilen = 0; ilen < len; ilen++) {
    copy_16_bytes(out, from);
    out += sz;
    from += sz;
  }

  return out;
}
#endif // __SSE2__

/* AVX2 version of chunk_memcpy() */
#if defined(__AVX2__)
static inline unsigned char *chunk_memcpy_32(unsigned char *out, const unsigned char *from, unsigned len) {
  unsigned sz = sizeof(__m256i);
  unsigned rem = len % sz;
  unsigned ilen;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
  copy_32_bytes(out, from);

  len /= sz;
  out += rem;
  from += rem;

  for (ilen = 0; ilen < len; ilen++) {
    copy_32_bytes(out, from);
    out += sz;
    from += sz;
  }

  return out;
}
#endif // __AVX2__

/* AVX2 *unrolled* version of chunk_memcpy() */
#if defined(__AVX2__)
static inline unsigned char *chunk_memcpy_32_unrolled(unsigned char *out, const unsigned char *from, unsigned len) {
  unsigned sz = sizeof(__m256i);
  unsigned rem = len % sz;
  unsigned by8;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
  copy_32_bytes(out, from);

  len /= sz;
  out += rem;
  from += rem;

  by8 = len % 8;
  len -= by8;
  switch (by8) {
    case 7:
      out = copy_32_bytes(out, from);
      from += sz;
    case 6:
      out = copy_32_bytes(out, from);
      from += sz;
    case 5:
      out = copy_32_bytes(out, from);
      from += sz;
    case 4:
      out = copy_32_bytes(out, from);
      from += sz;
    case 3:
      out = copy_32_bytes(out, from);
      from += sz;
    case 2:
      out = copy_32_bytes(out, from);
      from += sz;
    case 1:
      out = copy_32_bytes(out, from);
      from += sz;
    default:
      break;
  }

  while (len) {
    out = copy_32_bytes(out, from);
    from += sz;
    out = copy_32_bytes(out, from);
    from += sz;
    out = copy_32_bytes(out, from);
    from += sz;
    out = copy_32_bytes(out, from);
    from += sz;
    out = copy_32_bytes(out, from);
    from += sz;
    out = copy_32_bytes(out, from);
    from += sz;
    out = copy_32_bytes(out, from);
    from += sz;
    out = copy_32_bytes(out, from);
    from += sz;

    len -= 8;
  }

  return out;
}

#endif // __AVX2__


/* SSE2/AVX2 *unaligned* version of chunk_memcpy_aligned() */
#if defined(__SSE2__) || defined(__AVX2__)
static inline unsigned char *chunk_memcpy_unaligned(unsigned char *out, const unsigned char *from, unsigned len) {
#if defined(__AVX2__)
  unsigned sz = sizeof(__m256i);
  __m256i chunk;
#elif defined(__SSE2__)
  unsigned sz = sizeof(__m128i);
  __m128i chunk;
#endif
  unsigned rem = len % sz;
  unsigned ilen;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
#if defined(__AVX2__)
  copy_32_bytes(out, from);
#elif defined(__SSE2__)
  copy_16_bytes(out, from);
#endif

  len /= sz;
  out += rem;
  from += rem;

  for (ilen = 0; ilen < len; ilen++) {
#if defined(__AVX2__)
    copy_32_bytes(out, from);
#elif defined(__SSE2__)
    copy_16_bytes(out, from);
#endif
    out += sz;
    from += sz;
  }

  return out;
}
#endif // __SSE2__ || __AVX2__

#if defined(__SSE2__) || defined(__AVX2__)
/* SSE2/AVX2 *aligned* version of chunk_memcpy_aligned() */
static inline unsigned char *chunk_memcpy_aligned(unsigned char *out, const unsigned char *from, unsigned len) {
#if defined(__AVX2__)
  unsigned sz = sizeof(__m256i);
  __m256i chunk;
#elif defined(__SSE2__)
  unsigned sz = sizeof(__m128i);
  __m128i chunk;
#endif
  unsigned bytes_to_align = sz - (unsigned)(((uintptr_t)(const void *)(from)) % sz);
  unsigned corrected_len = len - bytes_to_align;
  unsigned rem = corrected_len % sz;
  unsigned ilen;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has aligned access. */
#if defined(__AVX2__)
  chunk = _mm256_loadu_si256((__m256i *) from);
  _mm256_storeu_si256((__m256i *) out, chunk);
#elif defined(__SSE2__)
  chunk = _mm_loadu_si128((__m128i *) from);
  _mm_storeu_si128((__m128i *) out, chunk);
#endif
  out += bytes_to_align;
  from += bytes_to_align;

  len = corrected_len / sz;
  for (ilen = 0; ilen < len; ilen++) {
#if defined(__AVX2__)
    chunk = _mm256_load_si256((__m256i *) from);
    _mm256_storeu_si256((__m256i *) out, chunk);
#elif defined(__SSE2__)
    chunk = _mm_load_si128((__m128i *) from);
    _mm_storeu_si128((__m128i *) out, chunk);
#endif
    out += sz;
    from += sz;
  }

  /* Copy remaining bytes */
  if (rem < 8) {
    out = copy_bytes(out, from, rem);
  }
  else {
    out = chunk_memcpy(out, from, rem);
  }

  return out;
}
#endif // __AVX2__ || __SSE2__

/* Byte by byte semantics: copy LEN bytes from FROM and write them to OUT. Return OUT + LEN. */
static inline unsigned char *fast_copy(unsigned char *out, const unsigned char *from, unsigned len) {
  if (len < sizeof(uint64_t)) {
    return copy_bytes(out, from, len);
  }
#if defined(__SSE2__)
  if (len < sizeof(__m128i)) {
    return chunk_memcpy(out, from, len);
  }
  if (len == sizeof(__m128i)) {
    return copy_16_bytes(out, from);
  }
#if !defined(__AVX2__)
  return chunk_memcpy_16(out, from, len);
#else
  if (len == sizeof(__m256i)) {
    return copy_32_bytes(out, from);
  }
  if (len < sizeof(__m256i)) {
    return chunk_memcpy_16(out, from, len);
  }
  return chunk_memcpy_32_unrolled(out, from, len);
#endif  // !__AVX2__
#endif  // __SSE2__
  return chunk_memcpy(out, from, len);
}

/* Same as fast_copy() but without overwriting origin or destination when they overlap */
static inline unsigned char* safe_copy(unsigned char *out, const unsigned char *from, unsigned len) {
  /* TODO: shortcut for a copy of 32 bytes */
#if defined(__AVX2__)
  unsigned sz = sizeof(__m256i);
#elif defined(__SSE2__)
  unsigned sz = sizeof(__m128i);
#else
  unsigned sz = sizeof(uint64_t);
#endif
  if (out - sz < from) {
    for (; len; --len) {
      *out++ = *from++;
    }
    return out;
  }
  else {
    return fast_copy(out, from, len);
  }
}


#endif /* MEMCOPY_H_ */
