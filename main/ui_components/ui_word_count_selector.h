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

// Word count selector structure
typedef struct {
  ui_menu_t *menu;
  word_count_callback_t on_select;
} ui_word_count_selector_t;

/**
 * @brief Create and show a word count selector menu
 *
 * The selector is displayed immediately. After selection, the on_select
 * callback is called with 12 or 24. The caller should destroy the selector
 * in their callback or cleanup routine.
 *
 * @param parent Parent LVGL object
 * @param back_cb Callback for back button (NULL for no back button)
 * @param on_select Callback when word count is selected (receives 12 or 24)
 * @return ui_word_count_selector_t* Pointer to selector, NULL on failure
 */
ui_word_count_selector_t *ui_word_count_selector_create(
    lv_obj_t *parent, ui_menu_callback_t back_cb, word_count_callback_t on_select);

/**
 * @brief Destroy the word count selector and free resources
 *
 * @param selector Pointer to the selector
 */
void ui_word_count_selector_destroy(ui_word_count_selector_t *selector);

#endif // UI_WORD_COUNT_SELECTOR_H
