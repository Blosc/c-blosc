/*********************************************************************
  Small benchmark for testing basic capabilities of Blosc.

  You can select different degrees of 'randomness' in input buffer, as
  well as external datafiles (uncomment the lines after "For data
  coming from a file" comment).

  To compile using GCC, go to src/ directory and do:

    gcc -O3 -msse2 -o bench bench.c \
        ../src/blosc.c ../src/blosclz.c ../src/shuffle.c -lpthread

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
#include "../src/blosc.h"

#define KB  1024
#define MB  (1024*1024)

/* #define NCHUNKS (100) */
/* #define NITER  (10)               /\* Number of iterations *\/ */
#define NCHUNKS (128)
#define NITER  (10)               /* Number of iterations */


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
  return (float)(((double)sec + usec*1e-6)/((double)NITER*NCHUNKS)*1e6);
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


do_bench(int nthreads, unsigned int size, int elsize, int rshift) {
  void *src, *srccpy;
  void **dest[NCHUNKS], *dest2;
  int nbytes, cbytes;
  size_t i, j;
  struct timeval last, current;
  float tmemcpy, tshuf, tunshuf;
  int clevel, doshuffle=1;
  unsigned char *orig, *round;

  blosc_set_nthreads(nthreads);

  /* Initialize buffers */
  src = malloc(size);
  srccpy = malloc(size);
  dest2 = malloc(size);
  init_buffer(src, size, rshift);
  memcpy(srccpy, src, size);
  for (j = 0; j < NCHUNKS; j++) {
    dest[j] = malloc(size);
  }

  /* Warm destination memory (memcpy() will go a bit faster later on) */
  for (j = 0; j < NCHUNKS; j++) {
    memcpy(dest[j], src, size);
  }

  printf("--> %d, %d, %d, %d\n", nthreads, size, elsize, rshift);
  printf("********************** Run info ******************************\n");
  printf("Blosc version: %s (%s)\n", BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);
  printf("Using synthetic data with %d significant bits (out of 32)\n", rshift);
  printf("Dataset size: %d bytes\tType size: %d bytes\n", size, elsize);
  printf("Working set: %.1f MB\t\t", (size*NCHUNKS) / (float)MB);
  printf("Number of threads: %d\n", nthreads);
  printf("********************** Running benchmarks *********************\n");

  gettimeofday(&last, NULL);
  for (i = 0; i < NITER; i++) {
    for (j = 0; j < NCHUNKS; j++) {
      memcpy(dest[j], src, size);
    }
  }
  gettimeofday(&current, NULL);
  tmemcpy = getseconds(last, current);
  printf("memcpy(write):\t\t %6.1f us, %.1f MB/s\n",
         tmemcpy, size/(tmemcpy*MB/1e6));

  gettimeofday(&last, NULL);
  for (i = 0; i < NITER; i++) {
    for (j = 0; j < NCHUNKS; j++) {
      memcpy(dest2, dest[j], size);
    }
  }
  gettimeofday(&current, NULL);
  tmemcpy = getseconds(last, current);
  printf("memcpy(read):\t\t %6.1f us, %.1f MB/s\n",
         tmemcpy, size/(tmemcpy*MB/1e6));

  for (clevel=1; clevel<10; clevel++) {

    printf("Compression level: %d\n", clevel);

    gettimeofday(&last, NULL);
    for (i = 0; i < NITER; i++) {
      for (j = 0; j < NCHUNKS; j++) {
        cbytes = blosc_compress(clevel, doshuffle, elsize, size, src, dest[j]);
      }
    }
    gettimeofday(&current, NULL);
    tshuf = getseconds(last, current);
    printf("comp(write):\t %6.1f us, %.1f MB/s\t  ",
           tshuf, size/(tshuf*MB/1e6));
    printf("Final bytes: %d  ", cbytes);
    if (cbytes > 0) {
      printf("Ratio: %3.2f", size/(float)cbytes);
    }
    printf("\n");

    /* Compressor was unable to compress.  Copy the buffer manually. */
    if (cbytes == 0) {
      for (j = 0; j < NCHUNKS; j++) {
        memcpy(dest[j], src, size);
      }
    }

    gettimeofday(&last, NULL);
    for (i = 0; i < NITER; i++) {
      for (j = 0; j < NCHUNKS; j++) {
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
    tunshuf = getseconds(last, current);
    printf("decomp(read):\t %6.1f us, %.1f MB/s\t  ",
           tunshuf, nbytes/(tunshuf*MB/1e6));
    if (nbytes < 0) {
      printf("FAILED.  Error code: %d\n", nbytes);
    }
    /* printf("Orig bytes: %d\tFinal bytes: %d\n", cbytes, nbytes); */

    /* Check if data has had a good roundtrip */
    orig = (unsigned char *)srccpy;
    round = (unsigned char *)dest2;
    for(i = 0; i<size; ++i){
      if (orig[i] != round[i]) {
        printf("\nError: Original data and round-trip do not match in pos %d\n",
               (int)i);
        printf("Orig--> %x, round-trip--> %x\n", orig[i], round[i]);
        break;
      }
    }

    if (i == size) printf("OK\n");

  } /* End clevel loop */

  free(src); free(srccpy); free(dest2);
  for (i = 0; i < NCHUNKS; i++) {
    free(dest[i]);
  }

}


int main(int argc, char *argv[]) {
  int suite = 0;
  int hard_suite = 0;
  int nthreads = 1;                /* The number of threads */
  unsigned int size = 2*1024*1024; /* Buffer size */
  unsigned int elsize = 8;         /* Datatype size */
  int rshift = 19;                 /* Significant bits */
  int j;

  if ((argc >= 2) && (strcmp(argv[1], "suite") == 0)) {
    suite = 1;
    if (argc == 3) {
      nthreads = atoi(argv[2]);
    }
  }
  else if ((argc >= 2) && (strcmp(argv[1], "hard_suite") == 0)) {
    hard_suite = 1;
    if (argc == 3) {
      nthreads = atoi(argv[2]);
    }
  }
  else {
    if (argc >= 2) {
      nthreads = atoi(argv[1]);
    }
    if (argc >= 3) {
      size = atoi(argv[2])*1024;
      if (size > 2*1024*1024) {
        printf("The test is going to require more than 256 MB of RAM!\n");
      }
    }
    if (argc >= 4) {
      elsize = atoi(argv[3]);
    }
    if (argc >= 5) {
      rshift = atoi(argv[4]);
    }
    if (argc >= 6) {
      printf("Usage: bench 'suite' [nthreads] | 'hard_suite' [nthreads] | [nthreads [bufsize(KB) [typesize [sbits ]]]]\n");
      exit(1);
    }
  }

  if (hard_suite) {
    for (j=1; j <= nthreads; j++) {
      for (size=64*KB; size <= 8*MB; size *=2) {
	for (elsize=1; elsize < 32; elsize *=2) {
	  for (rshift=0; rshift < 32; rshift +=5) {
	    do_bench(j, size, elsize, rshift);
	  }
	}
      }
    }
  }
  else if (suite) {
    for (j=1; j <= nthreads; j++) {
        do_bench(j, size, elsize, rshift);
    }
  }
  else {
    do_bench(nthreads, size, elsize, rshift);
  }

  /* Free blosc resources */
  blosc_free_resources();

  return 0;
}
