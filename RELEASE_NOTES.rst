===============================
 Release notes for Blosc 1.1.4
===============================

:Author: Francesc Alted i Abad
:Contact: faltet@pytables.org
:URL: http://blosc.pytables.org


Changes from 1.1.2 to 1.1.3
===========================

- (None yet)


Changes from 1.1.2 to 1.1.3
===========================

- Much improved compression ratio when using large blocks (> 64 KB) and
  high compression levels (> 6) under some circumstances (special data
  distribution).  Closes #7.


Changes from 1.1.1 to 1.1.2
===========================

- Fixes for small typesizes (#6 and #1 of python-blosc).


Changes from 1.1 to 1.1.1
=========================

- Added code to avoid calling blosc_set_nthreads more than necessary.
  That will improve performance up to 3x or more, specially for small
  chunksizes (< 1 MB).


Changes from 1.0 to 1.1
=======================

- Added code for emulating pthreads API on Windows.  No need to link
  explicitly with pthreads lib on Windows anymore.  However, performance
  is a somewhat worse because the new emulation layer does not support
  the `pthread_barrier_wait()` call natively.  But the big improvement
  in installation easiness is worth this penalty (most specially on
  64-bit Windows, where pthreads-win32 support is flaky).

- New BLOSC_MAX_BUFFERSIZE, BLOSC_MAX_TYPESIZE and BLOSC_MAX_THREADS
  symbols are available in blosc.h.  These can be useful for validating
  parameters in clients.  Thanks to Robert Smallshire for suggesting
  that.

- A new BLOSC_MIN_HEADER_LENGTH symbol in blosc.h tells how many bytes
  long is the minimum length of a Blosc header.  `blosc_cbuffer_sizes()`
  only needs these bytes to be passed to work correctly.

- Removed many warnings (related with potentially dangerous type-casting
  code) issued by MSVC 2008 in 64-bit mode.

- Fixed a problem with the computation of the blocksize in the Blosc
  filter for HDF5.

- Fixed a problem with large datatypes.  See
  http://www.pytables.org/trac/ticket/288 for more info.

- Now Blosc is able to work well even if you fork an existing process
  with a pool of threads.  Bug discovered when PyTables runs in
  multiprocess environments.  See http://pytables.org/trac/ticket/295
  for details.

- Added a new `blosc_getitem()` call to allow the retrieval of items in
  sizes smaller than the complete buffer.  That is useful for the carray
  project, but certainly for others too.


Changes from 0.9.5 to 1.0
=========================

- Added a filter for HDF5 so that people can use Blosc outside PyTables,
  if they want to.

- Many small improvements, specially in README files.

- Do not assume that size_t is uint_32 for every platform.

- Added more protection for large buffers or in allocation memory
  routines.

- The src/ directory has been renamed to blosc/.

- The `maxbytes` parameter in `blosc_compress()` has been renamed to
  `destsize`.  This is for consistency with the `blosc_decompress()`
  parameters.


Changes from 0.9.4 to 0.9.5
===========================

- Now, compression level 0 is allowed, meaning not compression at all.
  The overhead of this mode will be always BLOSC_MAX_OVERHEAD (16)
  bytes.  This mode actually represents using Blosc as a basic memory
  container.

- Supported a new parameter `maxbytes` for ``blosc_compress()``.  It
  represents a maximum of bytes for output.  Tests unit added too.

- Added 3 new functions for querying different metadata on compressed
  buffers.  A test suite for testing the new API has been added too.


Changes from 0.9.3 to 0.9.4
===========================

- Support for cross-platform big/little endian compatibility in Blosc
  headers has been added.

- Fixed several failures exposed by the extremesuite.  The problem was a
  bad check for limits in the buffer size while compressing.

- Added a new suite in bench.c called ``debugsuite`` that is
  appropriate for debugging purposes.  Now, the ``extremesuite`` can be
  used for running the complete (and extremely long) suite.


Changes from 0.9.0 to 0.9.3
===========================

- Fixed several nasty bugs uncovered by the new suites in bench.c.
  Thanks to Tony Theodore and Gabriel Beckers for their (very)
  responsive beta testing and feedback.

- Added several modes (suites), namely ``suite``, ``hardsuite`` and
  ``extremehardsuite`` in bench.c so as to allow different levels of
  testing.


Changes from 0.8.0 to 0.9
=========================

- Internal format version bumped to 2 in order to allow an easy way to
  indicate that a buffer is being saved uncompressed.  This is not
  supported yet, but it might be in the future.

- Blosc can use threads now for leveraging the increasing number of
  multi-core processors out there.  See README-threaded.txt for more
  info.

- Added a protection for MacOSX so that it has to not link against
  posix_memalign() funtion, which seems not available in old versions of
  MacOSX (for example, Tiger).  At nay rate, posix_memalign() is not
  necessary on Mac because 16 bytes alignment is ensured by default.
  Thanks to Ivan Vilata.  Fixes #3.




.. Local Variables:
.. mode: rst
.. coding: utf-8
.. fill-column: 72
.. End:
