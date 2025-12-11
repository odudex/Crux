/*
 * Load Menu Page - Intermediary menu for loading mnemonic options
 * Provides options for QR code scanning, manual input, and returning to login
 */

#include "load_menu.h"
#include "../../../ui_components/simple_dialog.h"
#include "../../../ui_components/theme.h"
#include "../../../ui_components/ui_menu.h"
#include "../../home_pages/home.h"
#include "../../qr_scanner.h"
#include "esp_log.h"
#include "lvgl.h"
#include "manual_input.h"
#include "mnemonic_loading.h"
#include <string.h>

static const char *TAG = "LOAD_MENU";

// Global variables for the load menu page
static ui_menu_t *load_menu = NULL;
static lv_obj_t *load_menu_screen = NULL;
static void (*return_callback)(void) = NULL;

// Forward declarations of menu callback functions
static void from_qr_code_cb(void);
static void from_manual_input_cb(void);
static void back_cb(void);

// QR scanner callback functions
static void return_from_qr_scanner_cb(void);
static void return_from_mnemonic_loading_cb(void);
static void success_from_mnemonic_loading_cb(void);

// Manual input callback functions
static void return_from_manual_input_cb(void);
static void success_from_manual_input_cb(void);

// Helper function to return from QR scanner page to load menu
static void return_from_qr_scanner_cb(void) {
  ESP_LOGI(TAG, "Returning from QR scanner page");

  // Check if QR scanner has completed content before destroying
  size_t content_len = 0;
  char *scanned_content =
      qr_scanner_get_completed_content_with_len(&content_len);
  if (scanned_content) {
    ESP_LOGI(TAG,
             "Found scanned content (len=%zu), transitioning to mnemonic "
             "loading page",
             content_len);

    // Destroy QR scanner page
    qr_scanner_page_destroy();

    // Create and show mnemonic loading page with the scanned content
    mnemonic_loading_page_create(
        lv_screen_active(), return_from_mnemonic_loading_cb,
        success_from_mnemonic_loading_cb, scanned_content, content_len);
    mnemonic_loading_page_show();

    // Free the content returned by the scanner
    free(scanned_content);
  } else {
    ESP_LOGI(TAG, "No scanned content, returning to load menu");
    qr_scanner_page_destroy();
    load_menu_page_show();
  }
}

// Helper function to return from mnemonic loading page to load menu
static void return_from_mnemonic_loading_cb(void) {
  ESP_LOGI(TAG, "Returning from mnemonic loading page to load menu");
  mnemonic_loading_page_destroy();
  load_menu_page_show();
}

// Helper function called when key is successfully loaded
static void success_from_mnemonic_loading_cb(void) {
  ESP_LOGI(TAG, "Key loaded successfully, transitioning to home page");

  // Destroy all login-related pages
  mnemonic_loading_page_destroy();
  load_menu_page_destroy();
  // Note: The calling code should also destroy the login menu if needed

  // Create and show the home page
  home_page_create(lv_screen_active());
  home_page_show();

  ESP_LOGI(TAG, "Home page is now active");
}

// Helper function to return from manual input page to load menu
static void return_from_manual_input_cb(void) {
  ESP_LOGI(TAG, "Returning from manual input page to load menu");
  manual_input_page_destroy();
  load_menu_page_show();
}

// Helper function called when key is successfully loaded from manual input
static void success_from_manual_input_cb(void) {
  ESP_LOGI(TAG, "Key loaded from manual input, transitioning to home page");

  // Destroy all login-related pages
  mnemonic_loading_page_destroy();
  manual_input_page_destroy();
  load_menu_page_destroy();

  // Create and show the home page
  home_page_create(lv_screen_active());
  home_page_show();

  ESP_LOGI(TAG, "Home page is now active");
}

// Load menu callback implementations
static void from_qr_code_cb(void) {
  ESP_LOGI(TAG, "From QR Code menu item selected");

  // Hide the load menu
  load_menu_page_hide();

  // Create and show the QR scanner page
  qr_scanner_page_create(lv_screen_active(), return_from_qr_scanner_cb);
  qr_scanner_page_show();
}

static void from_manual_input_cb(void) {
  ESP_LOGI(TAG, "From Manual Input menu item selected");

  // Hide the load menu
  load_menu_page_hide();

  // Create and show the manual input page
  manual_input_page_create(lv_screen_active(), return_from_manual_input_cb,
                           success_from_manual_input_cb);
  manual_input_page_show();
}

static void back_cb(void) {
  ESP_LOGI(TAG, "Back menu item selected");

  // Store the callback before destroying
  void (*callback)(void) = return_callback;

  // Hide and destroy the load menu
  load_menu_page_hide();
  load_menu_page_destroy();

  // Call the return callback to go back to login menu
  if (callback) {
    callback();
  }
}

void load_menu_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for load menu page");
    return;
  }

  ESP_LOGI(TAG, "Creating load menu page");

  // Store the return callback
  return_callback = return_cb;

  // Create screen container
  load_menu_screen = lv_obj_create(parent);
  lv_obj_set_size(load_menu_screen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(load_menu_screen, lv_color_hex(0x1e1e1e), 0);
  // Remove container borders and padding
  lv_obj_set_style_border_width(load_menu_screen, 0, 0);
  lv_obj_set_style_pad_all(load_menu_screen, 0, 0);
  lv_obj_set_style_radius(load_menu_screen, 0, 0);
  lv_obj_set_style_shadow_width(load_menu_screen, 0, 0);
  lv_obj_clear_flag(load_menu_screen, LV_OBJ_FLAG_SCROLLABLE);

  // Create the load menu
  load_menu = ui_menu_create(load_menu_screen, "Load Mnemonic");
  if (!load_menu) {
    ESP_LOGE(TAG, "Failed to create load menu");
    return;
  }

  // Add menu entries with their respective callbacks
  if (!ui_menu_add_entry(load_menu, "From QR Code", from_qr_code_cb)) {
    ESP_LOGE(TAG, "Failed to add From QR Code menu entry");
  }

  if (!ui_menu_add_entry(load_menu, "From Manual Input",
                         from_manual_input_cb)) {
    ESP_LOGE(TAG, "Failed to add From Manual Input menu entry");
  }

  if (!ui_menu_add_entry(load_menu, "Back", back_cb)) {
    ESP_LOGE(TAG, "Failed to add Back menu entry");
  }

  // Show the menu
  ui_menu_show(load_menu);

  ESP_LOGI(TAG, "Load menu page created successfully with %d menu entries",
           ui_menu_get_selected(load_menu) >= 0 ? 3 : 0);
}

void load_menu_page_show(void) {
  if (load_menu) {
    ui_menu_show(load_menu);
    ESP_LOGI(TAG, "Load menu page shown");
  } else {
    ESP_LOGE(TAG, "Load menu not initialized");
  }
}

void load_menu_page_hide(void) {
  if (load_menu) {
    ui_menu_hide(load_menu);
    ESP_LOGI(TAG, "Load menu page hidden");
  } else {
    ESP_LOGE(TAG, "Load menu not initialized");
  }
}

void load_menu_page_destroy(void) {
  if (load_menu) {
    ui_menu_destroy(load_menu);
    load_menu = NULL;
    ESP_LOGI(TAG, "Load menu destroyed");
  }

  if (load_menu_screen) {
    lv_obj_del(load_menu_screen);
    load_menu_screen = NULL;
    ESP_LOGI(TAG, "Load menu screen destroyed");
  }

  // Clear the return callback
  return_callback = NULL;
}

bool load_menu_page_navigate_next(void) {
  if (load_menu) {
    return ui_menu_navigate_next(load_menu);
  }
  return false;
}

bool load_menu_page_navigate_prev(void) {
  if (load_menu) {
    return ui_menu_navigate_prev(load_menu);
  }
  return false;
}

bool load_menu_page_execute_selected(void) {
  if (load_menu) {
    return ui_menu_execute_selected(load_menu);
  }
  return false;
}

int load_menu_page_get_selected(void) {
  if (load_menu) {
    return ui_menu_get_selected(load_menu);
  }
  return -1;
}