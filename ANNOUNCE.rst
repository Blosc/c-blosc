===============================================================
 Announcing c-blosc 1.7.0
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

A new internal acceleration mode for LZ4 (updated internally to 1.7.0)
and BloscLZ codecs that enters in operation with all compression
levels except for the highest (9).  This allows for an important boost
in speed with minimal compression ratio loss.

Also, Jack Pappas made great contributions allowing SSE2 operation in
more scenarios (like types larger than 16 bytes or buffers not being a
multiple of typesize * vectorsize).  Another contribution is a much
more comprehensive test suite for SSE2 and AVX2 operation.

Finally Zbyszek Szmek fixed compilation on non-Intel archs (tested on
ARM).

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
(shuffling) capabilities that comes with Blosc.


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

