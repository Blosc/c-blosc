/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/


#ifndef BLOSC_H
#define BLOSC_H

/* Version numbers */
#define BLOSC_VERSION_MAJOR    0    /* For major interface/format changes  */
#define BLOSC_VERSION_MINOR    1    /* For minor interface/format changes  */
#define BLOSC_VERSION_RELEASE  0    /* For tweaks, bug-fixes, or development */

#define BLOSC_VERSION_STRING   "0.2"             /* String version */
#define BLOSC_VERSION_DATE     "2010-02-15"      /* Date version */

/* The *_VERS_FORMAT should be just 1-byte long */
#define BLOSC_VERSION_FORMAT    1   /* Blosc format version, starting at 1 */
#define BLOSCLZ_VERSION_FORMAT  1   /* Blosclz format version, starting at 1 */

/* The combined blosc and blosclz formats */
#define BLOSC_VERSION_CFORMAT (BLOSC_VERSION_FORMAT << 8) & (BLOSCLZ_VERSION_FORMAT)


/**

  Set the compression level: a number between 0 (no compression) and 9
  (maximum compression).

*/

void set_complevel(int complevel);

/**

  Specify whether the shuffle compression preconditioner should be
  applyied or not.

*/

void set_shuffle(int doit);

/**

  Compress a block of data in the `src` buffer and returns the size of
  compressed block.  The size of `src` buffer is specified by
  `nbytes`.  The minimum `src` buffer size is 16.

  `bytesoftype` is the number of bytes for the atomic type in binary
  `src` buffer.  A bytesoftype > 1 will optimize the
  compression/decompression process by activating the shuffle filter.

  The `dest` buffer must be at least the size of `src` buffer and can
  not be smaller than 66 bytes.

  The `src` buffer and the `dest` buffer can not overlap.

  If `src` buffer is not compressible, the return value is zero and
  you should discard the contents of the `dest` buffer.

  A negative return value means that an internal error happened.  This
  should never happen.  If you see this, please report this together
  with the buffer and compression settings.

 */

unsigned int
blosc_compress(size_t bytesoftype, size_t nbytes, const void *src, void *dest);


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
