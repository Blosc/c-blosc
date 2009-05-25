/*
  blosc - Blocked Suffling and Compression Library

  See LICENSE.txt for details about copyright and rights to use.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "blosclz.h"

/*
 * Prevent accessing more than 8-bit at once, except on x86 architectures.
 */
#if !defined(BLOSCLZ_STRICT_ALIGN)
#define BLOSCLZ_STRICT_ALIGN
#if defined(__i386__) || defined(__386) || defined (__amd64)  /* GNU C, Sun Studio */
#undef BLOSCLZ_STRICT_ALIGN
#elif defined(__i486__) || defined(__i586__) || defined(__i686__)  /* GNU C */
#undef BLOSCLZ_STRICT_ALIGN
#elif defined(_M_IX86) /* Intel, MSVC */
#undef BLOSCLZ_STRICT_ALIGN
#elif defined(__386)
#undef BLOSCLZ_STRICT_ALIGN
#elif defined(_X86_) /* MinGW */
#undef BLOSCLZ_STRICT_ALIGN
#elif defined(__I86__) /* Digital Mars */
#undef BLOSCLZ_STRICT_ALIGN
#endif
#endif

/*
 * Always check for bound when decompressing.
 * Generally it is best to leave it defined.
 */
#define BLOSCLZ_SAFE

/*
 * Give hints to the compiler for branch prediction optimization.
 */
#if defined(__GNUC__) && (__GNUC__ > 2)
#define BLOSCLZ_EXPECT_CONDITIONAL(c)    (__builtin_expect((c), 1))
#define BLOSCLZ_UNEXPECT_CONDITIONAL(c)  (__builtin_expect((c), 0))
#else
#define BLOSCLZ_EXPECT_CONDITIONAL(c)    (c)
#define BLOSCLZ_UNEXPECT_CONDITIONAL(c)  (c)
#endif

/*
 * Use inlined functions for supported systems.
 */
#if defined(__GNUC__) || defined(__DMC__) || defined(__POCC__) || defined(__WATCOMC__) || defined(__SUNPRO_C)
#define BLOSCLZ_INLINE inline
#elif defined(__BORLANDC__) || defined(_MSC_VER) || defined(__LCC__)
#define BLOSCLZ_INLINE __inline
#else 
#define BLOSCLZ_INLINE
#endif

/*
 * FIXME: use preprocessor magic to set this on different platforms!
 */
typedef unsigned char  flzuint8;
typedef unsigned short flzuint16;
typedef unsigned int   flzuint32;


#define MAX_COPY       32
#define MAX_LEN       264  /* 256 + 8 */
#define MAX_DISTANCE 8191
#define MAX_FARDISTANCE (65535+MAX_DISTANCE-1)

#ifdef BLOSCLZ_STRICT_ALIGN
  #define BLOSCLZ_READU16(p) ((p)[0] | (p)[1]<<8)
#else
  #define BLOSCLZ_READU16(p) *((const flzuint16*)(p))
#endif

// HASH_LOG cannot be larger than 16
//#define HASH_LOG  13    // orig setting
//#define HASH_LOG  11     // somewhat faster and enough compression rate for small binary blocks
#define HASH_LOG  8     // somewhat faster and enough compression rate for small binary blocks
#define HASH_SIZE (1<< HASH_LOG)
#define HASH_MASK  (HASH_SIZE-1)
#define HASH_FUNCTION(v,p) { v = BLOSCLZ_READU16(p); v ^= BLOSCLZ_READU16(p+1)^(v>>(16-HASH_LOG));v &= HASH_MASK; }


BLOSCLZ_INLINE int blosclz_compress(const void* input, int length, void* output)
{
  flzuint8* ip = (flzuint8*) input;
  flzuint8* ibase = (flzuint8*) input;
  flzuint8* ip_bound = ip + length - 2;
  flzuint8* ip_limit = ip + length - 12;
  flzuint8* op = (flzuint8*) output;
  flzuint16 htab[HASH_SIZE] __attribute__((aligned(64)));

  size_t hslot;
  size_t hval;
  size_t copy;

  /* sanity check */
  if(BLOSCLZ_UNEXPECT_CONDITIONAL(length < 4)) {
    if(length) {
      /* create literal copy only */
      *op++ = length-1;
      ip_bound++;
      while(ip <= ip_bound)
        *op++ = *ip++;
      return length+1;
    }
    else
      return 0;
  }

  /* initializes hash table */
  for (hslot = 0; hslot < HASH_SIZE; hslot++)
    htab[hslot] = 0;

  /* we start with literal copy */
  copy = 2;
  *op++ = MAX_COPY-1;
  *op++ = *ip++;
  *op++ = *ip++;

  /* main loop */
  while(BLOSCLZ_EXPECT_CONDITIONAL(ip < ip_limit)) {
    const flzuint8* ref;
    size_t distance;
    size_t len = 3;         // minimum match length
    flzuint8* anchor = ip;  // comparison starting-point

    /* check for a run */
    if(ip[0] == ip[-1] && BLOSCLZ_READU16(ip-1)==BLOSCLZ_READU16(ip+1)) {
      distance = 1;
      ip += 3;
      ref = anchor - 1 + 3;
      goto match;
    }

    /* find potential match */
    HASH_FUNCTION(hval,ip);
    ref = ibase + htab[hval];
    /* update hash table */
    htab[hval] = anchor - ibase;

    /* calculate distance to the match */
    distance = anchor - ref;

    /* is this a match? check the first 3 bytes */
    if (distance==0 || (distance >= MAX_FARDISTANCE) ||
        *ref++ != *ip++ || *ref++!=*ip++ || *ref++!=*ip++)
      goto literal;

    /* far, needs at least 5-byte match */
    if (distance >= MAX_DISTANCE) {
      if (*ip++ != *ref++ || *ip++ != *ref++)
        goto literal;
      len += 2;
    }

    match:

    /* last matched byte */
    ip = anchor + len;

    /* distance is biased */
    distance--;

    if(!distance) {
      /* zero distance means a run */
      flzuint8 x = ip[-1];
      // Broadcast the value for every byte on a 64-bit register
      long long value, value2;
      value = x * 0x0101010101010101;
      while (ip < ip_bound) {
        value2 = ((long long *)ref)[0];
        if (value != value2) {
          // Find the byte that starts to differ
          while (ip < ip_bound) {
            if (*ref++ != x) break; else ip++;
          }
          break;
        }
        else {
          ip += 8;
          ref += 8;
        }
      }
      if (ip > ip_bound) {
        long l = ip - ip_bound;
        ip -= l;
        ref -= l;
      }
    }
    else {
      for(;;) {
        /* safe because the outer check against ip limit */
        while (ip < ip_bound) {
          if (*ref++ != *ip++) break;
          if (((long long *)ref)[0] != ((long long *)ip)[0]) {
            // Find the byte that starts to differ
            while (ip < ip_bound) {
              if (*ref++ != *ip++) break;
            }
            break;
          }
          else {
            ip += 8;
            ref += 8;
          }
        }
        // Last correction before exiting loop
        if (ip > ip_bound) {
          size_t l = ip - ip_bound;
          ip -= l;
          ref -= l;
        }   // End of optimization
        break;
      }
    }

    /* if we have copied something, adjust the copy count */
    if (copy)
      /* copy is biased, '0' means 1 byte copy */
      *(op-copy-1) = copy-1;
    else
      /* back, to overwrite the copy count */
      op--;

    /* reset literal counter */
    copy = 0;

    /* length is biased, '1' means a match of 3 bytes */
    ip -= 3;
    len = ip - anchor;

    /* encode the match */
    if(distance < MAX_DISTANCE) {
      if(len < 7) {
        *op++ = (len << 5) + (distance >> 8);
        *op++ = (distance & 255);
      }
      else {
        *op++ = (7 << 5) + (distance >> 8);
        for(len-=7; len >= 255; len-= 255)
          *op++ = 255;
        *op++ = len;
        *op++ = (distance & 255);
      }
    }
    else {
      /* far away, but not yet in the another galaxy... */
      if(len < 7) {
        distance -= MAX_DISTANCE;
        *op++ = (len << 5) + 31;
        *op++ = 255;
        *op++ = distance >> 8;
        *op++ = distance & 255;
      }
      else {
        distance -= MAX_DISTANCE;
        *op++ = (7 << 5) + 31;
        for(len-=7; len >= 255; len-= 255)
          *op++ = 255;
        *op++ = len;
        *op++ = 255;
        *op++ = distance >> 8;
        *op++ = distance & 255;
      }
    }

    /* update the hash at match boundary */
    HASH_FUNCTION(hval,ip);
    htab[hval] = ip++ - ibase;
    HASH_FUNCTION(hval,ip);
    htab[hval] = ip++ - ibase;

    /* assuming literal copy */
    *op++ = MAX_COPY-1;

    continue;

    literal:
      *op++ = *anchor++;
      ip = anchor;
      copy++;
      if(BLOSCLZ_UNEXPECT_CONDITIONAL(copy == MAX_COPY)) {
        copy = 0;
        *op++ = MAX_COPY-1;
      }
      //printf("ip, op--> %d, %d\n", ip - ibase, op - (flzuint8*)output);
      size_t l = ip - ibase;
      size_t lo = op - (flzuint8*)output;
      // TODO: Make the compression rate to depend on the compression level
      if (l > (HASH_SIZE+16) && lo > (l>>1)) {
        // Seems incompressible so far...
        return -1;
      }
  }

  /* left-over as literal copy */
  ip_bound++;
  while(ip <= ip_bound) {
    *op++ = *ip++;
    copy++;
    if(copy == MAX_COPY) {
      copy = 0;
      *op++ = MAX_COPY-1;
    }
  }

  /* if we have copied something, adjust the copy length */
  if(copy)
    *(op-copy-1) = copy-1;
  else
    op--;

  /* marker for blosclz */
  *(flzuint8*)output |= (1 << 5);

  return op - (flzuint8*)output;
}


BLOSCLZ_INLINE int blosclz_decompress(const void* input, int length, void* output, int maxout)
{
  const flzuint8* ip = (const flzuint8*) input;
  const flzuint8* ip_limit  = ip + length;
  flzuint8* op = (flzuint8*) output;
  flzuint8* op_limit = op + maxout;
  flzuint32 ctrl = (*ip++) & 31;
  size_t loop = 1;

  do {
    const flzuint8* ref = op;
    size_t len = ctrl >> 5;
    size_t ofs = (ctrl & 31) << 8;

    if(ctrl >= 32) {
      flzuint8 code;
      len--;
      ref -= ofs;
      if (len == 7-1)
        do {
          code = *ip++;
          len += code;
        } while (code==255);
      code = *ip++;
      ref -= code;

      /* match from 16-bit distance */
      if(BLOSCLZ_UNEXPECT_CONDITIONAL(code==255))
      if(BLOSCLZ_EXPECT_CONDITIONAL(ofs==(31 << 8))) {
        ofs = (*ip++) << 8;
        ofs += *ip++;
        ref = op - ofs - MAX_DISTANCE;
      }

#ifdef BLOSCLZ_SAFE
      if (BLOSCLZ_UNEXPECT_CONDITIONAL(op + len + 3 > op_limit))
        return 0;

      if (BLOSCLZ_UNEXPECT_CONDITIONAL(ref-1 < (flzuint8 *)output))
        return 0;
#endif

      if(BLOSCLZ_EXPECT_CONDITIONAL(ip < ip_limit))
        ctrl = *ip++;
      else
        loop = 0;

      if(ref == op) {
        /* optimize copy for a run */
        flzuint8 b = ref[-1];
        memset(op, b, len+3);
        op += len+3;
      }
      else {
        /* copy from reference */
        ref--;
        len += 3;
        if (abs(ref-op) <= len) {
          //printf("abs(ref-op), len: %d,%d,", abs(ref-op), len);
          // src and dst do overlap: do a loop
          for(; len; --len)
            *op++ = *ref++;
          // The memmove below does not work well (don't know why)
/*          memmove(op, ref, len);
          op += len;
          ref += len;
          len = 0;*/
        }
        else {
          memcpy(op, ref, len);
          op += len;
          ref += len;
        }
      }
    }
    else {
      ctrl++;
#ifdef BLOSCLZ_SAFE
      if (BLOSCLZ_UNEXPECT_CONDITIONAL(op + ctrl > op_limit))
        return 0;
      if (BLOSCLZ_UNEXPECT_CONDITIONAL(ip + ctrl > ip_limit))
        return 0;
#endif

      memcpy(op, ip, ctrl);
      ip += ctrl;
      op += ctrl;

      loop = BLOSCLZ_EXPECT_CONDITIONAL(ip < ip_limit);
      if(loop)
        ctrl = *ip++;
    }
  } while(BLOSCLZ_EXPECT_CONDITIONAL(loop));

  return op - (flzuint8*)output;
}
