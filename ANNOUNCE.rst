===============================================================
 Announcing c-blosc 1.6.0
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

Support for AVX2 is here!  The benchmarks with a 4-core Intel Haswell
machine report that both compression and decompression are accelerated
around a 10%, reaching peaks of 9.6 GB/s during compression and 26
GB/s during decompression (memcpy() speed for this machine is 7.5 GB/s
for writes and 11.7 GB/s for reads).  Many thanks to @littlezhou for
this nice work.

For more info, please see the release notes in:

https://github.com/Blosc/c-blosc/wiki/Release-notes


What is it?
===========

Blosc (http://www.blosc.org) is a high performance meta-compressor
optimized for binary data.  It has been designed to transmit data to
the processor cache faster than the traditional, non-compressed,
direct memory fetch approach via a memcpy() OS call.

Blosc has internal support for different compressors like its internal
BloscLZ, but also LZ4, LZ4HC, Snappy and Zlib.  This way these can
automatically leverage the multithreading and pre-filtering
(shuffling) capabilities that comes with Blosc for free.


Download sources
================

Please go to main web site:

http://www.blosc.org/

and proceed from there.  The github repository is over here:

https://github.com/Blosc

Blosc is distributed using the MIT license, see LICENSES/BLOSC.txt for
details.


Mailing list
============

There is an official Blosc mailing list at:

blosc@googlegroups.com
http://groups.google.es/group/blosc


Enjoy Data!

