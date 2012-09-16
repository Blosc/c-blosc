===============================================================
 Announcing Blosc 1.1.4
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

- Redefinition of the BLOSC_MAX_BUFFERSIZE constant as (INT_MAX -
  BLOSC_MAX_OVERHEAD) instead of just INT_MAX.  This prevents to produce
  outputs larger than INT_MAX, which is not supported.

- `exit()` call has been replaced by a ``return -1`` in blosc_compress()
  when checking for buffer sizes.  Now programs will not just exit when
  the buffer is too large, but return a negative code.

- Improvements in explicit casts.  Blosc compiles without warnings
  (with GCC) now.

- Lots of improvements in docs, in particular a nice ascii-art diagram
  of the Blosc format (Valentin Haenel).

- [HDF5 filter] Adapted HDF5 filter to use HDF5 1.8 by default
  (Antonio Valentino).

For more info, please see the RELEASE_NOTES.txt file.

What is it?
===========

Blosc (http://blosc.pytables.org) is a high performance compressor
optimized for binary data.  It has been designed to transmit data to
the processor cache faster than the traditional, non-compressed,
direct memory fetch approach via a memcpy() OS call.

Blosc is the first compressor (that I'm aware of) that is meant not
only to reduce the size of large datasets on-disk or in-memory, but
also to accelerate object manipulations that are memory-bound.

It also comes with a filter for HDF5 (http://www.hdfgroup.org/HDF5) so
that you can easily implement support for Blosc in your favourite HDF5
tool.

Download sources
================

Please go to main web site:

http://blosc.pytables.org/sources/

or the github repository:

https://github.com/FrancescAlted/blosc

and download the most recent release from there.

Blosc is distributed using the MIT license, see LICENSES/BLOSC.txt for
details.

Mailing list
============

There is an official Blosc blosc mailing list at:

blosc@googlegroups.com
http://groups.google.es/group/blosc


----

  **Enjoy data!**
