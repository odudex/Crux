/*
 * QR Scanner Page Header
 * Displays a 480x480 frame buffer that changes colors every second
 */

#ifndef QR_SCANNER_H
#define QR_SCANNER_H

#include "../../components/video/video.h"
#include <lvgl.h>
#include <stdbool.h>

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

/**
 * @brief Get completed QR content if available
 *
 * @return Completed QR content string (caller must free), or NULL if no
 * completed content
 */
char *qr_scanner_get_completed_content(void);

/**
 * @brief Check if QR scanner is fully initialized and ready
 *
 * @return true if scanner is ready, false otherwise
 */
bool qr_scanner_is_ready(void);

#endif // QR_SCANNER_H
