#ifndef ADDRESSES_H
#define ADDRESSES_H

#include <lvgl.h>

/**
 * Create the addresses page
 * @param parent Parent LVGL object
 * @param return_cb Callback function to call when returning to home
 */
void addresses_page_create(lv_obj_t *parent, void (*return_cb)(void));

/**
 * Show the addresses page
 */
void addresses_page_show(void);

/**
 * Hide the addresses page
 */
void addresses_page_hide(void);

/**
 * Destroy the addresses page and free resources
 */
void addresses_page_destroy(void);

#endif // ADDRESSES_H
