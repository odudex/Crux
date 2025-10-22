#include "home.h"
#include "../../key/key.h"
#include "../../ui_components/theme.h"
#include "../../ui_components/ui_menu.h"
#include "../../wallet/wallet.h"
#include "addresses.h"
#include "backup/mnemonic_words.h"
#include "public_key.h"
#include "sign.h"
#include <esp_log.h>
#include <esp_system.h>

static lv_obj_t *home_screen = NULL;
static ui_menu_t *main_menu = NULL;
static void menu_backup_cb(void);
static void menu_xpub_cb(void);
static void menu_addresses_cb(void);
static void menu_sign_cb(void);
static void menu_reboot_cb(void);
static void return_from_mnemonic_words_cb(void);
static void return_from_public_key_cb(void);
static void return_from_addresses_cb(void);
static void return_from_sign_cb(void);

static void menu_backup_cb(void) {
  home_page_hide();
  mnemonic_words_page_create(lv_screen_active(), return_from_mnemonic_words_cb);
  mnemonic_words_page_show();
}

static void menu_xpub_cb(void) {
  home_page_hide();
  public_key_page_create(lv_screen_active(), return_from_public_key_cb);
  public_key_page_show();
}

static void menu_addresses_cb(void) {
  home_page_hide();
  addresses_page_create(lv_screen_active(), return_from_addresses_cb);
  addresses_page_show();
}

static void menu_sign_cb(void) {
  home_page_hide();
  sign_page_create(lv_screen_active(), return_from_sign_cb);
  sign_page_show();
}

static void menu_reboot_cb(void) { esp_restart(); }

static void return_from_mnemonic_words_cb(void) {
  mnemonic_words_page_destroy();
  home_page_show();
}

static void return_from_public_key_cb(void) {
  public_key_page_destroy();
  home_page_show();
}

static void return_from_addresses_cb(void) {
  addresses_page_destroy();
  home_page_show();
}

static void return_from_sign_cb(void) {
  sign_page_destroy();
  home_page_show();
}

void home_page_create(lv_obj_t *parent) {
  if (!parent || !key_is_loaded() || !wallet_is_initialized()) {
    return;
  }

  char fingerprint_hex[BIP32_KEY_FINGERPRINT_LEN * 2 + 1];
  if (!key_get_fingerprint_hex(fingerprint_hex)) {
    return;
  }

  wallet_network_t network = wallet_get_network();
  const char *network_str =
      (network == WALLET_NETWORK_MAINNET) ? "Mainnet" : "Testnet";

  char title[64];
  snprintf(title, sizeof(title), "%s - %s", fingerprint_hex, network_str);

  home_screen = lv_obj_create(parent);
  lv_obj_set_size(home_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(home_screen);

  main_menu = ui_menu_create(home_screen, title);
  if (!main_menu) {
    return;
  }

  ui_menu_add_entry(main_menu, "Back Up", menu_backup_cb);
  ui_menu_add_entry(main_menu, "Extended Public Key", menu_xpub_cb);
  ui_menu_add_entry(main_menu, "Addresses", menu_addresses_cb);
  ui_menu_add_entry(main_menu, "Sign", menu_sign_cb);
  ui_menu_add_entry(main_menu, "Reboot", menu_reboot_cb);
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
  if (main_menu) {
    ui_menu_destroy(main_menu);
    main_menu = NULL;
  }

  if (home_screen) {
    lv_obj_del(home_screen);
    home_screen = NULL;
  }
}
