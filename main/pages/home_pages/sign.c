/*
 * Sign Page
 * Handles PSBT transaction signing
 */

#include "sign.h"
#include "../../key/key.h"
#include "../../ui_components/flash_error.h"
#include "../../ui_components/theme.h"
#include "../qr_scanner.h"
#include <esp_log.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <wally_address.h>
#include <wally_core.h>
#include <wally_psbt.h>
#include <wally_psbt_members.h>
#include <wally_script.h>
#include <wally_transaction.h>

static const char *TAG = "SIGN";

// UI components
static lv_obj_t *sign_screen = NULL;
static lv_obj_t *qr_scanner_container = NULL;
static lv_obj_t *psbt_info_container = NULL;
static void (*return_callback)(void) = NULL;

// PSBT data
static struct wally_psbt *current_psbt = NULL;
static char *psbt_base64 = NULL;
static bool is_testnet = false;

// Forward declarations
static void back_button_cb(lv_event_t *e);
static void return_from_qr_scanner_cb(void);
static bool parse_and_display_psbt(const char *base64_data);
static void cleanup_psbt_data(void);
static bool create_psbt_info_display(void);
static uint64_t get_input_value(const struct wally_psbt *psbt, size_t index);
static char *scriptpubkey_to_address(const unsigned char *script,
                                     size_t script_len);
static bool detect_network_from_psbt(const struct wally_psbt *psbt);

// Back button callback
static void back_button_cb(lv_event_t *e) {
  ESP_LOGI(TAG, "Back button pressed");

  if (return_callback) {
    return_callback();
  }
}

// Return from QR scanner callback
static void return_from_qr_scanner_cb(void) {
  ESP_LOGI(TAG, "Returning from QR scanner");

  // Get the scanned content
  char *qr_content = qr_scanner_get_completed_content();

  if (qr_content) {
    ESP_LOGI(TAG, "QR content received, length: %d", strlen(qr_content));

    // Parse and display the PSBT
    if (parse_and_display_psbt(qr_content)) {
      // Hide QR scanner
      qr_scanner_page_hide();
      qr_scanner_page_destroy();
      qr_scanner_container = NULL;

      // Show PSBT info
      if (!create_psbt_info_display()) {
        // Display failed, show error and return
        ESP_LOGE(TAG, "Failed to display PSBT info");
        show_flash_error("Invalid PSBT data", return_callback, 0);
      }
    } else {
      ESP_LOGE(TAG, "Failed to parse PSBT");
      // Hide QR scanner
      qr_scanner_page_hide();
      qr_scanner_page_destroy();
      qr_scanner_container = NULL;

      // Show error and return
      show_flash_error("Invalid PSBT format", return_callback, 0);
    }

    free(qr_content);
  } else {
    ESP_LOGW(TAG, "No QR content available, user likely cancelled");

    // Destroy QR scanner
    qr_scanner_page_hide();
    qr_scanner_page_destroy();
    qr_scanner_container = NULL;

    // Return to previous page
    if (return_callback) {
      return_callback();
    }
  }
}

// Get the value of an input from PSBT
static uint64_t get_input_value(const struct wally_psbt *psbt, size_t index) {
  uint64_t value = 0;
  struct wally_tx_output *utxo = NULL;

  if (wally_psbt_get_input_best_utxo_alloc(psbt, index, &utxo) == WALLY_OK &&
      utxo) {
    value = utxo->satoshi;
    wally_tx_output_free(utxo);
  }

  return value;
}

// Detect network (mainnet/testnet) from PSBT BIP32 derivation paths
// Returns true if testnet, false if mainnet
static bool detect_network_from_psbt(const struct wally_psbt *psbt) {
  if (!psbt) {
    return false;
  }

  size_t num_outputs = 0;
  wally_psbt_get_num_outputs(psbt, &num_outputs);

  // Check output keypaths for coin_type
  for (size_t i = 0; i < num_outputs; i++) {
    size_t keypaths_size = 0;
    if (wally_psbt_get_output_keypaths_size(psbt, i, &keypaths_size) ==
            WALLY_OK &&
        keypaths_size > 0) {

      unsigned char keypath_buf[100];
      size_t keypath_len = 0;

      if (wally_psbt_get_output_keypath(psbt, i, 0, keypath_buf,
                                        sizeof(keypath_buf),
                                        &keypath_len) == WALLY_OK &&
          keypath_len >= 12) {

        // BIP32 keypath format: [fingerprint(4)][purpose(4)][coin_type(4)]...
        // Extract coin_type at offset 8: 0'=mainnet, 1'=testnet
        uint32_t coin_type;
        memcpy(&coin_type, keypath_buf + 8, sizeof(uint32_t));
        uint32_t coin_value = coin_type & 0x7FFFFFFF;

        if (coin_value == 1) {
          ESP_LOGI(TAG, "Detected testnet from PSBT");
          return true;
        } else if (coin_value == 0) {
          ESP_LOGI(TAG, "Detected mainnet from PSBT");
          return false;
        }
      }
    }
  }

  // Check input keypaths as fallback
  size_t num_inputs = 0;
  wally_psbt_get_num_inputs(psbt, &num_inputs);

  for (size_t i = 0; i < num_inputs; i++) {
    size_t keypaths_size = 0;
    if (wally_psbt_get_input_keypaths_size(psbt, i, &keypaths_size) ==
            WALLY_OK &&
        keypaths_size > 0) {

      unsigned char keypath_buf[100];
      size_t keypath_len = 0;

      if (wally_psbt_get_input_keypath(psbt, i, 0, keypath_buf,
                                       sizeof(keypath_buf),
                                       &keypath_len) == WALLY_OK &&
          keypath_len >= 12) {

        uint32_t coin_type;
        memcpy(&coin_type, keypath_buf + 8, sizeof(uint32_t));
        uint32_t coin_value = coin_type & 0x7FFFFFFF;

        if (coin_value == 1) {
          ESP_LOGI(TAG, "Detected testnet from PSBT");
          return true;
        } else if (coin_value == 0) {
          ESP_LOGI(TAG, "Detected mainnet from PSBT");
          return false;
        }
      }
    }
  }

  // Default to mainnet if no keypaths found
  ESP_LOGW(TAG, "No network info in PSBT, defaulting to mainnet");
  return false;
}

// Convert scriptPubKey to Bitcoin address
static char *scriptpubkey_to_address(const unsigned char *script,
                                     size_t script_len) {
  if (!script || script_len == 0) {
    return NULL;
  }

  char *address = NULL;
  size_t script_type = 0;
  int ret;

  if (wally_scriptpubkey_get_type(script, script_len, &script_type) !=
      WALLY_OK) {
    return NULL;
  }

  // Convert based on script type
  if (script_type == WALLY_SCRIPT_TYPE_P2WPKH ||
      script_type == WALLY_SCRIPT_TYPE_P2WSH ||
      script_type == WALLY_SCRIPT_TYPE_P2TR) {
    // SegWit address
    const char *addr_family = is_testnet ? "tb" : "bc";
    ret = wally_addr_segwit_from_bytes(script, script_len, addr_family, 0,
                                       &address);
    if (ret != WALLY_OK) {
      return NULL;
    }
  } else if (script_type == WALLY_SCRIPT_TYPE_P2PKH ||
             script_type == WALLY_SCRIPT_TYPE_P2SH) {
    // Legacy address
    uint32_t network = is_testnet ? WALLY_NETWORK_BITCOIN_TESTNET
                                  : WALLY_NETWORK_BITCOIN_MAINNET;
    ret = wally_scriptpubkey_to_address(script, script_len, network, &address);
    if (ret != WALLY_OK) {
      return NULL;
    }
  } else if (script_type == WALLY_SCRIPT_TYPE_OP_RETURN) {
    address = strdup("OP_RETURN");
  } else {
    return NULL;
  }

  return address;
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

  // Detect network from PSBT
  is_testnet = detect_network_from_psbt(current_psbt);

  // Get number of inputs and outputs
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

  ESP_LOGI(TAG, "PSBT has %zu inputs and %zu outputs", num_inputs, num_outputs);

  // Calculate total input value
  uint64_t total_input_value = 0;
  for (size_t i = 0; i < num_inputs; i++) {
    total_input_value += get_input_value(current_psbt, i);
  }

  // Create info container
  psbt_info_container = lv_obj_create(sign_screen);
  lv_obj_set_size(psbt_info_container, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(psbt_info_container);
  lv_obj_add_event_cb(psbt_info_container, back_button_cb, LV_EVENT_CLICKED,
                      NULL);

  // Create scrollable content container
  lv_obj_t *content = lv_obj_create(psbt_info_container);
  lv_obj_set_size(content, LV_PCT(95), LV_PCT(95));
  lv_obj_center(content);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(content, 10, 0);
  lv_obj_set_style_pad_gap(content, 10, 0);
  theme_apply_frame(content);
  lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);

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

  // Outputs section
  lv_obj_t *outputs_title = theme_create_label(content, "Outputs:", false);
  theme_apply_label(outputs_title, true);
  lv_obj_set_width(outputs_title, LV_PCT(100));

  // Get the global transaction to access output values
  struct wally_tx *global_tx = NULL;
  int tx_ret = wally_psbt_get_global_tx_alloc(current_psbt, &global_tx);

  if (tx_ret != WALLY_OK || !global_tx) {
    ESP_LOGE(TAG, "Failed to get transaction from PSBT");
    return false;
  }

  // List each output
  for (size_t i = 0; i < num_outputs; i++) {
    uint64_t output_value = 0;

    // Get output amount from the global transaction
    if (tx_ret == WALLY_OK && global_tx && i < global_tx->num_outputs) {
      output_value = global_tx->outputs[i].satoshi;

      // Get the address from scriptPubKey
      char *address = scriptpubkey_to_address(global_tx->outputs[i].script,
                                              global_tx->outputs[i].script_len);

      // Display output amount
      char output_text[128];
      snprintf(output_text, sizeof(output_text), "Output %zu: %llu sats", i,
               output_value);
      lv_obj_t *output_label = theme_create_label(content, output_text, false);
      lv_obj_set_width(output_label, LV_PCT(100));

      // Display address if available
      if (address) {
        lv_obj_t *addr_label = theme_create_label(content, address, false);
        lv_obj_set_width(addr_label, LV_PCT(100));
        lv_label_set_long_mode(addr_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(addr_label, lv_color_hex(0xAAAAAA), 0);

        // Free the address string
        if (strcmp(address, "OP_RETURN") == 0) {
          free(address);
        } else {
          wally_free_string(address);
        }
      }
    }
  }

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

  // Instruction text
  lv_obj_t *hint_label = theme_create_label(content, "Tap to return", false);
  lv_obj_set_width(hint_label, LV_PCT(100));
  lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);

  return true;
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
  lv_obj_set_style_pad_all(qr_scanner_container, 0, 0);

  // Create QR scanner page
  qr_scanner_page_create(qr_scanner_container, return_from_qr_scanner_cb);
  qr_scanner_page_show();

  ESP_LOGI(TAG, "Sign page created successfully");
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
  ESP_LOGI(TAG, "Destroying sign page");

  // Clean up QR scanner if still active
  if (qr_scanner_container) {
    qr_scanner_page_destroy();
    qr_scanner_container = NULL;
  }

  // Clean up PSBT data
  cleanup_psbt_data();

  // Destroy info container
  psbt_info_container = NULL;

  // Destroy screen
  if (sign_screen) {
    lv_obj_del(sign_screen);
    sign_screen = NULL;
  }

  return_callback = NULL;

  ESP_LOGI(TAG, "Sign page destroyed");
}
