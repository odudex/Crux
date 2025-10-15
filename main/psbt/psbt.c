#include "psbt.h"
#include "../key/key.h"
#include <esp_log.h>
#include <string.h>
#include <wally_address.h>
#include <wally_bip32.h>
#include <wally_core.h>
#include <wally_psbt_members.h>
#include <wally_script.h>
#include <wally_transaction.h>

uint64_t psbt_get_input_value(const struct wally_psbt *psbt, size_t index) {
  struct wally_tx_output *utxo = NULL;
  uint64_t value = 0;

  if (wally_psbt_get_input_best_utxo_alloc(psbt, index, &utxo) == WALLY_OK &&
      utxo) {
    value = utxo->satoshi;
    wally_tx_output_free(utxo);
  }

  return value;
}

static bool check_keypath_network(const unsigned char *keypath,
                                  size_t keypath_len, bool *is_testnet) {
  if (keypath_len < 12) {
    return false;
  }

  uint32_t coin_type;
  memcpy(&coin_type, keypath + 8, sizeof(uint32_t));
  uint32_t coin_value = coin_type & 0x7FFFFFFF;

  if (coin_value == 0 || coin_value == 1) {
    *is_testnet = (coin_value == 1);
    return true;
  }

  return false;
}

bool psbt_detect_network(const struct wally_psbt *psbt) {
  if (!psbt) {
    return false;
  }

  unsigned char keypath[100];
  size_t keypath_len, keypaths_size;
  bool is_testnet;

  // Check outputs first
  size_t num_outputs = 0;
  wally_psbt_get_num_outputs(psbt, &num_outputs);

  for (size_t i = 0; i < num_outputs; i++) {
    if (wally_psbt_get_output_keypaths_size(psbt, i, &keypaths_size) ==
            WALLY_OK &&
        keypaths_size > 0 &&
        wally_psbt_get_output_keypath(psbt, i, 0, keypath, sizeof(keypath),
                                      &keypath_len) == WALLY_OK &&
        check_keypath_network(keypath, keypath_len, &is_testnet)) {
      return is_testnet;
    }
  }

  // Check inputs as fallback
  size_t num_inputs = 0;
  wally_psbt_get_num_inputs(psbt, &num_inputs);

  for (size_t i = 0; i < num_inputs; i++) {
    if (wally_psbt_get_input_keypaths_size(psbt, i, &keypaths_size) ==
            WALLY_OK &&
        keypaths_size > 0 &&
        wally_psbt_get_input_keypath(psbt, i, 0, keypath, sizeof(keypath),
                                     &keypath_len) == WALLY_OK &&
        check_keypath_network(keypath, keypath_len, &is_testnet)) {
      return is_testnet;
    }
  }

  return false; // Default to mainnet
}

char *psbt_scriptpubkey_to_address(const unsigned char *script,
                                   size_t script_len, bool is_testnet) {
  if (!script || script_len == 0) {
    return NULL;
  }

  size_t script_type = 0;
  if (wally_scriptpubkey_get_type(script, script_len, &script_type) !=
      WALLY_OK) {
    return NULL;
  }

  char *address = NULL;
  const char *hrp = is_testnet ? "tb" : "bc";
  uint32_t network = is_testnet ? WALLY_NETWORK_BITCOIN_TESTNET
                                : WALLY_NETWORK_BITCOIN_MAINNET;

  if (script_type == WALLY_SCRIPT_TYPE_P2WPKH ||
      script_type == WALLY_SCRIPT_TYPE_P2WSH ||
      script_type == WALLY_SCRIPT_TYPE_P2TR) {
    if (wally_addr_segwit_from_bytes(script, script_len, hrp, 0, &address) !=
        WALLY_OK) {
      return NULL;
    }
  } else if (script_type == WALLY_SCRIPT_TYPE_P2PKH ||
             script_type == WALLY_SCRIPT_TYPE_P2SH) {
    if (wally_scriptpubkey_to_address(script, script_len, network, &address) !=
        WALLY_OK) {
      return NULL;
    }
  } else if (script_type == WALLY_SCRIPT_TYPE_OP_RETURN) {
    address = strdup("OP_RETURN");
  }

  return address;
}

bool psbt_get_output_derivation(const struct wally_psbt *psbt,
                                size_t output_index, bool is_testnet,
                                bool *is_change, uint32_t *address_index) {
  if (!psbt || !is_change || !address_index) {
    return false;
  }

  size_t keypaths_size = 0;
  if (wally_psbt_get_output_keypaths_size(psbt, output_index, &keypaths_size) !=
          WALLY_OK ||
      keypaths_size == 0) {
    return false;
  }

  unsigned char our_fingerprint[BIP32_KEY_FINGERPRINT_LEN];
  if (!key_get_fingerprint(our_fingerprint)) {
    return false;
  }

  for (size_t i = 0; i < keypaths_size; i++) {
    unsigned char keypath[100];
    size_t keypath_len = 0;

    if (wally_psbt_get_output_keypath(psbt, output_index, i, keypath,
                                      sizeof(keypath), &keypath_len) !=
            WALLY_OK ||
        keypath_len < 24) {
      continue;
    }

    if (memcmp(keypath, our_fingerprint, BIP32_KEY_FINGERPRINT_LEN) != 0) {
      continue;
    }

    uint32_t purpose, coin_type, account, change_val, index_val;
    memcpy(&purpose, keypath + 4, sizeof(uint32_t));
    memcpy(&coin_type, keypath + 8, sizeof(uint32_t));
    memcpy(&account, keypath + 12, sizeof(uint32_t));
    memcpy(&change_val, keypath + 16, sizeof(uint32_t));
    memcpy(&index_val, keypath + 20, sizeof(uint32_t));

    uint32_t expected_coin = is_testnet ? (0x80000000 | 1) : (0x80000000 | 0);

    if (purpose == (0x80000000 | 84) && coin_type == expected_coin &&
        account == 0x80000000 && !(change_val & 0x80000000) &&
        !(index_val & 0x80000000)) {
      *is_change = (change_val == 1);
      *address_index = index_val;
      return true;
    }
  }

  return false;
}
