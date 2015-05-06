===============================================================
 Announcing c-blosc 1.6.1
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

Fixed a subtle, but long-standing bug in the blosclz codec that could
potentially overwrite an area beyond the output buffer.

Support for *runtime* detection of AVX2 and SSE2 SIMD instructions,
allowing running AVX2 capable c-blosc libraries to run on machines
with no AVX2 available (will use SSE2 instead).

Finally, a new blocksize computation allows for better compression
ratios for larger typesizes (> 8 bytes), without not penalizing the
speed too much (at least on modern CPUs).

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

