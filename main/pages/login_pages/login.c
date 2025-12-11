/*
 * Login Page - Sample test case for UI Menu System
 * Demonstrates how to use the generic UI menu component
 */

#include "login.h"
#include "../../ui_components/simple_dialog.h"
#include "../../ui_components/theme.h"
#include "../../ui_components/ui_menu.h"
#include "about.h"
#include "esp_log.h"
#include "load_pages/load_menu.h"
#include "new_mnemonic_pages/new_mnemonic_menu.h"
#include "lvgl.h"
#include <string.h>

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

// Load menu callback function
static void return_from_load_menu_cb(void);

// New mnemonic menu callback function
static void return_from_new_mnemonic_menu_cb(void);

// Helper function to return from about page to login menu
static void return_to_login_cb(void) {
  ESP_LOGI(TAG, "Returning from about page to login menu");
  about_page_destroy();
  login_page_show();
}

// Helper function to return from load menu page to login menu
static void return_from_load_menu_cb(void) {
  ESP_LOGI(TAG, "Returning from load menu page to login menu");
  login_page_show();
}

// Helper function to return from new mnemonic menu page to login menu
static void return_from_new_mnemonic_menu_cb(void) {
  ESP_LOGI(TAG, "Returning from new mnemonic menu page to login menu");
  login_page_show();
}

// Login menu callback implementations
static void load_mnemonic_cb(void) {
  ESP_LOGI(TAG, "Load mnemonic menu item selected");

  // Hide the login menu
  login_page_hide();

  // Create and show the load menu page
  load_menu_page_create(lv_screen_active(), return_from_load_menu_cb);
  load_menu_page_show();
}

static void new_mnemonic_cb(void) {
  ESP_LOGI(TAG, "New mnemonic menu item selected");

  // Hide the login menu
  login_page_hide();

  // Create and show the new mnemonic menu page
  new_mnemonic_menu_page_create(lv_screen_active(), return_from_new_mnemonic_menu_cb);
  new_mnemonic_menu_page_show();
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
