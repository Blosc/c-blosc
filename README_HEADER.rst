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
    (``uint8``) Blosc format version.
:versionlz:
    (``uint8``) Blosclz format  version (internal Lempel-Ziv algorithm).
:flags:
    (``bitfield``) The flags of the buffer.

    :bit 0 (``0x01``):
        Whether the shuffle filter has been applied or not.
    :bit 1 (``0x02``):
        Whether the internal buffer is a pure memcpy or not.

:typesize:
    (``uint8``) Number of bytes for the atomic type.
:nbytes:
    (``uint32``) Uncompressed size of the buffer.
:blocksize:
    (``uint32``) Size of internal blocks.
:ctbytes:
    (``uint32``) Compressed size of the buffer.

