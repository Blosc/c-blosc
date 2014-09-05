/*********************************************************************
  Blosc - Blocked Suffling and Compression Library

  Author: Francesc Alted <francesc@blosc.io>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/


#if defined(_WIN32) && !defined(__MINGW32__)
  #include <windows.h>
  #include "win32/stdint-windows.h"
#else
  #include <stdint.h>
  #include <inttypes.h>
#endif  /* _WIN32 */


/* cstrings encode/decode routines */

/* Encode a block.  This can never fail. */
int32_t cstrings_encode(int32_t max_stringsize, int32_t blocksize,
                        uint8_t* src, uint8_t* dest);

/* Decode a block.  This can never fail. */
int32_t cstrings_decode(int32_t max_stringsize, int32_t blocksize,
                        uint8_t* src, uint8_t* dest);
