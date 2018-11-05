/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: Francesc Alted <francesc@blosc.org>

  See LICENSES/BLOSC.txt for details about copyright and rights to use.
**********************************************************************/

#ifndef BLOSC_COMP_FEATURES_H
#define BLOSC_COMP_FEATURES_H

/* Use inlined functions for supported systems */
#if defined(_MSC_VER) && !defined(__cplusplus)   /* Visual Studio */
  #define BLOSC_INLINE __inline  /* Visual C is not C99, but supports some kind of inline */
#else
  #if defined __STDC__ && defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
    #define BLOSC_INLINE inline
  #else
    #define BLOSC_INLINE
  #endif
#endif

#endif /* BLOSC_COMP_FEATURES_H */