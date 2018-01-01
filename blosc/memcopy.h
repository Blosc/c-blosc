/* memcopy.h -- inline functions to copy small data chunks.
 * This is strongly based in memcopy.h from the zlib-ng project.
 * New implementations by Francesc Alted:
 *   fast_copy and safe_copy()
 *   Support for SSE2/AVX2 copy instructions for these routines
 *
 * See: https://github.com/Dead2/zlib-ng/blob/develop/zlib.h
 * Here it is a copy of the original license:
   zlib.h -- interface of the 'zlib-ng' compression library
   Forked from and compatible with zlib 1.2.11
  Copyright (C) 1995-2016 Jean-loup Gailly and Mark Adler
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu
  The data format used by the zlib library is described by RFCs (Request for
  Comments) 1950 to 1952 in the files http://tools.ietf.org/html/rfc1950
  (zlib format), rfc1951 (deflate format) and rfc1952 (gzip format).
 */

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

/* Copy LEN bytes (7 or fewer) from FROM into OUT. Return OUT + LEN. */
static inline unsigned char *set_bytes(unsigned char *out, const unsigned char *from, unsigned dist, unsigned len) {
  assert(len < 8);

#ifndef UNALIGNED_OK
  while (len--) {
    *out++ = *from++;
  }
  return out;
#else
  if (dist >= len)
        return copy_bytes(out, from, len);

    switch (dist) {
    case 6:
        assert(len == 7);
        out = copy_6_bytes(out, from);
        return copy_1_bytes(out, from);

    case 5:
        assert(len == 6 || len == 7);
        out = copy_5_bytes(out, from);
        return copy_bytes(out, from, len - 5);

    case 4:
        assert(len == 5 || len == 6 || len == 7);
        out = copy_4_bytes(out, from);
        return copy_bytes(out, from, len - 4);

    case 3:
        assert(4 <= len && len <= 7);
        out = copy_3_bytes(out, from);
        switch (len) {
        case 7:
            return copy_4_bytes(out, from);
        case 6:
            return copy_3_bytes(out, from);
        case 5:
            return copy_2_bytes(out, from);
        case 4:
            return copy_1_bytes(out, from);
        default:
            assert(0);
            break;
        }

    case 2:
        assert(3 <= len && len <= 7);
        out = copy_2_bytes(out, from);
        switch (len) {
        case 7:
            out = copy_4_bytes(out, from);
            out = copy_1_bytes(out, from);
            return out;
        case 6:
            out = copy_4_bytes(out, from);
            return out;
        case 5:
            out = copy_2_bytes(out, from);
            out = copy_1_bytes(out, from);
            return out;
        case 4:
            out = copy_2_bytes(out, from);
            return out;
        case 3:
            out = copy_1_bytes(out, from);
            return out;
        default:
            assert(0);
            break;
        }

    case 1:
        assert(2 <= len && len <= 7);
        unsigned char c = *from;
        switch (len) {
        case 7:
            memset(out, c, 7);
            return out + 7;
        case 6:
            memset(out, c, 6);
            return out + 6;
        case 5:
            memset(out, c, 5);
            return out + 5;
        case 4:
            memset(out, c, 4);
            return out + 4;
        case 3:
            memset(out, c, 3);
            return out + 3;
        case 2:
            memset(out, c, 2);
            return out + 2;
        default:
            assert(0);
            break;
        }
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
  unsigned by8;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
  copy_16_bytes(out, from);

  len /= sz;
  out += rem;
  from += rem;

  by8 = len % 8;
  len -= by8;
  switch (by8) {
    case 7:
      out = copy_16_bytes(out, from);
      from += sz;
    case 6:
      out = copy_16_bytes(out, from);
      from += sz;
    case 5:
      out = copy_16_bytes(out, from);
      from += sz;
    case 4:
      out = copy_16_bytes(out, from);
      from += sz;
    case 3:
      out = copy_16_bytes(out, from);
      from += sz;
    case 2:
      out = copy_16_bytes(out, from);
      from += sz;
    case 1:
      out = copy_16_bytes(out, from);
      from += sz;
    default:
      break;
  }

  while (len) {
    out = copy_16_bytes(out, from);
    from += sz;
    out = copy_16_bytes(out, from);
    from += sz;
    out = copy_16_bytes(out, from);
    from += sz;
    out = copy_16_bytes(out, from);
    from += sz;
    out = copy_16_bytes(out, from);
    from += sz;
    out = copy_16_bytes(out, from);
    from += sz;
    out = copy_16_bytes(out, from);
    from += sz;
    out = copy_16_bytes(out, from);
    from += sz;

    len -= 8;
  }

  return out;
}
#endif // __SSE2__

/* AVX2 version of chunk_memcpy() */
#if defined(__AVX2__)
static inline unsigned char *chunk_memcpy_32(unsigned char *out, const unsigned char *from, unsigned len) {
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

/* AVX2 *aligned* version of chunk_memcpy_32() */
static inline unsigned char *chunk_memcpy_32_aligned(unsigned char *out, const unsigned char *from, unsigned len) {
  unsigned sz = sizeof(__m256i);
  unsigned bytes_to_align = sz - (unsigned)(((uintptr_t)(const void *)(from)) % sz);
  unsigned corrected_len = len - bytes_to_align;
  unsigned rem = corrected_len % sz;
  unsigned by8;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has aligned 32-byte access. */
  copy_32_bytes(out, from);
  out += bytes_to_align;
  from += bytes_to_align;

  len = corrected_len / sz;
  by8 = len % 8;
  len -= by8;
  switch (by8) {
    case 7:
      out = copy_32_bytes_aligned(out, from);
      from += sz;
    case 6:
      out = copy_32_bytes_aligned(out, from);
      from += sz;
    case 5:
      out = copy_32_bytes_aligned(out, from);
      from += sz;
    case 4:
      out = copy_32_bytes_aligned(out, from);
      from += sz;
    case 3:
      out = copy_32_bytes_aligned(out, from);
      from += sz;
    case 2:
      out = copy_32_bytes_aligned(out, from);
      from += sz;
    case 1:
      out = copy_32_bytes_aligned(out, from);
      from += sz;
    default:
      break;
  }

  while (len) {
    out = copy_32_bytes_aligned(out, from);
    from += sz;
    out = copy_32_bytes_aligned(out, from);
    from += sz;
    out = copy_32_bytes_aligned(out, from);
    from += sz;
    out = copy_32_bytes_aligned(out, from);
    from += sz;
    out = copy_32_bytes_aligned(out, from);
    from += sz;
    out = copy_32_bytes_aligned(out, from);
    from += sz;
    out = copy_32_bytes_aligned(out, from);
    from += sz;
    out = copy_32_bytes_aligned(out, from);
    from += sz;

    len -= 8;
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
#endif // __AVX2__

/* Memset LEN bytes in OUT with the value at OUT - 1. Return OUT + LEN. */
static inline unsigned char *byte_memset(unsigned char *out, unsigned len) {
  unsigned sz = sizeof(uint64_t);
  unsigned char *from = out - 1;
  unsigned char c = *from;
  unsigned rem = len % sz;
  unsigned by8;

  assert(len >= sz);

  /* First, deal with the case when LEN is not a multiple of SZ. */
  memset(out, c, sz);
  len /= sz;
  out += rem;

  by8 = len % 8;
  len -= by8;
  switch (by8) {
    case 7:
      memset(out, c, sz);
      out += sz;
    case 6:
      memset(out, c, sz);
      out += sz;
    case 5:
      memset(out, c, sz);
      out += sz;
    case 4:
      memset(out, c, sz);
      out += sz;
    case 3:
      memset(out, c, sz);
      out += sz;
    case 2:
      memset(out, c, sz);
      out += sz;
    case 1:
      memset(out, c, sz);
      out += sz;
    default:
      assert(0);
      break;
  }

  while (len) {
    /* When sz is a constant, the compiler replaces __builtin_memset with an
       inline version that does not incur a function call overhead. */
    memset(out, c, sz);
    out += sz;
    memset(out, c, sz);
    out += sz;
    memset(out, c, sz);
    out += sz;
    memset(out, c, sz);
    out += sz;
    memset(out, c, sz);
    out += sz;
    memset(out, c, sz);
    out += sz;
    memset(out, c, sz);
    out += sz;
    memset(out, c, sz);
    out += sz;
    len -= 8;
  }

  return out;
}

/* Copy DIST bytes from OUT - DIST into OUT + DIST * k, for 0 <= k < LEN/DIST. Return OUT + LEN. */
static inline unsigned char *chunk_memset(unsigned char *out, const unsigned char *from, unsigned dist, unsigned len) {
  unsigned sz = sizeof(uint64_t);
  if (dist >= len)
    return chunk_memcpy(out, from, len);

  assert(len >= sizeof(uint64_t));

  /* Double up the size of the memset pattern until reaching the largest pattern of size less than SZ. */
  while (dist < len && dist < sz) {
    copy_8_bytes(out, from);

    out += dist;
    len -= dist;
    dist += dist;

    /* Make sure the next memcpy has at least SZ bytes to be copied.  */
    if (len < sz)
      /* Finish up byte by byte when there are not enough bytes left. */
      return set_bytes(out, from, dist, len);
  }

  return chunk_memcpy(out, from, len);
}

/* Byte by byte semantics: copy LEN bytes from OUT + DIST and write them to OUT. Return OUT + LEN. */
static inline unsigned char *chunk_copy(unsigned char *out, const unsigned char *from, unsigned dist, unsigned len) {
  if (len < sizeof(uint64_t)) {
    if (dist > 0)
      return set_bytes(out, from, dist, len);

    return copy_bytes(out, from, len);
  }

  if (dist == 1)
    return byte_memset(out, len);

  if (dist > 0)
    return chunk_memset(out, from, dist, len);

  return chunk_memcpy(out, from, len);
}

/* Byte by byte semantics: copy LEN bytes from FROM and write them to OUT. Return OUT + LEN. */
static inline unsigned char *fast_copy(unsigned char *out, const unsigned char *from, unsigned len) {
  if (len < sizeof(uint64_t)) {
    return copy_bytes(out, from, len);
  }
#if defined(__SSE2__)
  else if (len < sizeof(__m128i)) {
    return chunk_memcpy(out, from, len);
  }
#if !defined(__AVX2__)
  return chunk_memcpy_16(out, from, len);
#else
  else if (len < sizeof(__m256i)) {
    return chunk_memcpy_16(out, from, len);
  }
  if (len < 1 << 12) {
    return chunk_memcpy_32(out, from, len);
  }
  else {
    // This should be faster for larger buffers (> 4 KB)
    return chunk_memcpy_32_aligned(out, from, len);
  }
#endif  // !__AVX2__
#endif  // __SSE2__
  return chunk_memcpy(out, from, len);
}

/* Same as fast_copy() but without overwriting origin or destination when they overlap */
static inline unsigned char* safe_copy(unsigned char *out, const unsigned char *from, unsigned len) {
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
