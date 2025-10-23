/*
 * URTypes - Uniform Resources Type Handlers
 * Copyright (c) 2021-2025 Krux contributors
 */

#include "urtypes.h"
#include "../../components/cUR/src/cbor_lite.h"
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include <wally_core.h>

static const char *TAG = "URTYPES";

bool urtypes_ur_to_psbt(const char *ur_type, const uint8_t *cbor_data,
                        size_t cbor_len, uint8_t **psbt_out,
                        size_t *psbt_len_out) {
  if (!ur_type || !cbor_data || !psbt_out || !psbt_len_out) {
    return false;
  }

  if (strcmp(ur_type, UR_TYPE_CRYPTO_PSBT) != 0) {
    return false;
  }

  cbor_decoder_t decoder;
  cbor_decoder_init(&decoder, cbor_data, cbor_len);

  const uint8_t *psbt_bytes = NULL;
  size_t psbt_len = 0;

  if (!cbor_decode_bytes(&decoder, &psbt_bytes, &psbt_len)) {
    return false;
  }

  *psbt_out = (uint8_t *)malloc(psbt_len);
  if (!*psbt_out) {
    return false;
  }

  memcpy(*psbt_out, psbt_bytes, psbt_len);
  *psbt_len_out = psbt_len;
  return true;
}

bool urtypes_psbt_to_ur(const uint8_t *psbt_bytes, size_t psbt_len,
                        uint8_t **cbor_out, size_t *cbor_len_out) {
  if (!psbt_bytes || !cbor_out || !cbor_len_out) {
    return false;
  }

  cbor_encoder_t encoder;
  if (!cbor_encoder_init(&encoder, psbt_len + 10)) {
    return false;
  }

  if (!cbor_encode_bytes(&encoder, psbt_bytes, psbt_len)) {
    cbor_encoder_free(&encoder);
    return false;
  }

  *cbor_out = cbor_encoder_get_buffer(&encoder, cbor_len_out);
  if (!*cbor_out) {
    ESP_LOGE(TAG, "Failed to get CBOR buffer");
    return false;
  }
  return true;
}

bool urtypes_psbt_base64_to_ur(const char *psbt_base64, uint8_t **cbor_out,
                               size_t *cbor_len_out) {
  if (!psbt_base64 || !cbor_out || !cbor_len_out) {
    return false;
  }

  size_t input_len = strlen(psbt_base64);
  size_t max_decoded_len = (input_len * 3) / 4 + 1;

  uint8_t *psbt_bytes = (uint8_t *)malloc(max_decoded_len);
  if (!psbt_bytes) {
    return false;
  }

  size_t written = 0;
  int ret = wally_base64_to_bytes(psbt_base64, 0, psbt_bytes, max_decoded_len, &written);
  if (ret != WALLY_OK) {
    free(psbt_bytes);
    return false;
  }

  bool result = urtypes_psbt_to_ur(psbt_bytes, written, cbor_out, cbor_len_out);

  free(psbt_bytes);
  return result;
}

bool urtypes_ur_to_psbt_base64(const char *ur_type, const uint8_t *cbor_data,
                               size_t cbor_len, char **psbt_base64_out) {
  if (!ur_type || !cbor_data || !psbt_base64_out) {
    return false;
  }

  uint8_t *psbt_bytes = NULL;
  size_t psbt_len = 0;

  if (!urtypes_ur_to_psbt(ur_type, cbor_data, cbor_len, &psbt_bytes, &psbt_len)) {
    return false;
  }

  char *base64_str = NULL;
  int ret = wally_base64_from_bytes(psbt_bytes, psbt_len, 0, &base64_str);
  free(psbt_bytes);

  if (ret != WALLY_OK || !base64_str) {
    return false;
  }

  *psbt_base64_out = base64_str;
  return true;
}
