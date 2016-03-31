===============================================================
 Announcing c-blosc 1.8.0
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

This version introduces a new global lock during blosc_decompress()
operation.  As the blosc_compress() was already guarded by a global
lock, this means that the compression/decompression is again thread
safe.  However, when using C-Blosc from multi-threaded environments,
it is important to keep using the *_ctx() functions for performance
reasons.  NOTE: _ctx() functions will be replaced by more powerful
ones in C-Blosc 2.0.

Last but not least, the code is (again) compatible with VS2008 and
VS2010.  This is important for compatibility with Python
2.6/2.7/3.3/3.4 which use those compilers.

For more info, please see the release notes in:

https://github.com/Blosc/c-blosc/blob/master/RELEASE_NOTES.rst


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

