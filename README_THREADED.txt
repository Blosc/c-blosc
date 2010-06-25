Blosc supports threads now
==========================

It just happened: Blosc can be run in threaded mode for both
compressing and decompressing.  However, threaded Blosc doesn't work
better than the serial version in all cases and the reason is that
threads do add a non-negligible overhead (most specially, the cost of
synchronization between them).

In order to reduce the overhead of threads as much as possible, I've
decided to implement a pool of threads (the workers) that are waiting
for the main process (the master) to send them jobs (basically,
compressing and decompressing small blocks of the initial buffer).

Despite this and many other internal optimizations in the threaded
code, it does not work faster than the serial version for buffer sizes
around 64/128 KB or less.  This is for Intel Quad Core2 (Q8400 @ 2.66
GHz) / Linux (openSUSE 11.2, 64 bit), but your mileage may vary (and
will vary!) for other processors / operating systems.

In contrast, for buffers larger than 64/128 KB, the threaded version
starts to perform significantly better, being the sweet point at 1 MB
(again, this is with my setup).  For larger buffer sizes than 1 MB,
the threaded code slows down, but it is still considerably faster than
serial code.

This is why Blosc falls back to use the serial version for such a
'small' buffers.  So, you don't have to worry too much about deciding
whether you should set the number of threads to 1 (serial) or more
(parallel).  Just set it to the number of cores in your processor and
your are done!

Francesc Alted
2010-05-03
