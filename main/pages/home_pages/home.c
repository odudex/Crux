#include "home.h"
#include "../../key/key.h"
#include "../../ui_components/prompt_dialog.h"
#include "../../ui_components/theme.h"
#include "../../ui_components/ui_input_helpers.h"
#include "../../ui_components/ui_key_info.h"
#include "../../ui_components/ui_menu.h"
#include "../../wallet/wallet.h"
#include "addresses.h"
#include "backup/mnemonic_words.h"
#include "public_key.h"
#include "sign.h"
#include <esp_log.h>
#include <esp_system.h>

static lv_obj_t *home_screen = NULL;
static lv_obj_t *power_button = NULL;
static ui_menu_t *main_menu = NULL;
static void menu_backup_cb(void);
static void menu_xpub_cb(void);
static void menu_addresses_cb(void);
static void menu_sign_cb(void);
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

static void reboot_confirmed_cb(bool result, void *user_data) {
  (void)user_data;
  if (result) {
    key_unload();
    esp_restart();
  }
}

static void power_button_cb(lv_event_t *e) {
  (void)e;
  show_prompt_dialog_overlay("Unload key and reboot?", reboot_confirmed_cb,
                             NULL);
}

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
  if (!parent || !key_is_loaded() || !wallet_is_initialized())
    return;

  home_screen = theme_create_page_container(parent);

  main_menu = ui_menu_create(home_screen, "", NULL);
  if (!main_menu)
    return;

  // Replace empty title with key info header
  lv_obj_add_flag(main_menu->title_label,
                  LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_t *header = ui_key_info_create(main_menu->container);
  lv_obj_move_to_index(header, 0);

  ui_menu_add_entry(main_menu, "Sign", menu_sign_cb);
  ui_menu_add_entry(main_menu, "Extended Public Key", menu_xpub_cb);
  ui_menu_add_entry(main_menu, "Addresses", menu_addresses_cb);
  ui_menu_add_entry(main_menu, "Back Up", menu_backup_cb);

  // Power button (reboot) at top-left
  power_button = ui_create_power_button(home_screen, power_button_cb);
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
  if (power_button) {
    lv_obj_del(power_button);
    power_button = NULL;
  }

  if (main_menu) {
    ui_menu_destroy(main_menu);
    main_menu = NULL;
  }

  if (home_screen) {
    lv_obj_del(home_screen);
    home_screen = NULL;
  }
}
