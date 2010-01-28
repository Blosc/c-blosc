/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "blosc.h"
#include "blosclz.h"
#include "shuffle.h"



/* Starting point for the blocksize computation */
#define BLOCKSIZE (4*1024)     /* 4 KB (page size) */

// Block sizes for optimal use of the first-level cache
//#define BLOCKSIZE (2*1024)  /* 2K */
#define BLOCKSIZE (4*1024)  /* 4K */  /* Page size.  Optimal for P4. */
//#define BLOCKSIZE (8*1024)  /* 8K */  /* Seems optimal for Core2 and P4. */
//#define BLOCKSIZE (16*1024) /* 16K */  /* Seems optimal for Core2. */
//#define BLOCKSIZE (32*1024) /* 32K */


/* Defaults for compressing/shuffling actions */
int clevel = 5;   /* Medium optimization level */
void set_complevel(int complevel) {
  /* If clevel not in 0..9, do nothing */
  if (clevel < 0 || clevel > 9) {
    fprintf(stderr, "Optimization level must be between 0 and 9!\n");
    fsync(2);
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
    //memcpy(tmp, _src, blocksize);
    //_tmp = tmp;
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
  unsigned char *_src=NULL;   // Alias for source buffer
  unsigned char *_dest=NULL;  // Alias for destination buffer
  unsigned char *flags;       // Flags for header
  size_t nblocks;             // Number of complete blocks in buffer
  size_t neblock;             // Number of elements in block
  size_t j;                   // Local index variables
  size_t leftover;            // Extra bytes at end of buffer
  size_t blocksize;           // Length of the block in bytes
  unsigned int cbytes, ctbytes;
  // Temporary buffer for data block
  unsigned char *tmp;

  switch(opt_level) {
  case 4:
    blocksize = BLOCKSIZE * 2;
    break;
  case 5:
    blocksize = BLOCKSIZE * 4;
    break;
  case 6:
    blocksize = BLOCKSIZE * 8;
    break;
  case 7:
    blocksize = BLOCKSIZE * 16;
    break;
  case 8:
    blocksize = BLOCKSIZE * 32;
    break;
  case 9:
    blocksize = BLOCKSIZE * 64;
    break;
  default:
    blocksize = BLOCKSIZE;
  }

  // Create temporary area
  posix_memalign((void **)&tmp, 64, blocksize);

  nblocks = nbytes / blocksize;
  neblock = blocksize / typesize;
  leftover = nbytes % blocksize;
  _src = (unsigned char *)(src);
  _dest = (unsigned char *)(dest);

  // Write header for this block
  *_dest++ = BLOSC_VERSION;              // The blosc version
  flags = _dest++;                       // Flags (to be filled later on)
  *flags = 0;                            // All bits set to 0 initally
  ctbytes = 2;
  ((unsigned int *)(_dest))[0] = nbytes; // The size of the chunk
  _dest += sizeof(int);
  ctbytes += sizeof(int);

  if (opt_level == 0) {
    // No compression wanted.  Just do a memcpy a return.
    *flags = 4;                          // bit 2 set to 1 means no compression
    memcpy(_dest, _src, nbytes);
    ctbytes += nbytes;
    free(tmp);
    return ctbytes;
  }

  if (do_shuffle) {
    /* Signal that shuffle is active */
    *flags = 2;           /* bit 1 set to one, all the rest to 0 */
  }
  // Store the cummulative 'and' register in memory
  _mm_storeu_si128(ToVectorAddress(cmpresult), andreg);
  // Are all values equal?
  eqval = strncmp(cmpresult, ones, 16);
  if (eqval == 0) {
    // Trivial repetition pattern found
    *flags = 1;           // bit 0 set to one, all the rest to 0
    *_dest++ = 16;        // 16 repeating byte
    _mm_storeu_si128(ToVectorAddress(_dest[0]), value);    // The repeated bytes
    ctbytes += 1 + 16;
    free(tmp);
    return ctbytes;
  }
#endif  // __SSE2__

  if (do_shuffle) {
    // Signal that shuffle is active
    *flags = 2;           // bit 1 set to one, all the rest to 0
  }
  // Write the shuffle header
  ((unsigned int *)_dest)[0] = typesize;          // The type size
  ((unsigned int *)_dest)[1] = blocksize;         // The block size
  _dest += 8;
  ctbytes += 8;
  for (j = 0; j < nblocks; j++) {
    cbytes = _blosc_c(typesize, blocksize, _src, _dest, tmp);
    if (cbytes == 0) {
      free(tmp);
      return 0;    // Uncompressible data
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
      free(tmp);
      return -3;
    }
    if (cbytes > (int) leftover) {
      fprintf(stderr, too_long_message);
      fsync(2);
      free(tmp);
      return -4;
    }
    if ((cbytes == 0)  || (cbytes == (int) leftover)) {
      /* The compressor has been unable to compress data
         significantly.  Also, it may happen that the compressed
         buffer has exactly the same length than the buffer size, but
         this means incompressible data.  Before doing the copy, check
         that we are not running into a buffer overflow. */
      if ((ctbytes+leftover+sizeof(int)) > nbytes) {
        free(tmp);
        return 0;    /* Uncompressible data */
      }
      memcpy(_dest, tmp, leftover);
      cbytes = leftover;
    }
    ((unsigned int *)(_dest))[-1] = cbytes;
    _dest += cbytes;
    ctbytes += cbytes;
  }

  free(tmp);
  return ctbytes;
}


/* Decompress & unshuffle a single block */
static size_t
_blosc_d(int do_unshuffle, size_t typesize, size_t blocksize,
         unsigned char* _src, unsigned char* _dest, unsigned char *tmp)
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
  //_tmp = tmp;

  for (j = 0; j < typesize; j++) {
    cbytes = ((unsigned int *)(_src))[0];       // The number of compressed bytes
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
    unshuffle(typesize, blocksize, tmp, _dest);
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
  unsigned char *tmp;
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

  if (flags == 4) {       /* No compression */
    /* Just do a memcpy and return */
    memcpy(_dest, _src, nbytes);
    return nbytes;
  }

  if (flags == 2) {
    /* Input is shuffled.  Unshuffle it. */
    do_unshuffle = 1;
  }

  // Read header info
  unsigned int typesize = ((unsigned int *)_src)[0];
  unsigned int blocksize = ((unsigned int *)_src)[1];
  _src += 8;

  // Compute some params
  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;

  /* Create temporary area */
  posix_memalign((void **)&tmp, 64, blocksize);

  for (j = 0; j < nblocks; j++) {
    cbytes = _blosc_d(do_unshuffle, typesize, blocksize, _src, _dest, tmp);
    if (cbytes < 0) {
      free(tmp);
      return cbytes;
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
        free(tmp);
        return -3;
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

  free(tmp);
  return ntbytes;
}


int main(void) {
    unsigned int nbytes, cbytes;
    void *src, *dest, *srccpy;
    size_t i;
    clock_t last, current;
    float tmemcpy, tshuf, tunshuf;
    int size = 32*1024;         /* Buffer size */
    unsigned int elsize;                 /* Datatype size */

    posix_memalign((void **)&src, 64, size+6);
    posix_memalign((void **)&srccpy, 64, size+6);
    /* dest buffer must be aligned to 16 bytes at least! */
    posix_memalign((void **)&dest, 64, size+6);

    srand(1);

    /* Initialize the original buffer */
    /* float* _src = (float *)src; */
    /* float* _srccpy = (float *)srccpy; */
    /* elsize = sizeof(long long); */
    /* for(long long l = 0; l < SIZE/sizeof(long long); ++l){ */
    /*   ((long long *)_src)[l] = l; */
    /*   ((long long *)_src)[l] = rand() >> 24; */
    /*   ((long long *)_src)[l] = 1; */
    int* _src = (int *)src;
    int* _srccpy = (int *)srccpy;
    elsize = sizeof(int);
    for(i = 0; i < size/elsize; ++i) {
      /* Choose one below */
      /* _src[i] = 1; */
      /* _src[i] = 0x01010101; */
      /* _src[i] = 0x01020304; */
      /* _src[i] = i * 1/.3; */
      /* _src[i] = i; */
      /* _src[i] = rand() >> 30; */
      /* _src[i] = rand() >> 24; */
      _src[i] = rand() >> 22;
      /* _src[i] = rand() >> 13; */
      /* _src[i] = rand() >> 16; */
      /* _src[i] = rand() >> 6;    /\* minimum for getting compression *\/ */
    }

    /* /\* For data coming from a file *\/ */
    /* int fd; */
    /* ssize_t rbytes; */
    /* fd = open("32KB-block-24B-typesize.data", 0); */
    /* off_t seek = lseek(fd, 0, SEEK_SET); */
    /* rbytes = read(fd, src, 32*1024); */
    /* /\* rbytes = read(fd, src, 8192); *\/ */
    /* printf("Read %zd bytes with %zd seek\n", rbytes, seek); */
    /* if (rbytes == -1) { */
    /*   perror(NULL); */
    /* } */
    /* close(fd); */
    /* size = rbytes; */

    set_complevel(1);
    set_shuffle(1);

    memcpy(srccpy, src, size);

    last = clock();
    for (i = 0; i < NITER; i++) {
        memcpy(dest, src, size);
    }
    current = clock();
    tmemcpy = (current-last)/((float)CLK_NITER);
    printf("memcpy:\t\t %fs, %.1f MB/s\n", tmemcpy, size/(tmemcpy*MB));

    last = clock();
    for (i = 0; i < NITER; i++) {
        cbytes = blosc_compress(elsize, size, src, dest);
    }
    current = clock();
    tshuf = (current-last)/((float)CLK_NITER);
    printf("blosc_compress:\t %fs, %.1f MB/s\t", tshuf, size/(tshuf*MB));
    printf("Orig bytes: %d  Final bytes: %d\n", size, cbytes);

    last = clock();
    for (i = 0; i < NITER; i++)
      if (cbytes == 0) {
        memcpy(dest, src, size);
        nbytes = size;
      }
      else {
        nbytes = blosc_decompress(dest, src, size);
      }
    current = clock();
    tunshuf = (current-last)/((float)CLK_NITER);
    printf("blosc_d:\t %fs, %.1f MB/s\t", tunshuf, nbytes/(tunshuf*MB));
    printf("Orig bytes: %d  Final bytes: %d\n", cbytes, nbytes);

    /* Check if data has had a good roundtrip */
    _src = (int *)src;
    _srccpy = (int *)srccpy;
    for(i = 0; i < size/sizeof(int); ++i){
       if (_src[i] != _srccpy[i]) {
           printf("Error: original data and round-trip do not match in pos %d\n", (int)i);
           printf("Orig--> %x, Copy--> %x\n", _src[i], _srccpy[i]);
           goto out;
       }
    }

 out:
    free(src); free(srccpy);  free(dest);
    return 0;
}

