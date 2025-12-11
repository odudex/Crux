/*
 * Manual Mnemonic Input Page Header
 * Allows users to enter BIP39 mnemonic words one by one
 */

#ifndef MANUAL_INPUT_H
#define MANUAL_INPUT_H

#include <lvgl.h>
#include <stdbool.h>

/**
 * @brief Create the manual input page
 *
 * @param parent Parent LVGL object where the page will be created
 * @param return_cb Callback function to call when returning to previous page
 * @param success_cb Callback function to call when mnemonic is successfully
 * loaded
 */
void manual_input_page_create(lv_obj_t *parent, void (*return_cb)(void),
                              void (*success_cb)(void));

/**
 * @brief Show the manual input page
 */
void manual_input_page_show(void);

/**
 * @brief Hide the manual input page
 */
void manual_input_page_hide(void);

/**
 * @brief Destroy the manual input page and free resources
 */
void manual_input_page_destroy(void);

/**
 * @brief Navigate to next item (letter or word)
 *
 * @return true if navigation was successful, false otherwise
 */
bool manual_input_page_navigate_next(void);

/**
 * @brief Navigate to previous item (letter or word)
 *
 * @return true if navigation was successful, false otherwise
 */
bool manual_input_page_navigate_prev(void);

/**
 * @brief Execute the currently selected action (select letter or word)
 *
 * @return true if execution was successful, false otherwise
 */
bool manual_input_page_execute_selected(void);

/**
 * @brief Get the index of the currently selected item
 *
 * @return Index of selected item, or -1 if no item is selected
 */
int manual_input_page_get_selected(void);

#endif // MANUAL_INPUT_H
