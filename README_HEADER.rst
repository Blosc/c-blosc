Blosc Header Format
===================

Blosc (as of Version 1.0.0) has the following 16 byte header that stores
information about the compressed buffer::

    |-0-|-1-|-2-|-3-|-4-|-5-|-6-|-7-|-8-|-9-|-A-|-B-|-C-|-D-|-E-|-F-|
      ^   ^   ^   ^ |     nbytes    |   blocksize   |    ctbytes    |
      |   |   |   |
      |   |   |   +--typesize
      |   |   +------flags
      |   +----------versionlz
      +--------------version

Datatypes of the Header Entries
-------------------------------

All entries are little endian.

:version:
    ``uint8``
:versionlz:
    ``uint8``
:flags (bitfield):
    :bit 0 (``0x01``):
        whether the shuffle filter has been applied or not
    :bit 1 (``0x02``):
        whether the internal buffer is a pure memcpy or not
:typesize:
    ``uint8``
:nbytes:
    ``uint32``
:blocksize:
    ``uint32``
:ctbytes:
    ``uint32``

