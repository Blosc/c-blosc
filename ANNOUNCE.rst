===============================================================
 Announcing c-blosc 1.5.0
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

The most important addition for 1.5.0 is the support for internal
contexts, allowing Blosc to be used *simultaneously* (i.e. lock free)
from multi-threaded environments.  The new functions are:

  - blosc_compress_ctx(...)
  - blosc_decompress_ctx(...)

See the new docstrings in blosc.h for how to use them.  The previous
API should be completely unaffected.  Thanks to Christopher Speller!

Also, the BloscLZ decompressor underwent some optimizations allowing
up to 1.5x faster operation in some situations.  Moreover, LZ4 and
LZ4HC internal compressors have been updated to version 1.3.1.

For more info, please see the release notes in:

https://github.com/Blosc/c-blosc/wiki/Release-notes


What is it?
===========

Blosc (http://www.blosc.org) is a high performance compressor
optimized for binary data.  It has been designed to transmit data to
the processor cache faster than the traditional, non-compressed,
direct memory fetch approach via a memcpy() OS call.

Blosc is the first compressor (that I'm aware of) that is meant not
only to reduce the size of large datasets on-disk or in-memory, but
also to accelerate object manipulations that are memory-bound.

Blosc has a Python wrapper called python-blosc
(https://github.com/Blosc/python-blosc) with a high-performance
interface to NumPy too.  There is also a handy command line for Blosc
called Bloscpack (https://github.com/Blosc/bloscpack) that allows you to
compress large binary datafiles on-disk.


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


.. Local Variables:
.. mode: rst
.. coding: utf-8
.. fill-column: 70
.. End:
