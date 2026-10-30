/*
 * Structured seed generator for the c-blosc fuzz targets.
 *
 * Random mutation rarely gets past the checks that gate the interesting
 * blosc code paths, so this program constructs structurally-valid inputs
 * from the real library instead:
 *
 *   gen_seeds raw <outdir>
 *     Raw source-like buffers (byte ramps, constants, float patterns,
 *     text, random, sparse) for compress_fuzzer, which treats its input
 *     as the source buffer to compress.
 *
 *   gen_seeds frames <outdir>
 *     Valid blosc frames produced by compressing the same pattern matrix
 *     with every available compressor, all shuffle modes and several
 *     typesizes, including memcpyed frames and multi-block frames, for
 *     decompress_fuzzer / getitem-style targets, which treat their input
 *     as a compressed buffer.
 *
 * Output is deterministic (fixed LCG) and files are small; the script is
 * run at build time by the OSS-Fuzz build.sh and zipped into the
 * <fuzzer>_seed_corpus.zip archives.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blosc.h"

static uint32_t lcg_state = 0x12345678u;

static uint32_t rnd(void) {
  lcg_state = lcg_state * 1664525u + 1013904223u;
  return lcg_state >> 16;
}

static void fill_pattern(uint8_t *buf, size_t n, int kind) {
  size_t i;
  switch (kind % 6) {
    case 0: /* byte ramp: mildly compressible */
      for (i = 0; i < n; i++) buf[i] = (uint8_t)(i % 251);
      break;
    case 1: /* constant: maximally compressible */
      memset(buf, 0xAA, n);
      break;
    case 2: /* float32-like: repeating 4-byte groups (shuffle-friendly) */
      for (i = 0; i < n; i++) buf[i] = (uint8_t)((i % 4) * 16 + (i / 4) % 16);
      break;
    case 3: /* text */
      for (i = 0; i < n; i++) buf[i] = (uint8_t)('a' + (i % 26));
      break;
    case 4: /* random: incompressible -> memcpyed frames */
      for (i = 0; i < n; i++) buf[i] = (uint8_t)rnd();
      break;
    default: /* sparse runs */
      for (i = 0; i < n; i++) buf[i] = (uint8_t)((i % 2) ? 0x00 : (i & 0x0f));
      break;
  }
}

static int write_file(const char *dir, const char *name,
                      const uint8_t *buf, size_t n) {
  char path[1024];
  FILE *f;
  snprintf(path, sizeof(path), "%s/%s", dir, name);
  f = fopen(path, "wb");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", path);
    return -1;
  }
  if (n > 0 && fwrite(buf, 1, n, f) != n) {
    fprintf(stderr, "short write on %s\n", path);
    fclose(f);
    return -1;
  }
  fclose(f);
  return 0;
}

static const size_t kRawSizes[] = { 1, 8, 15, 16, 17, 100, 1000, 4096, 65536 };
static const size_t kFrameSizes[] = { 100, 4096, 32768 };

static int gen_raw(const char *dir) {
  size_t ki, kk;
  int kind;
  uint8_t *buf = malloc(65536);
  if (buf == NULL) return -1;
  for (kind = 0; kind < 6; kind++) {
    for (ki = 0; ki < sizeof(kRawSizes) / sizeof(kRawSizes[0]); ki++) {
      char name[128];
      size_t n = kRawSizes[ki];
      fill_pattern(buf, n, kind);
      snprintf(name, sizeof(name), "raw-k%d-s%zu.bin", kind, n);
      if (write_file(dir, name, buf, n) != 0) { free(buf); return -1; }
    }
  }
  /* a few trailing-parameter variants exercising the harness tail bytes */
  for (kk = 0; kk < 8; kk++) {
    char name[128];
    fill_pattern(buf, 1000, (int)kk);
    buf[999] = (uint8_t)kk;      /* typesize byte */
    snprintf(name, sizeof(name), "raw-tail-%zu.bin", kk);
    if (write_file(dir, name, buf, 1000) != 0) { free(buf); return -1; }
  }
  free(buf);
  return 0;
}

static int gen_frames(const char *dir) {
  const char *comps[8];
  int ncomps = 0, ci, mode, ts, ki;
  uint8_t *src;
  uint8_t *dst;
  const char *list = blosc_list_compressors();
  const char *p = list;
  static const int kModes[3] = { BLOSC_NOSHUFFLE, BLOSC_SHUFFLE, BLOSC_BITSHUFFLE };
  static const int kTypesizes[3] = { 1, 4, 8 };

  src = malloc(65536);
  dst = malloc(65536 + BLOSC_MAX_OVERHEAD + 64);
  if (src == NULL || dst == NULL) return -1;

  /* blosc_list_compressors() returns a comma-separated list */
  while (*p && ncomps < 8) {
    comps[ncomps++] = p;
    while (*p && *p != ',') p++;
    if (*p == ',') { p++; } else { break; }
  }

  for (ci = 0; ci < ncomps; ci++) {
    if (blosc_set_compressor(comps[ci]) == -1) continue;
    for (mode = 0; mode < 3; mode++) {
      for (ts = 0; ts < 3; ts++) {
        for (ki = 0; ki < (int)(sizeof(kFrameSizes) / sizeof(kFrameSizes[0])); ki++) {
          size_t n = kFrameSizes[ki];
          int cbytes;
          char name[128];
          /* one pattern kind per size class keeps the corpus compact:
             structured for the small ones, random (memcpyed) for the rest */
          int kind = (ki == 0) ? 2 : (ki == 1 ? 0 : 4);
          fill_pattern(src, n, kind);
          /* force a small blocksize on the middle size class so the frames
             are multi-block */
          blosc_set_blocksize(ki == 1 ? 1024 : 0);
          cbytes = blosc_compress(9, kModes[mode], kTypesizes[ts], n, src,
                                  dst, n + BLOSC_MAX_OVERHEAD + 64);
          blosc_set_blocksize(0);
          if (cbytes <= 0) continue;
          snprintf(name, sizeof(name), "frame-%s-m%d-t%d-s%zu.cdata",
                   comps[ci], kModes[mode], kTypesizes[ts], n);
          if (write_file(dir, name, dst, (size_t)cbytes) != 0) return -1;
        }
      }
    }
    /* explicit memcpyed frames: random data, generous dest, all typesizes */
    for (ts = 0; ts < 3; ts++) {
      size_t n = 8192;
      int cbytes;
      char name[128];
      fill_pattern(src, n, 4);
      blosc_set_blocksize(2048);
      cbytes = blosc_compress(9, BLOSC_SHUFFLE, kTypesizes[ts], n, src,
                              dst, n + BLOSC_MAX_OVERHEAD + 64);
      blosc_set_blocksize(0);
      if (cbytes <= 0) continue;
      snprintf(name, sizeof(name), "frame-memcpyed-t%d.cdata", kTypesizes[ts]);
      if (write_file(dir, name, dst, (size_t)cbytes) != 0) return -1;
    }
  }

  free(src);
  free(dst);
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s raw|frames <outdir>\n", argv[0]);
    return 1;
  }
  blosc_init();
  if (strcmp(argv[1], "raw") == 0)
    return gen_raw(argv[2]) == 0 ? 0 : 1;
  if (strcmp(argv[1], "frames") == 0)
    return gen_frames(argv[2]) == 0 ? 0 : 1;
  fprintf(stderr, "unknown mode %s\n", argv[1]);
  return 1;
}
