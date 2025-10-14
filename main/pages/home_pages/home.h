#ifndef HOME_H
#define HOME_H

#include <lvgl.h>

/**
 * Create the home page
 * @param parent Parent LVGL object
 */
void home_page_create(lv_obj_t *parent);

/**
 * Show the home page
 */
void home_page_show(void);

/**
 * Hide the home page
 */
void home_page_hide(void);

/**
 * Destroy the home page and free resources
 */
void home_page_destroy(void);

#endif // HOME_H
