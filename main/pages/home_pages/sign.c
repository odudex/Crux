/*
 * Sign Page
 * Handles PSBT transaction signing
 */

#include "sign.h"
#include "../../key/key.h"
#include "../../psbt/psbt.h"
#include "../../ui_components/flash_error.h"
#include "../../ui_components/theme.h"
#include "../../wallet/wallet.h"
#include "../qr_scanner.h"
#include <esp_log.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <wally_core.h>
#include <wally_psbt.h>
#include <wally_psbt_members.h>
#include <wally_script.h>
#include <wally_transaction.h>

static const char *TAG = "SIGN";

typedef enum {
  OUTPUT_TYPE_SELF_TRANSFER,
  OUTPUT_TYPE_CHANGE,
  OUTPUT_TYPE_SPEND,
} output_type_t;

typedef struct {
  size_t index;
  output_type_t type;
  uint64_t value;
  char *address;
  uint32_t address_index;
} classified_output_t;

// UI components
static lv_obj_t *sign_screen = NULL;
static lv_obj_t *qr_scanner_container = NULL;
static lv_obj_t *psbt_info_container = NULL;
static lv_obj_t *signed_qr_container = NULL;
static void (*return_callback)(void) = NULL;

// PSBT data
static struct wally_psbt *current_psbt = NULL;
static char *psbt_base64 = NULL;
static char *signed_psbt_base64 = NULL;
static bool is_testnet = false;

// Forward declarations
static void back_button_cb(lv_event_t *e);
static void return_from_qr_scanner_cb(void);
static bool parse_and_display_psbt(const char *base64_data);
static void cleanup_psbt_data(void);
static bool create_psbt_info_display(void);
static output_type_t classify_output(size_t output_index,
                                     const struct wally_tx_output *tx_output,
                                     uint32_t *address_index_out);
static void sign_button_cb(lv_event_t *e);
static void display_signed_psbt_qr(void);
static void signed_qr_back_cb(lv_event_t *e);
static void hide_message_timer_cb(lv_timer_t *timer);

// Classify output as self-transfer, change, or spend
static output_type_t classify_output(size_t output_index,
                                     const struct wally_tx_output *tx_output,
                                     uint32_t *address_index_out) {
  bool is_change = false;
  uint32_t address_index = 0;

  // Check if output has verified derivation path for our wallet
  if (!psbt_get_output_derivation(current_psbt, output_index, is_testnet,
                                  &is_change, &address_index)) {
    return OUTPUT_TYPE_SPEND;
  }

  // Verify scriptPubKey matches derived address
  unsigned char expected_script[WALLY_WITNESSSCRIPT_MAX_LEN];
  size_t expected_script_len;

  if (!wallet_get_scriptpubkey(is_change, address_index, expected_script,
                               &expected_script_len) ||
      tx_output->script_len != expected_script_len ||
      memcmp(tx_output->script, expected_script, expected_script_len) != 0) {
    ESP_LOGW(TAG, "Output %zu: Derivation path mismatch with scriptPubKey",
             output_index);
    return OUTPUT_TYPE_SPEND;
  }

  *address_index_out = address_index;
  return is_change ? OUTPUT_TYPE_CHANGE : OUTPUT_TYPE_SELF_TRANSFER;
}

static void back_button_cb(lv_event_t *e) {
  if (return_callback) {
    return_callback();
  }
}

static void return_from_qr_scanner_cb(void) {
  char *qr_content = qr_scanner_get_completed_content();

  if (qr_content) {
    // Parse and display the PSBT
    if (parse_and_display_psbt(qr_content)) {
      // Hide QR scanner
      qr_scanner_page_hide();
      qr_scanner_page_destroy();
      qr_scanner_container = NULL;

      // Show PSBT info
      if (!create_psbt_info_display()) {
        ESP_LOGE(TAG, "Failed to display PSBT info");
        show_flash_error("Invalid PSBT data", return_callback, 0);
      }
    } else {
      ESP_LOGE(TAG, "Failed to parse PSBT");
      // Hide QR scanner
      qr_scanner_page_hide();
      qr_scanner_page_destroy();
      qr_scanner_container = NULL;

      show_flash_error("Invalid PSBT format", return_callback, 0);
    }

    free(qr_content);
  } else {
    // User cancelled scanning
    qr_scanner_page_hide();
    qr_scanner_page_destroy();
    qr_scanner_container = NULL;

    if (return_callback) {
      return_callback();
    }
  }
}

// Parse base64 PSBT and display information
static bool parse_and_display_psbt(const char *base64_data) {
  if (!base64_data) {
    return false;
  }

  // Clean up any existing PSBT data
  cleanup_psbt_data();

  // Store the base64 data
  psbt_base64 = strdup(base64_data);
  if (!psbt_base64) {
    ESP_LOGE(TAG, "Failed to allocate memory for PSBT base64");
    return false;
  }

  // Parse PSBT from base64
  int ret = wally_psbt_from_base64(base64_data, 0, &current_psbt);
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to parse PSBT from base64: %d", ret);
    cleanup_psbt_data();
    return false;
  }

  // Network detection moved to create_psbt_info_display() where logs work
  return true;
}

// Create PSBT information display
// Returns false if PSBT is invalid or cannot be displayed
static bool create_psbt_info_display(void) {
  if (!sign_screen || !current_psbt) {
    ESP_LOGE(TAG, "Cannot create PSBT info: invalid state");
    return false;
  }

  is_testnet = psbt_detect_network(current_psbt);

  if (!wallet_is_initialized()) {
    ESP_LOGE(TAG, "Wallet not initialized");
    return false;
  }
  size_t num_inputs = 0;
  size_t num_outputs = 0;

  if (wally_psbt_get_num_inputs(current_psbt, &num_inputs) != WALLY_OK ||
      wally_psbt_get_num_outputs(current_psbt, &num_outputs) != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to get PSBT input/output count");
    return false;
  }

  if (num_inputs == 0 || num_outputs == 0) {
    ESP_LOGE(TAG, "Invalid PSBT: no inputs or outputs");
    return false;
  }

  // Calculate total input value
  uint64_t total_input_value = 0;
  for (size_t i = 0; i < num_inputs; i++) {
    total_input_value += psbt_get_input_value(current_psbt, i);
  }

  // Create info container
  psbt_info_container = lv_obj_create(sign_screen);
  lv_obj_set_size(psbt_info_container, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(psbt_info_container);
  lv_obj_add_event_cb(psbt_info_container, back_button_cb, LV_EVENT_CLICKED,
                      NULL);

  // Create scrollable content container
  lv_obj_t *content = lv_obj_create(psbt_info_container);
  lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(content, 10, 0);
  lv_obj_set_style_pad_gap(content, 10, 0);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_SCROLLABLE);

  // Title
  lv_obj_t *title = theme_create_label(content, "PSBT Transaction", false);
  theme_apply_label(title, true);
  lv_obj_set_width(title, LV_PCT(100));
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

  // Number of inputs
  char inputs_text[64];
  snprintf(inputs_text, sizeof(inputs_text), "Inputs: %zu", num_inputs);
  lv_obj_t *inputs_label = theme_create_label(content, inputs_text, false);
  lv_obj_set_width(inputs_label, LV_PCT(100));

  // Total input value
  char total_input_text[128];
  snprintf(total_input_text, sizeof(total_input_text), "Total Input: %llu sats",
           total_input_value);
  lv_obj_t *total_input_label =
      theme_create_label(content, total_input_text, false);
  lv_obj_set_width(total_input_label, LV_PCT(100));

  // Add separator
  lv_obj_t *separator1 = lv_obj_create(content);
  lv_obj_set_size(separator1, LV_PCT(100), 2);
  lv_obj_set_style_bg_color(separator1, main_color(), 0);
  lv_obj_set_style_bg_opa(separator1, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(separator1, 0, 0);

  // Get the global transaction to access output values
  struct wally_tx *global_tx = NULL;
  int tx_ret = wally_psbt_get_global_tx_alloc(current_psbt, &global_tx);

  if (tx_ret != WALLY_OK || !global_tx) {
    ESP_LOGE(TAG, "Failed to get transaction from PSBT");
    return false;
  }

  // Classify outputs
  classified_output_t *classified_outputs =
      calloc(num_outputs, sizeof(classified_output_t));
  if (!classified_outputs) {
    ESP_LOGE(TAG, "Failed to allocate memory for output classification");
    wally_tx_free(global_tx);
    return false;
  }

  // Classify all outputs
  for (size_t i = 0; i < num_outputs; i++) {
    classified_outputs[i].index = i;
    classified_outputs[i].value = global_tx->outputs[i].satoshi;
    classified_outputs[i].address = psbt_scriptpubkey_to_address(
        global_tx->outputs[i].script, global_tx->outputs[i].script_len,
        is_testnet);
    classified_outputs[i].type =
        classify_output(i, &global_tx->outputs[i], &classified_outputs[i].address_index);
  }

  // Display self-transfers
  bool has_self_transfers = false;
  for (size_t i = 0; i < num_outputs; i++) {
    if (classified_outputs[i].type == OUTPUT_TYPE_SELF_TRANSFER) {
      if (!has_self_transfers) {
        lv_obj_t *title = theme_create_label(content, "Self-Transfer:", false);
        theme_apply_label(title, true);
        lv_obj_set_width(title, LV_PCT(100));
        has_self_transfers = true;
      }

      char text[128];
      snprintf(text, sizeof(text), "Receive #%u: %llu sats",
               classified_outputs[i].address_index, classified_outputs[i].value);
      lv_obj_t *label = theme_create_label(content, text, false);
      lv_obj_set_width(label, LV_PCT(100));
      lv_obj_set_style_pad_left(label, 20, 0);

      if (classified_outputs[i].address) {
        lv_obj_t *addr = theme_create_label(content, classified_outputs[i].address, false);
        lv_obj_set_width(addr, LV_PCT(100));
        lv_label_set_long_mode(addr, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(addr, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_pad_left(addr, 20, 0);
      }
    }
  }

  // Display change
  bool has_change = false;
  for (size_t i = 0; i < num_outputs; i++) {
    if (classified_outputs[i].type == OUTPUT_TYPE_CHANGE) {
      if (!has_change) {
        lv_obj_t *title = theme_create_label(content, "Change:", false);
        theme_apply_label(title, true);
        lv_obj_set_width(title, LV_PCT(100));
        has_change = true;
      }

      char text[128];
      snprintf(text, sizeof(text), "Change #%u: %llu sats",
               classified_outputs[i].address_index, classified_outputs[i].value);
      lv_obj_t *label = theme_create_label(content, text, false);
      lv_obj_set_width(label, LV_PCT(100));
      lv_obj_set_style_pad_left(label, 20, 0);

      if (classified_outputs[i].address) {
        lv_obj_t *addr = theme_create_label(content, classified_outputs[i].address, false);
        lv_obj_set_width(addr, LV_PCT(100));
        lv_label_set_long_mode(addr, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(addr, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_pad_left(addr, 20, 0);
      }
    }
  }

  // Display spends
  bool has_spends = false;
  for (size_t i = 0; i < num_outputs; i++) {
    if (classified_outputs[i].type == OUTPUT_TYPE_SPEND) {
      if (!has_spends) {
        lv_obj_t *title = theme_create_label(content, "Spending:", false);
        theme_apply_label(title, true);
        lv_obj_set_width(title, LV_PCT(100));
        has_spends = true;
      }

      char text[128];
      snprintf(text, sizeof(text), "Output %zu: %llu sats",
               classified_outputs[i].index, classified_outputs[i].value);
      lv_obj_t *label = theme_create_label(content, text, false);
      lv_obj_set_width(label, LV_PCT(100));
      lv_obj_set_style_pad_left(label, 20, 0);

      if (classified_outputs[i].address) {
        lv_obj_t *addr = theme_create_label(content, classified_outputs[i].address, false);
        lv_obj_set_width(addr, LV_PCT(100));
        lv_label_set_long_mode(addr, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(addr, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_pad_left(addr, 20, 0);
      }
    }
  }

  // Clean up classified outputs
  for (size_t i = 0; i < num_outputs; i++) {
    if (classified_outputs[i].address) {
      if (strcmp(classified_outputs[i].address, "OP_RETURN") == 0) {
        free(classified_outputs[i].address);
      } else {
        wally_free_string(classified_outputs[i].address);
      }
    }
  }
  free(classified_outputs);

  // Calculate and display fee
  uint64_t total_output_value = 0;
  if (tx_ret == WALLY_OK && global_tx) {
    for (size_t i = 0; i < global_tx->num_outputs; i++) {
      total_output_value += global_tx->outputs[i].satoshi;
    }
  }

  // Free the global transaction
  if (global_tx) {
    wally_tx_free(global_tx);
    global_tx = NULL;
  }

  if (total_input_value > 0) {
    uint64_t fee = total_input_value - total_output_value;

    // Add separator
    lv_obj_t *separator2 = lv_obj_create(content);
    lv_obj_set_size(separator2, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(separator2, main_color(), 0);
    lv_obj_set_style_bg_opa(separator2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(separator2, 0, 0);

    char fee_text[128];
    snprintf(fee_text, sizeof(fee_text), "Fee: %llu sats", fee);
    lv_obj_t *fee_label = theme_create_label(content, fee_text, false);
    lv_obj_set_width(fee_label, LV_PCT(100));
  }

  // Sign button
  lv_obj_t *sign_button = lv_btn_create(content);
  lv_obj_set_width(sign_button, LV_PCT(90));
  lv_obj_set_height(sign_button, 50);
  lv_obj_set_style_bg_color(sign_button, main_color(), 0);
  lv_obj_set_style_radius(sign_button, 8, 0);
  lv_obj_add_event_cb(sign_button, sign_button_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_clear_flag(sign_button, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t *sign_label = theme_create_label(sign_button, "Sign", false);
  theme_apply_label(sign_label, true);
  lv_obj_center(sign_label);

  return true;
}

static void sign_button_cb(lv_event_t *e) {
  if (!current_psbt) {
    ESP_LOGE(TAG, "No PSBT to sign");
    show_flash_error("No PSBT loaded", NULL, 2000);
    return;
  }

  // Sign the PSBT
  size_t signatures_added = psbt_sign(current_psbt, is_testnet);

  if (signatures_added == 0) {
    ESP_LOGE(TAG, "No signatures added to PSBT");
    show_flash_error("Failed to sign PSBT", NULL, 2000);
    return;
  }

  // Convert signed PSBT to base64
  if (signed_psbt_base64) {
    wally_free_string(signed_psbt_base64);
    signed_psbt_base64 = NULL;
  }

  int ret = wally_psbt_to_base64(current_psbt, 0, &signed_psbt_base64);
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to convert signed PSBT to base64");
    show_flash_error("Failed to encode PSBT", NULL, 2000);
    return;
  }

  // Display QR code
  display_signed_psbt_qr();
}

static void hide_message_timer_cb(lv_timer_t *timer) {
  lv_obj_t *msgbox = (lv_obj_t *)lv_timer_get_user_data(timer);
  if (msgbox) {
    lv_obj_del(msgbox);
  }
}
static void display_signed_psbt_qr(void) {
  if (!sign_screen || !signed_psbt_base64) {
    ESP_LOGE(TAG, "Cannot display QR: invalid state");
    return;
  }

  if (psbt_info_container) {
    lv_obj_add_flag(psbt_info_container, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_set_style_bg_color(sign_screen, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_pad_all(sign_screen, 20, 0);
  lv_obj_add_event_cb(sign_screen, signed_qr_back_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_update_layout(sign_screen);

  int32_t available_width = lv_obj_get_content_width(sign_screen);
  int32_t available_height = lv_obj_get_content_height(sign_screen);
  int32_t qr_size = (available_width < available_height) ? available_width : available_height;
  signed_qr_container = lv_qrcode_create(sign_screen);
  if (!signed_qr_container) {
    ESP_LOGE(TAG, "Failed to create QR code");
    return;
  }
  lv_qrcode_set_size(signed_qr_container, qr_size);
  lv_qrcode_update(signed_qr_container, signed_psbt_base64, strlen(signed_psbt_base64));
  lv_obj_center(signed_qr_container);

  lv_obj_t *msgbox = lv_obj_create(sign_screen);
  lv_obj_set_size(msgbox, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(msgbox, LV_OPA_80, 0);
  lv_obj_set_style_border_width(msgbox, 2, 0);
  lv_obj_set_style_border_color(msgbox, main_color(), 0);
  lv_obj_set_style_radius(msgbox, 10, 0);
  lv_obj_set_style_pad_all(msgbox, 20, 0);
  lv_obj_add_flag(msgbox, LV_OBJ_FLAG_FLOATING);
  lv_obj_center(msgbox);

  lv_obj_t *msg_label = theme_create_label(msgbox, "Signed PSBT\nTap to return", false);
  lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(msg_label, lv_color_hex(0xFFFFFF), 0);

  lv_timer_t *timer = lv_timer_create(hide_message_timer_cb, 2000, msgbox);
  if (timer) {
    lv_timer_set_repeat_count(timer, 1);
  }
}

static void signed_qr_back_cb(lv_event_t *e) {
  if (return_callback) {
    return_callback();
  }
}

// Clean up PSBT data
static void cleanup_psbt_data(void) {
  if (current_psbt) {
    wally_psbt_free(current_psbt);
    current_psbt = NULL;
  }

  if (psbt_base64) {
    free(psbt_base64);
    psbt_base64 = NULL;
  }

  if (signed_psbt_base64) {
    wally_free_string(signed_psbt_base64);
    signed_psbt_base64 = NULL;
  }

  is_testnet = false;
}

void sign_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for sign page");
    return;
  }

  // Verify key is loaded
  if (!key_is_loaded()) {
    ESP_LOGE(TAG, "Cannot create sign page: no key loaded");
    return;
  }

  return_callback = return_cb;

  // Create the main screen
  sign_screen = lv_obj_create(parent);
  lv_obj_set_size(sign_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(sign_screen);

  // Create QR scanner container
  qr_scanner_container = lv_obj_create(sign_screen);
  lv_obj_set_size(qr_scanner_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(qr_scanner_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(qr_scanner_container, 0, 0);
  lv_obj_set_style_pad_all(qr_scanner_container, 20, 0);

  qr_scanner_page_create(qr_scanner_container, return_from_qr_scanner_cb);
  qr_scanner_page_show();
}

void sign_page_show(void) {
  if (sign_screen) {
    lv_obj_clear_flag(sign_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void sign_page_hide(void) {
  if (sign_screen) {
    lv_obj_add_flag(sign_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void sign_page_destroy(void) {
  if (qr_scanner_container) {
    qr_scanner_page_destroy();
    qr_scanner_container = NULL;
  }

  cleanup_psbt_data();

  psbt_info_container = NULL;
  signed_qr_container = NULL;

  if (sign_screen) {
    lv_obj_del(sign_screen);
    sign_screen = NULL;
  }

  return_callback = NULL;
}
