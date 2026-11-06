#include <stdint.h>
#include <stdlib.h>

#include "blosc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fuzz blosc_getitem(): no existing fuzz target calls this public API, and
 * it has fresh security history (an out-of-bounds write for negative
 * nitems was fixed in 1e344fc, October 2026).
 *
 * The input layout is (blosc frame || 4 tail bytes):
 *  - the frame part must pass the same early validation the decompress
 *    fuzzer applies (declared cbytes == actual size, blosc_cbuffer_validate),
 *    so mutations that corrupt the frame are rejected immediately and the
 *    fuzzer concentrates on the getitem-specific surface: start/nitems
 *    bounds handling, block selection, and the per-block decompress-or-
 *    memcpy copy into dest;
 *  - the tail bytes steer start (0..255 so that start*typesize stays
 *    within nbytes for most valid frames, hitting the copy path) and
 *    nitems (full signed 16-bit range: negative values exercise the
 *    post-1e344fc bounds error path, positive ones the regular path).
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  size_t nbytes, cbytes, blocksize, fsize;
  int start, nitems;
  void *dest;
  const uint8_t *frame;

  if (size < BLOSC_MIN_HEADER_LENGTH + 4) {
    return 0;
  }

  fsize = size - 4;
  frame = data;

  blosc_cbuffer_sizes(frame, &nbytes, &cbytes, &blocksize);
  if (cbytes != fsize) {
    return 0;
  }
  if (nbytes == 0 || nbytes > (1u << 26)) {
    /* Nothing to extract, or an implausibly large declared output that
       would only stress the allocator. */
    return 0;
  }
  if (blosc_cbuffer_validate(frame, fsize, &nbytes) != 0) {
    return 0;
  }

  start = (int)data[size - 4];
  nitems = (int)(int16_t)((uint16_t)data[size - 3] |
                          ((uint16_t)data[size - 2] << 8));

  dest = malloc(nbytes);
  if (dest == NULL) {
    return 0;
  }
  (void)blosc_getitem(frame, start, nitems, dest);

  free(dest);
  return 0;
}

#ifdef __cplusplus
}
#endif
