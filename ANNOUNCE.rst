===============================================================
 Announcing Blosc 1.1.4
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

*** Add blurb here ***

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

Please go to:

http://blosc.pytables.org/sources/

and download the stable release from here.

Blosc is distributed using the MIT license, see LICENSES/BLOSC.txt for
details.

Mailing list
============

There is an official Blosc blosc mailing list at:

blosc@googlegroups.com
http://groups.google.es/group/blosc


----

  **Enjoy data!**
