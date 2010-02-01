/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted (faltet@pytables.org)

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/


#ifndef BLOSC_H
#define BLOSC_H

/* Version numbers */
#define BLOSC_VERSION_MAJOR    0   /* For major interface/format changes  */
#define BLOSC_VERSION_MINOR    1   /* For minor interface/format changes  */
#define BLOSC_VERSION_RELEASE  0   /* For tweaks, bug-fixes, or development */

#define BLOSC_VERSION_STRING   "0.1"             /* String version */
#define BLOSC_VERSION_DATE     "2010-01-26"      /* Date version */

/* The *_VERS_FORMAT should be just 1-byte long */
#define BLOSC_VERSION_FORMAT    1    /* Blosc format version, starting at 1 */
#define BLOSCLZ_VERSION_FORMAT  1    /* Blosclz format version, starting at 1 */

/* The combined blosc and blosclz formats */
#define BLOSC_VERSION_CFORMAT (BLOSC_VERSION_FORMAT << 8) & (BLOSCLZ_VERSION_FORMAT)


/* Set parameters for compression */
void set_complevel(int complevel);
void set_shuffle(int doit);

/* Compression */
unsigned int
blosc_compress(size_t bytesoftype, size_t nbytes, const void *src, void *dest);

/* Decompression */
unsigned int
blosc_decompress(const void *src, void *dest, size_t dest_size);

#endif
