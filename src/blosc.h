#ifndef BLOSC_H
#define BLOSC_H


#define BLOSC_VERSION 1     /* Blosc version, starting at 1 */

// unsigned int
// blosc_compress (const void *const in_data,  unsigned int in_len,
//                 void  *out_data, unsigned int out_len);

unsigned int
blosc_compress(size_t bytesoftype, size_t nbytes, void *orig, void *dest);


// unsigned int
// blosc_decompress (const void *const in_data,  unsigned int in_len,
//                   void             *out_data, unsigned int out_len);

unsigned int
blosc_decompress(size_t bytesoftype, size_t cbbytes, void *orig, void *dest);

#endif
