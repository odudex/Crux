/*
 * Login Page Header
 * Sample test case for UI Menu System
 */

#ifndef LOGIN_H
#define LOGIN_H

#include "lvgl.h"

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

#endif // LOGIN_H
