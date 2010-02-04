Blosc: A blocking, shuffling and lossless compression library
=============================================================

.. #########################################################
.. ################### W A R N I N G ! #####################
.. #########################################################
.. ### This is ALPHA software.  Use it at your own risk! ###
.. #########################################################

Author: Francesc Alted
Official website: http://blosc.pytables.org

Blosc is distributed using the MIT license, see file LICENSES
directory for details.

Blosc consists of the next files:
blosc.h and blosc.c    -- the main routines
blosclz.h and blosclz.c  -- the actual compressor (based on FastLZ)
shuffle.h and shuffle.c  -- the shuffle code

Just add these files to your project in order to use Blosc. For
information on compression and decompression routines, see blosc.h.

A simple usage example is in the main function of the blosc.c file.

To compile using GCC:

  gcc -O3 -msse2 -o your_program your_program.c blosc.c blosclz.c shuffle.c

I have not tried to compile this with other compilers than GCC yet.
Please report your experiences with your own platforms.  Thank you!
