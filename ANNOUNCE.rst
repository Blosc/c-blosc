===============================================================
 Announcing C-Blosc 1.16.0
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

This is an important release in terms of improved safety for
untrusted/possibly corrupted inputs.  The additional checks seem
to not affect performance significantly (see some benchmarks in #258),
so this is why they are the default now.

The previous functions (with less safety) checks are still available
with a '_unsafe' suffix.  The complete list is:

  - blosc_decompress_unsafe()
  - blosc_decompress_ctx_unsafe()
  - blosc_getitem_unsafe()

Also, a new API function named blosc_cbuffer_validate(), for validating Blosc
compressed data, has been added.

Also, a couple of potential thread deadlock and a data race have been fixed.

Thanks to Jeremy Maitin-Shepard and @wenjuno for these great contributions.

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
