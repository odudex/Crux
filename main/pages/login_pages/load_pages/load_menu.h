/*
 * Load Menu Page Header
 * Intermediary menu for loading mnemonic options
 */

#ifndef LOAD_MENU_H
#define LOAD_MENU_H

#include <lvgl.h>

/**
 * @brief Create the load menu page
 *
 * @param parent Parent LVGL object where the load menu page will be created
 * @param return_cb Callback function to call when returning to previous page
 */
void load_menu_page_create(lv_obj_t *parent, void (*return_cb)(void));

/**
 * @brief Show the load menu page
 */
void load_menu_page_show(void);

/**
 * @brief Hide the load menu page
 */
void load_menu_page_hide(void);

/**
 * @brief Destroy the load menu page and free resources
 */
void load_menu_page_destroy(void);

/**
 * @brief Navigate to next menu item
 * 
 * @return true if navigation was successful, false otherwise
 */
bool load_menu_page_navigate_next(void);

/**
 * @brief Navigate to previous menu item
 * 
 * @return true if navigation was successful, false otherwise
 */
bool load_menu_page_navigate_prev(void);

/**
 * @brief Execute the currently selected menu item
 * 
 * @return true if execution was successful, false otherwise
 */
bool load_menu_page_execute_selected(void);

/**
 * @brief Get the index of the currently selected menu item
 * 
 * @return Index of selected item, or -1 if no item is selected
 */
int load_menu_page_get_selected(void);

#endif // LOAD_MENU_H