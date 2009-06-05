/*
  blosc - Blocked Suffling and Compression Library

  See LICENSE.txt for details about copyright and rights to use.
*/

#ifndef BLOSCLZ_H
#define BLOSCLZ_H

#define BLOSCLZ_VERSION 0x000100

#define BLOSCLZ_VERSION_MAJOR     0
#define BLOSCLZ_VERSION_MINOR     0
#define BLOSCLZ_VERSION_REVISION  0

#define BLOSCLZ_VERSION_STRING "0.1.0"

#if defined (__cplusplus)
extern "C" {
#endif

/**
  Compress a block of data in the input buffer and returns the size of 
  compressed block. The size of input buffer is specified by length. The 
  minimum input buffer size is 16.

  The output buffer must be at least 5% larger than the input buffer  
  and can not be smaller than 66 bytes.

  If the input is not compressible, the return value might be larger than
  length (input buffer size).

  The input buffer and the output buffer can not overlap.
*/

int blosclz_compress(const int opt_level, const void* input, int length, void* output);

/**
  Decompress a block of compressed data and returns the size of the 
  decompressed block. If error occurs, e.g. the compressed data is 
  corrupted or the output buffer is not large enough, then 0 (zero) 
  will be returned instead.

  The input buffer and the output buffer can not overlap.

  Decompression is memory safe and guaranteed not to write the output buffer
  more than what is specified in maxout.
 */

int blosclz_decompress(const void* input, int length, void* output, int maxout);

#if defined (__cplusplus)
}
#endif

#endif /* BLOSCLZ_H */
