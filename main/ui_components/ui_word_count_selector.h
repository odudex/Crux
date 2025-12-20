/*
 * UI Word Count Selector
 * Reusable component for selecting mnemonic word count (12 or 24)
 */

#ifndef UI_WORD_COUNT_SELECTOR_H
#define UI_WORD_COUNT_SELECTOR_H

#include "ui_menu.h"
#include <lvgl.h>

// Callback type for word count selection
typedef void (*word_count_callback_t)(int word_count);

/**
 * @brief Create and show a word count selector menu
 *
 * The selector is displayed immediately and auto-destroys after any action
 * (selection or back). The on_select callback receives 12 or 24.
 *
 * @param parent Parent LVGL object
 * @param back_cb Callback for back button (NULL for no back button)
 * @param on_select Callback when word count is selected (receives 12 or 24)
 */
void ui_word_count_selector_create(lv_obj_t *parent, ui_menu_callback_t back_cb,
                                   word_count_callback_t on_select);

#endif // UI_WORD_COUNT_SELECTOR_H
