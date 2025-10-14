#ifndef KEY_H
#define KEY_H

#include <stdbool.h>
#include <stddef.h>
#include <wally_bip32.h>

/**
 * Initialize the key management system
 * @return true on success, false on failure
 */
bool key_init(void);

/**
 * Check if a key is currently loaded
 * @return true if key is loaded, false otherwise
 */
bool key_is_loaded(void);

/**
 * Load a key from a mnemonic phrase
 * @param mnemonic The BIP39 mnemonic phrase
 * @param passphrase Optional passphrase (can be NULL)
 * @return true on success, false on failure
 */
bool key_load_from_mnemonic(const char *mnemonic, const char *passphrase);

/**
 * Unload the currently loaded key and clear sensitive data
 */
void key_unload(void);

/**
 * Get the master key fingerprint
 * @param fingerprint_out Output buffer for fingerprint (must be at least
 * BIP32_KEY_FINGERPRINT_LEN bytes)
 * @return true on success, false on failure
 */
bool key_get_fingerprint(unsigned char *fingerprint_out);

/**
 * Get the fingerprint as a hex string
 * @param hex_out Output buffer for hex string (must be at least
 * BIP32_KEY_FINGERPRINT_LEN*2+1 bytes)
 * @return true on success, false on failure
 */
bool key_get_fingerprint_hex(char *hex_out);

/**
 * Get the extended public key (xpub) at a specific derivation path
 * @param path Derivation path (e.g., "m/84'/0'/0'")
 * @param xpub_out Output buffer for xpub string (allocated by libwally, must be
 * freed with wally_free_string)
 * @return true on success, false on failure
 */
bool key_get_xpub(const char *path, char **xpub_out);

/**
 * Get the master extended public key
 * @param xpub_out Output buffer for xpub string (allocated by libwally, must be
 * freed with wally_free_string)
 * @return true on success, false on failure
 */
bool key_get_master_xpub(char **xpub_out);

/**
 * Get the mnemonic phrase
 * @param mnemonic_out Output buffer (caller must free)
 * @return true on success, false on failure
 */
bool key_get_mnemonic(char **mnemonic_out);

/**
 * Get individual mnemonic words
 * @param words_out Array of word strings (caller must free each string and the
 * array)
 * @param word_count_out Number of words
 * @return true on success, false on failure
 */
bool key_get_mnemonic_words(char ***words_out, size_t *word_count_out);

/**
 * Cleanup key management system
 */
void key_cleanup(void);

#endif // KEY_H
