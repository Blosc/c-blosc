#ifndef BLOSC_H
#define BLOSC_H


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
blosc_decompress(void *src, void *dest, size_t dest_size);

#endif
