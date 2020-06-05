===============================================================
 Announcing C-Blosc 1.19.0
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

The algorithm for choosing the blocksize automatically in fast codecs
(lz4 and blosclz) has been refined to provide better compression ratios
and better performance on modern CPUs (L2 cache sizes >= 256KB), while
staying reasonably fast on less powerful CPUs.

Also, new versions for blosclz (2.1.0) and zstd (1.4.5) codecs have
been integrated.  Expect better compression ratios and performance with
these new versions too.

For more info, please see the release notes in:

https://github.com/Blosc/c-blosc/blob/master/RELEASE_NOTES.rst


What is it?
===========

Blosc (http://www.blosc.org) is a high performance meta-compressor
optimized for binary data.  It has been designed to transmit data to
the processor cache faster than the traditional, non-compressed,
direct memory fetch approach via a memcpy() OS call.

Blosc has internal support for different compressors like its internal
BloscLZ, but also LZ4, LZ4HC, Snappy, Zlib and Zstd.  This way these can
automatically leverage the multithreading and pre-filtering
(shuffling) capabilities that comes with Blosc.


Download sources
================

The github repository is over here:

https://github.com/Blosc

Blosc is distributed using the BSD license, see LICENSES/BLOSC.txt for
details.


Mailing list
============

There is an official Blosc mailing list at:

blosc@googlegroups.com
http://groups.google.es/group/blosc


Enjoy Data!
