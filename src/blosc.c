/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "blosc.h"
#include "blosclz.h"
#include "shuffle.h"

#ifdef WIN32
  #include <windows.h>
#else
  #include <stdint.h>
  #include <unistd.h>
#endif


/* Starting point for the blocksize computation */
#define BLOCKSIZE (4*1024)     /* 4 KB (page size) */

/* The maximum number of splits in a block for compression */
#define MAXSPLITS 16


/* Defaults for compressing/shuffling actions */
int clevel = 5;   /* Medium optimization level */
void set_complevel(int complevel) {
  /* If clevel not in 0..9, do nothing */
  if (clevel < 0 || clevel > 9) {
    fprintf(stderr, "Optimization level must be between 0 and 9!\n");
    return;
  }
  clevel = complevel;
}

int do_shuffle = 1;  /* Shuffle is active by default */
void set_shuffle(int doit) {
  do_shuffle = doit;
}


/* Shuffle & Compress a single block */
static size_t
_blosc_c(size_t typesize, size_t blocksize,
         unsigned char* _src, unsigned char* _dest, unsigned char *tmp) {
  size_t j, neblock, nsplits;
  int cbytes, maxout;
  unsigned int ctbytes = 0;
  unsigned char* _tmp;

  if (do_shuffle && (typesize > 1)) {
    /* Shuffle this block (this makes sense only if typesize > 1) */
    shuffle(typesize, blocksize, _src, tmp);
    _tmp = tmp;
  }
  else {
    _tmp = _src;
  }

  /* Compress for each shuffled slice split for this block. */
  /* If the number of bytes is too large, do not split all. */
  if (typesize <= MAXSPLITS) {
    nsplits = typesize;
  }
  else {
    nsplits = 1;
  }
  neblock = blocksize / nsplits;
  for (j = 0; j < nsplits; j++) {
    _dest += sizeof(int);
    ctbytes += sizeof(int);
    maxout = neblock;
    if (ctbytes+maxout > blocksize) {
      maxout = blocksize - ctbytes;
    }
    cbytes = blosclz_compress(clevel, _tmp+j*neblock, neblock,
                              _dest, maxout);
    if (cbytes > maxout) {
      /* Buffer overrun caused by blosclz_compress (should never happen) */
      return -1;
    }
    if (cbytes < 0) {
      /* cbytes should never be negative */
      return -2;
    }
    if ((cbytes == 0) || (cbytes == (int) neblock)) {
      /* The compressor has been unable to compress data
         significantly.  Also, it may happen that the compressed
         buffer has exactly the same length than the buffer size, but
         this means incompressible data.  Before doing the copy, check
         that we are not running into a buffer overflow. */
      if ((ctbytes+neblock) > blocksize) {
        return 0;    /* Non-compressible data */
      }
      memcpy(_dest, _tmp+j*neblock, neblock);
      cbytes = neblock;
    }
    ((unsigned int *)(_dest))[-1] = cbytes;
    _dest += cbytes;
    ctbytes += cbytes;
  }  /* Close j < nsplits */

  return ctbytes;
}


unsigned int
blosc_compress(size_t typesize, size_t nbytes, const void *src, void *dest)
{
  unsigned char *_src=NULL;   /* Alias for source buffer */
  unsigned char *_dest=NULL;  /* Alias for destination buffer */
  unsigned char *flags;       /* Flags for header */
  size_t nblocks;             /* Number of complete blocks in buffer */
  size_t j;                   /* Local index variables */
  size_t leftover;            /* Extra bytes at end of buffer */
  size_t blocksize;           /* Length of the block in bytes */
  int cbytes;
  unsigned int ctbytes;
  unsigned char *tmp;         /* Temporary buffer for data block */
  const char *too_long_message = "The impossible happened: buffer overflow!\n";
  int i;

  /* Compute a blocksize depending on the optimization level */
  blocksize = BLOCKSIZE;
  /* 3 first optimization levels will not change blocksize */
  for (i=4; i<=clevel; i++) {
    /* Escape if blocksize grows more than nbytes */
    if (blocksize*2 > nbytes) break;
    blocksize *= 2;
  }

  /* blocksize must be a multiple of the typesize */
  blocksize = blocksize / typesize * typesize;

  /* Create temporary area */
#ifdef WIN32
  tmp = _aligned_malloc(blocksize, 16);
#else
  posix_memalign((void **)&tmp, 16, blocksize);
#endif  /* WIN32 */

  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;
  _src = (unsigned char *)(src);
  _dest = (unsigned char *)(dest);

  /* Write header for this block */
  *_dest++ = BLOSC_VERSION_FORMAT;       /* The blosc version format */
  flags = _dest++;                       /* Flags (to be filled later on) */
  *flags = 0;                            /* All bits set to 0 initally */
  ctbytes = 2;
  ((unsigned int *)(_dest))[0] = nbytes; /* The size of the chunk */
  _dest += sizeof(int);
  ctbytes += sizeof(int);

  if (clevel == 0) {
    /* No compression wanted.  Just return without doing anything else. */
    ctbytes = 0;
    goto out;
  }

  if (do_shuffle) {
    /* Signal that shuffle is active */
    *flags = 2;           /* bit 1 set to one, all the rest to 0 */
  }

  /* Write the shuffle header */
  ((unsigned int *)_dest)[0] = typesize;       /* The type size */
  ((unsigned int *)_dest)[1] = blocksize;      /* The block size */
  _dest += 8;
  ctbytes += 8;
  for (j = 0; j < nblocks; j++) {
    cbytes = _blosc_c(typesize, blocksize, _src, _dest, tmp);
    if (cbytes < 0) {
      fprintf(stderr, too_long_message);
      ctbytes = cbytes;    /* Signal error in _blosc_c */
      goto out;
    }
    if (cbytes == 0) {
      ctbytes = 0;    /* Uncompressible data */
      goto out;
    }
    _dest += cbytes;
    _src += blocksize;
    ctbytes += cbytes;
  }  /* Close k < nblocks */

  if(leftover > 0) {
    if (do_shuffle && (typesize > 1)) {
      shuffle(typesize, leftover, _src, tmp);
    }
    else {
      memcpy(tmp, _src, leftover);
    }
    _dest += sizeof(int);
    ctbytes += sizeof(int);
    cbytes = blosclz_compress(clevel, tmp, leftover, _dest, leftover);
    if (cbytes < 0) {
      ctbytes = -3;    /* Signal an error in blosclz_compress */
      goto out;
    }
    if (cbytes > (int) leftover) {
      fprintf(stderr, too_long_message);
      ctbytes = -4;    /* Signal too long compression size */
      goto out;
    }
    if ((cbytes == 0)  || (cbytes == (int) leftover)) {
      /* The compressor has been unable to compress data
         significantly.  Also, it may happen that the compressed
         buffer has exactly the same length than the buffer size, but
         this means incompressible data.  Before doing the copy, check
         that we are not running into a buffer overflow. */
      if ((ctbytes+leftover+sizeof(int)) > nbytes) {
        ctbytes = 0;    /* Uncompressible data */
        goto out;
      }
      memcpy(_dest, tmp, leftover);
      cbytes = leftover;
    }
    ((unsigned int *)(_dest))[-1] = cbytes;
    _dest += cbytes;
    ctbytes += cbytes;
  }

  if (ctbytes >= nbytes) {
    fprintf(stderr, too_long_message);
    ctbytes = -5;       /* Signal too large buffer */
    goto out;
  }

 out:
#ifdef WIN32
  _aligned_free(tmp);
#else
  free(tmp);
#endif  /* WIN32 */
  return ctbytes;

}


/* Decompress & unshuffle a single block */
static size_t
_blosc_d(int do_unshuffle, size_t typesize, size_t blocksize,
         unsigned char* _src, unsigned char* _dest,
         unsigned char *tmp, unsigned char *tmp2)
{
  size_t j, neblock, nsplits;
  size_t nbytes, cbytes, ctbytes = 0, ntbytes = 0;
  unsigned char* _tmp;

  if (do_unshuffle && (typesize > 1)) {
    _tmp = tmp;
  }
  else {
    _tmp = _dest;
  }

  /* Compress for each shuffled slice split for this block. */
  /* If the number of bytes is too large, do not split all. */
  if (typesize <= MAXSPLITS) {
    nsplits = typesize;
  }
  else {
    nsplits = 1;
  }
  neblock = blocksize / nsplits;
  for (j = 0; j < nsplits; j++) {
    cbytes = ((unsigned int *)(_src))[0];  /* The number of compressed bytes */
    _src += sizeof(int);
    ctbytes += sizeof(int);
    /* Uncompress */
    if (cbytes == neblock) {
      memcpy(_tmp, _src, neblock);
      nbytes = neblock;
    }
    else {
      nbytes = blosclz_decompress(_src, cbytes, _tmp, neblock);
      if (nbytes != neblock) {
        return -2;
      }
    }
    _src += cbytes;
    ctbytes += cbytes;
    _tmp += neblock;
    ntbytes += nbytes;
  }

  if (do_unshuffle && (typesize > 1)) {
    if ((uintptr_t)_dest % 16 == 0) {
      /* 16-bytes aligned _dest.  SSE2 unshuffle will work. */
      unshuffle(typesize, blocksize, tmp, _dest);
    }
    else {
      /* _dest is not aligned.  Use tmp2, which is aligned, and copy. */
      unshuffle(typesize, blocksize, tmp, tmp2);
      memcpy(_dest, tmp2, blocksize);
    }
  }
  return ctbytes;
}


unsigned int
blosc_decompress(const void *src, void *dest, size_t dest_size)
{
  unsigned char *_src=NULL;         /* Alias for source buffer */
  unsigned char *_dest=NULL;        /* Alias for destination buffer */
  unsigned char version, flags;     /* Info in compressed header */
  size_t leftover;                  /* Extra bytes at end of buffer */
  size_t nblocks;                   /* Number of complete blocks in buffer */
  size_t j;
  size_t nbytes, dbytes, ntbytes = 0;
  int cbytes;
  unsigned char *tmp, *tmp2;
  int do_unshuffle = 0;
  unsigned int typesize, blocksize;

  _src = (unsigned char *)(src);
  _dest = (unsigned char *)(dest);

  /* Read the header block */
  version = _src[0];                        /* The blosc version */
  flags = _src[1];                          /* The flags for this block */
  _src += 2;
  nbytes = ((unsigned int *)_src)[0];       /* The size of the chunk */

  if (nbytes > dest_size) {
    return -1;
  }
  _src += sizeof(int);

  if (flags == 2) {
    /* Input is shuffled.  Unshuffle it. */
    do_unshuffle = 1;
  }

  /* Read header info */
  typesize = ((unsigned int *)_src)[0];
  blocksize = ((unsigned int *)_src)[1];
  _src += 8;

  /* Compute some params */
  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;

  /* Create temporary area */
#ifdef WIN32
  tmp = _aligned_malloc(blocksize, 16);
  tmp2 = _aligned_malloc(blocksize, 16);
#else
  posix_memalign((void **)&tmp, 16, blocksize);
  posix_memalign((void **)&tmp2, 16, blocksize);
#endif  /* WIN32 */

  for (j = 0; j < nblocks; j++) {
    cbytes = _blosc_d(do_unshuffle, typesize, blocksize,
                      _src, _dest, tmp, tmp2);
    if (cbytes < 0) {
      ntbytes = cbytes;         /* Signal _blosc_d failure */
      goto out;
    }
    _src += cbytes;
    _dest += blocksize;
    ntbytes += blocksize;
  }

  if(leftover > 0) {
    cbytes = ((unsigned int *)(_src))[0];
    _src += sizeof(int);
    if (cbytes == (int) leftover) {
      memcpy(tmp, _src, leftover);
      dbytes = leftover;
    }
    else {
      dbytes = blosclz_decompress(_src, cbytes, tmp, leftover);
      if (dbytes != leftover) {
        ntbytes = -3;           /* Signal too large buffer */
        goto out;
      }
    }
    ntbytes += dbytes;
    if (do_unshuffle && (typesize > 1)) {
      unshuffle(typesize, leftover, tmp, _dest);
    }
    else {
      memcpy(_dest, tmp, leftover);
    }
  }

 out:
#ifdef WIN32
  _aligned_free(tmp);
  _aligned_free(tmp2);
#else
  free(tmp);
  free(tmp2);
#endif  /* WIN32 */
  return ntbytes;
}


