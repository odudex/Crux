#ifndef MNEMONIC_LOADING_H
#define MNEMONIC_LOADING_H

#include <lvgl.h>
#include <stddef.h>

/**
 * @brief Create the mnemonic loading page
 *
 * Validates and processes mnemonic content from QR codes.
 * Supports multiple formats (auto-detected):
 * - Plaintext: Space-separated BIP39 words
 * - Compact SeedQR: 16/32 bytes binary entropy
 * - SeedQR: 48/96 digit numeric string (4 digits per word index)
 *
 * @param parent Parent LVGL object
 * @param return_cb Callback when user wants to go back
 * @param success_cb Callback when mnemonic is successfully loaded
 * @param content QR content (plaintext, binary, or numeric)
 * @param content_len Length of content (required for binary detection)
 */
void mnemonic_loading_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                  void (*success_cb)(void), const char *content,
                                  size_t content_len);
void mnemonic_loading_page_show(void);
void mnemonic_loading_page_hide(void);
void mnemonic_loading_page_destroy(void);

#endif // MNEMONIC_LOADING_H