/*
 * Login Page Header
 * Sample test case for UI Menu System
 */

#ifndef LOGIN_H
#define LOGIN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the login page with menu
 * 
 * @param parent Parent LVGL object where the login page will be created
 */
void login_page_create(lv_obj_t *parent);

/**
 * @brief Show the login page
 */
void login_page_show(void);

/**
 * @brief Hide the login page
 */
void login_page_hide(void);

/**
 * @brief Destroy the login page and free resources
 */
void login_page_destroy(void);

/**
 * @brief Navigate to the next menu item
 * 
 * @return true on success, false on failure
 */
bool login_page_navigate_next(void);

/**
 * @brief Navigate to the previous menu item
 * 
 * @return true on success, false on failure
 */
bool login_page_navigate_prev(void);

/**
 * @brief Execute the currently selected menu item
 * 
 * @return true on success, false on failure
 */
bool login_page_execute_selected(void);

/**
 * @brief Get the currently selected menu item index
 * 
 * @return int Selected item index, -1 on error
 */
int login_page_get_selected(void);

#ifdef __cplusplus
}
#endif

#endif // LOGIN_H
