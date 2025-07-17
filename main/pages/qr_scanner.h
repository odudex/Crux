/*
 * QR Scanner Page Header
 * Displays a 480x480 frame buffer that changes colors every second
 */

#ifndef QR_SCANNER_H
#define QR_SCANNER_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the QR scanner page
 * 
 * @param parent Parent LVGL object where the QR scanner page will be created
 * @param return_cb Callback function to call when returning to previous page
 */
void qr_scanner_page_create(lv_obj_t *parent, void (*return_cb)(void));

/**
 * @brief Show the QR scanner page
 */
void qr_scanner_page_show(void);

/**
 * @brief Hide the QR scanner page
 */
void qr_scanner_page_hide(void);

/**
 * @brief Destroy the QR scanner page and free resources
 */
void qr_scanner_page_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // QR_SCANNER_H

