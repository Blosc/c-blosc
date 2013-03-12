/*********************************************************************
  Small benchmark for testing basic capabilities of Blosc.

  You can select different degrees of 'randomness' in input buffer, as
  well as external datafiles (uncomment the lines after "For data
  coming from a file" comment).

  For usage instructions of this benchmark, please see:

    http://blosc.pytables.org/trac/wiki/SyntheticBenchmarks

  I'm collecting speeds for different machines, so the output of your
  benchmarks and your processor specifications are welcome!

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_WIN32) && !defined(__MINGW32__)
  #include <time.h>
#else
  #include <unistd.h>
  #include <sys/time.h>
#endif
#include <math.h>

#if defined(_WIN32)
  #include "win32/pthread.h"
  #include "win32/pthread.c"
#else
  #include <pthread.h>
#endif

struct bench_wrap_args 
{
  int nthreads;
  int size;
  int elsize;
  int rshift;
  FILE * output_file;
};

void *bench_wrap(void * args);

#include "../blosc/blosc.h"

#define KB  1024
#define MB  (1024*KB)
#define GB  (1024*MB)

#define NCHUNKS (32*1024)       /* maximum number of chunks */
#define MAX_THREADS 16


int nchunks = NCHUNKS;
int niter = 3;                  /* default number of iterations */
double totalsize = 0.;          /* total compressed/decompressed size */

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    tmpres /= 10;  /*convert into microseconds*/
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }

  return 0;
}
#endif   /* _WIN32 */


/* Given two timeval stamps, return the difference in seconds */
float getseconds(struct timeval last, struct timeval current) {
  int sec, usec;

  sec = current.tv_sec - last.tv_sec;
  usec = current.tv_usec - last.tv_usec;
  return (float)(((double)sec + usec*1e-6));
}

/* Given two timeval stamps, return the time per chunk in usec */
float get_usec_chunk(struct timeval last, struct timeval current) {
  return (float)(getseconds(last, current)/(niter*nchunks)*1e6);
}


int get_value(int i, int rshift) {
  int v;

  v = (i<<26)^(i<<18)^(i<<11)^(i<<3)^i;
  if (rshift < 32) {
    v &= (1 << rshift) - 1;
  }
  return v;
}


void init_buffer(void *src, int size, int rshift) {
  unsigned int i;
  int *_src = (int *)src;

  /* To have reproducible results */
  srand(1);

  /* Initialize the original buffer */
  for (i = 0; i < size/sizeof(int); ++i) {
    /* Choose one below */
    //_src[i] = 0;
    //_src[i] = 0x01010101;
    //_src[i] = 0x01020304;
    //_src[i] = i * 1/.3;
    //_src[i] = i;
    //_src[i] = rand() >> (32-rshift);
    _src[i] = get_value(i, rshift);
  }
}


void do_parallel_bench(int parallel_threads, int nthreads, int size, int elsize, int rshift) {
  pthread_t threads_[MAX_THREADS];
  FILE * output_files[MAX_THREADS];
  static int bench_ncall;
  char filename[] = "suite_parallel_pt-XX_nt-XX_XXX.out";
  struct bench_wrap_args arg_wrap;
  arg_wrap.nthreads = nthreads;
  arg_wrap.size = size;
  arg_wrap.elsize = elsize;
  arg_wrap.rshift = rshift;

  int tid, rc;
  for (tid = 0; tid < parallel_threads; tid++) {
    bench_ncall++;
    sprintf(filename, "suite_parallel_pt-%02d_nt-%02d_%03d.out", parallel_threads, nthreads, bench_ncall);
    fprintf(stdout, "Thread %d started, writing output to %s...\n", tid, filename);
    output_files[tid] = fopen(filename, "w");
    arg_wrap.output_file = output_files[tid];
    if (output_files[tid] == NULL) {
      fprintf(stderr, "ERROR; Unable to open suite_parallel_pt-%02d_nt-%02d_%03d.out for writing\n", 
	      parallel_threads, nthreads, bench_ncall);
      exit(-1);
    }
#if !defined(_WIN32)
    rc = pthread_create(&threads_[tid], NULL, bench_wrap, (void *) &arg_wrap);//(void *)&tids[tid]);
#else
    rc = pthread_create(&threads_[tid], NULL, bench_wrap, (void *) &arg_wrap)//(void *)&tids[tid]);
#endif  
    if (rc) {
      fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
      fprintf(stderr, "\tError detail: %s\n", strerror(rc));
      exit(-1);
    }
  }
  for (tid = 0; tid < parallel_threads; tid++) {
      rc = pthread_join(threads_[tid], NULL);
      fclose(output_files[tid]);
  }
}


void do_bench(int nthreads, int size, int elsize, int rshift, FILE * ofile) {
  void *src, *srccpy;
  void **dest[NCHUNKS], *dest2;
  int nbytes = 0, cbytes = 0;
  int i, j;
  struct timeval last, current;
  float tmemcpy, tshuf, tunshuf;
  int clevel, doshuffle=1;
  unsigned char *orig, *round;

  blosc_set_nthreads(nthreads);

  /* Initialize buffers */
  src = malloc(size);
  srccpy = malloc(size);
  dest2 = malloc(size);
  /* zero src to initialize byte on it, and not only multiples of 4 */
  memset(src, 0, size);
  init_buffer(src, size, rshift);
  memcpy(srccpy, src, size);
  for (j = 0; j < nchunks; j++) {
    dest[j] = malloc(size+BLOSC_MAX_OVERHEAD);
  }

  /* Warm destination memory (memcpy() will go a bit faster later on) */
  for (j = 0; j < nchunks; j++) {
    memcpy(dest[j], src, size);
  }

  fprintf(ofile, "--> %d, %d, %d, %d\n", nthreads, size, elsize, rshift);
  fprintf(ofile, "********************** Run info ******************************\n");
  fprintf(ofile, "Blosc version: %s (%s)\n", BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);
  fprintf(ofile, "Using synthetic data with %d significant bits (out of 32)\n", rshift);
  fprintf(ofile, "Dataset size: %d bytes\tType size: %d bytes\n", size, elsize);
  fprintf(ofile, "Working set: %.1f MB\t\t", (size*nchunks) / (float)MB);
  fprintf(ofile, "Number of threads: %d\n", nthreads);
  fprintf(ofile, "********************** Running benchmarks *********************\n");

  gettimeofday(&last, NULL);
  for (i = 0; i < niter; i++) {
    for (j = 0; j < nchunks; j++) {
      memcpy(dest[j], src, size);
    }
  }
  gettimeofday(&current, NULL);
  tmemcpy = get_usec_chunk(last, current);
  fprintf(ofile, "memcpy(write):\t\t %6.1f us, %.1f MB/s\n",
         tmemcpy, size/(tmemcpy*MB/1e6));

  gettimeofday(&last, NULL);
  for (i = 0; i < niter; i++) {
    for (j = 0; j < nchunks; j++) {
      memcpy(dest2, dest[j], size);
    }
  }
  gettimeofday(&current, NULL);
  tmemcpy = get_usec_chunk(last, current);
  fprintf(ofile, "memcpy(read):\t\t %6.1f us, %.1f MB/s\n",
         tmemcpy, size/(tmemcpy*MB/1e6));

  for (clevel=0; clevel<10; clevel++) {

    fprintf(ofile, "Compression level: %d\n", clevel);

    gettimeofday(&last, NULL);
    for (i = 0; i < niter; i++) {
      for (j = 0; j < nchunks; j++) {
        cbytes = blosc_compress(clevel, doshuffle, elsize, size, src,
                                dest[j], size+BLOSC_MAX_OVERHEAD);
      }
    }
    gettimeofday(&current, NULL);
    tshuf = get_usec_chunk(last, current);
    fprintf(ofile, "comp(write):\t %6.1f us, %.1f MB/s\t  ",
           tshuf, size/(tshuf*MB/1e6));
    fprintf(ofile, "Final bytes: %d  ", cbytes);
    if (cbytes > 0) {
      fprintf(ofile, "Ratio: %3.2f", size/(float)cbytes);
    }
    fprintf(ofile, "\n");

    /* Compressor was unable to compress.  Copy the buffer manually. */
    if (cbytes == 0) {
      for (j = 0; j < nchunks; j++) {
        memcpy(dest[j], src, size);
      }
    }

    gettimeofday(&last, NULL);
    for (i = 0; i < niter; i++) {
      for (j = 0; j < nchunks; j++) {
        if (cbytes == 0) {
          memcpy(dest2, dest[j], size);
          nbytes = size;
        }
        else {
          nbytes = blosc_decompress(dest[j], dest2, size);
        }
      }
    }
    gettimeofday(&current, NULL);
    tunshuf = get_usec_chunk(last, current);
    fprintf(ofile, "decomp(read):\t %6.1f us, %.1f MB/s\t  ",
           tunshuf, nbytes/(tunshuf*MB/1e6));
    if (nbytes < 0) {
      fprintf(ofile, "FAILED.  Error code: %d\n", nbytes);
    }
    /* fprintf(ofile, "Orig bytes: %d\tFinal bytes: %d\n", cbytes, nbytes); */

    /* Check if data has had a good roundtrip */
    orig = (unsigned char *)srccpy;
    round = (unsigned char *)dest2;
    for(i = 0; i<size; ++i){
      if (orig[i] != round[i]) {
        fprintf(ofile, "\nError: Original data and round-trip do not match in pos %d\n",
               (int)i);
        fprintf(ofile, "Orig--> %x, round-trip--> %x\n", orig[i], round[i]);
        break;
      }
    }

    if (i == size) fprintf(ofile, "OK\n");

  } /* End clevel loop */


  /* To compute the totalsize, we should take into account the 10
     compression levels */
  totalsize += (size * nchunks * niter * 10.);

  free(src); free(srccpy); free(dest2);
  for (i = 0; i < nchunks; i++) {
    free(dest[i]);
  }

}


/* Compute a sensible value for nchunks */
int get_nchunks(int size_, int ws) {
  int nchunks;

  nchunks = ws / size_;
  if (nchunks > NCHUNKS) nchunks = NCHUNKS;
  if (nchunks < 1) nchunks = 1;
  return nchunks;
}

void *bench_wrap(void * args) 
{
    struct bench_wrap_args * arg = (struct bench_wrap_args *) args;
    do_bench(arg->nthreads, arg->size, arg->elsize, arg->rshift, arg->output_file);
    return 0;
}

int main(int argc, char *argv[]) {
  int single = 1;
  int suite = 0;
  int suite_parallel = 0;
  int hard_suite = 0;
  int extreme_suite = 0;
  int debug_suite = 0;
  int nthreads = 4;                     /* The number of threads */
  int parallel_threads = 4;             /* Number of thread executing the bench */
  int size = 2*MB;                      /* Buffer size */
  int elsize = 8;                       /* Datatype size */
  int rshift = 19;                      /* Significant bits */
  int workingset = 256*MB;              /* The maximum allocated memory */
  int nthreads_, size_, elsize_, rshift_, i;
  int parallel_threads_;
  FILE * output_file = stdout;
  struct timeval last, current;
  float totaltime;
  char *usage = "Usage: bench ['single' | 'suite' | 'suite_parallel' | 'hardsuite' | 'extremesuite' | 'debugsuite'] [nthreads [bufsize(bytes) [typesize [sbits ]]]]";


  if (argc == 1) {
    printf("%s\n", usage);
    exit(1);
  }

  if (strcmp(argv[1], "single") == 0) {
    single = 1;
  }
  else if (strcmp(argv[1], "suite") == 0) {
    suite = 1;
  }
  else if (strcmp(argv[1], "suite_parallel") == 0) {
    suite_parallel = 1; 
  }
  else if (strcmp(argv[1], "hardsuite") == 0) {
    hard_suite = 1;
    workingset = 64*MB;
    /* Values here are ending points for loops */
    nthreads = 2;
    size = 8*MB;
    elsize = 32;
    rshift = 32;
  }
  else if (strcmp(argv[1], "extremesuite") == 0) {
    extreme_suite = 1;
    workingset = 32*MB;
    niter = 1;
    /* Values here are ending points for loops */
    nthreads = 4;
    size = 16*MB;
    elsize = 32;
    rshift = 32;
  }
  else if (strcmp(argv[1], "debugsuite") == 0) {
    debug_suite = 1;
    workingset = 32*MB;
    niter = 1;
    /* Warning: values here are starting points for loops.  This is
       useful for debugging. */
    nthreads = 1;
    size = 16*KB;
    elsize = 1;
    rshift = 0;
  }
  else {
    printf("%s\n", usage);
    exit(1);
  }

  if (argc >= 3) {
    nthreads = atoi(argv[2]);
  }
  if (argc >= 4) {
    size = atoi(argv[3]);
  }
  if (argc >= 5) {
    elsize = atoi(argv[4]);
  }
  if (argc >= 6) {
    rshift = atoi(argv[5]);
  }

  if ((argc >= 7) || !(single || suite || suite_parallel || hard_suite || extreme_suite)) {
    printf("%s\n", usage);
    exit(1);
  }

  nchunks = get_nchunks(size, workingset);
  gettimeofday(&last, NULL);

  if (suite) {
    for (nthreads_=1; nthreads_ <= nthreads; nthreads_++) {
        do_bench(nthreads_, size, elsize, rshift, output_file);
    }
  }
  else if (suite_parallel) {
    for (parallel_threads_ = 1; parallel_threads_ <= parallel_threads; parallel_threads_++) {
      for (nthreads_=1; nthreads_ <= nthreads; nthreads_++) {
        do_parallel_bench(parallel_threads_, nthreads_, size, elsize, rshift);
      }
    }
  }
  else if (hard_suite) {
    /* Let's start the rshift loop by 4 so that 19 is visited.  This
       is to allow a direct comparison with the plain suite, that runs
       precisely at 19 significant bits. */
    for (rshift_ = 4; rshift_ <= rshift; rshift_ += 5) {
      for (elsize_ = 1; elsize_ <= elsize; elsize_ *= 2) {
        /* The next loop is for getting sizes that are not power of 2 */
        for (i = -elsize_; i <= elsize_; i += elsize_) {
          for (size_ = 32*KB; size_ <= size; size_ *= 2) {
            nchunks = get_nchunks(size_+i, workingset);
    	    niter = 1;
            for (nthreads_ = 1; nthreads_ <= nthreads; nthreads_++) {
              do_bench(nthreads_, size_+i, elsize_, rshift_, output_file);
              gettimeofday(&current, NULL);
              totaltime = getseconds(last, current);
              printf("Elapsed time:\t %6.1f s.  Processed data: %.1f GB\n",
                     totaltime, totalsize / GB);
            }
          }
        }
      }
    }
  }
  else if (extreme_suite) {
    for (rshift_ = 0; rshift_ <= rshift; rshift_++) {
      for (elsize_ = 1; elsize_ <= elsize; elsize_++) {
        /* The next loop is for getting sizes that are not power of 2 */
        for (i = -elsize_*2; i <= elsize_*2; i += elsize_) {
          for (size_ = 32*KB; size_ <= size; size_ *= 2) {
            nchunks = get_nchunks(size_+i, workingset);
            for (nthreads_ = 1; nthreads_ <= nthreads; nthreads_++) {
              do_bench(nthreads_, size_+i, elsize_, rshift_, output_file);
              gettimeofday(&current, NULL);
              totaltime = getseconds(last, current);
              printf("Elapsed time:\t %6.1f s.  Processed data: %.1f GB\n",
                     totaltime, totalsize / GB);
            }
          }
        }
      }
    }
  }
  else if (debug_suite) {
    for (rshift_ = rshift; rshift_ <= 32; rshift_++) {
      for (elsize_ = elsize; elsize_ <= 32; elsize_++) {
        /* The next loop is for getting sizes that are not power of 2 */
        for (i = -elsize_*2; i <= elsize_*2; i += elsize_) {
          for (size_ = size; size_ <= 16*MB; size_ *= 2) {
            nchunks = get_nchunks(size_+i, workingset);
            for (nthreads_ = nthreads; nthreads_ <= 6; nthreads_++) {
              do_bench(nthreads_, size_+i, elsize_, rshift_, output_file);
              gettimeofday(&current, NULL);
              totaltime = getseconds(last, current);
              printf("Elapsed time:\t %6.1f s.  Processed data: %.1f GB\n",
                     totaltime, totalsize / GB);
            }
          }
        }
      }
    }
  }
  /* Single mode */
  else {
    do_bench(nthreads, size, elsize, rshift, output_file);
  }

  /* Print out some statistics */
  gettimeofday(&current, NULL);
  totaltime = getseconds(last, current);
  printf("\nRound-trip compr/decompr on %.1f GB\n", totalsize / GB);
  printf("Elapsed time:\t %6.1f s, %.1f MB/s\n",
         totaltime, totalsize*2*1.1/(MB*totaltime));

  /* Free blosc resources */
  blosc_free_resources();

  return 0;
}
