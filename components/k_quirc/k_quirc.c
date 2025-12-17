/*
 * K-Quirc - Standalone QR Code Recognition Library
 * Adapted from OpenMV's quirc implementation for single-core ESP32 use
 *
 * Original Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 * OpenMV modifications Copyright (c) 2013-2021 Ibrahim Abdelkader
 * <iabdalkader@openmv.io> OpenMV modifications Copyright (c) 2013-2021 Kwabena
 * W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for
 * details.
 */

#include "k_quirc.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#define K_MALLOC(size)                                                         \
  heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define K_FREE(ptr) heap_caps_free(ptr)
#endif

/* Compiler optimization hints */
#ifndef __GNUC__
#define __attribute__(x)
#endif

#define HOT_FUNC __attribute__((hot))
#define ALWAYS_INLINE __attribute__((always_inline)) static inline
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifndef K_MALLOC
#define K_MALLOC(size) malloc(size)
#define K_FREE(ptr) free(ptr)
#endif

/*
 * Local helper functions
 */
ALWAYS_INLINE int fast_roundf(float x) { return (int)(x + 0.5f); }

/*
 * LIFO (stack) data structure for flood-fill algorithm
 */
typedef struct {
  uint8_t *data;
  size_t len;
  size_t capacity;
  size_t data_len;
} lifo_t;

static void lifo_init(lifo_t *lifo, size_t element_size, size_t max_elements) {
  lifo->data_len = element_size;
  lifo->capacity = max_elements;
  lifo->len = 0;
  lifo->data = K_MALLOC(element_size * max_elements);
}

static void lifo_free(lifo_t *lifo) {
  if (lifo->data) {
    K_FREE(lifo->data);
    lifo->data = NULL;
  }
  lifo->len = 0;
  lifo->capacity = 0;
}

/*
 * Quirc internal definitions
 */
#define QUIRC_PIXEL_WHITE 0
#define QUIRC_PIXEL_BLACK 1
#define QUIRC_PIXEL_REGION 2

#ifndef QUIRC_MAX_REGIONS
#define QUIRC_MAX_REGIONS 9409 /* Max Version 20 */
#endif

#define QUIRC_MAX_CAPSTONES 32
#define QUIRC_MAX_GRIDS 8

#define QUIRC_PERSPECTIVE_PARAMS 8

#if QUIRC_MAX_REGIONS < UINT8_MAX
typedef uint8_t quirc_pixel_t;
#elif QUIRC_MAX_REGIONS < UINT16_MAX
typedef uint16_t quirc_pixel_t;
#else
#error "QUIRC_MAX_REGIONS > 65534 is not supported"
#endif

struct quirc_point {
  int x;
  int y;
};

struct quirc_region {
  struct quirc_point seed;
  int count;
  int capstone;
};

struct quirc_capstone {
  int ring;
  int stone;
  struct quirc_point corners[4];
  struct quirc_point center;
  float c[QUIRC_PERSPECTIVE_PARAMS];
  int qr_grid;
};

struct quirc_grid {
  int caps[3];
  int align_region;
  struct quirc_point align;
  struct quirc_point tpep[3];
  int hscan;
  int vscan;
  int grid_size;
  float c[QUIRC_PERSPECTIVE_PARAMS];
};

struct quirc_code {
  struct quirc_point corners[4];
  int size;
  uint8_t cell_bitmap[K_QUIRC_MAX_BITMAP];
};

struct quirc_data {
  int version;
  int ecc_level;
  int mask;
  int data_type;
  uint8_t payload[K_QUIRC_MAX_PAYLOAD];
  int payload_len;
  uint32_t eci;
};

struct k_quirc {
  uint8_t *image;
  quirc_pixel_t *pixels;
  int w;
  int h;
  int num_regions;
  struct quirc_region regions[QUIRC_MAX_REGIONS];
  int num_capstones;
  struct quirc_capstone capstones[QUIRC_MAX_CAPSTONES];
  int num_grids;
  struct quirc_grid grids[QUIRC_MAX_GRIDS];
};

/*
 * QR-code version information database
 */
#define QUIRC_MAX_VERSION 40
#define QUIRC_MAX_ALIGNMENT 7

struct quirc_rs_params {
  uint8_t bs;
  uint8_t dw;
  uint8_t ns;
};

struct quirc_version_info {
  uint16_t data_bytes;
  uint8_t apat[QUIRC_MAX_ALIGNMENT];
  struct quirc_rs_params ecc[4];
};

static const struct quirc_version_info quirc_version_db[QUIRC_MAX_VERSION + 1] =
    {{0},
     {/* Version 1 */
      .data_bytes = 26,
      .apat = {0},
      .ecc = {{.bs = 26, .dw = 16, .ns = 1},
              {.bs = 26, .dw = 19, .ns = 1},
              {.bs = 26, .dw = 9, .ns = 1},
              {.bs = 26, .dw = 13, .ns = 1}}},
     {/* Version 2 */
      .data_bytes = 44,
      .apat = {6, 18, 0},
      .ecc = {{.bs = 44, .dw = 28, .ns = 1},
              {.bs = 44, .dw = 34, .ns = 1},
              {.bs = 44, .dw = 16, .ns = 1},
              {.bs = 44, .dw = 22, .ns = 1}}},
     {/* Version 3 */
      .data_bytes = 70,
      .apat = {6, 22, 0},
      .ecc = {{.bs = 70, .dw = 44, .ns = 1},
              {.bs = 70, .dw = 55, .ns = 1},
              {.bs = 35, .dw = 13, .ns = 2},
              {.bs = 35, .dw = 17, .ns = 2}}},
     {/* Version 4 */
      .data_bytes = 100,
      .apat = {6, 26, 0},
      .ecc = {{.bs = 50, .dw = 32, .ns = 2},
              {.bs = 100, .dw = 80, .ns = 1},
              {.bs = 25, .dw = 9, .ns = 4},
              {.bs = 50, .dw = 24, .ns = 2}}},
     {/* Version 5 */
      .data_bytes = 134,
      .apat = {6, 30, 0},
      .ecc = {{.bs = 67, .dw = 43, .ns = 2},
              {.bs = 134, .dw = 108, .ns = 1},
              {.bs = 33, .dw = 11, .ns = 2},
              {.bs = 33, .dw = 15, .ns = 2}}},
     {/* Version 6 */
      .data_bytes = 172,
      .apat = {6, 34, 0},
      .ecc = {{.bs = 43, .dw = 27, .ns = 4},
              {.bs = 86, .dw = 68, .ns = 2},
              {.bs = 43, .dw = 15, .ns = 4},
              {.bs = 43, .dw = 19, .ns = 4}}},
     {/* Version 7 */
      .data_bytes = 196,
      .apat = {6, 22, 38, 0},
      .ecc = {{.bs = 49, .dw = 31, .ns = 4},
              {.bs = 98, .dw = 78, .ns = 2},
              {.bs = 39, .dw = 13, .ns = 4},
              {.bs = 32, .dw = 14, .ns = 2}}},
     {/* Version 8 */
      .data_bytes = 242,
      .apat = {6, 24, 42, 0},
      .ecc = {{.bs = 60, .dw = 38, .ns = 2},
              {.bs = 121, .dw = 97, .ns = 2},
              {.bs = 40, .dw = 14, .ns = 4},
              {.bs = 40, .dw = 18, .ns = 4}}},
     {/* Version 9 */
      .data_bytes = 292,
      .apat = {6, 26, 46, 0},
      .ecc = {{.bs = 58, .dw = 36, .ns = 3},
              {.bs = 146, .dw = 116, .ns = 2},
              {.bs = 36, .dw = 12, .ns = 4},
              {.bs = 36, .dw = 16, .ns = 4}}},
     {/* Version 10 */
      .data_bytes = 346,
      .apat = {6, 28, 50, 0},
      .ecc = {{.bs = 69, .dw = 43, .ns = 4},
              {.bs = 86, .dw = 68, .ns = 2},
              {.bs = 43, .dw = 15, .ns = 6},
              {.bs = 43, .dw = 19, .ns = 6}}},
     {/* Version 11 */
      .data_bytes = 404,
      .apat = {6, 30, 54, 0},
      .ecc = {{.bs = 80, .dw = 50, .ns = 1},
              {.bs = 101, .dw = 81, .ns = 4},
              {.bs = 36, .dw = 12, .ns = 3},
              {.bs = 50, .dw = 22, .ns = 4}}},
     {/* Version 12 */
      .data_bytes = 466,
      .apat = {6, 32, 58, 0},
      .ecc = {{.bs = 58, .dw = 36, .ns = 6},
              {.bs = 116, .dw = 92, .ns = 2},
              {.bs = 42, .dw = 14, .ns = 7},
              {.bs = 46, .dw = 20, .ns = 4}}},
     {/* Version 13 */
      .data_bytes = 532,
      .apat = {6, 34, 62, 0},
      .ecc = {{.bs = 59, .dw = 37, .ns = 8},
              {.bs = 133, .dw = 107, .ns = 4},
              {.bs = 33, .dw = 11, .ns = 12},
              {.bs = 44, .dw = 20, .ns = 8}}},
     {/* Version 14 */
      .data_bytes = 581,
      .apat = {6, 26, 46, 66, 0},
      .ecc = {{.bs = 64, .dw = 40, .ns = 4},
              {.bs = 145, .dw = 115, .ns = 3},
              {.bs = 36, .dw = 12, .ns = 11},
              {.bs = 36, .dw = 16, .ns = 11}}},
     {/* Version 15 */
      .data_bytes = 655,
      .apat = {6, 26, 48, 70, 0},
      .ecc = {{.bs = 65, .dw = 41, .ns = 5},
              {.bs = 109, .dw = 87, .ns = 5},
              {.bs = 36, .dw = 12, .ns = 11},
              {.bs = 54, .dw = 24, .ns = 5}}},
     {/* Version 16 */
      .data_bytes = 733,
      .apat = {6, 26, 50, 74, 0},
      .ecc = {{.bs = 73, .dw = 45, .ns = 7},
              {.bs = 122, .dw = 98, .ns = 5},
              {.bs = 45, .dw = 15, .ns = 3},
              {.bs = 43, .dw = 19, .ns = 15}}},
     {/* Version 17 */
      .data_bytes = 815,
      .apat = {6, 30, 54, 78, 0},
      .ecc = {{.bs = 74, .dw = 46, .ns = 10},
              {.bs = 135, .dw = 107, .ns = 1},
              {.bs = 42, .dw = 14, .ns = 2},
              {.bs = 50, .dw = 22, .ns = 1}}},
     {/* Version 18 */
      .data_bytes = 901,
      .apat = {6, 30, 56, 82, 0},
      .ecc = {{.bs = 69, .dw = 43, .ns = 9},
              {.bs = 150, .dw = 120, .ns = 5},
              {.bs = 42, .dw = 14, .ns = 2},
              {.bs = 50, .dw = 22, .ns = 17}}},
     {/* Version 19 */
      .data_bytes = 991,
      .apat = {6, 30, 58, 86, 0},
      .ecc = {{.bs = 70, .dw = 44, .ns = 3},
              {.bs = 141, .dw = 113, .ns = 3},
              {.bs = 39, .dw = 13, .ns = 9},
              {.bs = 47, .dw = 21, .ns = 17}}},
     {/* Version 20 */
      .data_bytes = 1085,
      .apat = {6, 34, 62, 90, 0},
      .ecc = {{.bs = 67, .dw = 41, .ns = 3},
              {.bs = 135, .dw = 107, .ns = 3},
              {.bs = 43, .dw = 15, .ns = 15},
              {.bs = 54, .dw = 24, .ns = 15}}},
     {/* Version 21 */
      .data_bytes = 1156,
      .apat = {6, 28, 50, 72, 92, 0},
      .ecc = {{.bs = 68, .dw = 42, .ns = 17},
              {.bs = 144, .dw = 116, .ns = 4},
              {.bs = 46, .dw = 16, .ns = 19},
              {.bs = 50, .dw = 22, .ns = 17}}},
     {/* Version 22 */
      .data_bytes = 1258,
      .apat = {6, 26, 50, 74, 98, 0},
      .ecc = {{.bs = 74, .dw = 46, .ns = 17},
              {.bs = 139, .dw = 111, .ns = 2},
              {.bs = 37, .dw = 13, .ns = 34},
              {.bs = 54, .dw = 24, .ns = 7}}},
     {/* Version 23 */
      .data_bytes = 1364,
      .apat = {6, 30, 54, 78, 102, 0},
      .ecc = {{.bs = 75, .dw = 47, .ns = 4},
              {.bs = 151, .dw = 121, .ns = 4},
              {.bs = 45, .dw = 15, .ns = 16},
              {.bs = 54, .dw = 24, .ns = 11}}},
     {/* Version 24 */
      .data_bytes = 1474,
      .apat = {6, 28, 54, 80, 106, 0},
      .ecc = {{.bs = 73, .dw = 45, .ns = 6},
              {.bs = 147, .dw = 117, .ns = 6},
              {.bs = 46, .dw = 16, .ns = 30},
              {.bs = 54, .dw = 24, .ns = 11}}},
     {/* Version 25 */
      .data_bytes = 1588,
      .apat = {6, 32, 58, 84, 110, 0},
      .ecc = {{.bs = 75, .dw = 47, .ns = 8},
              {.bs = 132, .dw = 106, .ns = 8},
              {.bs = 45, .dw = 15, .ns = 22},
              {.bs = 54, .dw = 24, .ns = 7}}},
     {/* Version 26 */
      .data_bytes = 1706,
      .apat = {6, 30, 58, 86, 114, 0},
      .ecc = {{.bs = 74, .dw = 46, .ns = 19},
              {.bs = 142, .dw = 114, .ns = 10},
              {.bs = 46, .dw = 16, .ns = 33},
              {.bs = 50, .dw = 22, .ns = 28}}},
     {/* Version 27 */
      .data_bytes = 1828,
      .apat = {6, 34, 62, 90, 118, 0},
      .ecc = {{.bs = 73, .dw = 45, .ns = 22},
              {.bs = 152, .dw = 122, .ns = 8},
              {.bs = 45, .dw = 15, .ns = 12},
              {.bs = 53, .dw = 23, .ns = 8}}},
     {/* Version 28 */
      .data_bytes = 1921,
      .apat = {6, 26, 50, 74, 98, 122, 0},
      .ecc = {{.bs = 73, .dw = 45, .ns = 3},
              {.bs = 147, .dw = 117, .ns = 3},
              {.bs = 45, .dw = 15, .ns = 11},
              {.bs = 54, .dw = 24, .ns = 4}}},
     {/* Version 29 */
      .data_bytes = 2051,
      .apat = {6, 30, 54, 78, 102, 126, 0},
      .ecc = {{.bs = 73, .dw = 45, .ns = 21},
              {.bs = 146, .dw = 116, .ns = 7},
              {.bs = 45, .dw = 15, .ns = 19},
              {.bs = 53, .dw = 23, .ns = 1}}},
     {/* Version 30 */
      .data_bytes = 2185,
      .apat = {6, 26, 52, 78, 104, 130, 0},
      .ecc = {{.bs = 75, .dw = 47, .ns = 19},
              {.bs = 145, .dw = 115, .ns = 5},
              {.bs = 45, .dw = 15, .ns = 23},
              {.bs = 54, .dw = 24, .ns = 15}}},
     {/* Version 31 */
      .data_bytes = 2323,
      .apat = {6, 30, 56, 82, 108, 134, 0},
      .ecc = {{.bs = 74, .dw = 46, .ns = 2},
              {.bs = 145, .dw = 115, .ns = 13},
              {.bs = 45, .dw = 15, .ns = 23},
              {.bs = 54, .dw = 24, .ns = 42}}},
     {/* Version 32 */
      .data_bytes = 2465,
      .apat = {6, 34, 60, 86, 112, 138, 0},
      .ecc = {{.bs = 74, .dw = 46, .ns = 10},
              {.bs = 145, .dw = 115, .ns = 17},
              {.bs = 45, .dw = 15, .ns = 19},
              {.bs = 54, .dw = 24, .ns = 10}}},
     {/* Version 33 */
      .data_bytes = 2611,
      .apat = {6, 30, 58, 86, 114, 142, 0},
      .ecc = {{.bs = 74, .dw = 46, .ns = 14},
              {.bs = 145, .dw = 115, .ns = 17},
              {.bs = 45, .dw = 15, .ns = 11},
              {.bs = 54, .dw = 24, .ns = 29}}},
     {/* Version 34 */
      .data_bytes = 2761,
      .apat = {6, 34, 62, 90, 118, 146, 0},
      .ecc = {{.bs = 74, .dw = 46, .ns = 14},
              {.bs = 145, .dw = 115, .ns = 13},
              {.bs = 46, .dw = 16, .ns = 59},
              {.bs = 54, .dw = 24, .ns = 44}}},
     {/* Version 35 */
      .data_bytes = 2876,
      .apat = {6, 30, 54, 78, 102, 126, 150},
      .ecc = {{.bs = 75, .dw = 47, .ns = 12},
              {.bs = 151, .dw = 121, .ns = 12},
              {.bs = 45, .dw = 15, .ns = 22},
              {.bs = 54, .dw = 24, .ns = 39}}},
     {/* Version 36 */
      .data_bytes = 3034,
      .apat = {6, 24, 50, 76, 102, 128, 154},
      .ecc = {{.bs = 75, .dw = 47, .ns = 6},
              {.bs = 151, .dw = 121, .ns = 6},
              {.bs = 45, .dw = 15, .ns = 2},
              {.bs = 54, .dw = 24, .ns = 46}}},
     {/* Version 37 */
      .data_bytes = 3196,
      .apat = {6, 28, 54, 80, 106, 132, 158},
      .ecc = {{.bs = 74, .dw = 46, .ns = 29},
              {.bs = 152, .dw = 122, .ns = 17},
              {.bs = 45, .dw = 15, .ns = 24},
              {.bs = 54, .dw = 24, .ns = 49}}},
     {/* Version 38 */
      .data_bytes = 3362,
      .apat = {6, 32, 58, 84, 110, 136, 162},
      .ecc = {{.bs = 74, .dw = 46, .ns = 13},
              {.bs = 152, .dw = 122, .ns = 4},
              {.bs = 45, .dw = 15, .ns = 42},
              {.bs = 54, .dw = 24, .ns = 48}}},
     {/* Version 39 */
      .data_bytes = 3532,
      .apat = {6, 26, 54, 82, 110, 138, 166},
      .ecc = {{.bs = 75, .dw = 47, .ns = 40},
              {.bs = 147, .dw = 117, .ns = 20},
              {.bs = 45, .dw = 15, .ns = 10},
              {.bs = 54, .dw = 24, .ns = 43}}},
     {/* Version 40 */
      .data_bytes = 3706,
      .apat = {6, 30, 58, 86, 114, 142, 170},
      .ecc = {{.bs = 75, .dw = 47, .ns = 18},
              {.bs = 148, .dw = 118, .ns = 19},
              {.bs = 45, .dw = 15, .ns = 20},
              {.bs = 54, .dw = 24, .ns = 34}}}};

/*
 * Linear algebra routines
 */
static int line_intersect(const struct quirc_point *p0,
                          const struct quirc_point *p1,
                          const struct quirc_point *q0,
                          const struct quirc_point *q1, struct quirc_point *r) {
  int a = -(p1->y - p0->y);
  int b = p1->x - p0->x;
  int c = -(q1->y - q0->y);
  int d = q1->x - q0->x;
  int e = a * p1->x + b * p1->y;
  int f = c * q1->x + d * q1->y;
  int det = (a * d) - (b * c);

  if (!det)
    return 0;

  r->x = (d * e - b * f) / det;
  r->y = (-c * e + a * f) / det;
  return 1;
}

static void perspective_setup(float *c, const struct quirc_point *rect, float w,
                              float h) {
  float x0 = rect[0].x;
  float y0 = rect[0].y;
  float x1 = rect[1].x;
  float y1 = rect[1].y;
  float x2 = rect[2].x;
  float y2 = rect[2].y;
  float x3 = rect[3].x;
  float y3 = rect[3].y;

  float wden = w * (x2 * y3 - x3 * y2 + (x3 - x2) * y1 + x1 * (y2 - y3));
  float hden = h * (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1);

  c[0] = (x1 * (x2 * y3 - x3 * y2) +
          x0 * (-x2 * y3 + x3 * y2 + (x2 - x3) * y1) + x1 * (x3 - x2) * y0) /
         wden;
  c[1] = -(x0 * (x2 * y3 + x1 * (y2 - y3) - x2 * y1) - x1 * x3 * y2 +
           x2 * x3 * y1 + (x1 * x3 - x2 * x3) * y0) /
         hden;
  c[2] = x0;
  c[3] = (y0 * (x1 * (y3 - y2) - x2 * y3 + x3 * y2) + y1 * (x2 * y3 - x3 * y2) +
          x0 * y1 * (y2 - y3)) /
         wden;
  c[4] = (x0 * (y1 * y3 - y2 * y3) + x1 * y2 * y3 - x2 * y1 * y3 +
          y0 * (x3 * y2 - x1 * y2 + (x2 - x3) * y1)) /
         hden;
  c[5] = y0;
  c[6] = (x1 * (y3 - y2) + x0 * (y2 - y3) + (x2 - x3) * y1 + (x3 - x2) * y0) /
         wden;
  c[7] = (-x2 * y3 + x1 * y3 + x3 * y2 + x0 * (y1 - y2) - x3 * y1 +
          (x2 - x1) * y0) /
         hden;
}

ALWAYS_INLINE void perspective_map(const float *c, float u, float v,
                                   struct quirc_point *ret) {
  float den = c[6] * u + c[7] * v + 1.0f;
  float inv_den = 1.0f / den;
  float x = (c[0] * u + c[1] * v + c[2]) * inv_den;
  float y = (c[3] * u + c[4] * v + c[5]) * inv_den;

  ret->x = fast_roundf(x);
  ret->y = fast_roundf(y);
}

static void perspective_unmap(const float *c, const struct quirc_point *in,
                              float *u, float *v) {
  float x = in->x;
  float y = in->y;
  float den = -c[0] * c[7] * y + c[1] * c[6] * y +
              (c[3] * c[7] - c[4] * c[6]) * x + c[0] * c[4] - c[1] * c[3];

  *u = -(c[1] * (y - c[5]) - c[2] * c[7] * y + (c[5] * c[7] - c[4]) * x +
         c[2] * c[4]) /
       den;
  *v = (c[0] * (y - c[5]) - c[2] * c[6] * y + (c[5] * c[6] - c[3]) * x +
        c[2] * c[3]) /
       den;
}

/*
 * Span-based floodfill routine
 */
typedef void (*span_func_t)(void *user_data, int y, int left, int right);

HOT_FUNC

struct xylf_struct {
  int16_t x, y, l, r;
};
typedef struct xylf_struct xylf_t;

static void lifo_enqueue_fast(lifo_t *ptr, xylf_t *data) {
  if (ptr->len < ptr->capacity) {
    *((xylf_t *)(ptr->data + (ptr->len * ptr->data_len))) = *data;
    ptr->len += 1;
  }
}

static void lifo_dequeue_fast(lifo_t *ptr, xylf_t *data) {
  if (ptr->len > 0) {
    ptr->len -= 1;
    *data = *((xylf_t *)(ptr->data + (ptr->len * ptr->data_len)));
  }
}

static void flood_fill_seed(struct k_quirc *q, int x, int y,
                            quirc_pixel_t from_color, quirc_pixel_t to_color,
                            span_func_t func, void *user_data, int depth) {
  (void)depth;

  /* Use a large fixed stack size for flood fill */
  size_t max_stack = 32768;

  lifo_t lifo;
  lifo_init(&lifo, sizeof(xylf_t), max_stack);
  if (!lifo.data)
    return;

  for (;;) {
    int left = x;
    int right = x;
    quirc_pixel_t *row = q->pixels + y * q->w;

    while (left > 0 && row[left - 1] == from_color)
      left--;

    while (right < q->w - 1 && row[right + 1] == from_color)
      right++;

    for (int i = left; i <= right; i++)
      row[i] = to_color;

    if (func)
      func(user_data, y, left, right);

    for (;;) {
      bool recurse = false;

      if (lifo.len < lifo.capacity) {
        if (y > 0) {
          row = q->pixels + (y - 1) * q->w;
          for (int i = left; i <= right; i++) {
            if (row[i] == from_color) {
              xylf_t context = {(int16_t)x, (int16_t)y, (int16_t)left,
                                (int16_t)right};
              lifo_enqueue_fast(&lifo, &context);
              x = i;
              y = y - 1;
              recurse = true;
              break;
            }
          }
        }

        if (!recurse && y < q->h - 1) {
          row = q->pixels + (y + 1) * q->w;
          for (int i = left; i <= right; i++) {
            if (row[i] == from_color) {
              xylf_t context = {(int16_t)x, (int16_t)y, (int16_t)left,
                                (int16_t)right};
              lifo_enqueue_fast(&lifo, &context);
              x = i;
              y = y + 1;
              recurse = true;
              break;
            }
          }
        }
      }

      if (!recurse) {
        if (!lifo.len) {
          lifo_free(&lifo);
          return;
        }

        xylf_t context;
        lifo_dequeue_fast(&lifo, &context);
        x = context.x;
        y = context.y;
        left = context.l;
        right = context.r;
      } else {
        break;
      }
    }
  }
}

/*
 * Thresholding with Otsu's method
 * Uses 32-bit histogram counters to handle larger images without overflow
 */
static uint8_t otsu_threshold(uint32_t *histogram, uint32_t total) {
  // Calculate weighted sum of histogram values
  double sum = 0;
  for (int i = 0; i < 256; i++) {
    sum += (double)i * histogram[i];
  }

  double sumB = 0;
  uint32_t wB = 0;
  double varMax = 0;
  uint8_t threshold = 0;

  for (int i = 0; i < 256; i++) {
    // Weighted background
    wB += histogram[i];
    if (wB == 0)
      continue;

    // Weighted foreground
    uint32_t wF = total - wB;
    if (wF == 0)
      break;

    sumB += (double)i * histogram[i];
    double mB = sumB / wB;
    double mF = (sum - sumB) / wF;
    double mDiff = mB - mF;

    double varBetween = (double)wB * (double)wF * mDiff * mDiff;
    if (varBetween >= varMax) {
      varMax = varBetween;
      threshold = i;
    }
  }

  return threshold;
}

/* Percentage of image edges to ignore for histogram calculation */
#define OTSU_MARGIN_PERCENT 20

HOT_FUNC
static void threshold(struct k_quirc *q, bool inverted) {
  int width = q->w;
  int height = q->h;
  quirc_pixel_t *restrict pixels = q->pixels;
  uint32_t total_pixels = (uint32_t)width * height;

  /* Calculate margins - ignore outer 20% on each side for histogram */
  int margin_x = width * OTSU_MARGIN_PERCENT / 100;
  int margin_y = height * OTSU_MARGIN_PERCENT / 100;
  int start_x = margin_x;
  int end_x = width - margin_x;
  int start_y = margin_y;
  int end_y = height - margin_y;

  /* Build histogram over central region only */
  uint32_t histogram[256] = {0};
  uint32_t hist_pixels = (uint32_t)(end_x - start_x) * (end_y - start_y);

  for (int y = start_y; y < end_y; y++) {
    quirc_pixel_t *row = pixels + y * width + start_x;
    for (int x = start_x; x < end_x; x++) {
      histogram[*row++]++;
    }
  }

  uint8_t o_threshold = otsu_threshold(histogram, hist_pixels);

  /* Apply threshold to ALL pixels using branchless operations */
  if (inverted) {
    for (uint32_t i = 0; i < total_pixels; i++) {
      pixels[i] =
          (pixels[i] > o_threshold) ? QUIRC_PIXEL_BLACK : QUIRC_PIXEL_WHITE;
    }
  } else {
    for (uint32_t i = 0; i < total_pixels; i++) {
      pixels[i] =
          (pixels[i] < o_threshold) ? QUIRC_PIXEL_BLACK : QUIRC_PIXEL_WHITE;
    }
  }
}

ALWAYS_INLINE void area_count(void *user_data, int y, int left, int right) {
  ((struct quirc_region *)user_data)->count += right - left + 1;
}

HOT_FUNC
static int region_code(struct k_quirc *q, int x, int y) {
  int pixel;
  struct quirc_region *box;
  int region;

  if (x < 0 || y < 0 || x >= q->w || y >= q->h)
    return -1;

  pixel = q->pixels[y * q->w + x];

  if (pixel >= QUIRC_PIXEL_REGION)
    return pixel;

  if (pixel == QUIRC_PIXEL_WHITE)
    return -1;

  if (q->num_regions >= QUIRC_MAX_REGIONS)
    return -1;

  region = q->num_regions;
  box = &q->regions[q->num_regions++];

  memset(box, 0, sizeof(*box));

  box->seed.x = x;
  box->seed.y = y;
  box->capstone = -1;

  flood_fill_seed(q, x, y, pixel, region, area_count, box, 0);

  return region;
}

struct polygon_score_data {
  struct quirc_point ref;
  int scores[4];
  struct quirc_point *corners;
};

static void find_one_corner(void *user_data, int y, int left, int right) {
  struct polygon_score_data *psd = (struct polygon_score_data *)user_data;
  int xs[2] = {left, right};
  int dy = y - psd->ref.y;

  for (int i = 0; i < 2; i++) {
    int dx = xs[i] - psd->ref.x;
    int d = dx * dx + dy * dy;

    if (d > psd->scores[0]) {
      psd->scores[0] = d;
      psd->corners[0].x = xs[i];
      psd->corners[0].y = y;
    }
  }
}

static void find_other_corners(void *user_data, int y, int left, int right) {
  struct polygon_score_data *psd = (struct polygon_score_data *)user_data;
  int xs[2] = {left, right};

  for (int i = 0; i < 2; i++) {
    int up = xs[i] * psd->ref.x + y * psd->ref.y;
    int rt = xs[i] * -psd->ref.y + y * psd->ref.x;
    int scores[4] = {up, rt, -up, -rt};

    for (int j = 0; j < 4; j++) {
      if (scores[j] > psd->scores[j]) {
        psd->scores[j] = scores[j];
        psd->corners[j].x = xs[i];
        psd->corners[j].y = y;
      }
    }
  }
}

static void find_region_corners(struct k_quirc *q, int rcode,
                                const struct quirc_point *ref,
                                struct quirc_point *corners) {
  struct quirc_region *region = &q->regions[rcode];
  struct polygon_score_data psd;

  memset(&psd, 0, sizeof(psd));
  psd.corners = corners;

  memcpy(&psd.ref, ref, sizeof(psd.ref));
  psd.scores[0] = -1;
  flood_fill_seed(q, region->seed.x, region->seed.y, rcode, QUIRC_PIXEL_BLACK,
                  find_one_corner, &psd, 0);

  psd.ref.x = psd.corners[0].x - psd.ref.x;
  psd.ref.y = psd.corners[0].y - psd.ref.y;

  for (int i = 0; i < 4; i++)
    memcpy(&psd.corners[i], &region->seed, sizeof(psd.corners[i]));

  int i = region->seed.x * psd.ref.x + region->seed.y * psd.ref.y;
  psd.scores[0] = i;
  psd.scores[2] = -i;
  i = region->seed.x * -psd.ref.y + region->seed.y * psd.ref.x;
  psd.scores[1] = i;
  psd.scores[3] = -i;

  flood_fill_seed(q, region->seed.x, region->seed.y, QUIRC_PIXEL_BLACK, rcode,
                  find_other_corners, &psd, 0);
}

static void record_capstone(struct k_quirc *q, int ring, int stone) {
  struct quirc_region *stone_reg = &q->regions[stone];
  struct quirc_region *ring_reg = &q->regions[ring];
  struct quirc_capstone *capstone;
  int cs_index;

  if (q->num_capstones >= QUIRC_MAX_CAPSTONES)
    return;

  cs_index = q->num_capstones;
  capstone = &q->capstones[q->num_capstones++];

  memset(capstone, 0, sizeof(*capstone));

  capstone->qr_grid = -1;
  capstone->ring = ring;
  capstone->stone = stone;
  stone_reg->capstone = cs_index;
  ring_reg->capstone = cs_index;

  find_region_corners(q, ring, &stone_reg->seed, capstone->corners);
  perspective_setup(capstone->c, capstone->corners, 7.0f, 7.0f);
  perspective_map(capstone->c, 3.5f, 3.5f, &capstone->center);
}

static void test_capstone(struct k_quirc *q, int x, int y, int *pb) {
  int ring_right_x = x - pb[4];
  int ring_left_x = x - pb[4] - pb[3] - pb[2] - pb[1] - pb[0];
  int stone_x = x - pb[4] - pb[3] - pb[2];
  int ring_right = region_code(q, ring_right_x, y);
  int ring_left = region_code(q, ring_left_x, y);

  if (ring_left < 0 || ring_right < 0)
    return;

  if (ring_left != ring_right)
    return;

  int stone = region_code(q, stone_x, y);
  if (stone < 0)
    return;

  if (ring_left == stone)
    return;

  struct quirc_region *stone_reg = &q->regions[stone];
  struct quirc_region *ring_reg = &q->regions[ring_left];

  if (stone_reg->capstone >= 0 || ring_reg->capstone >= 0)
    return;

  int ratio = stone_reg->count * 100 / ring_reg->count;
  if (ratio < 10 || ratio > 70)
    return;

  record_capstone(q, ring_left, stone);
}

static void finder_scan(struct k_quirc *q, int y) {
  static const int check[5] = {1, 1, 3, 1, 1};
  quirc_pixel_t *row = q->pixels + y * q->w;
  uint8_t last_color;
  int run_length = 1;
  int run_count = 0;
  int pb[5];

  memset(pb, 0, sizeof(pb));
  last_color = row[0];
  for (int x = 1; x < q->w; x++) {
    uint8_t color = row[x];

    if (color != last_color) {
      pb[0] = pb[1];
      pb[1] = pb[2];
      pb[2] = pb[3];
      pb[3] = pb[4];
      pb[4] = run_length;
      run_length = 0;
      run_count++;

      if (!color && run_count >= 5) {
        int avg, err;
        int ok = 1;

        avg = (pb[0] + pb[1] + pb[3] + pb[4]) / 4;
        if (avg == 0)
          avg = 1;
        err = (avg * 3) / 4;

        for (int i = 0; i < 5; i++) {
          if (pb[i] < check[i] * avg - err || pb[i] > check[i] * avg + err) {
            ok = 0;
            break;
          }
        }

        if (ok) {
          test_capstone(q, x, y, pb);
        }
      }
    }

    run_length++;
    last_color = color;
  }
}

static void find_alignment_pattern(struct k_quirc *q, int index) {
  struct quirc_grid *qr = &q->grids[index];
  struct quirc_capstone *c0 = &q->capstones[qr->caps[0]];
  struct quirc_capstone *c2 = &q->capstones[qr->caps[2]];
  struct quirc_point a;
  struct quirc_point b;
  struct quirc_point c;
  int size_estimate;
  int step_size = 1;
  int dir = 0;
  float u, v;

  memcpy(&b, &qr->align, sizeof(b));

  perspective_unmap(c0->c, &b, &u, &v);
  perspective_map(c0->c, u, v + 1.0f, &a);
  perspective_unmap(c2->c, &b, &u, &v);
  perspective_map(c2->c, u + 1.0f, v, &c);

  size_estimate = abs((a.x - b.x) * -(c.y - b.y) + (a.y - b.y) * (c.x - b.x));

  while (step_size * step_size < size_estimate * 100) {
    static const int dx_map[] = {1, 0, -1, 0};
    static const int dy_map[] = {0, -1, 0, 1};

    for (int i = 0; i < step_size; i++) {
      int code = region_code(q, b.x, b.y);

      if (code >= 0) {
        struct quirc_region *reg = &q->regions[code];

        if (reg->count >= size_estimate / 2 &&
            reg->count <= size_estimate * 2) {
          qr->align_region = code;
          return;
        }
      }

      b.x += dx_map[dir];
      b.y += dy_map[dir];
    }

    dir = (dir + 1) % 4;
    if (!(dir & 1))
      step_size++;
  }
}

static void find_leftmost_to_line(void *user_data, int y, int left, int right) {
  struct polygon_score_data *psd = (struct polygon_score_data *)user_data;
  int xs[2] = {left, right};

  for (int i = 0; i < 2; i++) {
    int d = -psd->ref.y * xs[i] + psd->ref.x * y;

    if (d < psd->scores[0]) {
      psd->scores[0] = d;
      psd->corners[0].x = xs[i];
      psd->corners[0].y = y;
    }
  }
}

static int timing_scan(const struct k_quirc *q, const struct quirc_point *p0,
                       const struct quirc_point *p1) {
  int n = p1->x - p0->x;
  int d = p1->y - p0->y;
  int x = p0->x;
  int y = p0->y;
  int *dom, *nondom;
  int dom_step;
  int nondom_step;
  int a = 0;
  int run_length = 0;
  int count = 0;

  if (p0->x < 0 || p0->y < 0 || p0->x >= q->w || p0->y >= q->h)
    return -1;
  if (p1->x < 0 || p1->y < 0 || p1->x >= q->w || p1->y >= q->h)
    return -1;

  if (abs(n) > abs(d)) {
    int swap = n;
    n = d;
    d = swap;
    dom = &x;
    nondom = &y;
  } else {
    dom = &y;
    nondom = &x;
  }

  if (n < 0) {
    n = -n;
    nondom_step = -1;
  } else {
    nondom_step = 1;
  }

  if (d < 0) {
    d = -d;
    dom_step = -1;
  } else {
    dom_step = 1;
  }

  x = p0->x;
  y = p0->y;
  for (int i = 0; i <= d; i++) {
    int pixel;

    if (y < 0 || y >= q->h || x < 0 || x >= q->w)
      break;

    pixel = q->pixels[y * q->w + x];

    if (pixel) {
      if (run_length >= 2)
        count++;
      run_length = 0;
    } else {
      run_length++;
    }

    a += n;
    *dom += dom_step;
    if (a >= d) {
      *nondom += nondom_step;
      a -= d;
    }
  }

  return count;
}

HOT_FUNC
static int fitness_cell(const struct k_quirc *q, int index, int x, int y) {
  const struct quirc_grid *qr = &q->grids[index];
  int score = 0;
  struct quirc_point p;
  static const float offsets[] = {0.3f, 0.5f, 0.7f};
  int w = q->w;
  int h = q->h;
  const quirc_pixel_t *pixels = q->pixels;

  for (int v = 0; v < 3; v++) {
    float yoff = y + offsets[v];
    for (int u = 0; u < 3; u++) {
      perspective_map(qr->c, x + offsets[u], yoff, &p);

      if (LIKELY(p.y >= 0 && p.y < h && p.x >= 0 && p.x < w)) {
        score += pixels[p.y * w + p.x] ? 1 : -1;
      }
    }
  }

  return score;
}

static int fitness_ring(const struct k_quirc *q, int index, int cx, int cy,
                        int radius) {
  int score = 0;

  for (int i = 0; i < radius * 2; i++) {
    score += fitness_cell(q, index, cx - radius + i, cy - radius);
    score += fitness_cell(q, index, cx - radius, cy + radius - i);
    score += fitness_cell(q, index, cx + radius, cy - radius + i);
    score += fitness_cell(q, index, cx + radius - i, cy + radius);
  }

  return score;
}

static int fitness_apat(const struct k_quirc *q, int index, int cx, int cy) {
  return fitness_cell(q, index, cx, cy) - fitness_ring(q, index, cx, cy, 1) +
         fitness_ring(q, index, cx, cy, 2);
}

static int fitness_capstone(const struct k_quirc *q, int index, int x, int y) {
  x += 3;
  y += 3;

  return fitness_cell(q, index, x, y) + fitness_ring(q, index, x, y, 1) -
         fitness_ring(q, index, x, y, 2) + fitness_ring(q, index, x, y, 3);
}

/* Compute comprehensive fitness score for perspective transform */
static int fitness_all(const struct k_quirc *q, int index) {
  const struct quirc_grid *qr = &q->grids[index];
  int version = (qr->grid_size - 17) / 4;
  const struct quirc_version_info *info = &quirc_version_db[version];
  int score = 0;
  int ap_count;

  /* Check the timing pattern - alternating black/white cells */
  for (int i = 0; i < qr->grid_size - 14; i++) {
    int expect = (i & 1) ? 1 : -1;
    score += fitness_cell(q, index, i + 7, 6) * expect;
    score += fitness_cell(q, index, 6, i + 7) * expect;
  }

  /* Check capstones */
  score += fitness_capstone(q, index, 0, 0);
  score += fitness_capstone(q, index, qr->grid_size - 7, 0);
  score += fitness_capstone(q, index, 0, qr->grid_size - 7);

  if (version < 0 || version > QUIRC_MAX_VERSION)
    return score;

  /* Check alignment patterns */
  ap_count = 0;
  while ((ap_count < QUIRC_MAX_ALIGNMENT) && info->apat[ap_count])
    ap_count++;

  for (int i = 1; i + 1 < ap_count; i++) {
    score += fitness_apat(q, index, 6, info->apat[i]);
    score += fitness_apat(q, index, info->apat[i], 6);
  }

  for (int i = 1; i < ap_count; i++)
    for (int j = 1; j < ap_count; j++)
      score += fitness_apat(q, index, info->apat[i], info->apat[j]);

  return score;
}

static void jiggle_perspective(struct k_quirc *q, int index) {
  struct quirc_grid *qr = &q->grids[index];
  int best = fitness_all(q, index);
  int pass;
  float adjustments[8];

  /* Pre-compute initial adjustment steps */
  for (int i = 0; i < 8; i++)
    adjustments[i] = qr->c[i] * 0.02f;

  for (pass = 0; pass < 5; pass++) {
    for (int i = 0; i < 16; i++) {
      int j = i >> 1;
      int test;
      float old = qr->c[j];
      float step = adjustments[j];
      float new_val;

      if (i & 1)
        new_val = old + step;
      else
        new_val = old - step;

      qr->c[j] = new_val;
      test = fitness_all(q, index);

      if (test > best)
        best = test;
      else
        qr->c[j] = old;
    }

    /* Reduce step size for finer adjustments in next pass */
    for (int i = 0; i < 8; i++)
      adjustments[i] *= 0.5f;
  }
}

static void setup_qr_perspective(struct k_quirc *q, int index) {
  struct quirc_grid *qr = &q->grids[index];
  struct quirc_point rect[4];

  memcpy(&rect[0], &q->capstones[qr->caps[1]].corners[0], sizeof(rect[0]));
  memcpy(&rect[1], &q->capstones[qr->caps[2]].corners[0], sizeof(rect[1]));
  memcpy(&rect[2], &qr->align, sizeof(rect[2]));
  memcpy(&rect[3], &q->capstones[qr->caps[0]].corners[0], sizeof(rect[3]));

  perspective_setup(qr->c, rect, qr->grid_size - 7, qr->grid_size - 7);
  jiggle_perspective(q, index);
}

static float length(struct quirc_point a, struct quirc_point b) {
  float x = abs(a.x - b.x) + 1;
  float y = abs(a.y - b.y) + 1;
  return sqrtf(x * x + y * y);
}

/* Estimate grid size by determining distance between capstones */
static void measure_grid_size(struct k_quirc *q, int index) {
  struct quirc_grid *qr = &q->grids[index];

  struct quirc_capstone *a = &(q->capstones[qr->caps[0]]);
  struct quirc_capstone *b = &(q->capstones[qr->caps[1]]);
  struct quirc_capstone *c = &(q->capstones[qr->caps[2]]);

  float ab = length(b->corners[0], a->corners[3]);
  float capstone_ab_size = (length(b->corners[0], b->corners[3]) +
                            length(a->corners[0], a->corners[3])) /
                           2.0f;
  float ver_grid = 7.0f * ab / capstone_ab_size;

  float bc = length(b->corners[0], c->corners[1]);
  float capstone_bc_size = (length(b->corners[0], b->corners[1]) +
                            length(c->corners[0], c->corners[1])) /
                           2.0f;
  float hor_grid = 7.0f * bc / capstone_bc_size;

  float grid_size_estimate = (ver_grid + hor_grid) * 0.5f;

  int ver = (int)((grid_size_estimate - 15.0f) * 0.25f);

  qr->grid_size = 4 * ver + 17;
}

/* Rotate the capstone so that corner 0 is the leftmost with respect
 * to the given reference line.
 */
static void rotate_capstone(struct quirc_capstone *cap,
                            const struct quirc_point *h0,
                            const struct quirc_point *hd) {
  struct quirc_point copy[4];
  int best = 0;
  int best_score = 0;

  for (int j = 0; j < 4; j++) {
    struct quirc_point *p = &cap->corners[j];
    int score = (p->x - h0->x) * -hd->y + (p->y - h0->y) * hd->x;

    if (!j || score < best_score) {
      best = j;
      best_score = score;
    }
  }

  /* Rotate the capstone */
  for (int j = 0; j < 4; j++)
    memcpy(&copy[j], &cap->corners[(j + best) % 4], sizeof(copy[j]));
  memcpy(cap->corners, copy, sizeof(cap->corners));
  perspective_setup(cap->c, cap->corners, 7.0f, 7.0f);
}

static void record_qr_grid(struct k_quirc *q, int a, int b, int c) {
  struct quirc_point h0, hd;
  struct quirc_grid *qr;

  if (q->num_grids >= QUIRC_MAX_GRIDS)
    return;

  /* Construct the hypotenuse line from A to C. B should be to
   * the left of this line.
   */
  memcpy(&h0, &q->capstones[a].center, sizeof(h0));
  hd.x = q->capstones[c].center.x - q->capstones[a].center.x;
  hd.y = q->capstones[c].center.y - q->capstones[a].center.y;

  /* Make sure A-B-C is clockwise */
  if ((q->capstones[b].center.x - h0.x) * -hd.y +
          (q->capstones[b].center.y - h0.y) * hd.x >
      0) {
    int swap = a;
    a = c;
    c = swap;
    hd.x = -hd.x;
    hd.y = -hd.y;
  }

  qr = &q->grids[q->num_grids];
  memset(qr, 0, sizeof(*qr));
  qr->caps[0] = a;
  qr->caps[1] = b;
  qr->caps[2] = c;
  qr->align_region = -1;

  /* Rotate each capstone so that corner 0 is top-left with respect
   * to the grid.
   */
  for (int i = 0; i < 3; i++) {
    struct quirc_capstone *cap = &q->capstones[qr->caps[i]];
    rotate_capstone(cap, &h0, &hd);
    cap->qr_grid = q->num_grids;
  }

  /* Measure grid size using capstone distances (always gives valid version) */
  measure_grid_size(q, q->num_grids);

  if (qr->grid_size < 21)
    return;

  if (qr->grid_size > 177)
    return;

  line_intersect(&q->capstones[a].corners[0], &q->capstones[a].corners[1],
                 &q->capstones[c].corners[0], &q->capstones[c].corners[3],
                 &qr->align);

  /* On V2+ grids, we should use the alignment pattern. */
  if (qr->grid_size > 21) {
    struct polygon_score_data psd;
    struct quirc_point rough;

    /* Save the initial estimate from line_intersect */
    memcpy(&rough, &qr->align, sizeof(rough));

    /* Try to find the actual location of the alignment pattern */
    find_alignment_pattern(q, q->num_grids);

    if (qr->align_region >= 0) {
      struct quirc_region *r = &q->regions[qr->align_region];

      /* Use the alignment region seed as new reference */
      memcpy(&qr->align, &r->seed, sizeof(qr->align));

      memset(&psd, 0, sizeof(psd));
      psd.corners = &qr->align;
      memcpy(&psd.ref, &rough, sizeof(psd.ref));
      psd.scores[0] = INT32_MAX;

      flood_fill_seed(q, r->seed.x, r->seed.y, qr->align_region,
                      QUIRC_PIXEL_BLACK, find_leftmost_to_line, &psd, 0);
    }
  }

  qr->tpep[2].x = qr->align.x;
  qr->tpep[2].y =
      q->capstones[a].center.y + (q->capstones[a].center.y - qr->align.y);

  setup_qr_perspective(q, q->num_grids);
  q->num_grids++;
}

struct neighbour {
  int index;
  float distance;
};

struct neighbour_list {
  struct neighbour n[QUIRC_MAX_CAPSTONES];
  int count;
};

static void test_neighbours(struct k_quirc *q, int i,
                            const struct neighbour_list *hlist,
                            const struct neighbour_list *vlist) {
  /* Test each possible grouping - matching ESP quirc's simpler approach */
  for (int j = 0; j < hlist->count; j++) {
    const struct neighbour *hn = &hlist->n[j];
    for (int k = 0; k < vlist->count; k++) {
      const struct neighbour *vn = &vlist->n[k];
      float squareness = fabsf(1.0f - hn->distance / vn->distance);
      if (squareness < 0.2f)
        record_qr_grid(q, hn->index, i, vn->index);
    }
  }
}

static void test_grouping(struct k_quirc *q, int i) {
  struct quirc_capstone *c1 = &q->capstones[i];
  int j;
  struct neighbour_list hlist, vlist;

  hlist.count = 0;
  vlist.count = 0;

  /* Look for potential neighbours by examining the relative gradients
   * from this capstone to others.
   */
  for (j = 0; j < q->num_capstones; j++) {
    struct quirc_capstone *c2 = &q->capstones[j];
    float u, v;

    if (i == j)
      continue;

    perspective_unmap(c1->c, &c2->center, &u, &v);

    u = fabsf(u - 3.5f);
    v = fabsf(v - 3.5f);

    if (u < 0.2f * v) {
      struct neighbour *n = &hlist.n[hlist.count++];
      n->index = j;
      n->distance = v;
    }

    if (v < 0.2f * u) {
      struct neighbour *n = &vlist.n[vlist.count++];
      n->index = j;
      n->distance = u;
    }
  }

  if (!(hlist.count && vlist.count))
    return;

  test_neighbours(q, i, &hlist, &vlist);
}

static void pixels_setup(struct k_quirc *q) {
  if (sizeof(*q->image) == sizeof(*q->pixels)) {
    q->pixels = (quirc_pixel_t *)q->image;
  } else {
    for (int i = 0; i < q->w * q->h; i++)
      q->pixels[i] = q->image[i];
  }
}

/*
 * Decoding routines
 */
#define MAX_POLY 64

struct galois_field {
  int p;
  const uint8_t *log;
  const uint8_t *exp;
};

static const uint8_t gf16_exp[16] = {0x01, 0x02, 0x04, 0x08, 0x03, 0x06,
                                     0x0c, 0x0b, 0x05, 0x0a, 0x07, 0x0e,
                                     0x0f, 0x0d, 0x09, 0x01};

static const uint8_t gf16_log[16] = {0x00, 0x0f, 0x01, 0x04, 0x02, 0x08,
                                     0x05, 0x0a, 0x03, 0x0e, 0x09, 0x07,
                                     0x06, 0x0d, 0x0b, 0x0c};

static const struct galois_field gf16 = {
    .p = 15, .log = gf16_log, .exp = gf16_exp};

static const uint8_t gf256_exp[256] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1d, 0x3a, 0x74, 0xe8,
    0xcd, 0x87, 0x13, 0x26, 0x4c, 0x98, 0x2d, 0x5a, 0xb4, 0x75, 0xea, 0xc9,
    0x8f, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x9d, 0x27, 0x4e, 0x9c,
    0x25, 0x4a, 0x94, 0x35, 0x6a, 0xd4, 0xb5, 0x77, 0xee, 0xc1, 0x9f, 0x23,
    0x46, 0x8c, 0x05, 0x0a, 0x14, 0x28, 0x50, 0xa0, 0x5d, 0xba, 0x69, 0xd2,
    0xb9, 0x6f, 0xde, 0xa1, 0x5f, 0xbe, 0x61, 0xc2, 0x99, 0x2f, 0x5e, 0xbc,
    0x65, 0xca, 0x89, 0x0f, 0x1e, 0x3c, 0x78, 0xf0, 0xfd, 0xe7, 0xd3, 0xbb,
    0x6b, 0xd6, 0xb1, 0x7f, 0xfe, 0xe1, 0xdf, 0xa3, 0x5b, 0xb6, 0x71, 0xe2,
    0xd9, 0xaf, 0x43, 0x86, 0x11, 0x22, 0x44, 0x88, 0x0d, 0x1a, 0x34, 0x68,
    0xd0, 0xbd, 0x67, 0xce, 0x81, 0x1f, 0x3e, 0x7c, 0xf8, 0xed, 0xc7, 0x93,
    0x3b, 0x76, 0xec, 0xc5, 0x97, 0x33, 0x66, 0xcc, 0x85, 0x17, 0x2e, 0x5c,
    0xb8, 0x6d, 0xda, 0xa9, 0x4f, 0x9e, 0x21, 0x42, 0x84, 0x15, 0x2a, 0x54,
    0xa8, 0x4d, 0x9a, 0x29, 0x52, 0xa4, 0x55, 0xaa, 0x49, 0x92, 0x39, 0x72,
    0xe4, 0xd5, 0xb7, 0x73, 0xe6, 0xd1, 0xbf, 0x63, 0xc6, 0x91, 0x3f, 0x7e,
    0xfc, 0xe5, 0xd7, 0xb3, 0x7b, 0xf6, 0xf1, 0xff, 0xe3, 0xdb, 0xab, 0x4b,
    0x96, 0x31, 0x62, 0xc4, 0x95, 0x37, 0x6e, 0xdc, 0xa5, 0x57, 0xae, 0x41,
    0x82, 0x19, 0x32, 0x64, 0xc8, 0x8d, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0,
    0xdd, 0xa7, 0x53, 0xa6, 0x51, 0xa2, 0x59, 0xb2, 0x79, 0xf2, 0xf9, 0xef,
    0xc3, 0x9b, 0x2b, 0x56, 0xac, 0x45, 0x8a, 0x09, 0x12, 0x24, 0x48, 0x90,
    0x3d, 0x7a, 0xf4, 0xf5, 0xf7, 0xf3, 0xfb, 0xeb, 0xcb, 0x8b, 0x0b, 0x16,
    0x2c, 0x58, 0xb0, 0x7d, 0xfa, 0xe9, 0xcf, 0x83, 0x1b, 0x36, 0x6c, 0xd8,
    0xad, 0x47, 0x8e, 0x01};

static const uint8_t gf256_log[256] = {
    0x00, 0xff, 0x01, 0x19, 0x02, 0x32, 0x1a, 0xc6, 0x03, 0xdf, 0x33, 0xee,
    0x1b, 0x68, 0xc7, 0x4b, 0x04, 0x64, 0xe0, 0x0e, 0x34, 0x8d, 0xef, 0x81,
    0x1c, 0xc1, 0x69, 0xf8, 0xc8, 0x08, 0x4c, 0x71, 0x05, 0x8a, 0x65, 0x2f,
    0xe1, 0x24, 0x0f, 0x21, 0x35, 0x93, 0x8e, 0xda, 0xf0, 0x12, 0x82, 0x45,
    0x1d, 0xb5, 0xc2, 0x7d, 0x6a, 0x27, 0xf9, 0xb9, 0xc9, 0x9a, 0x09, 0x78,
    0x4d, 0xe4, 0x72, 0xa6, 0x06, 0xbf, 0x8b, 0x62, 0x66, 0xdd, 0x30, 0xfd,
    0xe2, 0x98, 0x25, 0xb3, 0x10, 0x91, 0x22, 0x88, 0x36, 0xd0, 0x94, 0xce,
    0x8f, 0x96, 0xdb, 0xbd, 0xf1, 0xd2, 0x13, 0x5c, 0x83, 0x38, 0x46, 0x40,
    0x1e, 0x42, 0xb6, 0xa3, 0xc3, 0x48, 0x7e, 0x6e, 0x6b, 0x3a, 0x28, 0x54,
    0xfa, 0x85, 0xba, 0x3d, 0xca, 0x5e, 0x9b, 0x9f, 0x0a, 0x15, 0x79, 0x2b,
    0x4e, 0xd4, 0xe5, 0xac, 0x73, 0xf3, 0xa7, 0x57, 0x07, 0x70, 0xc0, 0xf7,
    0x8c, 0x80, 0x63, 0x0d, 0x67, 0x4a, 0xde, 0xed, 0x31, 0xc5, 0xfe, 0x18,
    0xe3, 0xa5, 0x99, 0x77, 0x26, 0xb8, 0xb4, 0x7c, 0x11, 0x44, 0x92, 0xd9,
    0x23, 0x20, 0x89, 0x2e, 0x37, 0x3f, 0xd1, 0x5b, 0x95, 0xbc, 0xcf, 0xcd,
    0x90, 0x87, 0x97, 0xb2, 0xdc, 0xfc, 0xbe, 0x61, 0xf2, 0x56, 0xd3, 0xab,
    0x14, 0x2a, 0x5d, 0x9e, 0x84, 0x3c, 0x39, 0x53, 0x47, 0x6d, 0x41, 0xa2,
    0x1f, 0x2d, 0x43, 0xd8, 0xb7, 0x7b, 0xa4, 0x76, 0xc4, 0x17, 0x49, 0xec,
    0x7f, 0x0c, 0x6f, 0xf6, 0x6c, 0xa1, 0x3b, 0x52, 0x29, 0x9d, 0x55, 0xaa,
    0xfb, 0x60, 0x86, 0xb1, 0xbb, 0xcc, 0x3e, 0x5a, 0xcb, 0x59, 0x5f, 0xb0,
    0x9c, 0xa9, 0xa0, 0x51, 0x0b, 0xf5, 0x16, 0xeb, 0x7a, 0x75, 0x2c, 0xd7,
    0x4f, 0xae, 0xd5, 0xe9, 0xe6, 0xe7, 0xad, 0xe8, 0x74, 0xd6, 0xf4, 0xea,
    0xa8, 0x50, 0x58, 0xaf};

static const struct galois_field gf256 = {
    .p = 255, .log = gf256_log, .exp = gf256_exp};

static void poly_add(uint8_t *dst, const uint8_t *src, uint8_t c, int shift,
                     const struct galois_field *gf) {
  int log_c = gf->log[c];

  if (!c)
    return;

  for (int i = 0; i < MAX_POLY; i++) {
    int p = i + shift;
    uint8_t v = src[i];

    if (p < 0 || p >= MAX_POLY)
      continue;
    if (!v)
      continue;

    dst[p] ^= gf->exp[(gf->log[v] + log_c) % gf->p];
  }
}

static uint8_t poly_eval(const uint8_t *s, uint8_t x,
                         const struct galois_field *gf) {
  uint8_t sum = 0;
  uint8_t log_x = gf->log[x];

  if (!x)
    return s[0];

  for (int i = 0; i < MAX_POLY; i++) {
    uint8_t c = s[i];

    if (!c)
      continue;

    sum ^= gf->exp[(gf->log[c] + log_x * i) % gf->p];
  }

  return sum;
}

static void berlekamp_massey(const uint8_t *s, int N,
                             const struct galois_field *gf, uint8_t *sigma) {
  uint8_t C[MAX_POLY];
  uint8_t B[MAX_POLY];
  int L = 0;
  int m = 1;
  uint8_t b = 1;

  memset(B, 0, sizeof(B));
  memset(C, 0, sizeof(C));
  B[0] = 1;
  C[0] = 1;

  for (int n = 0; n < N; n++) {
    uint8_t d = s[n];
    uint8_t mult;

    for (int i = 1; i <= L; i++) {
      if (!(C[i] && s[n - i]))
        continue;

      d ^= gf->exp[(gf->log[C[i]] + gf->log[s[n - i]]) % gf->p];
    }

    mult = gf->exp[(gf->p - gf->log[b] + gf->log[d]) % gf->p];

    if (!d) {
      m++;
    } else if (L * 2 <= n) {
      uint8_t T[MAX_POLY];

      memcpy(T, C, sizeof(T));
      poly_add(C, B, mult, m, gf);
      memcpy(B, T, sizeof(B));
      L = n + 1 - L;
      b = d;
      m = 1;
    } else {
      poly_add(C, B, mult, m, gf);
      m++;
    }
  }

  memcpy(sigma, C, MAX_POLY);
}

static int block_syndromes(const uint8_t *data, int bs, int npar, uint8_t *s) {
  int nonzero = 0;

  memset(s, 0, MAX_POLY);

  for (int i = 0; i < npar; i++) {
    for (int j = 0; j < bs; j++) {
      uint8_t c = data[bs - j - 1];

      if (!c)
        continue;

      s[i] ^= gf256_exp[((int)gf256_log[c] + i * j) % 255];
    }

    if (s[i])
      nonzero = 1;
  }

  return nonzero;
}

static void eloc_poly(uint8_t *omega, const uint8_t *s, const uint8_t *sigma,
                      int npar) {
  memset(omega, 0, MAX_POLY);

  for (int i = 0; i < npar; i++) {
    const uint8_t a = sigma[i];
    const uint8_t log_a = gf256_log[a];

    if (!a)
      continue;

    for (int j = 0; j + 1 < MAX_POLY; j++) {
      const uint8_t b = s[j + 1];

      if (i + j >= npar)
        break;

      if (!b)
        continue;

      omega[i + j] ^= gf256_exp[(log_a + gf256_log[b]) % 255];
    }
  }
}

static k_quirc_error_t correct_block(uint8_t *data,
                                     const struct quirc_rs_params *ecc) {
  int npar = ecc->bs - ecc->dw;
  uint8_t s[MAX_POLY];
  uint8_t sigma[MAX_POLY];
  uint8_t sigma_deriv[MAX_POLY];
  uint8_t omega[MAX_POLY];

  if (!block_syndromes(data, ecc->bs, npar, s))
    return K_QUIRC_SUCCESS;

  berlekamp_massey(s, npar, &gf256, sigma);

  memset(sigma_deriv, 0, MAX_POLY);
  for (int i = 0; i + 1 < MAX_POLY; i += 2)
    sigma_deriv[i] = sigma[i + 1];

  eloc_poly(omega, s, sigma, npar - 1);

  for (int i = 0; i < ecc->bs; i++) {
    uint8_t xinv = gf256_exp[255 - i];

    if (!poly_eval(sigma, xinv, &gf256)) {
      uint8_t sd_x = poly_eval(sigma_deriv, xinv, &gf256);
      uint8_t omega_x = poly_eval(omega, xinv, &gf256);
      uint8_t error =
          gf256_exp[(255 - gf256_log[sd_x] + gf256_log[omega_x]) % 255];

      data[ecc->bs - i - 1] ^= error;
    }
  }

  if (block_syndromes(data, ecc->bs, npar, s))
    return K_QUIRC_ERROR_DATA_ECC;

  return K_QUIRC_SUCCESS;
}

#define FORMAT_MAX_ERROR 3
#define FORMAT_SYNDROMES (FORMAT_MAX_ERROR * 2)
#define FORMAT_BITS 15

static int format_syndromes(uint16_t u, uint8_t *s) {
  int nonzero = 0;

  memset(s, 0, MAX_POLY);

  for (int i = 0; i < FORMAT_SYNDROMES; i++) {
    s[i] = 0;
    for (int j = 0; j < FORMAT_BITS; j++)
      if (u & (1 << j))
        s[i] ^= gf16_exp[((i + 1) * j) % 15];

    if (s[i])
      nonzero = 1;
  }

  return nonzero;
}

static k_quirc_error_t correct_format(uint16_t *f_ret) {
  uint16_t u = *f_ret;
  uint8_t s[MAX_POLY];
  uint8_t sigma[MAX_POLY];

  if (!format_syndromes(u, s))
    return K_QUIRC_SUCCESS;

  berlekamp_massey(s, FORMAT_SYNDROMES, &gf16, sigma);

  for (int i = 0; i < 15; i++)
    if (!poly_eval(sigma, gf16_exp[15 - i], &gf16))
      u ^= (1 << i);

  if (format_syndromes(u, s))
    return K_QUIRC_ERROR_FORMAT_ECC;

  *f_ret = u;
  return K_QUIRC_SUCCESS;
}

struct datastream {
  uint8_t raw[K_QUIRC_MAX_PAYLOAD];
  int data_bits;
  int ptr;
  uint8_t data[K_QUIRC_MAX_PAYLOAD];
};

static inline int grid_bit(const struct quirc_code *code, int x, int y) {
  int p = y * code->size + x;
  return (code->cell_bitmap[p >> 3] >> (p & 7)) & 1;
}

static k_quirc_error_t read_format(const struct quirc_code *code,
                                   struct quirc_data *data, int which) {
  uint16_t format = 0;
  uint16_t fdata;
  k_quirc_error_t err;

  if (which) {
    for (int i = 0; i < 7; i++)
      format = (format << 1) | grid_bit(code, 8, code->size - 1 - i);
    for (int i = 0; i < 8; i++)
      format = (format << 1) | grid_bit(code, code->size - 8 + i, 8);
  } else {
    static const int xs[15] = {8, 8, 8, 8, 8, 8, 8, 8, 7, 5, 4, 3, 2, 1, 0};
    static const int ys[15] = {0, 1, 2, 3, 4, 5, 7, 8, 8, 8, 8, 8, 8, 8, 8};

    for (int i = 14; i >= 0; i--)
      format = (format << 1) | grid_bit(code, xs[i], ys[i]);
  }

  format ^= 0x5412;

  err = correct_format(&format);
  if (err)
    return err;

  fdata = format >> 10;
  data->ecc_level = fdata >> 3;
  data->mask = fdata & 7;

  return K_QUIRC_SUCCESS;
}

static int mask_bit(int mask, int i, int j) {
  switch (mask) {
  case 0:
    return !((i + j) % 2);
  case 1:
    return !(i % 2);
  case 2:
    return !(j % 3);
  case 3:
    return !((i + j) % 3);
  case 4:
    return !(((i / 2) + (j / 3)) % 2);
  case 5:
    return !((i * j) % 2 + (i * j) % 3);
  case 6:
    return !(((i * j) % 2 + (i * j) % 3) % 2);
  case 7:
    return !(((i * j) % 3 + (i + j) % 2) % 2);
  }

  return 0;
}

static int reserved_cell(int version, int i, int j) {
  const struct quirc_version_info *ver = &quirc_version_db[version];
  int size = version * 4 + 17;
  int ai = -1, aj = -1, a;

  if (i < 9 && j < 9)
    return 1;
  if (i < 9 && j >= size - 8)
    return 1;
  if (i >= size - 8 && j < 9)
    return 1;

  if (i == 6 || j == 6)
    return 1;

  if (version >= 7) {
    if (i < 6 && j >= size - 11)
      return 1;
    if (i >= size - 11 && j < 6)
      return 1;
  }

  a = 0;
  while (a < QUIRC_MAX_ALIGNMENT && ver->apat[a])
    a++;

  if (a) {
    int p;

    for (p = 0; p < a; p++) {
      if (abs(ver->apat[p] - i) < 3)
        ai = p;
      if (abs(ver->apat[p] - j) < 3)
        aj = p;
    }

    if (ai >= 0 && aj >= 0) {
      if (ai == 0 && aj == 0)
        return 0;
      if (ai == 0 && aj == a - 1)
        return 0;
      if (ai == a - 1 && aj == 0)
        return 0;
      return 1;
    }
  }

  return 0;
}

static void read_bit(const struct quirc_code *code, struct quirc_data *data,
                     struct datastream *ds, int i, int j) {
  int bitpos = ds->data_bits & 7;
  int bytepos = ds->data_bits >> 3;
  int v = grid_bit(code, j, i);

  if (mask_bit(data->mask, i, j))
    v ^= 1;

  if (v)
    ds->raw[bytepos] |= (0x80 >> bitpos);

  ds->data_bits++;
}

static void read_data(const struct quirc_code *code, struct quirc_data *data,
                      struct datastream *ds) {
  int y = code->size - 1;
  int x = code->size - 1;
  int dir = -1;

  while (x > 0) {
    if (x == 6)
      x--;

    if (!reserved_cell(data->version, y, x))
      read_bit(code, data, ds, y, x);
    if (!reserved_cell(data->version, y, x - 1))
      read_bit(code, data, ds, y, x - 1);

    y += dir;
    if (y < 0 || y >= code->size) {
      dir = -dir;
      x -= 2;
      y += dir;
    }
  }
}

static k_quirc_error_t codestream_ecc(struct quirc_data *data,
                                      struct datastream *ds) {
  const struct quirc_version_info *ver = &quirc_version_db[data->version];
  const struct quirc_rs_params *sb_ecc = &ver->ecc[data->ecc_level];
  struct quirc_rs_params lb_ecc;
  const int lb_count =
      (ver->data_bytes - sb_ecc->bs * sb_ecc->ns) / (sb_ecc->bs + 1);
  const int bc = lb_count + sb_ecc->ns;
  const int ecc_offset = sb_ecc->dw * bc + lb_count;
  int dst_offset = 0;

  memcpy(&lb_ecc, sb_ecc, sizeof(lb_ecc));
  lb_ecc.bs++;
  lb_ecc.dw++;

  for (int i = 0; i < bc; i++) {
    uint8_t *dst = ds->data + dst_offset;
    const struct quirc_rs_params *ecc = (i < sb_ecc->ns) ? sb_ecc : &lb_ecc;
    const int num_ec = ecc->bs - ecc->dw;
    k_quirc_error_t err;

    for (int j = 0; j < ecc->dw; j++)
      dst[j] = ds->raw[j * bc + i];
    for (int j = 0; j < num_ec; j++)
      dst[ecc->dw + j] = ds->raw[ecc_offset + j * bc + i];

    err = correct_block(dst, ecc);
    if (err)
      return err;

    dst_offset += ecc->dw;
  }

  ds->data_bits = dst_offset * 8;

  return K_QUIRC_SUCCESS;
}

static inline int bits_remaining(const struct datastream *ds) {
  return ds->data_bits - ds->ptr;
}

static int take_bits(struct datastream *ds, int len) {
  int ret = 0;

  while (len && (ds->ptr < ds->data_bits)) {
    uint8_t b = ds->data[ds->ptr >> 3];
    int bitpos = ds->ptr & 7;

    ret <<= 1;
    if ((b << bitpos) & 0x80)
      ret |= 1;

    ds->ptr++;
    len--;
  }

  return ret;
}

static int numeric_tuple(struct quirc_data *data, struct datastream *ds,
                         int bits, int digits) {
  int tuple;
  int i;

  if (bits_remaining(ds) < bits)
    return -1;

  tuple = take_bits(ds, bits);

  for (i = digits - 1; i >= 0; i--) {
    data->payload[data->payload_len + i] = tuple % 10 + '0';
    tuple /= 10;
  }

  data->payload_len += digits;
  return 0;
}

static k_quirc_error_t decode_numeric(struct quirc_data *data,
                                      struct datastream *ds) {
  int bits = 14;
  int count;

  if (data->version < 10)
    bits = 10;
  else if (data->version < 27)
    bits = 12;

  count = take_bits(ds, bits);
  if (data->payload_len + count + 1 > K_QUIRC_MAX_PAYLOAD)
    return K_QUIRC_ERROR_DATA_OVERFLOW;

  while (count >= 3) {
    if (numeric_tuple(data, ds, 10, 3) < 0)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;
    count -= 3;
  }

  if (count >= 2) {
    if (numeric_tuple(data, ds, 7, 2) < 0)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;
    count -= 2;
  }

  if (count) {
    if (numeric_tuple(data, ds, 4, 1) < 0)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;
  }

  return K_QUIRC_SUCCESS;
}

static const char *alpha_map = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

static k_quirc_error_t decode_alpha(struct quirc_data *data,
                                    struct datastream *ds) {
  int bits = 13;
  int count;

  if (data->version < 10)
    bits = 9;
  else if (data->version < 27)
    bits = 11;

  count = take_bits(ds, bits);
  if (data->payload_len + count + 1 > K_QUIRC_MAX_PAYLOAD)
    return K_QUIRC_ERROR_DATA_OVERFLOW;

  while (count >= 2) {
    int d;

    if (bits_remaining(ds) < 11)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;

    d = take_bits(ds, 11);
    data->payload[data->payload_len++] = alpha_map[d / 45];
    data->payload[data->payload_len++] = alpha_map[d % 45];
    count -= 2;
  }

  if (count) {
    int d;

    if (bits_remaining(ds) < 6)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;

    d = take_bits(ds, 6);
    data->payload[data->payload_len++] = alpha_map[d];
  }

  return K_QUIRC_SUCCESS;
}

static k_quirc_error_t decode_byte(struct quirc_data *data,
                                   struct datastream *ds) {
  int bits = 16;
  int count;

  if (data->version < 10)
    bits = 8;

  count = take_bits(ds, bits);
  if (data->payload_len + count + 1 > K_QUIRC_MAX_PAYLOAD)
    return K_QUIRC_ERROR_DATA_OVERFLOW;

  while (count) {
    if (bits_remaining(ds) < 8)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;

    data->payload[data->payload_len++] = take_bits(ds, 8);
    count--;
  }

  return K_QUIRC_SUCCESS;
}

static k_quirc_error_t decode_kanji(struct quirc_data *data,
                                    struct datastream *ds) {
  int bits = 12;
  int count;

  if (data->version < 10)
    bits = 8;
  else if (data->version < 27)
    bits = 10;

  count = take_bits(ds, bits);
  if (data->payload_len + count * 2 + 1 > K_QUIRC_MAX_PAYLOAD)
    return K_QUIRC_ERROR_DATA_OVERFLOW;

  while (count) {
    int d;
    int ms_byte, ls_byte;

    if (bits_remaining(ds) < 13)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;

    d = take_bits(ds, 13);
    ms_byte = d / 0xc0;
    ls_byte = d % 0xc0;
    d = ms_byte * 256 + ls_byte;

    if (d + 0x8140 <= 0x9ffc)
      d += 0x8140;
    else
      d += 0xc140;

    data->payload[data->payload_len++] = d >> 8;
    data->payload[data->payload_len++] = d & 0xff;
    count--;
  }

  return K_QUIRC_SUCCESS;
}

static k_quirc_error_t decode_eci(struct quirc_data *data,
                                  struct datastream *ds) {
  if (bits_remaining(ds) < 8)
    return K_QUIRC_ERROR_DATA_UNDERFLOW;

  data->eci = take_bits(ds, 8);

  if ((data->eci & 0xc0) == 0x80) {
    if (bits_remaining(ds) < 8)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;

    data->eci = (data->eci << 8) | take_bits(ds, 8);
  } else if ((data->eci & 0xe0) == 0xc0) {
    if (bits_remaining(ds) < 16)
      return K_QUIRC_ERROR_DATA_UNDERFLOW;

    data->eci = (data->eci << 16) | take_bits(ds, 16);
  }

  return K_QUIRC_SUCCESS;
}

static k_quirc_error_t decode_payload(struct quirc_data *data,
                                      struct datastream *ds) {
  while (bits_remaining(ds) >= 4) {
    k_quirc_error_t err = K_QUIRC_SUCCESS;
    int type = take_bits(ds, 4);

    switch (type) {
    case K_QUIRC_DATA_TYPE_NUMERIC:
      err = decode_numeric(data, ds);
      break;

    case K_QUIRC_DATA_TYPE_ALPHA:
      err = decode_alpha(data, ds);
      break;

    case K_QUIRC_DATA_TYPE_BYTE:
      err = decode_byte(data, ds);
      break;

    case K_QUIRC_DATA_TYPE_KANJI:
      err = decode_kanji(data, ds);
      break;

    case 7:
      err = decode_eci(data, ds);
      break;

    default:
      goto done;
    }

    if (err)
      return err;

    if (type != 7)
      data->data_type = type;
  }
done:

  data->payload[data->payload_len] = 0;
  return K_QUIRC_SUCCESS;
}

static k_quirc_error_t quirc_decode_internal(const struct quirc_code *code,
                                             struct quirc_data *data) {
  k_quirc_error_t err;
  struct datastream *ds;

  if ((code->size - 17) % 4)
    return K_QUIRC_ERROR_INVALID_GRID_SIZE;

  ds = K_MALLOC(sizeof(*ds));
  if (!ds)
    return K_QUIRC_ERROR_ALLOC_FAILED;

  memset(ds, 0, sizeof(*ds));
  memset(data, 0, sizeof(*data));

  data->version = (code->size - 17) / 4;

  if (data->version < 1 || data->version > QUIRC_MAX_VERSION) {
    K_FREE(ds);
    return K_QUIRC_ERROR_INVALID_VERSION;
  }

  err = read_format(code, data, 0);
  if (err) {
    err = read_format(code, data, 1);
    if (err) {
      K_FREE(ds);
      return err;
    }
  }

  read_data(code, data, ds);
  err = codestream_ecc(data, ds);
  if (err) {
    K_FREE(ds);
    return err;
  }

  err = decode_payload(data, ds);
  K_FREE(ds);
  return err;
}

static void quirc_extract_internal(const struct k_quirc *q, int index,
                                   struct quirc_code *code) {
  const struct quirc_grid *qr = &q->grids[index];

  if (index < 0 || index >= q->num_grids)
    return;

  memset(code, 0, sizeof(*code));

  perspective_map(qr->c, 0.0f, 0.0f, &code->corners[0]);
  perspective_map(qr->c, qr->grid_size, 0.0f, &code->corners[1]);
  perspective_map(qr->c, qr->grid_size, qr->grid_size, &code->corners[2]);
  perspective_map(qr->c, 0.0f, qr->grid_size, &code->corners[3]);

  code->size = qr->grid_size;

  int i = 0;
  for (int y = 0; y < qr->grid_size; y++) {
    for (int x = 0; x < qr->grid_size; x++) {
      struct quirc_point p;

      perspective_map(qr->c, x + 0.5f, y + 0.5f, &p);

      if (p.y >= 0 && p.y < q->h && p.x >= 0 && p.x < q->w) {
        if (q->pixels[p.y * q->w + p.x])
          code->cell_bitmap[i >> 3] |= (1 << (i & 7));
      }

      i++;
    }
  }
}

/*
 * Public API implementation
 */
k_quirc_t *k_quirc_new(void) {
  k_quirc_t *q = K_MALLOC(sizeof(*q));
  if (q)
    memset(q, 0, sizeof(*q));
  return q;
}

void k_quirc_destroy(k_quirc_t *q) {
  if (q) {
    if (q->image)
      K_FREE(q->image);
    if (sizeof(*q->image) != sizeof(*q->pixels) && q->pixels)
      K_FREE(q->pixels);
    K_FREE(q);
  }
}

int k_quirc_resize(k_quirc_t *q, int w, int h) {
  if (q->image)
    K_FREE(q->image);

  uint8_t *new_image = K_MALLOC(w * h);
  if (!new_image)
    return -1;

  if (sizeof(*q->image) != sizeof(*q->pixels)) {
    size_t new_size = w * h * sizeof(quirc_pixel_t);
    if (q->pixels)
      K_FREE(q->pixels);
    quirc_pixel_t *new_pixels = K_MALLOC(new_size);
    if (!new_pixels) {
      K_FREE(new_image);
      return -1;
    }
    q->pixels = new_pixels;
  }

  q->image = new_image;
  q->w = w;
  q->h = h;

  return 0;
}

uint8_t *k_quirc_begin(k_quirc_t *q, int *w, int *h) {
  q->num_regions = QUIRC_PIXEL_REGION;
  q->num_capstones = 0;
  q->num_grids = 0;

  if (w)
    *w = q->w;
  if (h)
    *h = q->h;

  return q->image;
}

void k_quirc_end(k_quirc_t *q, bool find_inverted) {
  pixels_setup(q);
  threshold(q, false);

  for (int i = 0; i < q->h; i++)
    finder_scan(q, i);

  for (int i = 0; i < q->num_capstones; i++)
    test_grouping(q, i);

  if (q->num_grids == 0 && find_inverted) {
    q->num_regions = QUIRC_PIXEL_REGION;
    q->num_capstones = 0;
    q->num_grids = 0;

    pixels_setup(q);
    threshold(q, true);

    for (int i = 0; i < q->h; i++)
      finder_scan(q, i);

    for (int i = 0; i < q->num_capstones; i++)
      test_grouping(q, i);
  }
}

int k_quirc_count(const k_quirc_t *q) { return q->num_grids; }

k_quirc_error_t k_quirc_decode(k_quirc_t *q, int index,
                               k_quirc_result_t *result) {
  struct quirc_code *code = K_MALLOC(sizeof(struct quirc_code));
  struct quirc_data *data = K_MALLOC(sizeof(struct quirc_data));

  if (!code || !data) {
    if (code)
      K_FREE(code);
    if (data)
      K_FREE(data);
    return K_QUIRC_ERROR_ALLOC_FAILED;
  }

  memset(result, 0, sizeof(*result));
  result->valid = false;

  if (index < 0 || index >= q->num_grids) {
    K_FREE(code);
    K_FREE(data);
    return K_QUIRC_ERROR_INVALID_GRID_SIZE;
  }

  quirc_extract_internal(q, index, code);

  k_quirc_error_t err = quirc_decode_internal(code, data);
  if (err == K_QUIRC_SUCCESS) {
    result->valid = true;
    for (int i = 0; i < 4; i++) {
      result->corners[i].x = code->corners[i].x;
      result->corners[i].y = code->corners[i].y;
    }
    result->data.version = data->version;
    result->data.ecc_level = data->ecc_level;
    result->data.mask = data->mask;
    result->data.data_type = data->data_type;
    result->data.payload_len = data->payload_len;
    result->data.eci = data->eci;
    memcpy(result->data.payload, data->payload, data->payload_len + 1);
  }

  K_FREE(code);
  K_FREE(data);

  return err;
}

const char *k_quirc_strerror(k_quirc_error_t err) {
  static const char *error_table[] = {
      [K_QUIRC_SUCCESS] = "Success",
      [K_QUIRC_ERROR_INVALID_GRID_SIZE] = "Invalid grid size",
      [K_QUIRC_ERROR_INVALID_VERSION] = "Invalid version",
      [K_QUIRC_ERROR_FORMAT_ECC] = "Format data ECC failure",
      [K_QUIRC_ERROR_DATA_ECC] = "ECC failure",
      [K_QUIRC_ERROR_UNKNOWN_DATA_TYPE] = "Unknown data type",
      [K_QUIRC_ERROR_DATA_OVERFLOW] = "Data overflow",
      [K_QUIRC_ERROR_DATA_UNDERFLOW] = "Data underflow",
      [K_QUIRC_ERROR_ALLOC_FAILED] = "Memory allocation failed"};

  if (err >= 0 && err < sizeof(error_table) / sizeof(error_table[0]))
    return error_table[err];

  return "Unknown error";
}

int k_quirc_decode_grayscale(const uint8_t *grayscale_data, int width,
                             int height, k_quirc_result_t *results,
                             int max_results, bool find_inverted) {
  k_quirc_t *q = k_quirc_new();
  if (!q)
    return 0;

  if (k_quirc_resize(q, width, height) < 0) {
    k_quirc_destroy(q);
    return 0;
  }

  uint8_t *buf = k_quirc_begin(q, NULL, NULL);
  memcpy(buf, grayscale_data, width * height);

  k_quirc_end(q, find_inverted);

  int count = k_quirc_count(q);
  int decoded = 0;

  for (int i = 0; i < count && decoded < max_results; i++) {
    k_quirc_error_t err = k_quirc_decode(q, i, &results[decoded]);
    if (err == K_QUIRC_SUCCESS) {
      decoded++;
    }
  }

  k_quirc_destroy(q);
  return decoded;
}
