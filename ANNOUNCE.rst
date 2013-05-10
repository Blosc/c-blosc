===============================================================
 Announcing Blosc 1.2.2
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

- All important warnings removed for all tested platforms.  This will
  allow less intrusiveness compilation experiences with applications
  including Blosc source code.

- The `bench/bench.c` has been updated so that it can be compiled on
  Windows again.

- The new web site has been set to: http://www.blosc.org

For more info, please see the release notes in:

https://github.com/FrancescAlted/blosc/wiki/Release-notes

What is it?
===========

Blosc (http://www.blosc.org) is a high performance compressor
optimized for binary data.  It has been designed to transmit data to
the processor cache faster than the traditional, non-compressed,
direct memory fetch approach via a memcpy() OS call.

Blosc is the first compressor (that I'm aware of) that is meant not
only to reduce the size of large datasets on-disk or in-memory, but
also to accelerate object manipulations that are memory-bound.

There is also a handy command line for Blosc called Bloscpack
(https://github.com/esc/bloscpack) that allows you to compress large
binary datafiles on-disk.  Although the format for Bloscpack has not
stabilized yet, it allows you to effectively use Blosc from you
favorite shell.

Download sources
================

For more details on what it is, please go to main web site:

http://www.blosc.org/

The github repository is over here:

https://github.com/FrancescAlted/blosc

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
