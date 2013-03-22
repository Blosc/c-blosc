===============================================================
 Blosc: A blocking, shuffling and lossless compression library
===============================================================

:Author: Francesc Alted i Abad
:Contact: faltet@pytables.org
:URL: http://blosc.pytables.org

What is it?
===========

Blosc [1]_ is a high performance compressor optimized for binary data.
It has been designed to transmit data to the processor cache faster
than the traditional, non-compressed, direct memory fetch approach via
a memcpy() OS call.  Blosc is the first compressor (that I'm aware of)
that is meant not only to reduce the size of large datasets on-disk or
in-memory, but also to accelerate memory-bound computations.

It uses the blocking technique (as described in [2]_) to reduce
activity on the memory bus as much as possible.  In short, this
technique works by dividing datasets in blocks that are small enough
to fit in caches of modern processors and perform compression /
decompression there.  It also leverages, if available, SIMD
instructions (SSE2) and multi-threading capabilities of CPUs, in order
to accelerate the compression / decompression process to a maximum.

You can see some recent benchmarks about Blosc performance in [3]_

Blosc is distributed using the MIT license, see LICENSES/BLOSC.txt for
details.

.. [1] http://blosc.pytables.org
.. [2] http://www.pytables.org/docs/CISE-12-2-ScientificPro.pdf
.. [3] http://blosc.pytables.org/trac/wiki/SyntheticBenchmarks

Meta-compression and other advantages over existing compressors
===============================================================

Blosc is not like other compressors: it should rather be called a
meta-compressor.  This is so because it can use different compressors
and pre-conditioners (programs that generally improve compression
ratio).  At any rate, it can also be called a compressor because it
happens that it already integrates one compressor and one
pre-conditioner, so it can actually work like so.

Currently it uses BloscLZ, a compressor heavily based on FastLZ
(http://fastlz.org/), and a highly optimized (it can use SSE2
instructions, if available) Shuffle pre-conditioner. However,
different compressors or pre-conditioners may be added in the future.

Blosc is in charge of coordinating the compressor and pre-conditioners
so that they can leverage the blocking technique (described above) as
well as multi-threaded execution (if several cores are available)
automatically. That makes that every compressor and pre-conditioner
will work at very high speeds, even if it was not initially designed
for doing blocking or multi-threading.

Other advantages of Blosc are:

* Meant for binary data: can take advantage of the type size
  meta-information for improved compression ratio (using the
  integrated shuffle pre-conditioner).

* Small overhead on non-compressible data: only a maximum of 16
  additional bytes over the source buffer length are needed to
  compress *every* input.

* Maximum destination length: contrarily to many other
  compressors, both compression and decompression routines have
  support for maximum size lengths for the destination buffer.

* Replacement for memcpy(): it supports a 0 compression level that
  does not compress at all and only adds 16 bytes of overhead. In
  this mode Blosc can copy memory usually faster than a plain
  memcpy().

When taken together, all these features set Blosc apart from other
similar solutions.

Compiling your application with Blosc
=====================================

Blosc consists of the next files (in blosc/ directory)::

    blosc.h and blosc.c      -- the main routines
    blosclz.h and blosclz.c  -- the actual compressor
    shuffle.h and shuffle.c  -- the shuffle code

Just add these files to your project in order to use Blosc.  For
information on compression and decompression routines, see blosc.h.

To compile using GCC (4.4 or higher recommended) on Unix::

  gcc -O3 -msse2 -o myprog myprog.c blosc/*.c -lpthread

Using Windows and MINGW::

  gcc -O3 -msse2 -o myprog myprog.c blosc\*.c

Using Windows and MSVC (2008 or higher recommended)::

  cl /Ox /Femyprog.exe myprog.c blosc\*.c

A simple usage example is the benchmark in the bench/bench.c file.
Also, another example for using Blosc as a generic HDF5 filter is in
the hdf5/ directory.

I have not tried to compile this with compilers other than GCC, MINGW,
Intel ICC or MSVC yet. Please report your experiences with your own
platforms.

Testing Blosc
=============

Go to the test/ directory and issue::

  $ make test

These tests are very basic, and only valid for platforms where GNU
make/gcc tools are available.  If you really want to test Blosc the
hard way, look at:

http://blosc.pytables.org/trac/wiki/SyntheticBenchmarks

where instructions on how to intensively test (and benchmark) Blosc
are given.  If while running these tests you get some error, please
report it back!

Compiling the Blosc library with CMake
======================================

Blosc can also be built, tested and installed using CMake_.
The following procedure describes the "out of source" build.

Create the build directory and move into it::

  $ mkdir build
  $ cd build

Configure Blosc in release mode (enable optimizations) specifying the
installation directory::

  $ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=INSTALL_DIR \
      PATH_TO_BLOSC_SOURCE_DIR

Please note that configuration can also be performed using UI tools
provided by CMake_ (ccmake or cmake-gui)::

  $ cmake-gui PATH_TO_BLOSC_SOURCE_DIR

Build, test and install Blosc::

  $ make
  $ make test
  $ make install 

The static and dynamic version of the Bloasc library, together with
header files, will be installed into the specified INSTALL_DIR.

.. _CMake: http://www.cmake.org

Wrapper for Python
==================

Blosc has an official wrapper for Python.  See:

https://github.com/FrancescAlted/python-blosc

Filter for HDF5
===============

For those that want to use Blosc as a filter in the HDF5 library,
there is a sample implementation in the hdf5/ directory.

Mailing list
============

There is an official mailing list for Blosc at:

blosc@googlegroups.com
http://groups.google.es/group/blosc

Acknowledgments
===============

I'd like to thank the PyTables community that have collaborated in the
exhaustive testing of Blosc.  With an aggregate amount of more than
300 TB of different datasets compressed *and* decompressed
successfully, I can say that Blosc is pretty safe now and ready for
production purposes.  Also, Valentin Haenel did a terrific work fixing
typos and improving docs and the plotting script.


----

  **Enjoy data!**
