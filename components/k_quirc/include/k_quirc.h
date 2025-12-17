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

#ifndef K_QUIRC_H
#define K_QUIRC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Limits on the maximum size of QR-codes and their content. */
#define K_QUIRC_MAX_BITMAP 3917
#define K_QUIRC_MAX_PAYLOAD 8896

/* QR-code ECC types. */
#define K_QUIRC_ECC_LEVEL_M 0
#define K_QUIRC_ECC_LEVEL_L 1
#define K_QUIRC_ECC_LEVEL_H 2
#define K_QUIRC_ECC_LEVEL_Q 3

/* QR-code data types. */
#define K_QUIRC_DATA_TYPE_NUMERIC 1
#define K_QUIRC_DATA_TYPE_ALPHA 2
#define K_QUIRC_DATA_TYPE_BYTE 4
#define K_QUIRC_DATA_TYPE_KANJI 8

/* Decoder error codes */
typedef enum {
  K_QUIRC_SUCCESS = 0,
  K_QUIRC_ERROR_INVALID_GRID_SIZE,
  K_QUIRC_ERROR_INVALID_VERSION,
  K_QUIRC_ERROR_FORMAT_ECC,
  K_QUIRC_ERROR_DATA_ECC,
  K_QUIRC_ERROR_UNKNOWN_DATA_TYPE,
  K_QUIRC_ERROR_DATA_OVERFLOW,
  K_QUIRC_ERROR_DATA_UNDERFLOW,
  K_QUIRC_ERROR_ALLOC_FAILED,
} k_quirc_error_t;

/* Point structure for corners */
typedef struct {
  int x;
  int y;
} k_quirc_point_t;

/* This structure holds the decoded QR-code data */
typedef struct {
  int version;
  int ecc_level;
  int mask;
  int data_type;
  uint8_t payload[K_QUIRC_MAX_PAYLOAD];
  int payload_len;
  uint32_t eci;
} k_quirc_data_t;

/* QR code detection result */
typedef struct {
  k_quirc_point_t corners[4];
  k_quirc_data_t data;
  bool valid;
} k_quirc_result_t;

/* Opaque decoder context */
typedef struct k_quirc k_quirc_t;

/**
 * Create a new QR-code decoder instance.
 * @return Decoder instance or NULL on allocation failure
 */
k_quirc_t *k_quirc_new(void);

/**
 * Destroy a QR-code decoder instance and free all resources.
 * @param q Decoder instance (can be NULL)
 */
void k_quirc_destroy(k_quirc_t *q);

/**
 * Resize the decoder for a specific image size.
 * Must be called before decoding.
 * @param q Decoder instance
 * @param w Image width
 * @param h Image height
 * @return 0 on success, -1 on allocation failure
 */
int k_quirc_resize(k_quirc_t *q, int w, int h);

/**
 * Begin decoding - get pointer to grayscale image buffer.
 * Fill this buffer with grayscale image data before calling k_quirc_end().
 * @param q Decoder instance
 * @param w Optional pointer to receive width
 * @param h Optional pointer to receive height
 * @return Pointer to grayscale buffer
 */
uint8_t *k_quirc_begin(k_quirc_t *q, int *w, int *h);

/**
 * End decoding - process the image and detect QR codes.
 * @param q Decoder instance
 * @param find_inverted If true, also try to find inverted (white on black) QR
 * codes
 */
void k_quirc_end(k_quirc_t *q, bool find_inverted);

/**
 * Get the number of QR codes detected.
 * @param q Decoder instance
 * @return Number of detected QR codes
 */
int k_quirc_count(const k_quirc_t *q);

/**
 * Decode a specific QR code and get its data.
 * @param q Decoder instance
 * @param index QR code index (0 to k_quirc_count()-1)
 * @param result Pointer to result structure to fill
 * @return K_QUIRC_SUCCESS on success, error code otherwise
 */
k_quirc_error_t k_quirc_decode(k_quirc_t *q, int index,
                               k_quirc_result_t *result);

/**
 * Get a human-readable error message.
 * @param err Error code
 * @return Error message string
 */
const char *k_quirc_strerror(k_quirc_error_t err);

/**
 * Convenience function: Decode QR codes from grayscale image.
 * This combines resize, begin, end, and decode into a single call.
 *
 * @param grayscale_data Grayscale image data (8-bit per pixel)
 * @param width Image width
 * @param height Image height
 * @param results Array to store results (caller allocated)
 * @param max_results Maximum number of results to return
 * @param find_inverted If true, also try inverted QR codes
 * @return Number of QR codes successfully decoded
 */
int k_quirc_decode_grayscale(const uint8_t *grayscale_data, int width,
                             int height, k_quirc_result_t *results,
                             int max_results, bool find_inverted);

#ifdef __cplusplus
}
#endif

#endif /* K_QUIRC_H */
