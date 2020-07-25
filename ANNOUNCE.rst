===============================================================
 Announcing C-Blosc 1.20.0
 A blocking, shuffling and lossless compression library for C
===============================================================

What is new?
============

More safety checks have been implemented so that potential flaws
discovered by new fuzzers in OSS-Fuzzer are fixed (@nmoinvaz).
Also, the `_xgetbv()` collision has been fixed (@mgorny).

Also, a new version of blosclz (2.3.0) codec has been backported from
C-Blosc2.  Expect better compression ratios for faster codecs.  For
details, see our new blog post: https://blosc.org/posts/beast-release/

Last but not least, the chunk format has been fully described so
that 3rd party software may come with a different implementation,
but still compatible with C-Blosc chunks.

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
