#ifndef WALLET_H
#define WALLET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Wallet types supported
 */
typedef enum {
  WALLET_TYPE_NATIVE_SEGWIT = 0, // P2WPKH (BIP84, m/84'/0'/0')
} wallet_type_t;

/**
 * Initialize the wallet subsystem
 * Must be called after key is loaded
 * Creates a Native SegWit (P2WPKH) wallet output descriptor
 * @return true on success, false on failure
 */
bool wallet_init(void);

/**
 * Check if wallet is initialized
 * @return true if wallet is initialized, false otherwise
 */
bool wallet_is_initialized(void);

/**
 * Get the wallet type
 * @return the current wallet type
 */
wallet_type_t wallet_get_type(void);

/**
 * Get the account xpub (e.g., for BIP84 this is m/84'/0'/0')
 * @param xpub_out Output buffer for xpub string (allocated by libwally, must be
 * freed with wally_free_string)
 * @return true on success, false on failure
 */
bool wallet_get_account_xpub(char **xpub_out);

/**
 * Get a receive address at a specific index
 * Derives m/84'/0'/0'/0/index for Native SegWit
 * @param index The address index (0, 1, 2, ...)
 * @param address_out Output buffer for address string (allocated by libwally,
 * must be freed with wally_free_string)
 * @return true on success, false on failure
 */
bool wallet_get_receive_address(uint32_t index, char **address_out);

/**
 * Get a change address at a specific index
 * Derives m/84'/0'/0'/1/index for Native SegWit
 * @param index The address index (0, 1, 2, ...)
 * @param address_out Output buffer for address string (allocated by libwally,
 * must be freed with wally_free_string)
 * @return true on success, false on failure
 */
bool wallet_get_change_address(uint32_t index, char **address_out);

/**
 * Cleanup wallet subsystem
 */
void wallet_cleanup(void);

#endif // WALLET_H
