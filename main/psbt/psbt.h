#ifndef PSBT_H
#define PSBT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wally_psbt.h>

/**
 * Get the value of an input from a PSBT
 * @param psbt The PSBT to get the input value from
 * @param index The index of the input
 * @return The value of the input in satoshis, or 0 if not found
 */
uint64_t psbt_get_input_value(const struct wally_psbt *psbt, size_t index);

/**
 * Detect network (mainnet/testnet) from PSBT BIP32 derivation paths
 * @param psbt The PSBT to analyze
 * @return true if testnet, false if mainnet
 */
bool psbt_detect_network(const struct wally_psbt *psbt);

/**
 * Convert a scriptPubKey to a Bitcoin address
 * @param script The scriptPubKey bytes
 * @param script_len The length of the scriptPubKey
 * @param is_testnet Whether to use testnet address format
 * @return The address string (must be freed with wally_free_string or free
 * depending on type), or NULL on error
 */
char *psbt_scriptpubkey_to_address(const unsigned char *script,
                                   size_t script_len, bool is_testnet);

#endif // PSBT_H
