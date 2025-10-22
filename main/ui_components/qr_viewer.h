#ifndef QR_VIEWER_H
#define QR_VIEWER_H

#include <lvgl.h>

/**
 * Create the QR viewer page
 * @param parent Parent LVGL object
 * @param qr_content Content to display as QR code
 * @param title Optional title to display (can be NULL)
 * @param return_cb Callback function to call when returning
 */
void qr_viewer_page_create(lv_obj_t *parent, const char *qr_content,
                           const char *title, void (*return_cb)(void));

/**
 * Show the QR viewer page
 */
void qr_viewer_page_show(void);

/**
 * Hide the QR viewer page
 */
void qr_viewer_page_hide(void);

/**
 * Destroy the QR viewer page and free resources
 */
void qr_viewer_page_destroy(void);

#endif // QR_VIEWER_H
