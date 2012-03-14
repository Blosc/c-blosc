Using the Blosc filter from HDF5
================================

In order to register Blosc into your HDF5 application, you only need
to call a function in blosc_filter.h, with the following signature:

    int register_blosc(char **version, char **date)

Calling this will register the filter with the HDF5 library and will
return info about the Blosc release in `**version` and `**date`
char pointers.

A non-negative return value indicates success.  If the registration
fails, an error is pushed onto the current error stack and a negative
value is returned.

An example C program ("example.c") is included which demonstrates the
proper use of the filter.

This filter has been tested against HDF5 versions 1.6.5 through 1.8.5.  It
is released under the MIT license (see LICENSE.txt for details).


Compiling
=========

The filter consists of a single '.c' source file and '.h' header,
along with an embedded version of the BLOSC compression library.
Also, as Blosc uses SSE2 and multithreading, you must remember to use
special flags and libraries to make sure that these features are used.

To compile using GCC on UNIX:

  gcc -O3 -msse2 -lhdf5 ../blosc/*.c blosc_filter.c \
        myprog.c -o myprog -lpthread

To compile using MINGW on Windows:

  gcc -O3 -msse2 -lhdf5 ..\blosc\*.c blosc_filter.c myprog.c -o myprog

Using Windows and MSVC (2008 or higher recommended):

  cl /Ox /Femyprog.exe myprog.c ..\blosc\*.c blosc_filter.c

Intel ICC compilers should work too.


Acknowledgments
===============

This HDF5 filter interface and its example is base the LZF interface
(http://h5py.alfven.org) by Andrew Collette.
