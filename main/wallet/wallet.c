/*
 * Wallet Management
 * Handles wallet output descriptors and address generation
 * Currently supports single-signature Native SegWit (P2WPKH/BIP84)
 */

#include "wallet.h"
#include "../key/key.h"
#include <esp_log.h>
#include <string.h>
#include <wally_address.h>
#include <wally_bip32.h>
#include <wally_core.h>
#include <wally_crypto.h>
#include <wally_script.h>

static const char *TAG = "WALLET";

// Wallet state
static bool wallet_initialized = false;
static wallet_type_t wallet_type = WALLET_TYPE_NATIVE_SEGWIT;
static struct ext_key *account_key =
    NULL; // Account level key (e.g., m/84'/0'/0')

// BIP84 Native SegWit path for mainnet
static const char *BIP84_ACCOUNT_PATH = "m/84'/0'/0'";

bool wallet_init(void) {
  if (wallet_initialized) {
    ESP_LOGW(TAG, "Wallet already initialized");
    return true;
  }

  if (!key_is_loaded()) {
    ESP_LOGE(TAG, "Cannot initialize wallet: no key loaded");
    return false;
  }

  ESP_LOGI(TAG, "Initializing wallet (Native SegWit - BIP84)");

  // Derive the account key at m/84'/0'/0'
  if (!key_get_derived_key(BIP84_ACCOUNT_PATH, &account_key)) {
    ESP_LOGE(TAG, "Failed to derive account key");
    return false;
  }

  wallet_initialized = true;
  wallet_type = WALLET_TYPE_NATIVE_SEGWIT;

  ESP_LOGI(TAG, "Wallet initialized successfully");
  return true;
}

bool wallet_is_initialized(void) { return wallet_initialized; }

wallet_type_t wallet_get_type(void) { return wallet_type; }

bool wallet_get_account_xpub(char **xpub_out) {
  if (!wallet_initialized || !account_key) {
    ESP_LOGE(TAG, "Wallet not initialized");
    return false;
  }

  if (!xpub_out) {
    ESP_LOGE(TAG, "Invalid output parameter");
    return false;
  }

  // Export account key as xpub (public only)
  int ret = bip32_key_to_base58(account_key, BIP32_FLAG_KEY_PUBLIC, xpub_out);
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to export account xpub: %d", ret);
    return false;
  }

  return true;
}

// Helper function to derive an address from account key
// chain: 0 for receive, 1 for change
// index: address index
static bool derive_address(uint32_t chain, uint32_t index, char **address_out) {
  if (!wallet_initialized || !account_key) {
    ESP_LOGE(TAG, "Wallet not initialized");
    return false;
  }

  if (chain > 1) {
    ESP_LOGE(TAG, "Invalid chain: %u (must be 0 or 1)", chain);
    return false;
  }

  // Derive chain key (0 or 1) from account key
  uint32_t chain_path[1] = {chain};
  struct ext_key *chain_key = NULL;
  int ret = bip32_key_from_parent_path_alloc(
      account_key, chain_path, 1, BIP32_FLAG_KEY_PRIVATE, &chain_key);

  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to derive chain key: %d", ret);
    return false;
  }

  // Derive address key from chain key
  uint32_t addr_path[1] = {index};
  struct ext_key *addr_key = NULL;
  ret = bip32_key_from_parent_path_alloc(chain_key, addr_path, 1,
                                         BIP32_FLAG_KEY_PUBLIC, &addr_key);

  // Free chain key
  bip32_key_free(chain_key);

  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to derive address key: %d", ret);
    return false;
  }

  // Generate P2WPKH scriptPubKey from public key
  // For P2WPKH: witness version 0 + HASH160(pubkey)
  unsigned char script[WALLY_WITNESSSCRIPT_MAX_LEN];
  size_t script_len;

  ret = wally_witness_program_from_bytes(addr_key->pub_key, EC_PUBLIC_KEY_LEN,
                                         WALLY_SCRIPT_HASH160, script,
                                         sizeof(script), &script_len);

  // Free address key
  bip32_key_free(addr_key);

  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to create scriptPubKey: %d", ret);
    return false;
  }

  // Convert scriptPubKey to bech32 address
  ret = wally_addr_segwit_from_bytes(script, script_len, "bc", 0, address_out);
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to create address: %d", ret);
    return false;
  }

  return true;
}

bool wallet_get_receive_address(uint32_t index, char **address_out) {
  if (!address_out) {
    ESP_LOGE(TAG, "Invalid output parameter");
    return false;
  }

  // Derive receive address (chain 0)
  if (!derive_address(0, index, address_out)) {
    ESP_LOGE(TAG, "Failed to derive receive address at index %u", index);
    return false;
  }

  ESP_LOGI(TAG, "Derived receive address at index %u: %s", index, *address_out);
  return true;
}

bool wallet_get_change_address(uint32_t index, char **address_out) {
  if (!address_out) {
    ESP_LOGE(TAG, "Invalid output parameter");
    return false;
  }

  // Derive change address (chain 1)
  if (!derive_address(1, index, address_out)) {
    ESP_LOGE(TAG, "Failed to derive change address at index %u", index);
    return false;
  }

  ESP_LOGI(TAG, "Derived change address at index %u: %s", index, *address_out);
  return true;
}

void wallet_cleanup(void) {
  if (account_key) {
    bip32_key_free(account_key);
    account_key = NULL;
  }

  wallet_initialized = false;
  ESP_LOGI(TAG, "Wallet cleaned up");
}
