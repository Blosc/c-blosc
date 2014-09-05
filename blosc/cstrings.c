/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted <francesc@blosc.io>
  Creation date: 2014-09-04

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cstrings.h"


/* Encode len in buf */
static int32_t len_encode(int32_t len, uint8_t *buf) {
  int32_t size = -1;

  if (len < (1<<7)) {
    /* Can be encoded with 1 bytes */
    buf[0] = len & 0xFF;
    size = 1;
  }
  else if (len < (1<<15)) {
    /* Can be encoded with 2 bytes */
    buf[0] = len >> 8;
    buf[1] = len & 0xFF;
    size = 2;
  }
  else {
    /* Else, encode it with 4 bytes */
    buf[0] = len >> 24;
    buf[1] = len >> 16;
    buf[2] = len >> 8;
    buf[3] = len & 0xFF;
    size = 4;
  }
  buf += size;
  return size;
}

/* Decode len in buf */
static int32_t len_decode(uint8_t *buf) {
  int32_t len = -1;
  
  if (buf[0] < (1<<7)) {
    /* 1-byte encoding */
    len = *buf++;
  }
  else if (buf[1] < (1<<7)) {
    /* 2-byte encoding */
    len = *buf++ << 8;
    len += *buf++;
  }
  else {
    /* 4-byte encoding */
    len = *buf++ << 24;
    len += *buf++ << 16;
    len += *buf++ << 8;
    len += *buf++;
  }
  return len;
}

/* Convert 4 bytes from `*pa` to int32_t, changing endianness if necessary. */
static int32_t sw32_(uint8_t *pa)
{
  int32_t idest;
  uint8_t *dest = (uint8_t *)&idest;
  int i = 1;                    /* for big/little endian detection */
  char *p = (char *)&i;

  if (p[0] != 1) {
    /* big endian */
    dest[0] = pa[3];
    dest[1] = pa[2];
    dest[2] = pa[1];
    dest[3] = pa[0];
  }
  else {
    /* little endian */
    dest[0] = pa[0];
    dest[1] = pa[1];
    dest[2] = pa[2];
    dest[3] = pa[3];
  }
  return idest;
}

/* Copy 4 bytes from `a` to `*dest`, changing endianness if necessary. */
void _sw32(uint8_t* dest, int32_t a)
{
  uint8_t *pa = (uint8_t *)&a;
  int i = 1;                    /* for big/little endian detection */
  char *p = (char *)&i;

  if (p[0] != 1) {
    /* big endian */
    dest[0] = pa[3];
    dest[1] = pa[2];
    dest[2] = pa[1];
    dest[3] = pa[0];
  }
  else {
    /* little endian */
    dest[0] = pa[0];
    dest[1] = pa[1];
    dest[2] = pa[2];
    dest[3] = pa[3];
  }
}

/* Encode a block.  This can never fail. */
int32_t cstrings_encode(int32_t max_stringsize, int32_t blocksize,
                        uint8_t* src, uint8_t* dest)
{
  int32_t i, lesize, ssize, csize;
  uint8_t *ip = src, *op = dest, *ipend = src + blocksize;

  op += 4;       /* Reserve first 4 bytes for storing compressed size */
  csize = 4;

  do {
    for (i = 0; i < max_stringsize; i++) {
      if ((ip[i] == 0) || (i == max_stringsize - 1)) {
	if ((ip[i] != 0) && (i == max_stringsize - 1)) {
	  /* Reached end of string and no \0 was found.  Encode max length. */
	  ssize = max_stringsize;
	}
	else {
	  ssize = i;
	}
	lesize = len_encode(ssize, op);
	/* printf("ssize, lesize: %d, %d, ", lesize, ssize); */
	memcpy(op, ip, ssize);	/* does not include the trailing \0 */
	ip += max_stringsize;
	op += ssize;
	csize += lesize + ssize;
	if (csize > blocksize) {
	  /* The filter was unable to reduce the size.  Give up. */
	  return -1;
	}
	break;
      }
    }
  } while (ip < ipend);

  assert(csize <= blocksize);
  printf("csize, blocksize: %d, %d, ", csize, blocksize);
  _sw32(src, csize);
  return csize;
}

/* Decode a block.  This can never fail. */
int32_t cstrings_decode(int32_t max_stringsize, int32_t blocksize,
                        uint8_t* src, uint8_t* dest)
{
  int32_t ssize, csize, tsize = 0;
  uint8_t *ip = src, *op = dest, *ipend = src + blocksize;

  csize = sw32_(src);
  ip += 4;

  do {
    ssize = len_decode(ip);
    printf("ssize(_decode): %d, ", ssize);
    memcpy(op, ip, ssize);
    ip += ssize;
    /* Properly terminate the string if it fits in buffer */
    if (ssize < max_stringsize) {
      op[ssize] = 0;
    }
    op += max_stringsize;
    tsize += max_stringsize;
  } while (ip < ipend);

  //assert(tsize == blocksize);
  return tsize;
}
