#ifndef PUBLIC_KEY_H
#define PUBLIC_KEY_H

#include <lvgl.h>

/**
 * Create the public key display page
 * @param parent Parent LVGL object
 * @param return_cb Callback function to call when returning to home
 */
void public_key_page_create(lv_obj_t *parent, void (*return_cb)(void));

/**
 * Show the public key page
 */
void public_key_page_show(void);

/**
 * Hide the public key page
 */
void public_key_page_hide(void);

/**
 * Destroy the public key page and free resources
 */
void public_key_page_destroy(void);

#endif // PUBLIC_KEY_H
