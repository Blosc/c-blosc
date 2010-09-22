########################################################################
#
#       License: MIT
#       Created: September 22, 2010
#       Author:  Francesc Alted - faltet@pytables.org
#
########################################################################

"""
blosc: a Python package that wraps the Blosc compressor
=======================================================

The functions in this package allow compression and decompression
using the Blosc library (http://blosc.pytables.org).

Public variables
----------------

* __version__ : the version of blosc package
* blosclib_version : the version of the Blosc C library
* ncores : the number of detected cores

Public functions
----------------

compress(string, typesize[, clevel=5, shuffle=True]])::
    Compress string, with a given type size.

decompress(string)::
    Decompresses a compressed string.

detect_number_of_cores()::
    Returns the number of cores in the system.

free_resources()::
    Free possible memory temporaries and thread resources.

set_nthreads(nthreads)::
    Set the number of threads to be used during Blosc operation.

"""

from blosc.version import __version__

from blosc.toplevel import (
    compress,
    decompress,
    detect_number_of_cores,
    free_resources,
    set_nthreads,
    )

# Blosc symbols that we want to export
from blosc.blosc_extension import (
    BLOSC_VERSION_STRING,
    BLOSC_VERSION_DATE,
    BLOSC_MAX_BUFFERSIZE,
    BLOSC_MAX_THREADS,
    )


# Initialize Blosc
ncores = detect_number_of_cores()
set_nthreads(ncores)
blosclib_version = "%s (%s)" % (BLOSC_VERSION_STRING, BLOSC_VERSION_DATE)
