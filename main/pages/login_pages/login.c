/*
 * Login Page - Sample test case for UI Menu System
 * Demonstrates how to use the generic UI menu component
 */

#include "login.h"
#include "../../ui_components/tron_theme.h"
#include "../../ui_components/ui_menu.h"
#include "../qr_scanner.h"
#include "about.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "LOGIN";

// Global variables for the login page
static ui_menu_t *login_menu = NULL;
static lv_obj_t *login_screen = NULL;

// Forward declarations of menu callback functions
static void load_mnemonic_cb(void);
static void new_mnemonic_cb(void);
static void settings_cb(void);
static void tools_cb(void);
static void about_cb(void);
static void shutdown_cb(void);
static void exit_cb(void);

// Helper function for closing dialogs
static void close_dialog_cb(lv_event_t *e) {
  lv_obj_t *dialog = (lv_obj_t *)lv_event_get_user_data(e);
  lv_obj_del(dialog);
}

// Helper function to return from about page to login menu
static void return_to_login_cb(void) {
  ESP_LOGI(TAG, "Returning from about page to login menu");
  about_page_destroy();
  login_page_show();
}

// Helper function to return from QR scanner page to login menu
static void return_from_qr_scanner_cb(void) {
  ESP_LOGI(TAG, "Returning from QR scanner page to login menu");
  qr_scanner_page_destroy();
  login_page_show();
}

// Helper function to create simple dialogs
static void show_simple_dialog(const char *title, const char *message) {
  // Create modal dialog
  lv_obj_t *modal = lv_obj_create(lv_screen_active());
  lv_obj_set_size(modal, 400, 220);
  lv_obj_center(modal);

  // Apply TRON theme to modal
  tron_theme_apply_modal(modal);

  // Title
  lv_obj_t *title_label = tron_theme_create_label(modal, title, false);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 0);

  // Message
  lv_obj_t *msg_label = tron_theme_create_label(modal, message, false);
  lv_obj_set_width(msg_label, 340);                      // Leave margins
  lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP); // Enable text wrapping
  lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER,
                              0); // Center the wrapped text
  lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, -10);

  // Close button
  lv_obj_t *btn = tron_theme_create_button(modal, "OK", true);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Add event to close the modal
  lv_obj_add_event_cb(btn, close_dialog_cb, LV_EVENT_CLICKED, modal);
}

// Login menu callback implementations
static void load_mnemonic_cb(void) {
  ESP_LOGI(TAG, "Load mnemonic menu item selected");

  // Hide the login menu
  login_page_hide();

  // Create and show the QR scanner page
  qr_scanner_page_create(lv_screen_active(), return_from_qr_scanner_cb);
  qr_scanner_page_show();
}

static void new_mnemonic_cb(void) {
  // TODO: Implement
  show_simple_dialog("Login", "New mnemonic not implemented yet");
}

static void settings_cb(void) {
  // TODO: Implement
  show_simple_dialog("Login", "Settings not implemented yet");
}

static void tools_cb(void) {
  // TODO: Implement
  show_simple_dialog("Login", "Tools not implemented yet");
}

static void about_cb(void) {
  ESP_LOGI(TAG, "About menu item selected");

  // Hide the login menu
  login_page_hide();

  // Create and show the about page
  about_page_create(lv_screen_active(), return_to_login_cb);
  about_page_show();
}

static void shutdown_cb(void) {
  // TODO: Implement
  show_simple_dialog("Login", "Shutdown not implemented yet");
}

void login_page_create(lv_obj_t *parent) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for login page");
    return;
  }

  ESP_LOGI(TAG, "Creating login page");

  // Create screen container
  login_screen = lv_obj_create(parent);
  lv_obj_set_size(login_screen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(login_screen, lv_color_hex(0x1e1e1e), 0);
  // Remove container borders and padding
  lv_obj_set_style_border_width(login_screen, 0, 0);
  lv_obj_set_style_pad_all(login_screen, 0, 0);
  lv_obj_set_style_radius(login_screen, 0, 0);
  lv_obj_set_style_shadow_width(login_screen, 0, 0);
  lv_obj_clear_flag(login_screen, LV_OBJ_FLAG_SCROLLABLE);

  // Create the login menu
  login_menu = ui_menu_create(login_screen, "Login");
  if (!login_menu) {
    ESP_LOGE(TAG, "Failed to create login menu");
    return;
  }

  // Add menu entries with their respective callbacks
  if (!ui_menu_add_entry(login_menu, "Load Mnemonic", load_mnemonic_cb)) {
    ESP_LOGE(TAG, "Failed to add menu entry");
  }

  if (!ui_menu_add_entry(login_menu, "New Mnemonic", new_mnemonic_cb)) {
    ESP_LOGE(TAG, "Failed to add menu entry");
  }

  if (!ui_menu_add_entry(login_menu, "Settings", settings_cb)) {
    ESP_LOGE(TAG, "Failed to add menu entry");
  }

  if (!ui_menu_add_entry(login_menu, "Tools", tools_cb)) {
    ESP_LOGE(TAG, "Failed to add menu entry");
  }

  if (!ui_menu_add_entry(login_menu, "About", about_cb)) {
    ESP_LOGE(TAG, "Failed to add menu entry");
  }

  if (!ui_menu_add_entry(login_menu, "Shutdown", shutdown_cb)) {
    ESP_LOGE(TAG, "Failed to add menu entry");
  }

  // Show the menu
  ui_menu_show(login_menu);

  ESP_LOGI(TAG, "Login page created successfully with %d menu entries",
           ui_menu_get_selected(login_menu) >= 0 ? 6 : 0);
}

void login_page_show(void) {
  if (login_menu) {
    ui_menu_show(login_menu);
    ESP_LOGI(TAG, "Login page shown");
  } else {
    ESP_LOGE(TAG, "Login menu not initialized");
  }
}

void login_page_hide(void) {
  if (login_menu) {
    ui_menu_hide(login_menu);
    ESP_LOGI(TAG, "Login page hidden");
  } else {
    ESP_LOGE(TAG, "Login menu not initialized");
  }
}

void login_page_destroy(void) {
  if (login_menu) {
    ui_menu_destroy(login_menu);
    login_menu = NULL;
    ESP_LOGI(TAG, "Login menu destroyed");
  }

  if (login_screen) {
    lv_obj_del(login_screen);
    login_screen = NULL;
    ESP_LOGI(TAG, "Login screen destroyed");
  }
}

bool login_page_navigate_next(void) {
  if (login_menu) {
    return ui_menu_navigate_next(login_menu);
  }
  return false;
}

bool login_page_navigate_prev(void) {
  if (login_menu) {
    return ui_menu_navigate_prev(login_menu);
  }
  return false;
}

bool login_page_execute_selected(void) {
  if (login_menu) {
    return ui_menu_execute_selected(login_menu);
  }
  return false;
}

int login_page_get_selected(void) {
  if (login_menu) {
    return ui_menu_get_selected(login_menu);
  }
  return -1;
}
