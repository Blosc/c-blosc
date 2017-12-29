/* memcopy.h -- inline functions to copy small data chunks.
 * This is strongly based in memcopy.h from the zlib-ng project
 * See copyright at: https://github.com/Dead2/zlib-ng/blob/develop/zlib.h
 * Here it is a copy of the license:
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

#include <assert.h>

#ifndef MEMCOPY_H_
#define MEMCOPY_H_

#if defined(_WIN32) && !defined(__MINGW32__)
  #include <windows.h>

  /* stdint.h only available in VS2010 (VC++ 16.0) and newer */
  #if defined(_MSC_VER) && _MSC_VER < 1600
    #include "win32/stdint-windows.h"
  #else
    #include <stdint.h>
  #endif

  /* Use inlined functions for supported systems */
  #if defined(_MSC_VER) && !defined(__cplusplus)   /* Visual Studio */
    #define inline __inline  /* Visual C is not C99, but supports some kind of inline */
  #endif

#else
  #include <stdint.h>
#endif  // _WIN32

#if (defined(__GNUC__) || defined(__clang__))
#define MEMCPY __builtin_memcpy
#define MEMSET __builtin_memset
#else
#define MEMCPY memcpy
#define MEMSET memset
#endif


static inline unsigned char *copy_1_bytes(unsigned char *out, unsigned char *from) {
  *out++ = *from;
  return out;
}

static inline unsigned char *copy_2_bytes(unsigned char *out, unsigned char *from) {
  uint16_t chunk;
  unsigned sz = sizeof(chunk);
  MEMCPY(&chunk, from, sz);
  MEMCPY(out, &chunk, sz);
  return out + sz;
}

static inline unsigned char *copy_3_bytes(unsigned char *out, unsigned char *from) {
  out = copy_1_bytes(out, from);
  return copy_2_bytes(out, from + 1);
}

static inline unsigned char *copy_4_bytes(unsigned char *out, unsigned char *from) {
  uint32_t chunk;
  unsigned sz = sizeof(chunk);
  MEMCPY(&chunk, from, sz);
  MEMCPY(out, &chunk, sz);
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

static inline unsigned char *copy_8_bytes(unsigned char *out, unsigned char *from) {
  uint64_t chunk;
  unsigned sz = sizeof(chunk);
  MEMCPY(&chunk, from, sz);
  MEMCPY(out, &chunk, sz);
  return out + sz;
}

/* Copy LEN bytes (7 or fewer) from FROM into OUT. Return OUT + LEN. */
static inline unsigned char *copy_bytes(unsigned char *out, unsigned char *from, unsigned len) {
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
static inline unsigned char *set_bytes(unsigned char *out, unsigned char *from, unsigned dist, unsigned len) {
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
            MEMSET(out, c, 7);
            return out + 7;
        case 6:
            MEMSET(out, c, 6);
            return out + 6;
        case 5:
            MEMSET(out, c, 5);
            return out + 5;
        case 4:
            MEMSET(out, c, 4);
            return out + 4;
        case 3:
            MEMSET(out, c, 3);
            return out + 3;
        case 2:
            MEMSET(out, c, 2);
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
static inline unsigned char *chunk_memcpy(unsigned char *out, unsigned char *from, unsigned len) {
  unsigned sz = sizeof(uint64_t);
  unsigned rem = len % sz;
  unsigned by8;

  assert(len >= sz);

  /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
  copy_8_bytes(out, from);

  len /= sz;
  out += rem;
  from += rem;

  by8 = len % sz;
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

/* Memset LEN bytes in OUT with the value at OUT - 1. Return OUT + LEN. */
static inline unsigned char *byte_memset(unsigned char *out, unsigned len) {
  unsigned sz = sizeof(uint64_t);
  unsigned char *from = out - 1;
  unsigned char c = *from;
  unsigned rem = len % sz;
  unsigned by8;

  assert(len >= sz);

  /* First, deal with the case when LEN is not a multiple of SZ. */
  MEMSET(out, c, sz);
  len /= sz;
  out += rem;
  from += rem;

  by8 = len % 8;
  len -= by8;
  switch (by8) {
    case 7:
      MEMSET(out, c, sz);
      out += sz;
    case 6:
      MEMSET(out, c, sz);
      out += sz;
    case 5:
      MEMSET(out, c, sz);
      out += sz;
    case 4:
      MEMSET(out, c, sz);
      out += sz;
    case 3:
      MEMSET(out, c, sz);
      out += sz;
    case 2:
      MEMSET(out, c, sz);
      out += sz;
    case 1:
      MEMSET(out, c, sz);
      out += sz;
  }

  while (len) {
    /* When sz is a constant, the compiler replaces __builtin_memset with an
       inline version that does not incur a function call overhead. */
    MEMSET(out, c, sz);
    out += sz;
    MEMSET(out, c, sz);
    out += sz;
    MEMSET(out, c, sz);
    out += sz;
    MEMSET(out, c, sz);
    out += sz;
    MEMSET(out, c, sz);
    out += sz;
    MEMSET(out, c, sz);
    out += sz;
    MEMSET(out, c, sz);
    out += sz;
    MEMSET(out, c, sz);
    out += sz;
    len -= 8;
  }

  return out;
}

/* Copy DIST bytes from OUT - DIST into OUT + DIST * k, for 0 <= k < LEN/DIST. Return OUT + LEN. */
static inline unsigned char *chunk_memset(unsigned char *out, unsigned char *from, unsigned dist, unsigned len) {
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
static inline unsigned char *chunk_copy(unsigned char *out, unsigned char *from, int dist, unsigned len) {
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

#endif /* MEMCOPY_H_ */
