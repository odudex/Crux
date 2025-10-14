/*
 * Mnemonic Loading Page Header
 * Displays a loading screen while processing mnemonic data
 */

#ifndef MNEMONIC_LOADING_H
#define MNEMONIC_LOADING_H

#include <lvgl.h>

/**
 * @brief Create the mnemonic loading page
 *
 * @param parent Parent LVGL object where the loading page will be created
 * @param return_cb Callback function to call when returning to previous page
 * @param success_cb Callback function to call when key is successfully loaded
 * @param content The QR content to display (will be copied)
 */
void mnemonic_loading_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                  void (*success_cb)(void),
                                  const char *content);

/**
 * @brief Show the mnemonic loading page
 */
void mnemonic_loading_page_show(void);

/**
 * @brief Hide the mnemonic loading page
 */
void mnemonic_loading_page_hide(void);

/**
 * @brief Destroy the mnemonic loading page and free resources
 */
void mnemonic_loading_page_destroy(void);

#endif // MNEMONIC_LOADING_H