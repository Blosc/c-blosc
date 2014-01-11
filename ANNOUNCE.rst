===============================================================
 Announcing Blosc 1.3.0
 A blocking, shuffling and lossless compression library
===============================================================

What is new?
============

We are thrilled to announce that support for a nice handful of
compressors have been added to Blosc:

* LZ4 (http://code.google.com/p/lz4/): A very fast
  compressor/decompressor.  Could be thought as a replacement of the
  original BloscLZ, but it can behave better is some scenarios.

* LZ4HC (http://code.google.com/p/lz4/): This is a variation of LZ4
  that achieves much better compression ratio at the cost of being
  much slower for compressing.  Decompression speed is unaffected (and
  sometimes better than when using LZ4 itself!), so this is very good
  for read-only datasets.

* Snappy (http://code.google.com/p/snappy/): A very fast
  compressor/decompressor.  Could be thought as a replacement of the
  original BloscLZ, but it can behave better is some scenarios.

* Zlib (http://www.zlib.net/): This is a classic.  It achieves very
  good compression ratios, at the cost of speed.  However,
  decompression speed is still pretty good, so it is a good candidate
  for read-only datasets.

Blosc stores the compression library in its headers, so when the
decompression time comes, the proper decompressor can be selected
automatically for the task.  This actually fulfills the claim that
Blosc was actually a meta-compressor instead of just a regular one.

With this, you can select the compression library with the new
function::

  int blosc_set_complib(char* complib);

where you pass the library that you want to use (currently "blosclz",
"lz4", "lz4hc", "snappy" and "zlib", but the list can grow in the
future).

You can map between compressor names and its codes for the current
build through these functions:

  int blosc_compcode_to_compname(int compcode, char **compname);
  int blosc_compname_to_compcode(const char *compname);

You can get more info about compressors supported for the current
build by using these functions::

  char* blosc_list_compressors(void);
  int blosc_get_complib_info(char *compressor, char **complib, char **version);


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
