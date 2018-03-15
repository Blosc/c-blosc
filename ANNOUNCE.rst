===============================================================
 Announcing C-Blosc 1.14.1
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

The main reason for this release is a bug that caused C-Blosc to crash
on platforms requiring strict alignment (as in some kinds of ARM CPUs).
Thanks to Elvis Stansvik and Michael Hudson-Doyle for their help.
For the discussion, see https://github.com/Blosc/c-blosc/issues/223 .

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
