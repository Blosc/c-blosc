########################################################################
#
#       License: MIT
#       Created: September 22, 2010
#       Author:  Francesc Alted - faltet@pytables.org
#
########################################################################

"""

The functions in this module allow compression and decompression using
the Blosc library (http://blosc.pytables.org).

compress(string, typesize[, clevel, shuffle]])::
   Compress string, with a given type size.

decompress(string)::
  Decompresses a compressed string.

"""

from blosc.toplevel import (
    compress, decompress)

# Blosc symbols that we want to export
from blosc.blosc_extension import (
    BLOSC_VERSION_STRING,
    BLOSC_VERSION_DATE,
    BLOSC_MAX_BUFFERSIZE,
    BLOSC_MAX_THREADS,
    )







