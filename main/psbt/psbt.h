#ifndef PSBT_H
#define PSBT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wally_psbt.h>

// Get input value in satoshis
uint64_t psbt_get_input_value(const struct wally_psbt *psbt, size_t index);

// Detect network from derivation paths (returns true if testnet)
bool psbt_detect_network(const struct wally_psbt *psbt);

// Convert scriptPubKey to address string (caller must free)
char *psbt_scriptpubkey_to_address(const unsigned char *script,
                                   size_t script_len, bool is_testnet);

// Verify output belongs to our wallet and extract derivation info
bool psbt_get_output_derivation(const struct wally_psbt *psbt,
                                size_t output_index, bool is_testnet,
                                bool *is_change, uint32_t *address_index);

#endif // PSBT_H
