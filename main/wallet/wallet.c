#include "wallet.h"
#include "../key/key.h"
#include <esp_log.h>
#include <string.h>
#include <wally_address.h>
#include <wally_bip32.h>
#include <wally_core.h>
#include <wally_crypto.h>
#include <wally_script.h>

static bool wallet_initialized = false;
static wallet_type_t wallet_type = WALLET_TYPE_NATIVE_SEGWIT;
static wallet_network_t wallet_network = WALLET_NETWORK_MAINNET;
static struct ext_key *account_key = NULL;

static const char *BIP84_MAINNET_PATH = "m/84'/0'/0'";
static const char *BIP84_TESTNET_PATH = "m/84'/1'/0'";

bool wallet_init(wallet_network_t network) {
  if (wallet_initialized) {
    return true;
  }

  if (!key_is_loaded()) {
    return false;
  }

  wallet_network = network;
  const char *account_path = (network == WALLET_NETWORK_MAINNET) ? BIP84_MAINNET_PATH : BIP84_TESTNET_PATH;

  if (!key_get_derived_key(account_path, &account_key)) {
    return false;
  }

  wallet_initialized = true;
  wallet_type = WALLET_TYPE_NATIVE_SEGWIT;

  return true;
}

bool wallet_is_initialized(void) { return wallet_initialized; }

wallet_type_t wallet_get_type(void) { return wallet_type; }

wallet_network_t wallet_get_network(void) { return wallet_network; }

bool wallet_get_account_xpub(char **xpub_out) {
  if (!wallet_initialized || !account_key || !xpub_out) {
    return false;
  }

  int ret = bip32_key_to_base58(account_key, BIP32_FLAG_KEY_PUBLIC, xpub_out);
  return (ret == WALLY_OK);
}

// chain: 0 = receive, 1 = change
static bool derive_address(uint32_t chain, uint32_t index, char **address_out) {
  if (!wallet_initialized || !account_key || chain > 1) {
    return false;
  }

  uint32_t chain_path[1] = {chain};
  struct ext_key *chain_key = NULL;
  int ret = bip32_key_from_parent_path_alloc(account_key, chain_path, 1,
                                             BIP32_FLAG_KEY_PRIVATE, &chain_key);
  if (ret != WALLY_OK) {
    return false;
  }

  uint32_t addr_path[1] = {index};
  struct ext_key *addr_key = NULL;
  ret = bip32_key_from_parent_path_alloc(chain_key, addr_path, 1,
                                         BIP32_FLAG_KEY_PUBLIC, &addr_key);
  bip32_key_free(chain_key);

  if (ret != WALLY_OK) {
    return false;
  }

  unsigned char script[WALLY_WITNESSSCRIPT_MAX_LEN];
  size_t script_len;

  ret = wally_witness_program_from_bytes(addr_key->pub_key, EC_PUBLIC_KEY_LEN,
                                         WALLY_SCRIPT_HASH160, script,
                                         sizeof(script), &script_len);
  bip32_key_free(addr_key);

  if (ret != WALLY_OK) {
    return false;
  }

  const char *hrp = (wallet_network == WALLET_NETWORK_MAINNET) ? "bc" : "tb";
  ret = wally_addr_segwit_from_bytes(script, script_len, hrp, 0, address_out);
  return (ret == WALLY_OK);
}

bool wallet_get_receive_address(uint32_t index, char **address_out) {
  if (!address_out) {
    return false;
  }
  return derive_address(0, index, address_out);
}

bool wallet_get_change_address(uint32_t index, char **address_out) {
  if (!address_out) {
    return false;
  }
  return derive_address(1, index, address_out);
}

void wallet_cleanup(void) {
  if (account_key) {
    bip32_key_free(account_key);
    account_key = NULL;
  }
  wallet_initialized = false;
}
