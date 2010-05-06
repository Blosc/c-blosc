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
#include <assert.h>
#include <pthread.h>
#include "blosc.h"
#include "blosclz.h"
#include "shuffle.h"

#ifdef _WIN32
  #include <windows.h>
  #include "stdint-windows.h"
#else
  #include <stdint.h>
  #include <unistd.h>
  #include <inttypes.h>
#endif  /* _WIN32 */



/* Minimal buffer size to be compressed */
#define MIN_BUFFERSIZE 128       /* Cannot be smaller than 66 */

/* Maximum typesize before considering buffer as a stream of bytes. */
#define MAX_TYPESIZE 255         /* Cannot be larger than 255 */

/* The maximum number of splits in a block for compression */
#define MAX_SPLITS 16            /* Cannot be larger than 128 */

/* The maximum number of threads (for some static arrays) */
#define MAX_THREADS 64

/* The size of L1 cache.  32 KB is quite common nowadays. */
#define L1 (32*1024)


/* Global variables for main logic */
int32_t init_temps_done = 0;    /* temporaries for compr/decompr initialized? */
size_t force_blocksize = 0;     /* should we force the use of a blocksize? */

/* Global variables for threads */
int32_t nthreads = 1;            /* number of desired threads in pool */
int32_t init_threads_done = 0;   /* pool of threads initialized? */
int32_t end_threads = 0;         /* should exisiting threads end? */
int32_t giveup;                  /* should (de-)compression give up? */
int32_t nblock = -1;             /* block counter */
pthread_t threads[MAX_THREADS];  /* opaque structure for threads */
int32_t tids[MAX_THREADS];       /* ID per each thread */
pthread_attr_t ct_attr;          /* creation time attributes for threads */

/* Syncronization variables */
pthread_mutex_t count_mutex;
pthread_barrier_t barr_init;
pthread_barrier_t barr_inter;
pthread_barrier_t barr_finish;

/* Structure for parameters in (de-)compression threads */
struct thread_data {
  size_t typesize;
  size_t blocksize;
  int32_t compress;
  int32_t clevel;
  int32_t shuffle;
  int32_t ntbytes;
  uint32_t nbytes;
  uint32_t nblocks;
  uint32_t leftover;
  uint32_t *bstarts;             /* start pointers for each block */
  uint8_t *src;
  uint8_t *dest;
  uint8_t *tmp[MAX_THREADS];
  uint8_t *tmp2[MAX_THREADS];
} params;


/* Structure for parameters meant for keeping track of current temporaries */
struct temp_data {
  int32_t nthreads;
  size_t typesize;
  size_t blocksize;
} current_temp;



/* Shuffle & compress a single block */
static int blosc_c(size_t blocksize, int32_t leftoverblock,
                   uint32_t ntbytes, uint32_t nbytes,
                   uint8_t *src, uint8_t *dest, uint8_t *tmp)
{
  size_t j, neblock, nsplits;
  int32_t cbytes;                   /* number of compressed bytes in split */
  int32_t ctbytes = 0;              /* number of compressed bytes in block */
  int32_t maxout;
  uint8_t *_tmp;
  size_t typesize = params.typesize;

  if (params.shuffle && (typesize > 1)) {
    /* Shuffle this block (this makes sense only if typesize > 1) */
    shuffle(typesize, blocksize, src, tmp);
    _tmp = tmp;
  }
  else {
    _tmp = src;
  }

  /* Compress for each shuffled slice split for this block. */
  /* If typesize is too large, neblock is too small or we are in a
     leftover block, do not split at all. */
  if ((typesize <= MAX_SPLITS) && (blocksize/typesize) >= MIN_BUFFERSIZE &&
      (!leftoverblock)) {
    nsplits = typesize;
  }
  else {
    nsplits = 1;
  }
  neblock = blocksize / nsplits;
  for (j = 0; j < nsplits; j++) {
    dest += sizeof(int32_t);
    ntbytes += sizeof(int32_t);
    ctbytes += sizeof(int32_t);
    maxout = neblock;
    if (ntbytes+maxout > nbytes) {
      maxout = nbytes - ntbytes;   /* avoid buffer overrun */
      if (maxout <= 0) {
        return 0;                  /* non-compressible block */
      }
    }
    cbytes = blosclz_compress(params.clevel, _tmp+j*neblock, neblock,
                              dest, maxout-1);
    if (cbytes >= maxout) {
      /* Buffer overrun caused by blosclz_compress (should never happen) */
      return -1;
    }
    else if (cbytes < 0) {
      /* cbytes should never be negative */
      return -2;
    }
    else if (cbytes == 0) {
      /* The compressor has been unable to compress data significantly. */
      /* Before doing the copy, check that we are not running into a
         buffer overflow. */
      if ((ntbytes+neblock) > nbytes) {
        return 0;    /* Non-compressible data */
      }
      memcpy(dest, _tmp+j*neblock, neblock);
      cbytes = neblock;
    }
    ((uint32_t *)(dest))[-1] = cbytes;
    dest += cbytes;
    ntbytes += cbytes;
    ctbytes += cbytes;
  }  /* Closes j < nsplits */

  return ctbytes;
}


/* Decompress & unshuffle a single block */
static int blosc_d(size_t blocksize, int32_t leftoverblock,
                   uint8_t *src, uint8_t *dest, uint8_t *tmp, uint8_t *tmp2)
{
  int32_t j, neblock, nsplits;
  int32_t nbytes;                /* number of decompressed bytes in split */
  int32_t cbytes;                /* number of compressed bytes in split */
  int32_t ctbytes = 0;           /* number of compressed bytes in block */
  int32_t ntbytes = 0;           /* number of uncompressed bytes in block */
  uint8_t *_tmp;
  size_t typesize = params.typesize;
  size_t shuffle = params.shuffle;

  if (shuffle && (typesize > 1)) {
    _tmp = tmp;
  }
  else {
    _tmp = dest;
  }

  /* Compress for each shuffled slice split for this block. */
  if ((typesize <= MAX_SPLITS) && (blocksize/typesize) >= MIN_BUFFERSIZE &&
      (!leftoverblock)) {
    nsplits = typesize;
  }
  else {
    nsplits = 1;
  }
  neblock = blocksize / nsplits;
  for (j = 0; j < nsplits; j++) {
    cbytes = ((uint32_t *)(src))[0];   /* amount of compressed bytes */
    src += sizeof(int32_t);
    ctbytes += sizeof(int32_t);
    /* Uncompress */
    if (cbytes == neblock) {
      memcpy(_tmp, src, neblock);
      nbytes = neblock;
    }
    else {
      nbytes = blosclz_decompress(src, cbytes, _tmp, neblock);
      if (nbytes != neblock) {
        return -2;
      }
    }
    src += cbytes;
    ctbytes += cbytes;
    _tmp += nbytes;
    ntbytes += nbytes;
  } /* Closes j < nsplits */

  if (shuffle && (typesize > 1)) {
    if ((uintptr_t)dest % 16 == 0) {
      /* 16-bytes aligned dest.  SSE2 unshuffle will work. */
      unshuffle(typesize, blocksize, tmp, dest);
    }
    else {
      /* dest is not aligned.  Use tmp2, which is aligned, and copy. */
      unshuffle(typesize, blocksize, tmp, tmp2);
      memcpy(dest, tmp2, blocksize);
    }
  }

  /* Return the number of uncompressed bytes */
  return ntbytes;
}


/* Serial version for compression/decompression */
int serial_blosc(void)
{
  uint32_t j, bsize, leftoverblock;
  int32_t cbytes;
  int32_t compress = params.compress;
  size_t blocksize = params.blocksize;
  int32_t ntbytes = params.ntbytes;
  int32_t nbytes = params.nbytes;
  uint32_t nblocks = params.nblocks;
  int32_t leftover = params.nbytes % params.blocksize;
  uint32_t *bstarts = params.bstarts;
  uint8_t *src = params.src;
  uint8_t *dest = params.dest;
  uint8_t *tmp = params.tmp[0];     /* tmp for thread 0 */
  uint8_t *tmp2 = params.tmp2[0];   /* tmp2 for thread 0 */

  for (j = 0; j < nblocks; j++) {
    if (compress) {
      bstarts[j] = ntbytes;
    }
    bsize = blocksize;
    leftoverblock = 0;
    if ((j == nblocks - 1) && (leftover > 0)) {
      bsize = leftover;
      leftoverblock = 1;
    }
    if (compress) {
      cbytes = blosc_c(bsize, leftoverblock, ntbytes, nbytes,
                       src+j*blocksize, dest+bstarts[j], tmp);
      if (cbytes == 0) {
        ntbytes = 0;              /* uncompressible data */
        break;
      }
    }
    else {
      cbytes = blosc_d(bsize, leftoverblock,
                       src+bstarts[j], dest+j*blocksize, tmp, tmp2);
    }
    if (cbytes < 0) {
      ntbytes = cbytes;         /* error in blosc_c / blosc_d */
      break;
    }
    ntbytes += cbytes;
  }

  /* Final check for ntbytes (only in compression mode) */
  if (compress) {
    if (ntbytes == nbytes) {
      ntbytes = 0;               /* non-compressible data */
    }
    else if (ntbytes > nbytes) {
      fprintf(stderr, "The impossible happened: buffer overflow!\n");
      ntbytes = -5;               /* too large buffer */
    }
  }  /* Close j < nblocks */

  return ntbytes;
}


/* Threaded version for compression/decompression */
int parallel_blosc(void)
{
  int32_t rc;

  /* Synchronization point for all threads (wait for initialization) */
  rc = pthread_barrier_wait(&barr_init);
  if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    printf("Could not wait on barrier (init)\n");
    exit(-1);
  }
  /* Synchronization point for all threads (wait for finalization) */
  rc = pthread_barrier_wait(&barr_finish);
  if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    printf("Could not wait on barrier (finish)\n");
    exit(-1);
  }

  /* Return the total bytes decompressed in threads */
  return params.ntbytes;
}


/* Convenience functions for creating and releasing temporaries */
void create_temporaries(void)
{
  int32_t tid;
  size_t typesize = params.typesize;
  size_t blocksize = params.blocksize;
  /* Extended blocksize for temporary destination.  Extended blocksize
   is only useful for compression in parallel mode, but it doesn't
   hurt other modes either. */
  size_t ebsize = blocksize + typesize*sizeof(int32_t);
  uint8_t *tmp, *tmp2;

  /* Create temporary area for each thread */
  for (tid = 0; tid < nthreads; tid++) {
#ifdef _WIN32
    tmp = (uint8_t *)_aligned_malloc(blocksize, 16);
    tmp2 = (uint8_t *)_aligned_malloc(ebsize, 16);
#elif defined __APPLE__
    /* Mac OS X guarantees 16-byte alignment in small allocs */
    tmp = (uint8_t *)malloc(blocksize);
    tmp2 = (uint8_t *)malloc(ebsize);
#else
    posix_memalign((void **)&tmp, 16, blocksize);
    posix_memalign((void **)&tmp2, 16, ebsize);
#endif  /* _WIN32 */
    params.tmp[tid] = tmp;
    params.tmp2[tid] = tmp2;
  }

  init_temps_done = 1;
  /* Update params for current temporaries */
  current_temp.nthreads = nthreads;
  current_temp.typesize = typesize;
  current_temp.blocksize = blocksize;

}


void release_temporaries(void)
{
  int32_t tid;
  uint8_t *tmp, *tmp2;

  /* Release buffers */
  for (tid = 0; tid < nthreads; tid++) {
    tmp = params.tmp[tid];
    tmp2 = params.tmp2[tid];
#ifdef _WIN32
    _aligned_free(tmp);
    _aligned_free(tmp2);
#else
    free(tmp);
    free(tmp2);
#endif  /* _WIN32 */
  }

  init_temps_done = 0;

}


/* Do the compression or decompression of the buffer depending on the
   global params. */
int do_job(void) {
  int32_t ntbytes;

  /* Initialize/reset temporaries if needed */
  if (!init_temps_done) {
    create_temporaries();
  }
  else if (current_temp.nthreads != nthreads ||
           current_temp.typesize != params.typesize ||
           current_temp.blocksize != params.blocksize) {
    release_temporaries();
    create_temporaries();
  }

  /* Run the serial version when nthreads is 1 or when the buffers are
     not much larger than blocksize */
  if (nthreads == 1 || (params.nbytes / params.blocksize) <= 1) {
    ntbytes = serial_blosc();
  }
  else {
    ntbytes = parallel_blosc();
  }

  return ntbytes;
}


size_t compute_blocksize(int32_t clevel, size_t typesize, size_t nbytes)
{
  size_t blocksize;

  blocksize = nbytes;           /* Start by a whole buffer as blocksize */

  if (force_blocksize) {
    blocksize = force_blocksize;
    /* Check that forced blocksize is not too small nor too large */
    if (blocksize < MIN_BUFFERSIZE) {
      blocksize = MIN_BUFFERSIZE;
    }
  }
  else if (nbytes >= L1*typesize) {
    blocksize = L1 * typesize;
    if (clevel == 1) {
      blocksize /= 8;
    }
    else if (clevel <= 3) {
      blocksize /= 4;
    }
    else if (clevel <= 6) {
      blocksize /= 2;
    }
    else if (clevel < 9) {
      blocksize *= 1;
    }
    else {
      blocksize *= 2;
    }
  }

  /* Check that blocksize is not too large */
  if (blocksize > nbytes) {
    blocksize = nbytes;
  }

  /* blocksize must be a multiple of the typesize */
  blocksize = blocksize / typesize * typesize;

  return blocksize;
}


unsigned int blosc_compress(int clevel, int shuffle, size_t typesize,
                            size_t nbytes, const void *src, void *dest)
{
  uint8_t *_dest=NULL;         /* current pos for destination buffer */
  uint8_t *flags;              /* flags for header */
  uint32_t nblocks;            /* number of total blocks in buffer */
  uint32_t leftover;           /* extra bytes at end of buffer */
  uint32_t *bstarts;           /* start pointers for each block */
  size_t blocksize;            /* length of the block in bytes */
  uint32_t ntbytes = 0;        /* the number of compressed bytes */
  uint32_t *ntbytes_;          /* placeholder for bytes in output buffer */

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
  else if (nbytes < MIN_BUFFERSIZE) {
    /* Too little buffer.  Just return without doing anything else. */
    return 0;
  }

  /* Shuffle */
  if (shuffle != 0 && shuffle != 1) {
    fprintf(stderr, "`shuffle` parameter must be either 0 or 1!\n");
    return -10;
  }

  /* Get the blocksize */
  blocksize = compute_blocksize(clevel, typesize, nbytes);

  /* Compute number of blocks in buffer */
  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;
  nblocks = (leftover>0)? nblocks+1: nblocks;

  /* Check typesize limits */
  if (typesize > MAX_TYPESIZE) {
    /* If typesize is too large, treat buffer as an 1-byte stream. */
    typesize = 1;
  }

  _dest = (uint8_t *)(dest);
  /* Write header for this block */
  _dest[0] = BLOSC_VERSION_FORMAT;         /* blosc format version */
  _dest[1] = BLOSCLZ_VERSION_FORMAT;       /* blosclz format version */
  flags = _dest+2;                         /* flags */
  _dest[2] = 0;                            /* zeroes flags */
  _dest[3] = (uint8_t)typesize;            /* type size */
  _dest += 4;
  ntbytes += 4;
  ((uint32_t *)_dest)[0] = nbytes;         /* size of the buffer */
  ((uint32_t *)_dest)[1] = blocksize;      /* block size */
  ntbytes_ = (uint32_t *)(_dest+8);        /* compressed buffer size */
  _dest += sizeof(int32_t)*3;
  ntbytes += sizeof(int32_t)*3;
  bstarts = (uint32_t *)_dest;             /* starts for every block */
  _dest += sizeof(int32_t)*nblocks;        /* book space for pointers to */
  ntbytes += sizeof(int32_t)*nblocks;      /* every block in output */

  if (shuffle == 1) {
    /* Shuffle is active */
    *flags |= 0x1;                         /* bit 0 set to one in flags */
  }

  /* Populate parameters for compression routines */
  params.compress = 1;
  params.clevel = clevel;
  params.shuffle = shuffle;
  params.typesize = typesize;
  params.blocksize = blocksize;
  params.ntbytes = ntbytes;
  params.nbytes = nbytes;
  params.nblocks = nblocks;
  params.leftover = leftover;
  params.bstarts = bstarts;
  params.src = (uint8_t *)src;
  params.dest = (uint8_t *)dest;

  /* Do the actual compression */
  ntbytes = do_job();
  /* Set the number of compressed bytes in header */
  *ntbytes_ = ntbytes;

  assert((int32_t)ntbytes < (int32_t)nbytes);
  return ntbytes;
}


unsigned int blosc_decompress(const void *src, void *dest, size_t dest_size)
{
  uint8_t *_src=NULL;            /* current pos for source buffer */
  uint8_t *_dest=NULL;           /* current pos for destination buffer */
  uint8_t version, versionlz;    /* versions for compressed header */
  uint8_t flags;                 /* flags for header */
  int32_t shuffle = 0;           /* do unshuffle? */
  int32_t ntbytes;               /* the number of uncompressed bytes */
  uint32_t nblocks;              /* number of total blocks in buffer */
  uint32_t leftover;             /* extra bytes at end of buffer */
  uint32_t *bstarts;             /* start pointers for each block */
  uint32_t typesize, blocksize, nbytes, ctbytes;

  _src = (uint8_t *)(src);
  _dest = (uint8_t *)(dest);

  /* Read the header block */
  version = _src[0];                   /* blosc format version */
  versionlz = _src[1];                 /* blosclz format version */
  flags = _src[2];                     /* flags */
  typesize = (uint32_t)_src[3];        /* typesize */
  _src += 4;
  nbytes = ((uint32_t *)_src)[0];      /* buffer size */
  blocksize = ((uint32_t *)_src)[1];   /* block size */
  ctbytes = ((uint32_t *)_src)[2];     /* compressed buffer size */
  _src += sizeof(int32_t)*3;
  bstarts = (uint32_t *)_src;
  /* Compute some params */
  /* Total blocks */
  nblocks = nbytes / blocksize;
  leftover = nbytes % blocksize;
  nblocks = (leftover>0)? nblocks+1: nblocks;
  _src += sizeof(int32_t)*nblocks;

  /* Check zero typesizes.  From Blosc version format 2 on, this value
   has been reserved for future use (most probably to indicate
   uncompressible buffers). */
  if ((version == 1) && (typesize == 0)) {
    typesize = 256;             /* 0 means 256 in format version 1 */
  }

  if (nbytes > dest_size) {
    /* This should never happen but just in case */
    return -1;
  }

  if ((flags & 0x1) == 1) {
    /* Input is shuffled.  Unshuffle it. */
    shuffle = 1;
  }

  /* Populate parameters for decompression routines */
  params.compress = 0;
  params.clevel = 0;            /* specific for compression */
  params.shuffle = shuffle;
  params.typesize = typesize;
  params.blocksize = blocksize;
  params.ntbytes = 0;
  params.nbytes = nbytes;
  params.nblocks = nblocks;
  params.leftover = leftover;
  params.bstarts = bstarts;
  params.src = (uint8_t *)src;
  params.dest = (uint8_t *)dest;

  /* Do the actual decompression */
  ntbytes = do_job();

  assert(ntbytes <= (int32_t)dest_size);
  return ntbytes;
}


/* Decompress & unshuffle several blocks in a single thread */
void *t_blosc(void *tids)
{
  int32_t tid = *(int32_t *)tids;
  int32_t cbytes, ntdest;
  uint32_t tblocks;              /* number of blocks per thread */
  uint32_t leftover2;
  uint32_t tblock;               /* limit block on a thread */
  uint32_t nblock_;              /* private copy of nblock */
  int32_t rc;
  uint32_t bsize, leftoverblock;
  /* Parameters for threads */
  size_t blocksize;
  size_t ebsize;
  int32_t compress;
  uint32_t nbytes;
  uint32_t ntbytes;
  uint32_t nblocks;
  uint32_t leftover;
  uint32_t *bstarts;
  uint8_t *src;
  uint8_t *dest;
  uint8_t *tmp;
  uint8_t *tmp2;

  while (1) {
    /* Meeting point for all threads (wait for initialization) */
    rc = pthread_barrier_wait(&barr_init);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
      printf("Could not wait on barrier (init)\n");
      exit(-1);
    }
    if (end_threads) {
      return(0);
    }

    /* Get parameters for this thread */
    blocksize = params.blocksize;
    ebsize = blocksize + params.typesize*sizeof(int32_t);
    compress = params.compress;
    nbytes = params.nbytes;
    ntbytes = params.ntbytes;
    nblocks = params.nblocks;
    leftover = params.leftover;
    bstarts = params.bstarts;
    src = params.src;
    dest = params.dest;
    tmp = params.tmp[tid];
    tmp2 = params.tmp2[tid];

    giveup = 0;
    if (compress) {
      /* Compression always has to follow the block order */
      pthread_mutex_lock(&count_mutex);
      nblock++;
      nblock_ = nblock;
      pthread_mutex_unlock(&count_mutex);
      tblock = nblocks;
    }
    else {
      /* Decompression can happen using any order.  We choose
       sequential block order on each thread */

      /* Blocks per thread */
      tblocks = nblocks / nthreads;
      leftover2 = nblocks % nthreads;
      tblocks = (leftover2>0)? tblocks+1: tblocks;

      nblock_ = tid*tblocks;
      tblock = nblock_ + tblocks;
      if (tblock > nblocks) {
        tblock = nblocks;
      }
      ntbytes = 0;
    }

    /* Loop over blocks */
    leftoverblock = 0;
    while ((nblock_ < tblock) && !giveup) {
      bsize = blocksize;
      if (nblock_ == (nblocks - 1) && (leftover > 0)) {
        bsize = leftover;
        leftoverblock = 1;
      }
      if (compress) {
        cbytes = blosc_c(bsize, leftoverblock, 0, ebsize,
                         src+nblock_*blocksize, tmp2, tmp);
      }
      else {
        cbytes = blosc_d(bsize, leftoverblock,
                         src+bstarts[nblock_], dest+nblock_*blocksize,
                         tmp, tmp2);
      }

      /* Check whether current thread has to giveup */
      /* This is critical in order to not overwrite variables */
      if (giveup != 0) {
        break;
      }

      /* Check results for the decompressed block */
      if (cbytes < 0) {            /* compr/decompr failure */
        /* Set cbytes code error first */
        pthread_mutex_lock(&count_mutex);
        params.ntbytes = cbytes;
        pthread_mutex_unlock(&count_mutex);
        giveup = 1;                 /* give up (de-)compressing after */
        break;
      }

      if (compress) {
        /* Start critical section */
        pthread_mutex_lock(&count_mutex);
        if (cbytes == 0) {            /* uncompressible buffer */
          params.ntbytes = 0;
          pthread_mutex_unlock(&count_mutex);
          giveup = 1;                 /* give up compressing */
          break;
        }
        bstarts[nblock_] = params.ntbytes;   /* update block start counter */
        ntdest = params.ntbytes;
        if (ntdest+cbytes > (int32_t)nbytes) {   /* uncompressible buffer */
          params.ntbytes = 0;
          pthread_mutex_unlock(&count_mutex);
          giveup = 1;
          break;
        }
        nblock++;
        nblock_ = nblock;
        params.ntbytes += cbytes;       /* update return bytes counter */
        pthread_mutex_unlock(&count_mutex);
        /* End of critical section */

        /* Copy the compressed buffer to destination */
        memcpy(dest+ntdest, tmp2, cbytes);
      }
      else {
        nblock_++;
      }
      /* Update counter for this thread */
      ntbytes += cbytes;

    } /* closes while (nblock_) */

    if (!compress && !giveup) {
      /* Update global counter for all threads (decompression only) */
      pthread_mutex_lock(&count_mutex);
      params.ntbytes += ntbytes;
      pthread_mutex_unlock(&count_mutex);
    }

    /* Meeting point for all threads (wait for finalization) */
    rc = pthread_barrier_wait(&barr_finish);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
      printf("Could not wait on barrier (finish)\n");
      exit(-1);
    }
    /* Reset block counter */
    nblock = -1;

  }  /* closes while(1) */

  /* This should never be reached, but anyway */
  return(0);
}


int init_threads(void)
{
  int32_t tid, rc;

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init(&count_mutex, NULL);

  /* Barrier initialization */
  pthread_barrier_init(&barr_init, NULL, nthreads+1);
  pthread_barrier_init(&barr_inter, NULL, nthreads);
  pthread_barrier_init(&barr_finish, NULL, nthreads+1);

  /* Initialize and set thread detached attribute */
  pthread_attr_init(&ct_attr);
  pthread_attr_setdetachstate(&ct_attr, PTHREAD_CREATE_JOINABLE);

  /* Finally, create the threads in detached state */
  for (tid = 0; tid < nthreads; tid++) {
    tids[tid] = tid;
    rc = pthread_create(&threads[tid], &ct_attr, t_blosc, (void *)&tids[tid]);
    if (rc) {
      fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
      fprintf(stderr, "\tError detail: %s\n", strerror(rc));
      exit(-1);
    }
  }

  init_threads_done = 1;                 /* Initialization done! */

  return(0);
}


int blosc_set_nthreads(int nthreads_new)
{
  int32_t nthreads_old = nthreads;
  int32_t t, rc;
  void *status;

  if (nthreads_new > MAX_THREADS) {
    fprintf(stderr, "Error.  nthreads cannot be larger than MAX_THREADS (%d)",
            MAX_THREADS);
    return -1;
  }
  else if (nthreads_new <= 0) {
    fprintf(stderr, "Error.  nthreads must be a positive integer");
    return -1;
  }
  else if (nthreads_new != nthreads) {
    if (nthreads > 1 && init_threads_done) {
      /* Tell all existing threads to end */
      end_threads = 1;
      rc = pthread_barrier_wait(&barr_init);
      if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier (init)\n");
        exit(-1);
      }
      /* Join exiting threads */
      for (t=0; t<nthreads; t++) {
        rc = pthread_join(threads[t], &status);
        if (rc) {
          fprintf(stderr, "ERROR; return code from pthread_join() is %d\n", rc);
          fprintf(stderr, "\tError detail: %s\n", strerror(rc));
          exit(-1);
        }
      }
      init_threads_done = 0;
      end_threads = 0;
    }
    nthreads = nthreads_new;
    if (nthreads > 1) {
      /* Launch a new pool of threads */
      init_threads();
    }
    return nthreads_old;
  }
  return 0;
}


/* Free possible memory temporaries and thread resources */
void blosc_free_resources(void)
{
  int32_t t, rc;
  void *status;

  /* Release temporaries */
  if (init_temps_done) {
    release_temporaries();
  }

  /* Finish the possible thread pool */
  if (nthreads > 1 && init_threads_done) {
    /* Tell all existing threads to end */
    end_threads = 1;
    rc = pthread_barrier_wait(&barr_init);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
      exit(-1);
    }
    /* Join exiting threads */
    for (t=0; t<nthreads; t++) {
      rc = pthread_join(threads[t], &status);
      if (rc) {
        fprintf(stderr, "ERROR; return code from pthread_join() is %d\n", rc);
        fprintf(stderr, "\tError detail: %s\n", strerror(rc));
        exit(-1);
      }
    }

    /* Release mutex and condition variable objects */
    pthread_mutex_destroy(&count_mutex);

    /* Barriers */
    pthread_barrier_destroy(&barr_init);
    pthread_barrier_destroy(&barr_inter);
    pthread_barrier_destroy(&barr_finish);

    /* Thread attributes */
    pthread_attr_destroy(&ct_attr);

    init_threads_done = 0;
    end_threads = 0;
  }
}


/* Force the use of a specific blocksize.  If 0, an automatic
   blocksize will be used (the default). */
void blosc_set_blocksize(size_t size)
{
  force_blocksize = size;
}

