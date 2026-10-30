#include <stdint.h>
#include <stdlib.h>

#include "blosc.h"

#ifdef __cplusplus
extern "C" {
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const char *const compressors[] = { "blosclz", "lz4", "lz4hc", "snappy", "zlib", "zstd" };
  int level = 9, filter = BLOSC_BITSHUFFLE, cindex = 0, i = 0;
  size_t typesize = 1, slack = 0, destsize;
  int use_ctx = 0;
  size_t nbytes, cbytes, blocksize;
  void *output, *input;

  blosc_set_nthreads(1);

  if (size > 0)
    level = data[0] % (9 + 1);
  if (size > 1)
    filter = data[1] % (BLOSC_BITSHUFFLE + 1);
  if (size > 2)
    cindex = data[2];

  /* Find next available compressor */
  while (blosc_set_compressor(compressors[cindex % 6]) == -1 && i < 6) {
    cindex++, i++;
  }
  if (i == 6) {
    /* No compressors available */
    return 0;
  }

  if (size > 3 && data[3] % 7 == 0) {
    blosc_set_blocksize(4096);
  }
  else {
    /* The forced blocksize is global and sticky, so reset it explicitly;
       otherwise one input saying "4096" would pin every later iteration. */
    blosc_set_blocksize(0);
  }

  if (size > 4)
    blosc_set_splitmode(data[4] % BLOSC_FORWARD_COMPAT_SPLIT + 1);

  /* Trailing bytes steer parameters the original harness held fixed:
     - typesize was hardcoded to 1, so the shuffle/bitshuffle paths for
       typesize > 1 (and the split-mode heuristics that depend on it) never
       ran;
     - destsize was hardcoded to nbytes, so compression could only succeed
       by shrinking, and the always-succeeds memcpyed regime
       (destsize >= nbytes + BLOSC_MAX_OVERHEAD) was never exercised.
     They are read from the end of the buffer so the meaning of the leading
     bytes (and hence the accumulated production corpus) is preserved. */
  if (size > 5)
    typesize = (size_t)(data[size - 1] % BLOSC_MAX_TYPESIZE) + 1;
  if (size > 6)
    slack = (size_t)(data[size - 2] % 8) * 8;   /* 0..56 extra dest bytes */
  if (size > 7)
    use_ctx = data[size - 3] & 1;               /* blosc_compress_ctx variant */

  destsize = size + slack;
  output = malloc(destsize);
  if (output == NULL)
    return 0;

  if (use_ctx) {
    cbytes = blosc_compress_ctx(level, filter, typesize, size, data,
                                output, destsize,
                                compressors[cindex % 6], 0, 1);
  }
  else {
    cbytes = blosc_compress(level, filter, typesize, size, data,
                            output, destsize);
  }
  if (cbytes <= 0) {
    /* Cannot compress src buffer into dest */
    free(output);
    return 0;
  }

  blosc_cbuffer_sizes(output, &nbytes, &cbytes, &blocksize);

  input = malloc(cbytes);
  if (input != NULL) {
    blosc_decompress(output, input, cbytes);
    free(input);
  }

  free(output);

  return 0;
}

#ifdef __cplusplus
}
#endif
