/*
 * QR Scanner Page - Displays a 480x480 frame buffer that changes colors
 * Returns to login page when screen is touched
 */

#include "qr_scanner.h"
#include "../ui_components/tron_theme.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "QR_SCANNER";

// Global variables for the QR scanner page
static lv_obj_t *qr_scanner_screen = NULL;
static lv_obj_t *frame_buffer = NULL;
static lv_timer_t *color_timer = NULL;
static void (*return_callback)(void) = NULL;

// Color array for cycling through different colors
static const lv_color_t colors[] = {
    LV_COLOR_MAKE(255, 0, 0),     // Red
    LV_COLOR_MAKE(0, 255, 0),     // Green  
    LV_COLOR_MAKE(0, 0, 255),     // Blue
    LV_COLOR_MAKE(255, 255, 0),   // Yellow
    LV_COLOR_MAKE(255, 0, 255),   // Magenta
    LV_COLOR_MAKE(0, 255, 255),   // Cyan
    LV_COLOR_MAKE(255, 128, 0),   // Orange
    LV_COLOR_MAKE(128, 0, 255),   // Purple
};

static uint8_t current_color_index = 0;

// Touch event handler - returns to login when screen is touched
static void touch_event_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Screen touched, returning to previous page");
    
    if (return_callback) {
        return_callback();
    }
}

// Timer callback to change colors every second
static void color_timer_cb(lv_timer_t *timer)
{
    if (!frame_buffer) {
        return;
    }
    
    // Cycle through colors
    current_color_index = (current_color_index + 1) % (sizeof(colors) / sizeof(colors[0]));
    
    // Apply new color to frame buffer
    lv_obj_set_style_bg_color(frame_buffer, colors[current_color_index], 0);
    
    ESP_LOGI(TAG, "Frame buffer color changed to index %d", current_color_index);
}

void qr_scanner_page_create(lv_obj_t *parent, void (*return_cb)(void))
{
    if (!parent) {
        ESP_LOGE(TAG, "Invalid parent object for QR scanner page");
        return;
    }
    
    ESP_LOGI(TAG, "Creating QR scanner page");
    
    // Store the return callback
    return_callback = return_cb;
    
    // Create screen container
    qr_scanner_screen = lv_obj_create(parent);
    lv_obj_set_size(qr_scanner_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(qr_scanner_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Remove container borders and padding
    lv_obj_set_style_border_width(qr_scanner_screen, 0, 0);
    lv_obj_set_style_pad_all(qr_scanner_screen, 0, 0);
    lv_obj_set_style_radius(qr_scanner_screen, 0, 0);
    lv_obj_set_style_shadow_width(qr_scanner_screen, 0, 0);
    lv_obj_clear_flag(qr_scanner_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Add touch event to the main screen
    lv_obj_add_event_cb(qr_scanner_screen, touch_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create title label
    lv_obj_t *title_label = tron_theme_create_label(qr_scanner_screen, "QR Scanner", false);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create 480x480 frame buffer container
    frame_buffer = lv_obj_create(qr_scanner_screen);
    lv_obj_set_size(frame_buffer, 480, 480);
    lv_obj_center(frame_buffer);
    
    // Style the frame buffer
    lv_obj_set_style_bg_color(frame_buffer, colors[current_color_index], 0);
    lv_obj_set_style_border_color(frame_buffer, lv_color_white(), 0);
    lv_obj_set_style_border_width(frame_buffer, 2, 0);
    lv_obj_set_style_radius(frame_buffer, 10, 0);
    lv_obj_clear_flag(frame_buffer, LV_OBJ_FLAG_SCROLLABLE);
    
    // Add touch event to frame buffer as well
    lv_obj_add_event_cb(frame_buffer, touch_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create instruction label
    lv_obj_t *instruction_label = tron_theme_create_label(qr_scanner_screen, "Touch screen to return", false);
    lv_obj_align(instruction_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Create timer to change colors every second (1000ms)
    color_timer = lv_timer_create(color_timer_cb, 1000, NULL);
    
    ESP_LOGI(TAG, "QR scanner page created successfully");
}

void qr_scanner_page_show(void)
{
    if (qr_scanner_screen) {
        lv_obj_clear_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "QR scanner page shown");
        
        // Resume the color timer
        if (color_timer) {
            lv_timer_resume(color_timer);
        }
    } else {
        ESP_LOGE(TAG, "QR scanner screen not initialized");
    }
}

void qr_scanner_page_hide(void)
{
    if (qr_scanner_screen) {
        lv_obj_add_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "QR scanner page hidden");
        
        // Pause the color timer
        if (color_timer) {
            lv_timer_pause(color_timer);
        }
    } else {
        ESP_LOGE(TAG, "QR scanner screen not initialized");
    }
}

void qr_scanner_page_destroy(void)
{
    // Destroy the color timer
    if (color_timer) {
        lv_timer_del(color_timer);
        color_timer = NULL;
        ESP_LOGI(TAG, "Color timer destroyed");
    }
    
    // Destroy the screen and all its children
    if (qr_scanner_screen) {
        lv_obj_del(qr_scanner_screen);
        qr_scanner_screen = NULL;
        frame_buffer = NULL; // Will be deleted with parent
        ESP_LOGI(TAG, "QR scanner screen destroyed");
    }
    
    // Clear the return callback
    return_callback = NULL;
}

