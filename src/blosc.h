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

#define BLOSC_FORMAT_VERSION 1     //  Should be 1-byte long

// unsigned int
// blosc_compress (const void *const in_data,  unsigned int in_len,
//                 void  *out_data, unsigned int out_len);

unsigned int
blosc_compress(size_t bytesoftype, size_t nbytes, void *orig, void *dest);


// unsigned int
// blosc_decompress (const void *const in_data,  unsigned int in_len,
//                   void             *out_data, unsigned int out_len);

unsigned int
blosc_decompress(const void *src, void *dest, size_t dest_size);

#endif
