#ifndef KEY_H
#define KEY_H

#include <stdbool.h>
#include <stddef.h>
#include <wally_bip32.h>

bool key_init(void);
bool key_is_loaded(void);
bool key_load_from_mnemonic(const char *mnemonic, const char *passphrase,
                            bool is_testnet);
void key_unload(void);
bool key_get_fingerprint(unsigned char *fingerprint_out);
bool key_get_fingerprint_hex(char *hex_out);
bool key_get_xpub(const char *path, char **xpub_out);
bool key_get_master_xpub(char **xpub_out);
bool key_get_mnemonic(char **mnemonic_out);
bool key_get_mnemonic_words(char ***words_out, size_t *word_count_out);
bool key_get_derived_key(const char *path, struct ext_key **key_out);
void key_cleanup(void);

#endif // KEY_H
