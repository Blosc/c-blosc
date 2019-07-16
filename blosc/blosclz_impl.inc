/**
 * Defines blosclz_decompress for a particular choice of BLOSCLZ_SAFE.
 *
 * This is included from blosclz.c once for each value of BLOSCLZ_SAFE.
 */

// See https://habr.com/en/company/yandex/blog/457612/
#ifdef __AVX2__

#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif
#endif
#define ALIGNED_TYPE_(t, x) t ALIGNED_(x)

static unsigned char* copy_match_16(unsigned char *op, const unsigned char *match, int32_t len)
{
  unsigned offset = op - match;
  while (len >= 16) {

    static const ALIGNED_TYPE_(uint8_t, 16) masks[] =
      {
        0,  1,  2,  1,  4,  1,  4,  2,  8,  7,  6,  5,  4,  3,  2,  1, // offset = 0, not used as mask, but for shift
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // offset = 1
        0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,
        0,  1,  2,  0,  1,  2,  0,  1,  2,  0,  1,  2,  0,  1,  2,  0,
        0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,
        0,  1,  2,  3,  4,  0,  1,  2,  3,  4,  0,  1,  2,  3,  4,  0,
        0,  1,  2,  3,  4,  5,  0,  1,  2,  3,  4,  5,  0,  1,  2,  3,
        0,  1,  2,  3,  4,  5,  6,  0,  1,  2,  3,  4,  5,  6,  0,  1,
        0,  1,  2,  3,  4,  5,  6,  7,  0,  1,  2,  3,  4,  5,  6,  7,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  0,  1,  2,  3,  4,  5,  6,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  1,  2,  3,  4,  5,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  0,  1,  2,  3,  4,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,  0,  1,  2,  3,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,  0,  1,  2,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,  0,  1,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  0,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  15, // offset = 16
      };

    _mm_storeu_si128((__m128i *)(op),
                     _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(match)),
                                      _mm_load_si128((const __m128i *)(masks) + offset)));

    match += masks[offset];

    op += 16;
    len -= 16;
  }
  // Deal with remainders
  for (; len > 0; len--) {
    *op++ = *match++;
  }
  return op;
}
#endif


int blosclz_decompress(const void* input, int length, void* output, int maxout) {
  const uint8_t* ip = (const uint8_t*)input;
  const uint8_t* ip_limit = ip + length;
  uint8_t* op = (uint8_t*)output;
  int32_t ctrl = (*ip++) & 31;
  int32_t loop = 1;
#ifdef BLOSCLZ_SAFE
  uint8_t* op_limit = op + maxout;
  if (BLOSCLZ_UNEXPECT_CONDITIONAL(length == 0)) {
    return 0;
  }
#endif

  do {
    uint8_t* ref = op;
    int32_t len = ctrl >> 5;
    int32_t ofs = (ctrl & 31) << 8;

    if (ctrl >= 32) {
      uint8_t code;
      len--;
      ref -= ofs;
      if (len == 7 - 1) {
        do {
#ifdef BLOSCLZ_SAFE
          if (BLOSCLZ_UNEXPECT_CONDITIONAL(ip + 1 >= ip_limit)) {
            return 0;
          }
#endif
          code = *ip++;
          len += code;
        } while (code == 255);
      } else {
#ifdef BLOSCLZ_SAFE
        if (BLOSCLZ_UNEXPECT_CONDITIONAL(ip >= ip_limit)) {
          return 0;
        }
#endif
      }
      code = *ip++;
      ref -= code;

      /* match from 16-bit distance */
      if (BLOSCLZ_UNEXPECT_CONDITIONAL(code == 255)) if (BLOSCLZ_EXPECT_CONDITIONAL(ofs == (31 << 8))) {
#ifdef BLOSCLZ_SAFE
          if (BLOSCLZ_UNEXPECT_CONDITIONAL(ip + 1 >= ip_limit)) {
            return 0;
          }
#endif
          ofs = (*ip++) << 8;
          ofs += *ip++;
          ref = op - ofs - MAX_DISTANCE;
        }

#ifdef BLOSCLZ_SAFE
      if (BLOSCLZ_UNEXPECT_CONDITIONAL(op + len + 3 > op_limit)) {
        return 0;
      }

      if (BLOSCLZ_UNEXPECT_CONDITIONAL(ref - 1 < (uint8_t*)output)) {
        return 0;
      }
#endif

      if (BLOSCLZ_EXPECT_CONDITIONAL(ip < ip_limit))
        ctrl = *ip++;
      else
        loop = 0;

      if (ref == op) {
        /* optimized copy for a run */
        uint8_t b = ref[-1];
        memset(op, b, len + 3);
        op += len + 3;
      }
      else {
        /* copy from reference */
        ref--;
        len += 3;
#ifdef __AVX2__
        if (op - ref <= 16) {
          // This is not faster on a combination of compilers (clang, gcc, icc) or machines, but
          // it is not slower either.  Let's activate here for experimentation.
          op = copy_match_16(op, ref, len);
        }
        else {
#endif
          // We absolutely need a blosc_internal_copy_match here
          op = blosc_internal_copy_match(op, ref, (unsigned) len);
#ifdef __AVX2__
        }
#endif
      }
    }
    else {
      ctrl++;
#ifdef BLOSCLZ_SAFE
      if (BLOSCLZ_UNEXPECT_CONDITIONAL(op + ctrl > op_limit)) {
        return 0;
      }
      if (BLOSCLZ_UNEXPECT_CONDITIONAL(ip + ctrl > ip_limit)) {
        return 0;
      }
#endif

      // memcpy(op, ip, ctrl); op += ctrl; ip += ctrl;
      // On GCC-6, blosc_internal_fastcopy this is still faster than plain memcpy
      // However, using recent CLANG/LLVM 9.0, there is almost no difference
      // in performance.
      op = blosc_internal_fastcopy(op, ip, (unsigned) ctrl); ip += ctrl;

      loop = (int32_t)BLOSCLZ_EXPECT_CONDITIONAL(ip < ip_limit);
      if (loop)
        ctrl = *ip++;
    }
  } while (BLOSCLZ_EXPECT_CONDITIONAL(loop));

  return (int)(op - (uint8_t*)output);
}
