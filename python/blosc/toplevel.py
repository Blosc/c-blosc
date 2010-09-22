########################################################################
#
#       License: MIT
#       Created: September 22, 2010
#       Author:  Francesc Alted - faltet@pytables.org
#
########################################################################

import os

import blosc
from blosc import blosc_extension as _ext


def detect_number_of_cores():
    """
    detect_number_of_cores()

    Detect the number of cores in this system.

    Returns
    -------
    out : int
        The number of cores in this system.

    """
    # Linux, Unix and MacOS:
    if hasattr(os, "sysconf"):
        if "SC_NPROCESSORS_ONLN" in os.sysconf_names:
            # Linux & Unix:
            ncpus = os.sysconf("SC_NPROCESSORS_ONLN")
            if isinstance(ncpus, int) and ncpus > 0:
                return ncpus
        else: # OSX:
            return int(os.popen2("sysctl -n hw.ncpu")[1].read())
    # Windows:
    if "SC_NPROCESSORS_ONLN" in os.environ:
        ncpus = int(os.environ["NUMBER_OF_PROCESSORS"]);
        if ncpus > 0:
            return ncpus
    return 1 # Default


def set_nthreads(nthreads):
    """
    set_nthreads(nthreads)

    Set the number of threads to be used during Blosc operation.

    Parameters
    ----------
    nthreads : int
        The number of threads to be used during Blosc operation.

    Returns
    -------
    out : int
        The previous number of used threads.

    Notes
    -----
    The number of threads for Blosc is the maximum number of cores
    detected on your machine (via `detect_number_of_cores`).  In some
    cases Blosc gets better results if you set the number of threads
    to a value slightly below than your number of cores.

    Examples
    --------
    Set the number of threads to 2 and then to 1:

    >>> oldn = set_nthreads(2)
    >>> set_nthreads(1)
    2

    """
    if nthreads > _ext.BLOSC_MAX_THREADS:
        raise ValueError("the number of threads cannot be larger than %d" % \
                         _ext.BLOSC_MAX_THREADS)

    return _ext.set_nthreads(nthreads)


def free_resources():
    """
    free_resources()

    Free possible memory temporaries and thread resources.

    Returns
    -------
        out : None

    Notes
    -----
    Blosc maintain a pool of threads waiting for work as well as some
    temporary space.  You can use this function to release these
    resources when you are not going to use Blosc for a long while.

    Examples
    --------

    >>> free_resources()
    >>>
    """
    _ext.free_resources()


def compress(string, typesize, clevel=5, shuffle=True):
    """compress(string, typesize[, clevel=5, shuffle=True]])

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

    Examples
    --------

    >>> import array
    >>> a = array.array('i', range(1000*1000))
    >>> a_string = a.tostring()
    >>> c_string = compress(a_string, typesize=4)
    >>> len(c_string) < len(a_string)
    True

    """

    if type(string) is not bytes:
        raise ValueError(
            "only string (2.x) or bytes (3.x) objects supported as input")

    if len(string) > _ext.BLOSC_MAX_BUFFERSIZE:
        raise ValueError("string length cannot be larger than %d bytes" % \
                         _ext.BLOSC_MAX_BUFFERSIZE)

    if clevel < 0 or clevel > 9:
        raise ValueError("clevel can only be in the 0-9 range.")

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

    Examples
    --------

    >>> import array
    >>> a = array.array('i', range(1000*1000))
    >>> a_string = a.tostring()
    >>> c_string = compress(a_string, typesize=4)
    >>> a_string2 = decompress(c_string)
    >>> a_string == a_string2
    True

    """

    if type(string) is not bytes:
        raise ValueError(
            "only string (2.x) or bytes (3.x) objects supported as input")

    return _ext.decompress(string)


if __name__ == '__main__':
    # test myself
    import doctest
    print("Testing blosc version: %s [%s]" % \
          (blosc.__version__, blosc.blosclib_version))
    nfail, ntests = doctest.testmod()
    if nfail == 0:
        print("All %d tests passed successfuly!" % ntests)

    # detect_ncores cannot be safely tested
    #print("ncores-->", detect_number_of_cores())
