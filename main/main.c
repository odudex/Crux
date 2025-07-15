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
#include "pages/login_pages/login.h"
#include "ui_components/dark_theme.h"

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

    // Initialize dark theme
    dark_theme_init();
    ESP_LOGI(TAG, "Dark theme initialized");

    // Lock display for LVGL operations
    ESP_LOGI(TAG, "Creating Hello World label...");
    bsp_display_lock(0);

    // Set up screen with dark theme
    lv_obj_t *screen = lv_screen_active();
    dark_theme_apply_screen(screen);

    // Show splash screen first
    draw_krux_logo(screen);
    
    // Unlock display to allow LVGL to render the splash screen
    bsp_display_unlock();
    
    // Wait for a few seconds to show the splash
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Lock display again for modifications
    bsp_display_lock(0);
    
    // Clear the screen and show login page
    lv_obj_clean(screen);
    
    // Create and show the login page as a demonstration
    login_page_create(screen);

    // Unlock display
    bsp_display_unlock();
}
