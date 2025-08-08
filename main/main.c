#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>
#include <bsp/esp-bsp.h>
#include <bsp/display.h>
#include "pages/splash_screen.h"
#include "pages/login_pages/login.h"
#include "ui_components/tron_theme.h"

static const char *TAG = "Krux";

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
        }
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();
    bsp_display_brightness_set(50);


    // lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
    // if (display == NULL) {
    //     ESP_LOGE(TAG, "Failed to initialize display");
    //     return;
    // }
    ESP_LOGI(TAG, "Display initialized successfully");

    // esp_err_t ret = bsp_display_backlight_on();
    // if (ret != ESP_OK) {
    //     ESP_LOGW(TAG, "Failed to turn on backlight: %s", esp_err_to_name(ret));
    // }
    // ESP_LOGI(TAG, "Backlight set to 100%%");

    // Give some time for display to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Initialize tron theme
    tron_theme_init();
    ESP_LOGI(TAG, "Tron theme initialized");

    // Lock display for LVGL operations
    ESP_LOGI(TAG, "Creating Hello World label...");
    bsp_display_lock(0);

    // Set up screen with tron theme
    lv_obj_t *screen = lv_screen_active();
    tron_theme_apply_screen(screen);

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
