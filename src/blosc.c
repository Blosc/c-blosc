/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)
  Creation date: 2009-05-20

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
#ifdef _WIN32
  #include <windows.h>
#else
  #include <stdint.h>
  #include <unistd.h>
#endif  /* _WIN32 */


/* Starting point for the blocksize computation */
#define BLOCKSIZE (4*1024)      /* 4 KB (page size) */

/* Maximum typesize before considering buffer as a stream of bytes. */
#define MAXTYPESIZE 255         /* Cannot be larger than 255 */

/* The maximum number of splits in a block for compression */
#define MAXSPLITS 16         /* Cannot be larger than 128 */


/* Global variables for compressing/shuffling actions */
int clevel;                     /* Compression level */
int do_shuffle;                 /* Shuffle is active? */



/* Shuffle & Compress a single block */
static size_t
_blosc_c(int clevel, int doshuffle, size_t typesize, size_t blocksize,
         int leftoverblock, int ctbytes, int nbytes,
         unsigned char* _src, unsigned char* _dest, unsigned char *tmp) {
  size_t j, neblock, nsplits;
  int cbytes, maxout;
  unsigned int btbytes = 0;
  unsigned char* _tmp;

  if (doshuffle && (typesize > 1)) {
    /* Shuffle this block (this makes sense only if typesize > 1) */
    shuffle(typesize, blocksize, _src, tmp);
    _tmp = tmp;
  }
  else {
    _tmp = _src;
  }

  /* Compress for each shuffled slice split for this block. */
  /* If the number of bytes is too large, or we are in a leftover
     block, do not split all. */
  if ((typesize <= MAXSPLITS) && (!leftoverblock)) {
    nsplits = typesize;
  }
  else {
    nsplits = 1;
  }
  neblock = blocksize / nsplits;
  for (j = 0; j < nsplits; j++) {
    _dest += sizeof(int);
    btbytes += sizeof(int);
    ctbytes += sizeof(int);
    maxout = neblock;
    if (ctbytes+maxout > nbytes) {
      maxout = nbytes - ctbytes;   /* avoid buffer overrun */
      if (maxout <= 0) {
        return 0;                  /* non-compressible block */
      }
    }
    cbytes = blosclz_compress(clevel, _tmp+j*neblock, neblock,
                              _dest, maxout);
    if (cbytes > maxout) {
      /* Buffer overrun caused by blosclz_compress (should never happen) */
      return -1;
    }
    else if (cbytes < 0) {
      /* cbytes should never be negative */
      return -2;
    }
    else if ((cbytes == 0) || (cbytes == (int) neblock)) {
      /* The compressor has been unable to compress data
         significantly.  Also, it may happen that the compressed
         buffer has exactly the same length than the buffer size, but
         this means uncompressible data.  Before doing the copy, check
         that we are not running into a buffer overflow. */
      if ((ctbytes+neblock) > nbytes) {
        return 0;    /* Non-compressible data */
      }
      memcpy(_dest, _tmp+j*neblock, neblock);
      cbytes = neblock;
    }
    ((unsigned int *)(_dest))[-1] = cbytes;
    _dest += cbytes;
    btbytes += cbytes;
    ctbytes += cbytes;
  }  /* Closes j < nsplits */

  return btbytes;
}


unsigned int
blosc_compress(int clevel, int doshuffle, size_t typesize, size_t nbytes,
               const void *src, void *dest)
{
  unsigned char *_src=NULL;        /* alias for source buffer */
  unsigned char *_dest=NULL;       /* alias for destination buffer */
  unsigned char *flags;            /* flags for header */
  unsigned int *starts;            /* start pointers for each block */
  size_t nblocks;                  /* number of complete blocks in buffer */
  size_t tblocks;                  /* number of total blocks in buffer */
  size_t leftover;                 /* extra bytes at end of buffer */
  size_t blocksize;                /* length of the block in bytes */
  size_t bsize;                    /* corrected blocksize for last block */
  unsigned int ctbytes = 0;        /* the number of bytes in output buffer */
  unsigned int *ctbytes_;          /* the number of bytes in output buffer */
  unsigned char *tmp;              /* temporary buffer for data block */
  int cbytes;                      /* temporary compressed buffer length */
  int maxout;                      /* maximum length for leftover compression */
  int leftoverblock;               /* left over block? */
  int i, j;                        /* local index variables */
  const char *too_long_message = "The impossible happened: buffer overflow!\n";

  /* Compression level */
  if (clevel < 0 || clevel > 9) {
    /* If clevel not in 0..9, print an error */
    fprintf(stderr, "`clevel` parameter must be between 0 and 9!\n");
    return -10;
  }
  else if (clevel == 0) {
    /* No compression wanted.  Just return without doing anything else. */
    return 0;
  }

  /* Shuffle */
  if (doshuffle != 0 && doshuffle != 1) {
    /* If shuffle not in 0,1, print an error */
    fprintf(stderr, "`doshuffle` parameter must be either 0 or 1!\n");
    return -10;
  }

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
#ifdef _WIN32
  tmp = (unsigned char *)_aligned_malloc(blocksize, 16);
#else
  posix_memalign((void **)&tmp, 16, blocksize);
#endif  /* _WIN32 */

  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;
  _src = (unsigned char *)(src);
  _dest = (unsigned char *)(dest);
  tblocks = (leftover>0)? nblocks+1: nblocks;
  /* If typesize is too large, treat buffer as an 1-byte stream. */
  if (typesize > MAXTYPESIZE) {
    typesize = 1;
  }

  /* Write header for this block */
  _dest[0] = BLOSC_VERSION_FORMAT;         /* blosc format version */
  _dest[1] = BLOSCLZ_VERSION_FORMAT;       /* blosclz format version */
  _dest[2] = (unsigned char)typesize;      /* type size */
  flags = _dest+3;                         /* flags */
  *flags = 0;                              /* zeroes flags */
  _dest += 4;
  ctbytes += 4;
  ctbytes_ = (unsigned int *)_dest;        /* compressed chunk size (pointer) */
  ((unsigned int *)_dest)[1] = nbytes;     /* size of the chunk */
  ((unsigned int *)_dest)[2] = blocksize;  /* block size */
  _dest += sizeof(int)*3;
  ctbytes += sizeof(int)*3;
  starts = (unsigned int *)_dest;          /* starts for every block */
  _dest += sizeof(int)*tblocks;            /* book space for pointers to */
  ctbytes += sizeof(int)*tblocks;          /* every block in output */

  if (doshuffle == 1) {
    /* Shuffle is active */
    *flags |= 0x1;                         /* bit 0 set to one in flags */
  }

  for (j = 0; j < tblocks; j++) {
    starts[j] = ctbytes;
    bsize = blocksize;
    leftoverblock = 0;
    if ((j == tblocks - 1) && (leftover > 0)) {
      bsize = leftover;
      leftoverblock = 1;
    }
    cbytes = _blosc_c(clevel, doshuffle, typesize, bsize, leftoverblock,
                      ctbytes, nbytes, _src, _dest, tmp);
    if (cbytes < 0) {
      fprintf(stderr, too_long_message);
      ctbytes = cbytes;         /* error in _blosc_c */
      goto out;
    }
    if (cbytes == 0) {
      ctbytes = 0;              /* uncompressible data */
      goto out;
    }
    _dest += cbytes;
    _src += blocksize;
    ctbytes += cbytes;
  }  /* Close j < tblocks */

  if (ctbytes == nbytes) {
    ctbytes = 0;               /* non-compressible data */
    goto out;
  }
  else if (ctbytes > nbytes) {
    fprintf(stderr, too_long_message);
    ctbytes = -5;               /* too large buffer */
    goto out;
  }

 out:
#ifdef _WIN32
  _aligned_free(tmp);
#else
  free(tmp);
#endif  /* _WIN32 */

  *ctbytes_ = ctbytes;   /* set the number of compressed bytes in header */
  return ctbytes;

}


/* Decompress & unshuffle a single block */
static size_t
_blosc_d(int dounshuffle, size_t typesize, size_t blocksize, int leftoverblock,
         unsigned char* _src, unsigned char* _dest,
         unsigned char *tmp, unsigned char *tmp2)
{
  size_t j, neblock, nsplits;
  size_t nbytes, cbytes, ctbytes = 0, ntbytes = 0;
  unsigned char* _tmp;

  if (dounshuffle && (typesize > 1)) {
    _tmp = tmp;
  }
  else {
    _tmp = _dest;
  }

  /* Compress for each shuffled slice split for this block. */
  /* If the number of bytes is too large, do not split all. */
  if ((typesize <= MAXSPLITS) && (!leftoverblock)) {
    nsplits = typesize;
  }
  else {
    nsplits = 1;
  }
  neblock = blocksize / nsplits;
  for (j = 0; j < nsplits; j++) {
    cbytes = ((unsigned int *)(_src))[0]; /* amount of compressed bytes */
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
  } /* Closes j < nsplits */

  if (dounshuffle && (typesize > 1)) {
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
  unsigned char *_src=NULL;          /* alias for source buffer */
  unsigned char *_dest=NULL;         /* alias for destination buffer */
  unsigned char version, versionlz;  /* versions for compressed header */
  unsigned char flags;               /* flags for header */
  size_t leftover;                   /* extra bytes at end of buffer */
  size_t nblocks;                    /* number of complete blocks in buffer */
  size_t tblocks;                    /* number of total blocks in buffer */
  size_t j;
  size_t nbytes, dbytes, ntbytes = 0;
  int cbytes;
  unsigned char *tmp, *tmp2;
  int dounshuffle = 0;
  unsigned int typesize, blocksize, bsize, ctbytes_;
  int leftoverblock;               /* left over block? */

  _src = (unsigned char *)(src);
  _dest = (unsigned char *)(dest);

  /* Read the header block */
  version = _src[0];                        /* blosc format version */
  versionlz = _src[1];                      /* blosclz format version */
  typesize = (unsigned int)_src[2];         /* typesize */
  flags = _src[3];                          /* flags */
  _src += 4;
  ctbytes_ = ((unsigned int *)_src)[0];     /* compressed chunk size */
  nbytes = ((unsigned int *)_src)[1];       /* chunk size */
  blocksize = ((unsigned int *)_src)[2];    /* block size */
  _src += sizeof(int)*3;
  /* Compute some params */
  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;
  tblocks = (leftover>0)? nblocks+1: nblocks;
  _src += sizeof(int)*tblocks;              /* skip starts of blocks */

  if (nbytes > dest_size) {
    /* This should never happen, but just in case. */
    return -1;
  }

  if ((flags & 0x1) == 1) {
    /* Input is shuffled.  Unshuffle it. */
    dounshuffle = 1;
  }

  /* Create temporary area */
#ifdef _WIN32
  tmp = (unsigned char*)_aligned_malloc(blocksize, 16);
  tmp2 = (unsigned char*)_aligned_malloc(blocksize, 16);
#else
  posix_memalign((void **)&tmp, 16, blocksize);
  posix_memalign((void **)&tmp2, 16, blocksize);
#endif  /* _WIN32 */

  for (j = 0; j < tblocks; j++) {
    bsize = blocksize;
    leftoverblock = 0;
    if ((j == tblocks - 1) && (leftover > 0)) {
      bsize = leftover;
      leftoverblock = 1;
    }
    cbytes = _blosc_d(dounshuffle, typesize, bsize, leftoverblock,
                      _src, _dest, tmp, tmp2);
    if (cbytes < 0) {
      ntbytes = cbytes;          /* _blosc_d failure */
      goto out;
    }
    _src += cbytes;
    _dest += blocksize;
    ntbytes += blocksize;
  }

 out:
#ifdef _WIN32
  _aligned_free(tmp);
  _aligned_free(tmp2);
#else
  free(tmp);
  free(tmp2);
#endif  /* _WIN32 */
  return ntbytes;
}


