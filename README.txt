===============================================================
 Blosc: A blocking, shuffling and lossless compression library
===============================================================

:Author: Francesc Alted i Abad
:Contact: faltet@pytables.org
:URL: http://blosc.pytables.org


What is it?
===========

Blosc is a high performance compressor optimized for binary data.  It
has been designed to transmit data to the processor cache faster than
the traditional, non-compressed, direct memory fetch approach via a
memcpy() OS call.  Blosc is the first compressor (that I'm aware of)
that is meant not only to reduce the size of large datasets on-disk or
in-memory, but also to accelerate memory-bound computations.

It uses the blocking technique (as described in [1]_) to reduce
activity on the memory bus as much as possible.  In short, this
technique works by dividing datasets in blocks that are small enough
to fit in caches of modern processors and perform compression /
decompression there.  It also leverages, if available, SIMD
instructions (SSE2) and multi-threading capabilities of CPUs, in order
to accelerate the compression / decompression process to a maximum.

You can see some recent benchmarks about Blosc performance in [2]_

Blosc is distributed using the MIT license, see file LICENSES
directory for details.

.. [1] http://www.pytables.org/docs/CISE-12-2-ScientificPro.pdf
.. [2] http://blosc.pytables.org/trac/wiki/SyntheticBenchmarks


Meta-compresion and advantages over other existing compressors
==============================================================

Blosc is not like many other compressors: it can be called a
meta-compressor.  It can use different compressors and
pre-conditioners (programs that generally improve compression ratio)
so as to compress binary data effectively.

Currently it uses BloscLZ (a compresssor heavily based on FastLZ [3]_)
and a highly optimized (it uses SSE2 instructions, if available)
"shuffle" pre-conditioner.  But different compressors or
pre-conditioners can be added in the future.

The use of the blocking technique described above as well as
multi-threading, makes that every compressor and pre-conditioner works
at very high speeds in CPU caches, achieving optimal speeds.

Other advantages of Blosc are:

  * Meant for binary data: can take advantage of the type size
    meta-information for improved compression ratio.

  * Small overhead on non-compressible data: only 16 additional bytes
    over the source buffer length are needed to compress *every*
    input.

  * ... ens hem quedat aci...

.. [3] http://fastlz.org/


Compiling your application with Blosc
=====================================

Blosc consists of the next files (in blosc/ directory):

blosc.h and blosc.c      -- the main routines
blosclz.h and blosclz.c  -- the actual compressor
shuffle.h and shuffle.c  -- the shuffle code

Just add these files to your project in order to use Blosc.  For
information on compression and decompression routines, see blosc.h.

To compile using GCC/MINGW (4.4 or higher recommended):

  gcc -O3 -msse2 -o myprog myprog.c blosc/*.c -lpthread

Using Windows and MSVC (2008 or higher recommended):

  cl /Ox /Femyprog.exe myprog.c blosc\*.c  /link pthreadvc2.lib

[remember to set the LIB and INCLUDE environment variables to
pthread-win32 directories first]

A simple usage example is the benchmark in the bench/bench.c file.
Also, another example for using Blosc as a generic HDF5 filter is in
the hdf5/ directory.

I have not tried to compile this with compilers other than GCC, MINGW,
Intel ICC or MSVC yet. Please report your experiences with your own
platforms.


Filter for HDF5
===============

For those that want to use Blosc as a filter in the HDF5 library,
there is an implementation in the hdf5/ directory.


Acknowledgments
===============

I'd like to thank the PyTables community that have collaborated in the
exhaustive testing of Blosc.  With an aggregate amount of more than
300 TB of different datasets compressed *and* decompressed
successfully, I can say that Blosc is pretty safe by now and ready for
production purposes.


----

  **Enjoy data!**
