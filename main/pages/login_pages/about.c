/*
 * About Page - Displays application information
 * Shows sample text and returns to menu when touched
 */

#include "about.h"
#include "../../ui_components/tron_theme.h"
#include "esp_log.h"
#include "lvgl.h"
# include "string.h"

static const char *TAG = "ABOUT";

// Global variables for the about page
static lv_obj_t *about_screen = NULL;
static void (*return_callback)(void) = NULL;

// Event handler for touch/click on the about screen
static void about_screen_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
        if (return_callback) {
            return_callback();
        }
    }
}

void about_page_create(lv_obj_t *parent, void (*return_cb)(void))
{
    if (!parent) {
        ESP_LOGE(TAG, "Invalid parent object for about page");
        return;
    }
    
    ESP_LOGI(TAG, "Creating about page");
    
    // Store the return callback
    return_callback = return_cb;

    // Create screen container
    about_screen = lv_obj_create(parent);
    lv_obj_set_size(about_screen, LV_PCT(100), LV_PCT(100));
    
    // Apply tron theme to the screen
    tron_theme_apply_screen(about_screen);
  
    // Make the screen clickable
    lv_obj_add_flag(about_screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(about_screen, about_screen_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create title label
    lv_obj_t *title_label = tron_theme_create_label(about_screen, "About", false);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create main content label with sample text
    lv_obj_t *content_label = tron_theme_create_label(about_screen, 
        "Krux written in C\n"
        "LVGL UI",
        false);
    
    lv_obj_set_style_text_font(content_label, &lv_font_montserrat_24, 0);
    lv_obj_align(content_label, LV_ALIGN_CENTER, 0, -120);
    lv_obj_set_style_text_align(content_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create a QR code with a link to the project
    lv_obj_t *qr = lv_qrcode_create(about_screen);
    lv_qrcode_set_size(qr, 300);
    const char *data = "selfcustody.github.io/krux/";
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 120);
    lv_obj_set_style_border_color(qr, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr, 5, 0);
    
    // Create a footer instruction
    lv_obj_t *footer_label = tron_theme_create_label(about_screen, 
        "Tap to return", true);
    lv_obj_align(footer_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_align(footer_label, LV_TEXT_ALIGN_CENTER, 0);
    
    ESP_LOGI(TAG, "About page created successfully");
}

void about_page_show(void)
{
    if (about_screen) {
        lv_obj_clear_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "About page shown");
    } else {
        ESP_LOGE(TAG, "About screen not initialized");
    }
}

void about_page_hide(void)
{
    if (about_screen) {
        lv_obj_add_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "About page hidden");
    } else {
        ESP_LOGE(TAG, "About screen not initialized");
    }
}

void about_page_destroy(void)
{
    if (about_screen) {
        lv_obj_del(about_screen);
        about_screen = NULL;
        ESP_LOGI(TAG, "About screen destroyed");
    }
    
    // Clear the return callback
    return_callback = NULL;
}
