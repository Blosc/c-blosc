/*********************************************************************
  Small benchmark for testing basic capabilities of Blosc.

  You can select different degrees of 'randomness' in input buffer, as
  well as external datafiles (uncomment the lines after "For data
  coming from a file" comment).

  To compile using GCC:

    gcc -O3 -msse2 -o bench bench.c blosc.c blosclz.c shuffle.c

  I'm collecting speeds for different machines, so the output of your
  benchmarks and your processor specifications are welcome!

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "blosc.h"

#define MB  (1024*1024)

#define NITER  (20*1000)               /* Number of iterations */


#ifdef _WIN32
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
  return (float)(((double)sec + usec*1e-6)/(double)NITER*1e6);
}


int main(void) {
  int nbytes, cbytes;
  void *src, *dest, *srccpy;
  size_t i;
  struct timeval last, current;
  float tmemcpy, tshuf, tunshuf;
  int *_src;
  int *_srccpy;
  int rshift = 22;           /* For random data */
  int clevel;
  int doshuffle = 1;
  int fd;
  int status;
  char *filename = "128KB-block-4B-typesize.data";
  int size = 128*1024;                       /* Buffer size */
  unsigned int elsize = 4;                   /* Datatype size */

  src = malloc(size);
  srccpy = malloc(size);
  dest = malloc(size);

  srand(1);

  /* Initialize the original buffer */
  /* _src = (int *)src; */
  /* _srccpy = (int *)srccpy; */
  /* elsize = sizeof(int); */
  /* for(i = 0; i < size/elsize; ++i) { */
  /*   /\* Choose one below *\/ */
  /*   /\* _src[i] = 1; *\/ */
  /*   /\* _src[i] = 0x01010101; *\/ */
  /*   /\* _src[i] = 0x01020304; *\/ */
  /*   /\* _src[i] = i * 1/.3; *\/ */
  /*   /\* _src[i] = i; *\/ */
  /*   _src[i] = rand() >> rshift; */
  /* } */

  /* For data coming from a file */
  fd = open(filename, 0);
  status = read(fd, src, size);
  if (status == -1) {
    perror(NULL);
  }
  close(fd);

  printf("********************** Setup info *****************************\n");
  printf("Blosc version: %s (%s)\n", BLOSC_VERSION_STRING, BLOSC_VERSION_DATE);
/*  printf("Using random data with %d significant bits (out of 32)\n", 32-rshift); */
  printf("Using data coming from file: %s\n", filename);
  printf("Dataset size: %d bytes\t Type size: %d bytes\n", size, elsize);
  printf("Shuffle active?  %s\n", doshuffle ? "Yes" : "No");
  printf("********************** Running benchmarks *********************\n");

  memcpy(srccpy, src, size);

  gettimeofday(&last, NULL);
  for (i = 0; i < NITER; i++) {
    memcpy(dest, src, size);
  }
  gettimeofday(&current, NULL);
  tmemcpy = getseconds(last, current);
  printf("memcpy:\t\t %6.1f us, %.1f MB/s\n", tmemcpy, size/(tmemcpy*MB/1e6));

  for (clevel=1; clevel<10; clevel++) {

    printf("Compression level: %d\n", clevel);

    gettimeofday(&last, NULL);
    for (i = 0; i < NITER; i++) {
      cbytes = blosc_compress(clevel, doshuffle, elsize, size, src, dest);
    }
    gettimeofday(&current, NULL);
    tshuf = getseconds(last, current);
    printf("compression:\t %6.1f us, %.1f MB/s\t  ",
           tshuf, size/(tshuf*MB/1e6));
    printf("Final bytes: %d  ", cbytes);
    if (cbytes > 0) {
      printf("Compr ratio: %3.2f", size/(float)cbytes);
    }
    printf("\n");

    gettimeofday(&last, NULL);
    for (i = 0; i < NITER; i++)
      if (cbytes == 0) {
        memcpy(dest, src, size);
        nbytes = size;
      }
      else {
        nbytes = blosc_decompress(dest, src, size);
      }
    gettimeofday(&current, NULL);
    tunshuf = getseconds(last, current);
    printf("decompression:\t %6.1f us, %.1f MB/s\t  ",
           tunshuf, nbytes/(tunshuf*MB/1e6));
    if (nbytes < 0) {
      printf("FAIL.  Error code: %d\n", nbytes);
    }
    /* printf("Orig bytes: %d\tFinal bytes: %d\n", cbytes, nbytes); */

    /* Check if data has had a good roundtrip */
    _src = (int *)src;
    _srccpy = (int *)srccpy;
    for(i = 0; i < size/sizeof(int); ++i){
      if (_src[i] != _srccpy[i]) {
        printf("Error: original data and round-trip do not match in pos %d\n",
               (int)i);
        printf("Orig--> %x, Copy--> %x\n", _src[i], _srccpy[i]);
        goto out;
      }
    }

    printf("OK\n");

  } /* End clevel loop */

 out:
  free(src); free(srccpy);  free(dest);
  return 0;
}
