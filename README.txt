=============================================================
Blosc: A blocking, shuffling and lossless compression library
=============================================================

Author: Francesc Alted
Official website: http://blosc.pytables.org

Blosc is a high performance compressor optimized for binary data. It
has been designed to transmit data to the processor cache faster than
the traditional, non-compressed, direct memory fetch approach via a
memcpy() OS call. Blosc is the first compressor (that I'm aware of)
that is meant not only to reduce the size of large datasets on-disk or
in-memory, but also to accelerate memory-bound computations (which is
typical in vector-vector operations).

It uses the blocking technique (as described in [1]_) to reduce
activity on the memory bus as much as possible. In short, the blocking
technique works by dividing datasets in blocks that are small enough
to fit in L1 cache of modern processor and perform
compression/decompression there.  It also leverages SIMD instructions
(SSE2) and multi-threading capabilities present in nowadays multicore
processors so as to accelerate the compression/decompression process
to a maximum.

You can see some recent bencharks about Blosc performance in [2]_

Blosc is distributed using the MIT license, see file LICENSES
directory for details.

..[1] http://www.pytables.org/docs/CISE-12-2-ScientificPro.pdf
..[2] http://blosc.pytables.org/trac/wiki/SyntheticBenchmarks


Compiling your application with Blosc
=====================================

Blosc consists of the next files (in src/ directory):
blosc.h and blosc.c      -- the main routines
blosclz.h and blosclz.c  -- the actual compressor
shuffle.h and shuffle.c  -- the shuffle code

Just add these files to your project in order to use Blosc. For
information on compression and decompression routines, see blosc.h.

To compile using GCC/MINGW (4.4 or higher recommended):

  gcc -O3 -msse2 -o myprog myprog.c src/*.c -lpthread

Using Windows and MSVC (2008 or higher recommended):

  cl /Ox /Femyprog.exe myprog.c src\*.c  /link pthreadvc2.lib

[remember to set the LIB and INCLUDE environment variables to
pthread-win32 directories first]

A simple usage example is the benchmark in the bench/bench.c file.
Also, another example for using Blosc as a generic HDF5 filter is in
the hdf5/ directory.

I have not tried to compile this with compilers other than GCC, MINGW,
Intel ICC or MSVC yet. Please report your experiences with your own
platforms.

Thank you!
