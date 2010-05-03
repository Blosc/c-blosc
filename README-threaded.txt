Blosc supports threads now
==========================

It just happened: Blosc can be run in threaded mode for both
compressing and decompressing.  However, threaded Blosc doesn't work
better than the serial version in all cases and the reason is that
threads, and most specially, the cost of synchronization between them.

In order to reduce the overhead of threads as much as possible, I've
decided to implement a pool of threads (the workers) that are waiting
for the main process (the master) to send them jobs (basically,
compressing and decompressing small blocks of the initial buffer).

Despite this and many other internal optimizations in the threaded
code, it does not work faster than the serial version for buffer sizes
around 128 KB or less (Intel Quad Core2 / Linux).  This is why Blosc
falls back to use the serial version for such a 'small' buffers.

In contrast, for buffers larger than 128 KB, the threaded version
starts to behave significantly better, being the sweet point at 1 MB.
For larger buffer sizes than 1 MB, the threaded code slows down, but
it is still considerably faster than serial code.

For this reason, I decided that Blosc will automatically enable the
threaded version only when the buffer to be compressed/decompressed
would be larger than 128 KB, while still using the serial version for
smaller buffer sizes.

The 128 KB limit might seem a bit arbitrary, and certainly is.  I
still have to study other multi-core processors to fine-tune this.

Francesc Alted
2010-04-28
