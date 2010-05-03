Blosc: A blocking, shuffling and lossless compression library
=============================================================

.. #########################################################
.. ################### W A R N I N G ! #####################
.. #########################################################
.. ### This is BETA software.  Use it at your own risk! ###
.. #########################################################

Author: Francesc Alted
Official website: http://blosc.pytables.org

Blosc is distributed using the MIT license, see file LICENSES
directory for details.

Blosc consists of the next files (in src/ directory):
blosc.h and blosc.c      -- the main routines
blosclz.h and blosclz.c  -- the actual compressor
shuffle.h and shuffle.c  -- the shuffle code

Just add these files to your project in order to use Blosc. For
information on compression and decompression routines, see blosc.h.

Blosc is a compressor for binary data, and it gets best results if you
can provide the size of the data type that originated the data file.

To compile using GCC:

  gcc -O3 -msse2 -o your_program your_program.c \
                    blosc.c blosclz.c shuffle.c -lpthread

Using Windows and MSVC (remember to set the LIB and INCLUDE environment
variables to pthread-win32 directories first):

  cl /Ox /Feyour_program.exe your_program.c \
          blosc.c blosclz.c shuffle.c  /link pthreadvc2.lib


A simple usage example is the benchmark in the src/bench.c file.

I have not tried to compile this with other compilers than GCC and
MSVC yet.  Please report your experiences with your own platforms.

Thank you!
