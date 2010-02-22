/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/


#ifndef BLOSC_H
#define BLOSC_H

/* Version numbers */
#define BLOSC_VERSION_MAJOR    0    /* for major interface/format changes  */
#define BLOSC_VERSION_MINOR    6    /* for minor interface/format changes  */
#define BLOSC_VERSION_RELEASE  0    /* for tweaks, bug-fixes, or development */

#define BLOSC_VERSION_STRING   "0.6"    /* string version.  Sync with above! */
#define BLOSC_VERSION_DATE     "2010-02-22"      /* date version */

/* The *_VERS_FORMAT should be just 1-byte long */
#define BLOSC_VERSION_FORMAT    1   /* Blosc format version, starting at 1 */
#define BLOSCLZ_VERSION_FORMAT  1   /* Blosclz format version, starting at 1 */

/* The combined blosc and blosclz formats */
#define BLOSC_VERSION_CFORMAT (BLOSC_VERSION_FORMAT << 8) & (BLOSCLZ_VERSION_FORMAT)


/**

  Compress a block of data in the `src` buffer and returns the size of
  compressed block.  The size of `src` buffer is specified by
  `nbytes`.

  `clevel` is the desired compression level and must be a number
  between 0 (no compression) and 9 (maximum compression).

  `doshuffle` specifies whether the shuffle compression preconditioner
  should be applyied or not.  0 means not applying it and 1 means
  applying it.

  `bytesoftype` is the number of bytes for the atomic type in binary
  `src` buffer.  This is only useful for the shuffle preconditioner.
  Only a bytesoftype > 1 the will allow the shuffle to work.

  The minimum `src` buffer size is 16.  The `dest` buffer must have at
  least the size of the `src` buffer and can not be smaller than 66
  bytes.  The `src` buffer and the `dest` buffer can not overlap.

  If `src` buffer is not compressible (len(`dest`) >= len(`src`)), the
  return value is zero and you should discard the contents of the
  `dest` buffer.

  A negative return value means that an internal error happened.  This
  should never happen.  If you see this, please report it back
  together with the buffer data causing this and compression settings.

  Compression is memory safe and guaranteed not to write the `dest`
  buffer more than what is specified in `nbytes`.

 */

unsigned int
blosc_compress(int clevel, int doshuffle, size_t bytesoftype, size_t nbytes,
               const void *src, void *dest);


/**

  Decompress a block of compressed data in `src`, put the result in
  `dest` and returns the size of the decompressed block. If error
  occurs, e.g. the compressed data is corrupted or the output buffer
  is not large enough, then 0 (zero) or a negative value will be
  returned instead.

  The `src` buffer and the `dest` buffer can not overlap.

  Decompression is memory safe and guaranteed not to write the `dest`
  buffer more than what is specified in `dest_size`.

*/

unsigned int
blosc_decompress(const void *src, void *dest, size_t dest_size);

#endif
