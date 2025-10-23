/*
 * URTypes - Uniform Resources Type Handlers
 * Copyright (c) 2021-2025 Krux contributors
 */

#ifndef URTYPES_H
#define URTYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// crypto-psbt registry type
#define UR_TYPE_CRYPTO_PSBT "crypto-psbt"
#define UR_REGISTRY_CRYPTO_PSBT 310

/**
 * @brief Convert UR CBOR data to PSBT raw bytes
 *
 * Takes the CBOR payload from a UR decoder result and extracts
 * the raw PSBT bytes. For crypto-psbt, this is a simple CBOR
 * byte string extraction.
 *
 * @param ur_type UR type string (e.g., "crypto-psbt")
 * @param cbor_data CBOR-encoded data from UR decoder
 * @param cbor_len Length of CBOR data
 * @param psbt_out Output buffer for PSBT bytes (allocated by function, caller
 * must free)
 * @param psbt_len_out Output length of PSBT bytes
 * @return true on success, false on failure
 */
bool urtypes_ur_to_psbt(const char *ur_type, const uint8_t *cbor_data,
                        size_t cbor_len, uint8_t **psbt_out,
                        size_t *psbt_len_out);

/**
 * @brief Convert PSBT raw bytes to UR CBOR format
 *
 * Takes raw PSBT bytes and encodes them as CBOR for use with
 * UR encoder. For crypto-psbt, this wraps the bytes as a CBOR
 * byte string.
 *
 * @param psbt_bytes Raw PSBT bytes
 * @param psbt_len Length of PSBT bytes
 * @param cbor_out Output buffer for CBOR data (allocated by function, caller
 * must free)
 * @param cbor_len_out Output length of CBOR data
 * @return true on success, false on failure
 */
bool urtypes_psbt_to_ur(const uint8_t *psbt_bytes, size_t psbt_len,
                        uint8_t **cbor_out, size_t *cbor_len_out);

/**
 * @brief Convert base64 PSBT string to UR CBOR format
 *
 * Convenience function that decodes base64 PSBT and converts to UR CBOR.
 *
 * @param psbt_base64 Base64-encoded PSBT string
 * @param cbor_out Output buffer for CBOR data (allocated by function, caller
 * must free)
 * @param cbor_len_out Output length of CBOR data
 * @return true on success, false on failure
 */
bool urtypes_psbt_base64_to_ur(const char *psbt_base64, uint8_t **cbor_out,
                               size_t *cbor_len_out);

/**
 * @brief Convert UR CBOR data to base64 PSBT string
 *
 * Convenience function that extracts PSBT bytes from UR and encodes to base64.
 *
 * @param ur_type UR type string (e.g., "crypto-psbt")
 * @param cbor_data CBOR-encoded data from UR decoder
 * @param cbor_len Length of CBOR data
 * @param psbt_base64_out Output base64 string (allocated by function, caller
 * must free)
 * @return true on success, false on failure
 */
bool urtypes_ur_to_psbt_base64(const char *ur_type, const uint8_t *cbor_data,
                               size_t cbor_len, char **psbt_base64_out);

#endif // URTYPES_H
