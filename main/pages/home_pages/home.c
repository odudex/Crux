/*
 * Home Page
 * Main page displayed after key is loaded
 */

#include "home.h"
#include "../../key/key.h"
#include "../../ui_components/theme.h"
#include "../../ui_components/ui_menu.h"
#include "public_key.h"
#include <esp_log.h>
#include <esp_system.h>

static const char *TAG = "HOME";

// UI components
static lv_obj_t *home_screen = NULL;
static ui_menu_t *main_menu = NULL;

// Forward declarations for menu callbacks
static void menu_backup_cb(void);
static void menu_xpub_cb(void);
static void menu_sign_cb(void);
static void menu_reboot_cb(void);

// Forward declaration for return callbacks
static void return_from_public_key_cb(void);

// Menu callback implementations
static void menu_backup_cb(void) {
  ESP_LOGI(TAG, "Back Up selected");
  // TODO: Implement backup functionality
}

static void menu_xpub_cb(void) {
  ESP_LOGI(TAG, "Extended Public Key selected");

  // Hide home page
  home_page_hide();

  // Create and show public key page
  public_key_page_create(lv_screen_active(), return_from_public_key_cb);
  public_key_page_show();
}

static void menu_sign_cb(void) {
  ESP_LOGI(TAG, "Sign selected");
  // TODO: Implement signing functionality
}

static void menu_reboot_cb(void) {
  ESP_LOGI(TAG, "Reboot selected - restarting system");
  esp_restart();
}

// Return callback from public key page
static void return_from_public_key_cb(void) {
  ESP_LOGI(TAG, "Returning from public key page to home");

  // Destroy public key page
  public_key_page_destroy();

  // Show home page
  home_page_show();
}

void home_page_create(lv_obj_t *parent) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for home page");
    return;
  }

  // Verify key is loaded
  if (!key_is_loaded()) {
    ESP_LOGE(TAG, "Cannot create home page: no key loaded");
    return;
  }

  // Get key fingerprint for menu title
  char fingerprint_hex[BIP32_KEY_FINGERPRINT_LEN * 2 + 1];
  if (!key_get_fingerprint_hex(fingerprint_hex)) {
    ESP_LOGE(TAG, "Failed to get key fingerprint");
    return;
  }

  // Create the main screen
  home_screen = lv_obj_create(parent);
  lv_obj_set_size(home_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(home_screen);

  // Create the main menu with fingerprint as title
  main_menu = ui_menu_create(home_screen, fingerprint_hex);
  if (!main_menu) {
    ESP_LOGE(TAG, "Failed to create main menu");
    return;
  }

  // Add menu entries
  ui_menu_add_entry(main_menu, "Back Up", menu_backup_cb);
  ui_menu_add_entry(main_menu, "Extended Public Key", menu_xpub_cb);
  ui_menu_add_entry(main_menu, "Sign", menu_sign_cb);
  ui_menu_add_entry(main_menu, "Reboot", menu_reboot_cb);

  ESP_LOGI(TAG, "Home page created for key: %s", fingerprint_hex);

  ESP_LOGI(TAG, "Home page created successfully");
}

void home_page_show(void) {
  if (home_screen) {
    lv_obj_clear_flag(home_screen, LV_OBJ_FLAG_HIDDEN);
  }
  if (main_menu) {
    ui_menu_show(main_menu);
  }
}

void home_page_hide(void) {
  if (home_screen) {
    lv_obj_add_flag(home_screen, LV_OBJ_FLAG_HIDDEN);
  }
  if (main_menu) {
    ui_menu_hide(main_menu);
  }
}

void home_page_destroy(void) {
  ESP_LOGI(TAG, "Destroying home page");

  // Destroy menu
  if (main_menu) {
    ui_menu_destroy(main_menu);
    main_menu = NULL;
  }

  // Destroy screen
  if (home_screen) {
    lv_obj_del(home_screen);
    home_screen = NULL;
  }

  ESP_LOGI(TAG, "Home page destroyed");
}
