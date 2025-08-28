/*
 * Mnemonic Loading Page
 * Displays the scanned QR code content
 */

#include "mnemonic_loading.h"
#include "../../../ui_components/theme.h"
#include <esp_log.h>
#include <lvgl.h>
#include <string.h>
#include <wally_bip39.h>

static const char *TAG = "MNEMONIC_LOADING";

// UI components
static lv_obj_t *mnemonic_loading_screen = NULL;
static lv_obj_t *content_label = NULL;
static void (*return_callback)(void) = NULL;
static char *qr_content = NULL;

// Function declarations
static void touch_event_cb(lv_event_t *e);

// UI Event handlers
static void touch_event_cb(lv_event_t *e) {
  ESP_LOGI(TAG, "Screen touched, returning to previous page");
  if (return_callback) {
    return_callback();
  }
}

// Public API functions
void mnemonic_loading_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                  const char *content) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for mnemonic loading page");
    return;
  }

  return_callback = return_cb;

  // Store the content
  if (qr_content) {
    free(qr_content);
    qr_content = NULL;
  }

  if (content) {
    qr_content = malloc(strlen(content) + 1);
    if (qr_content) {
      strcpy(qr_content, content);
    }
  }

  mnemonic_loading_screen = lv_obj_create(parent);
  lv_obj_set_size(mnemonic_loading_screen, LV_PCT(100), LV_PCT(100));
  lv_obj_add_event_cb(mnemonic_loading_screen, touch_event_cb, LV_EVENT_CLICKED,
                      NULL);

  // Apply tron theme to the screen
  theme_apply_screen(mnemonic_loading_screen);

  // Create title label
  lv_obj_t *title_label =
      theme_create_label(mnemonic_loading_screen, "QR Content", false);
  theme_apply_label(title_label, true);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 0);

  // Create content label
  content_label = theme_create_label(
      mnemonic_loading_screen, qr_content ? qr_content : "No content", false);
  theme_apply_label(content_label, false);
  lv_obj_align(content_label, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_long_mode(content_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(content_label, LV_PCT(90));

  // Create validation result label
  lv_obj_t *validation_label;
  if (qr_content && bip39_mnemonic_validate(NULL, qr_content) == WALLY_OK) {
    validation_label =
        theme_create_label(mnemonic_loading_screen, "Valid mnemonic", false);
    lv_obj_set_style_text_color(validation_label, lv_color_hex(0x00FF00),
                                0); // Green
  } else {
    validation_label =
        theme_create_label(mnemonic_loading_screen, "Invalid mnemonic", false);
    lv_obj_set_style_text_color(validation_label, lv_color_hex(0xFF0000),
                                0); // Red
  }
  lv_obj_align(validation_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void mnemonic_loading_page_show(void) {
  if (mnemonic_loading_screen) {
    lv_obj_clear_flag(mnemonic_loading_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void mnemonic_loading_page_hide(void) {
  if (mnemonic_loading_screen) {
    lv_obj_add_flag(mnemonic_loading_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void mnemonic_loading_page_destroy(void) {
  // Clean up content
  if (qr_content) {
    free(qr_content);
    qr_content = NULL;
  }

  // Clean up UI objects
  if (mnemonic_loading_screen) {
    lv_obj_del(mnemonic_loading_screen);
    mnemonic_loading_screen = NULL;
  }

  // Reset references
  content_label = NULL;
  return_callback = NULL;
}
