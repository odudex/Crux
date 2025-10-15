/*
 * PSBT Helpers
 * Helper functions for working with PSBTs (Partially Signed Bitcoin
 * Transactions) including network detection, script type handling, and address
 * conversion
 */

#include "psbt.h"
#include <esp_log.h>
#include <string.h>
#include <wally_address.h>
#include <wally_core.h>
#include <wally_psbt_members.h>
#include <wally_script.h>
#include <wally_transaction.h>

static const char *TAG = "PSBT";

// Get the value of an input from PSBT
uint64_t psbt_get_input_value(const struct wally_psbt *psbt, size_t index) {
  uint64_t value = 0;
  struct wally_tx_output *utxo = NULL;

  if (wally_psbt_get_input_best_utxo_alloc(psbt, index, &utxo) == WALLY_OK &&
      utxo) {
    value = utxo->satoshi;
    wally_tx_output_free(utxo);
  }

  return value;
}

// Detect network (mainnet/testnet) from PSBT BIP32 derivation paths
// Returns true if testnet, false if mainnet
bool psbt_detect_network(const struct wally_psbt *psbt) {
  if (!psbt) {
    return false;
  }

  size_t num_outputs = 0;
  wally_psbt_get_num_outputs(psbt, &num_outputs);

  // Check output keypaths for coin_type
  for (size_t i = 0; i < num_outputs; i++) {
    size_t keypaths_size = 0;
    if (wally_psbt_get_output_keypaths_size(psbt, i, &keypaths_size) ==
            WALLY_OK &&
        keypaths_size > 0) {

      unsigned char keypath_buf[100];
      size_t keypath_len = 0;

      if (wally_psbt_get_output_keypath(psbt, i, 0, keypath_buf,
                                        sizeof(keypath_buf),
                                        &keypath_len) == WALLY_OK &&
          keypath_len >= 12) {

        // BIP32 keypath format: [fingerprint(4)][purpose(4)][coin_type(4)]...
        // Extract coin_type at offset 8: 0'=mainnet, 1'=testnet
        uint32_t coin_type;
        memcpy(&coin_type, keypath_buf + 8, sizeof(uint32_t));
        uint32_t coin_value = coin_type & 0x7FFFFFFF;

        if (coin_value == 1) {
          ESP_LOGI(TAG, "Detected testnet from PSBT");
          return true;
        } else if (coin_value == 0) {
          ESP_LOGI(TAG, "Detected mainnet from PSBT");
          return false;
        }
      }
    }
  }

  // Check input keypaths as fallback
  size_t num_inputs = 0;
  wally_psbt_get_num_inputs(psbt, &num_inputs);

  for (size_t i = 0; i < num_inputs; i++) {
    size_t keypaths_size = 0;
    if (wally_psbt_get_input_keypaths_size(psbt, i, &keypaths_size) ==
            WALLY_OK &&
        keypaths_size > 0) {

      unsigned char keypath_buf[100];
      size_t keypath_len = 0;

      if (wally_psbt_get_input_keypath(psbt, i, 0, keypath_buf,
                                       sizeof(keypath_buf),
                                       &keypath_len) == WALLY_OK &&
          keypath_len >= 12) {

        uint32_t coin_type;
        memcpy(&coin_type, keypath_buf + 8, sizeof(uint32_t));
        uint32_t coin_value = coin_type & 0x7FFFFFFF;

        if (coin_value == 1) {
          ESP_LOGI(TAG, "Detected testnet from PSBT");
          return true;
        } else if (coin_value == 0) {
          ESP_LOGI(TAG, "Detected mainnet from PSBT");
          return false;
        }
      }
    }
  }

  // Default to mainnet if no keypaths found
  ESP_LOGW(TAG, "No network info in PSBT, defaulting to mainnet");
  return false;
}

// Convert scriptPubKey to Bitcoin address
char *psbt_scriptpubkey_to_address(const unsigned char *script,
                                   size_t script_len, bool is_testnet) {
  if (!script || script_len == 0) {
    return NULL;
  }

  char *address = NULL;
  size_t script_type = 0;
  int ret;

  if (wally_scriptpubkey_get_type(script, script_len, &script_type) !=
      WALLY_OK) {
    return NULL;
  }

  // Convert based on script type
  if (script_type == WALLY_SCRIPT_TYPE_P2WPKH ||
      script_type == WALLY_SCRIPT_TYPE_P2WSH ||
      script_type == WALLY_SCRIPT_TYPE_P2TR) {
    // SegWit address
    const char *addr_family = is_testnet ? "tb" : "bc";
    ret = wally_addr_segwit_from_bytes(script, script_len, addr_family, 0,
                                       &address);
    if (ret != WALLY_OK) {
      return NULL;
    }
  } else if (script_type == WALLY_SCRIPT_TYPE_P2PKH ||
             script_type == WALLY_SCRIPT_TYPE_P2SH) {
    // Legacy address
    uint32_t network = is_testnet ? WALLY_NETWORK_BITCOIN_TESTNET
                                  : WALLY_NETWORK_BITCOIN_MAINNET;
    ret = wally_scriptpubkey_to_address(script, script_len, network, &address);
    if (ret != WALLY_OK) {
      return NULL;
    }
  } else if (script_type == WALLY_SCRIPT_TYPE_OP_RETURN) {
    address = strdup("OP_RETURN");
  } else {
    return NULL;
  }

  return address;
}
