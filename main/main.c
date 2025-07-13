/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "pages/splash_screen.h"

static const char *TAG = "Krux";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Hello World LVGL demo");

    // Initialize display
    ESP_LOGI(TAG, "Initializing display...");
    
    lv_display_t *display = bsp_display_start();
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
    if (display == NULL) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return;
    }
    ESP_LOGI(TAG, "Display initialized successfully");
    
    esp_err_t ret = bsp_display_backlight_on();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to turn on backlight: %s", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "Backlight set to 100%%");

    // Give some time for display to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Lock display for LVGL operations
    ESP_LOGI(TAG, "Creating Hello World label...");
    bsp_display_lock(0);

    // Set a background color first
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0); // Black background
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    draw_krux_logo(screen);

    // Unlock display
    bsp_display_unlock();
}
