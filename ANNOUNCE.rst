===============================================================
 Announcing c-blosc 1.9.0
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

This release adds the possibility to use environment variables
(BLOSC_NTHREADS, BLOSC_CLEVEL, BLOSC_COMPRESSOR, BLOSC_SHUFFLE and
BLOSC_TYPESIZE) to change Blosc internal paramaters without the need
to touch applications using Blosc.  In addition, setting the
BLOSC_NOLOCK environment variable makes Blosc to not use internal
locks at all, which is interesting for multi-threaded applications.

Also, a handful of functions (blosc_get_nthreads(),
blosc_get_compressor(), blosc_get_blocksize()) for getting info of
internal parameters have been added.

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

