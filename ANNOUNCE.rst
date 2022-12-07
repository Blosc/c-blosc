===============================================================
 Announcing C-Blosc 1.21.2
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

This is a maintenance release.  Upgrade internal-complib zstd from
1.5.0 to 1.5.2 and many small code improvements, improved consistency
and typo fixes. An upgrade is recommended.

For more info, please see the release notes in:

https://github.com/Blosc/c-blosc/blob/main/RELEASE_NOTES.rst


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
