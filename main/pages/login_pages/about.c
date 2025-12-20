/*
 * About Page - Displays application information
 * Shows sample text and returns to menu when touched
 */

#include "about.h"
#include "../../ui_components/logo/kern_logo_lvgl.h"
#include "../../ui_components/theme.h"
#include "esp_log.h"
#include "lvgl.h"
#include "string.h"

static const char *TAG = "ABOUT";

// Global variables for the about page
static lv_obj_t *about_screen = NULL;
static void (*return_callback)(void) = NULL;

// Event handler for touch/click on the about screen
static void about_screen_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
    if (return_callback) {
      return_callback();
    }
  }
}

void about_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for about page");
    return;
  }

  ESP_LOGI(TAG, "Creating about page");

  // Store the return callback
  return_callback = return_cb;

  // Create screen container
  about_screen = theme_create_page_container(parent);

  // Make the screen clickable
  lv_obj_add_flag(about_screen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(about_screen, about_screen_event_cb, LV_EVENT_CLICKED,
                      NULL);

  // Create title label
  theme_create_page_title(about_screen, "About");

  // Create logo with name
  kern_logo_with_text(about_screen, 0, 130);

  // Create a QR code with a link to the project
  lv_obj_t *qr = lv_qrcode_create(about_screen);
  lv_qrcode_set_size(qr, 250);
  const char *data = "https://github.com/odudex/Kern";
  lv_qrcode_update(qr, data, strlen(data));
  lv_obj_align(qr, LV_ALIGN_CENTER, 0, 140);
  lv_obj_set_style_border_color(qr, lv_color_white(), 0);
  lv_obj_set_style_border_width(qr, 10, 0);

  // Create a footer instruction
  lv_obj_t *footer_label =
      theme_create_label(about_screen, "Tap to return", true);
  lv_obj_align(footer_label, LV_ALIGN_BOTTOM_MID, 0,
               -theme_get_default_padding());
  lv_obj_set_style_text_align(footer_label, LV_TEXT_ALIGN_CENTER, 0);

  ESP_LOGI(TAG, "About page created successfully");
}

void about_page_show(void) {
  if (about_screen) {
    lv_obj_clear_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
    ESP_LOGI(TAG, "About page shown");
  } else {
    ESP_LOGE(TAG, "About screen not initialized");
  }
}

void about_page_hide(void) {
  if (about_screen) {
    lv_obj_add_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
    ESP_LOGI(TAG, "About page hidden");
  } else {
    ESP_LOGE(TAG, "About screen not initialized");
  }
}

void about_page_destroy(void) {
  if (about_screen) {
    lv_obj_del(about_screen);
    about_screen = NULL;
    ESP_LOGI(TAG, "About screen destroyed");
  }

  // Clear the return callback
  return_callback = NULL;
}
