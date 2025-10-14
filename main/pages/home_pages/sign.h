#ifndef SIGN_H
#define SIGN_H

#include <lvgl.h>

/**
 * Create the PSBT signing page
 * @param parent Parent LVGL object
 * @param return_cb Callback function to call when returning to home
 */
void sign_page_create(lv_obj_t *parent, void (*return_cb)(void));

/**
 * Show the sign page
 */
void sign_page_show(void);

/**
 * Hide the sign page
 */
void sign_page_hide(void);

/**
 * Destroy the sign page and free resources
 */
void sign_page_destroy(void);

#endif // SIGN_H
