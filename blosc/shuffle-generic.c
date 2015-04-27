/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#include "shuffle-generic.h"

/* Shuffle a block.  This can never fail. */
void shuffle_generic(const size_t bytesoftype, const size_t blocksize,
		     const uint8_t* const _src, uint8_t* const _dest)
{
  size_t i, j, neblock, leftover;

  /* Non-optimized shuffle */
  neblock = blocksize / bytesoftype;  /* Number of elements in a block */
  for (j = 0; j < bytesoftype; j++) {
    for (i = 0; i < neblock; i++) {
      _dest[j*neblock+i] = _src[i*bytesoftype+j];
    }
  }
  leftover = blocksize % bytesoftype;
  memcpy(_dest + neblock*bytesoftype, _src + neblock*bytesoftype, leftover);
}

/* Unshuffle a block.  This can never fail. */
void unshuffle_generic(const size_t bytesoftype, const size_t blocksize,
                       const uint8_t* const _src, uint8_t* const _dest)
{
  size_t i, j, neblock, leftover;

  /* Non-optimized unshuffle */
  neblock = blocksize / bytesoftype;  /* Number of elements in a block */
  for (i = 0; i < neblock; i++) {
    for (j = 0; j < bytesoftype; j++) {
      _dest[i*bytesoftype+j] = _src[j*neblock+i];
    }
  }
  leftover = blocksize % bytesoftype;
  memcpy(_dest + neblock*bytesoftype, _src + neblock*bytesoftype, leftover);
}
