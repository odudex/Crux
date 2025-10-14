/*
 * Mnemonic Loading Page
 * Displays the scanned QR code content
 */

#include "mnemonic_loading.h"
#include "../../../key/key.h"
#include "../../../ui_components/flash_error.h"
#include "../../../ui_components/prompt_dialog.h"
#include "../../../ui_components/theme.h"
#include "../../../utils/memory_utils.h"
#include <esp_log.h>
#include <lvgl.h>
#include <string.h>
#include <wally_bip32.h>
#include <wally_bip39.h>
#include <wally_crypto.h>

static const char *TAG = "MNEMONIC_LOADING";

// UI components
static lv_obj_t *mnemonic_loading_screen = NULL;
static lv_obj_t *content_label = NULL;
static lv_obj_t *fingerprint_label = NULL;
static void (*return_callback)(void) = NULL;
static void (*success_callback)(void) = NULL;
static char *qr_content = NULL;
static unsigned char key_fingerprint[BIP32_KEY_FINGERPRINT_LEN];
static bool is_valid_mnemonic = false;

// Function declarations
static void load_key_prompt_callback(bool result, void *user_data);
static int calculate_fingerprint_from_mnemonic(const char *mnemonic,
                                               unsigned char *fingerprint_out);

// Helper function to calculate fingerprint from mnemonic
static int calculate_fingerprint_from_mnemonic(const char *mnemonic,
                                               unsigned char *fingerprint_out) {
  int ret;
  unsigned char seed[BIP39_SEED_LEN_512];
  struct ext_key *master_key = NULL;

  // Convert mnemonic to seed (no passphrase)
  ret = bip39_mnemonic_to_seed512(mnemonic, NULL, seed, sizeof(seed));
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to convert mnemonic to seed: %d", ret);
    return ret;
  }

  // Create master extended key from seed
  ret = bip32_key_from_seed_alloc(seed, sizeof(seed), BIP32_VER_MAIN_PRIVATE, 0,
                                  &master_key);
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to create master key from seed: %d", ret);
    // Clear sensitive data
    memset(seed, 0, sizeof(seed));
    return ret;
  }

  // Get the fingerprint
  ret = bip32_key_get_fingerprint(master_key, fingerprint_out,
                                  BIP32_KEY_FINGERPRINT_LEN);
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to get key fingerprint: %d", ret);
  }

  // Clean up sensitive data
  memset(seed, 0, sizeof(seed));
  bip32_key_free(master_key);

  return ret;
}

// UI Event handlers
static void load_key_prompt_callback(bool result, void *user_data) {
  if (result) {
    // User selected "Yes" - proceed with loading the key
    ESP_LOGI(TAG, "User confirmed key loading");

    // Load the key from mnemonic
    if (key_load_from_mnemonic(qr_content, NULL)) {
      ESP_LOGI(TAG, "Key loaded successfully");

      // Call success callback to transition to home page
      if (success_callback) {
        success_callback();
      }
    } else {
      ESP_LOGE(TAG, "Failed to load key");
      show_flash_error("Failed to load key", return_callback, 0);
    }
  } else {
    // User selected "No" - return to previous page
    ESP_LOGI(TAG, "User cancelled key loading");
    if (return_callback) {
      return_callback();
    }
  }
}

// Public API functions
void mnemonic_loading_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                  void (*success_cb)(void),
                                  const char *content) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for mnemonic loading page");
    return;
  }

  return_callback = return_cb;
  success_callback = success_cb;

  // Store the content
  SAFE_FREE_STATIC(qr_content);
  qr_content = SAFE_STRDUP(content);

  // Create the main screen
  mnemonic_loading_screen = lv_obj_create(parent);
  lv_obj_set_size(mnemonic_loading_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(mnemonic_loading_screen);

  // Validate mnemonic first
  is_valid_mnemonic =
      (qr_content && bip39_mnemonic_validate(NULL, qr_content) == WALLY_OK);

  if (!is_valid_mnemonic) {
    // Invalid mnemonic - show error and auto-return
    ESP_LOGW(TAG, "Invalid mnemonic detected");
    show_flash_error("Invalid mnemonic phrase", return_callback, 0);
    return;
  } else {
    // Valid mnemonic - calculate fingerprint and show confirmation
    ESP_LOGI(TAG, "Valid mnemonic, calculating fingerprint");

    int ret = calculate_fingerprint_from_mnemonic(qr_content, key_fingerprint);
    if (ret != WALLY_OK) {
      ESP_LOGE(TAG, "Failed to calculate fingerprint: %d", ret);
      show_flash_error("Failed to process mnemonic", return_callback, 0);
      return;
    }

    // Convert fingerprint to hex string
    char *fingerprint_hex = NULL;
    ret = wally_hex_from_bytes(key_fingerprint, BIP32_KEY_FINGERPRINT_LEN,
                               &fingerprint_hex);
    if (ret != WALLY_OK || !fingerprint_hex) {
      ESP_LOGE(TAG, "Failed to convert fingerprint to hex: %d", ret);
      show_flash_error("Failed to format fingerprint", return_callback, 0);
      return;
    }

    ESP_LOGI(TAG, "Key fingerprint: %s", fingerprint_hex);

    // Create UI to show fingerprint

    // Show the generic prompt dialog
    char prompt_text[128];
    snprintf(prompt_text, sizeof(prompt_text),
             "Load this key?\n\nFingerprint: %s", fingerprint_hex);
    show_prompt_dialog(prompt_text, load_key_prompt_callback, NULL);

    // Free the hex string
    wally_free_string(fingerprint_hex);
  }
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
  SAFE_FREE_STATIC(qr_content);

  // Clear sensitive data
  memset(key_fingerprint, 0, sizeof(key_fingerprint));
  is_valid_mnemonic = false;

  // Clean up UI objects
  if (mnemonic_loading_screen) {
    lv_obj_del(mnemonic_loading_screen);
    mnemonic_loading_screen = NULL;
  }

  // Reset references
  content_label = NULL;
  fingerprint_label = NULL;
  return_callback = NULL;
  success_callback = NULL;
}
