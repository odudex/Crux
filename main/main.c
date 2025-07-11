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

static const char *TAG = "hello_world";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Hello World LVGL demo");

    // Initialize display
    ESP_LOGI(TAG, "Initializing display...");
    
    lv_display_t *display = bsp_display_start();
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

    // Create a simple "Hello World" label with more visible styling
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Hello World!");
    
    // Try a larger, more visible font first
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0); // Pure white
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    
    // Add background to the label to make it more visible
    lv_obj_set_style_bg_color(label, lv_color_hex(0xFF0000), 0); // Red background
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(label, 10, 0);
    
    lv_obj_center(label);

    // Force a screen refresh
    lv_obj_invalidate(screen);

    // Unlock display
    bsp_display_unlock();
}
