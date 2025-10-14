/*
 * Public Key Display Page
 * Shows extended public key information for the loaded key
 */

#include "public_key.h"
#include "../../key/key.h"
#include "../../ui_components/theme.h"
#include <esp_log.h>
#include <lvgl.h>
#include <stdio.h>
#include <wally_core.h>

static const char *TAG = "PUBLIC_KEY";

// UI components
static lv_obj_t *public_key_screen = NULL;
static void (*return_callback)(void) = NULL;

// Forward declarations
static void back_button_cb(lv_event_t *e);

// Back button callback
static void back_button_cb(lv_event_t *e) {
  ESP_LOGI(TAG, "Back button pressed");

  if (return_callback) {
    return_callback();
  }
}

void public_key_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for public key page");
    return;
  }

  // Verify key is loaded
  if (!key_is_loaded()) {
    ESP_LOGE(TAG, "Cannot create public key page: no key loaded");
    return;
  }

  return_callback = return_cb;

  // Create the main screen - entire screen is clickable to go back
  public_key_screen = lv_obj_create(parent);
  lv_obj_set_size(public_key_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(public_key_screen);
  lv_obj_add_event_cb(public_key_screen, back_button_cb, LV_EVENT_CLICKED,
                      NULL);

  // Create main container using full screen with flex layout
  lv_obj_t *main_container = lv_obj_create(public_key_screen);
  lv_obj_set_size(main_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(main_container, 10, 0);
  lv_obj_set_style_pad_gap(main_container, 10, 0);
  theme_apply_screen(main_container);
  lv_obj_add_flag(main_container,
                  LV_OBJ_FLAG_EVENT_BUBBLE); // Bubble events to parent

  // Create a content wrapper for centered content (info, QR, xpub)
  lv_obj_t *content_wrapper = lv_obj_create(main_container);
  lv_obj_set_size(content_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(content_wrapper, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(content_wrapper, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(content_wrapper, 0, 0);
  lv_obj_set_style_pad_gap(content_wrapper, 10, 0);
  lv_obj_set_style_border_width(content_wrapper, 0, 0);
  lv_obj_set_style_bg_opa(content_wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_grow(content_wrapper, 1);
  lv_obj_add_flag(content_wrapper,
                  LV_OBJ_FLAG_EVENT_BUBBLE); // Bubble events to parent

  // Info labels (compact format without titles)
  lv_obj_t *wallet_type_value =
      theme_create_label(content_wrapper, "Single-sig Native Segwit", false);
  lv_obj_set_width(wallet_type_value, LV_PCT(100));
  lv_obj_set_style_text_align(wallet_type_value, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *path_value =
      theme_create_label(content_wrapper, "m/84'/0'/0'", false);
  lv_obj_set_width(path_value, LV_PCT(100));
  lv_obj_set_style_text_align(path_value, LV_TEXT_ALIGN_CENTER, 0);

  // Get fingerprint for key origin
  char fingerprint_hex[BIP32_KEY_FINGERPRINT_LEN * 2 + 1];
  if (!key_get_fingerprint_hex(fingerprint_hex)) {
    ESP_LOGE(TAG, "Failed to get fingerprint");
    return;
  }

  // Get the XPUB at the Native Segwit derivation path
  char *xpub_str = NULL;
  if (key_get_xpub("m/84'/0'/0'", &xpub_str)) {
    // Create key origin string: [fingerprint/84h/0h/0h]xpub
    char key_origin[512];
    snprintf(key_origin, sizeof(key_origin), "[%s/84h/0h/0h]%s",
             fingerprint_hex, xpub_str);

    // Calculate square size: 60% of screen width
    int32_t square_size = lv_disp_get_hor_res(NULL) * 60 / 100;

    // Create white container for QR code (quiet zone)
    lv_obj_t *qr_container = lv_obj_create(content_wrapper);
    lv_obj_set_size(qr_container, square_size, square_size);
    lv_obj_set_style_bg_color(qr_container, lv_color_hex(0xFFFFFF),
                              0); // White background
    lv_obj_set_style_bg_opa(qr_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(qr_container, 0, 0);
    lv_obj_set_style_pad_all(qr_container, 15, 0); // Quiet zone padding
    lv_obj_set_style_radius(qr_container, 0, 0);
    lv_obj_clear_flag(qr_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(qr_container,
                    LV_OBJ_FLAG_EVENT_BUBBLE); // Bubble events to parent

    // Force layout update to get actual qr_container size
    lv_obj_update_layout(qr_container);

    // Calculate QR code size (container size minus padding)
    int32_t container_width = lv_obj_get_content_width(qr_container);
    int32_t container_height = lv_obj_get_content_height(qr_container);
    int32_t qr_size = (container_width < container_height) ? container_width
                                                           : container_height;

    // Create QR code inside white container
    lv_obj_t *qr = lv_qrcode_create(qr_container);
    lv_qrcode_set_size(qr, qr_size); // Use calculated pixel size
    lv_qrcode_update(qr, key_origin, strlen(key_origin));
    lv_obj_center(qr);

    // XPUB value below QR code (with text wrapping)
    lv_obj_t *xpub_value = theme_create_label(content_wrapper, xpub_str, false);
    lv_obj_set_width(xpub_value, LV_PCT(95));
    lv_label_set_long_mode(xpub_value, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(xpub_value, LV_TEXT_ALIGN_CENTER, 0);

    ESP_LOGI(TAG, "Displayed xpub with key origin: %s", key_origin);

    // Free the xpub string allocated by libwally
    wally_free_string(xpub_str);
  } else {
    // Error message
    lv_obj_t *error_value =
        theme_create_label(content_wrapper, "Error: Failed to get XPUB", false);
    lv_obj_set_style_text_color(error_value, error_color(), 0);
    lv_obj_set_width(error_value, LV_PCT(100));

    ESP_LOGE(TAG, "Failed to get xpub");
  }

  // Instruction text at the bottom
  lv_obj_t *hint_label =
      theme_create_label(main_container, "Tap to go back", false);
  lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);

  ESP_LOGI(TAG, "Public key page created successfully");
}

void public_key_page_show(void) {
  if (public_key_screen) {
    lv_obj_clear_flag(public_key_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void public_key_page_hide(void) {
  if (public_key_screen) {
    lv_obj_add_flag(public_key_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void public_key_page_destroy(void) {
  ESP_LOGI(TAG, "Destroying public key page");

  if (public_key_screen) {
    lv_obj_del(public_key_screen);
    public_key_screen = NULL;
  }

  return_callback = NULL;

  ESP_LOGI(TAG, "Public key page destroyed");
}
