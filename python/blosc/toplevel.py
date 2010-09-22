########################################################################
#
#       License: MIT
#       Created: September 22, 2010
#       Author:  Francesc Alted - faltet@pytables.org
#
########################################################################

from blosc import blosc_extension as _ext

def compress(string, typesize, clevel=5, shuffle=True):
    """compress(string, typesize[, clevel, shuffle]])

    Returns compressed string.

    Parameters
    ----------
        string : str
            This is data to be compressed.
        typesize : int
            The data type size.
        clevel : int (optional)
            The compression level from 0 (no compression) to 9
            (maximum compression).  The default is 5.
        shuffle : bool (optional)
            Whether you want to activate the shuffle filter or not.
            The default is True.

    Returns
    -------
        out : str
            The compressed data in form of a Python string.

    """

    if type(string) is not str:
        raise ValueError, "only string objects supported as input"

    if len(string) > _ext.BLOSC_MAX_BUFFERSIZE:
        raise ValueError, "string length cannot be larger than %d bytes" % \
              _ext.BLOSC_MAX_BUFFERSIZE

    if clevel < 0 or clevel > 9:
        raise ValueError, "clevel can only be in the 0-9 range."

    return _ext.compress(string, typesize, clevel, shuffle)


def decompress(string):
    """decompress(string)

    Returns decompressed string.

    Parameters
    ----------
        string : str
            This is data to be decompressed.

    Returns
    -------
        out : str
            The decompressed data in form of a Python string.

    """

    if type(string) is not str:
        raise ValueError, "only string objects supported as input"

    return _ext.decompress(string)


if __name__ == '__main__':
    s = "asa"
    cs = compress("asa", 1)
    s2 = decompress(cs)
    assert s == s2
