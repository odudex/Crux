/*
 * About Page Header
 * Displays information about the application
 */

#ifndef ABOUT_H
#define ABOUT_H

#include <lvgl.h>

/**
 * @brief Create the about page
 *
 * @param parent Parent LVGL object where the about page will be created
 * @param return_cb Callback function to call when returning to previous page
 */
void about_page_create(lv_obj_t *parent, void (*return_cb)(void));

/**
 * @brief Show the about page
 */
void about_page_show(void);

/**
 * @brief Hide the about page
 */
void about_page_hide(void);

/**
 * @brief Destroy the about page and free resources
 */
void about_page_destroy(void);

#endif // ABOUT_H
