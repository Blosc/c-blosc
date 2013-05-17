===============================================================
 Announcing Blosc 1.2.3
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

New `blosc_init()` and `blosc_destroy()` functions have been added so
that the global lock can be initialized safely. These new functions
will also allow for other kind of initializations/destructions in the
future.

Existing applications using Blosc do not need to start using the new
functions right away, as long as they calling `blosc_set_nthreads()`
previous to anything else.  However, using them is highly recommended.

Thanks to Oscar Villellas for the init/destroy suggestion, it is a
nice idea indeed!

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

Please go to main web site:

http://www.blosc.org/

and proceed from there.  The github repository is over here:

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
