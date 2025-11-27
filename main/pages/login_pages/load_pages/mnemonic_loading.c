#include "mnemonic_loading.h"
#include "../../../key/key.h"
#include "../../../ui_components/flash_error.h"
#include "../../../ui_components/prompt_dialog.h"
#include "../../../ui_components/theme.h"
#include "../../../ui_components/ui_menu.h"
#include "../../../utils/memory_utils.h"
#include "../../../wallet/wallet.h"
#include <esp_log.h>
#include <lvgl.h>
#include <string.h>
#include <wally_bip32.h>
#include <wally_bip39.h>
#include <wally_crypto.h>

static lv_obj_t *mnemonic_loading_screen = NULL;
static ui_menu_t *main_menu = NULL;
static void (*return_callback)(void) = NULL;
static void (*success_callback)(void) = NULL;
static char *qr_content = NULL;
static unsigned char key_fingerprint[BIP32_KEY_FINGERPRINT_LEN];
static bool is_valid_mnemonic = false;
static wallet_network_t selected_network = WALLET_NETWORK_MAINNET;
static void menu_load_wallet_cb(void);
static void menu_network_toggle_cb(void);
static void menu_back_cb(void);
static int calculate_fingerprint_from_mnemonic(const char *mnemonic,
                                               unsigned char *fingerprint_out);
static void update_network_menu_label(void);

static int calculate_fingerprint_from_mnemonic(const char *mnemonic,
                                               unsigned char *fingerprint_out) {
  int ret;
  unsigned char seed[BIP39_SEED_LEN_512];
  struct ext_key *master_key = NULL;

  ret = bip39_mnemonic_to_seed512(mnemonic, NULL, seed, sizeof(seed));
  if (ret != WALLY_OK) {
    return ret;
  }

  ret = bip32_key_from_seed_alloc(seed, sizeof(seed), BIP32_VER_MAIN_PRIVATE, 0,
                                  &master_key);
  if (ret != WALLY_OK) {
    memset(seed, 0, sizeof(seed));
    return ret;
  }

  ret = bip32_key_get_fingerprint(master_key, fingerprint_out,
                                  BIP32_KEY_FINGERPRINT_LEN);

  memset(seed, 0, sizeof(seed));
  bip32_key_free(master_key);

  return ret;
}

static void menu_load_wallet_cb(void) {
  bool is_testnet = (selected_network == WALLET_NETWORK_TESTNET);
  if (key_load_from_mnemonic(qr_content, NULL, is_testnet)) {
    if (!wallet_init(selected_network)) {
      show_flash_error("Failed to initialize wallet", return_callback, 0);
      return;
    }

    if (success_callback) {
      success_callback();
    }
  } else {
    show_flash_error("Failed to load key", return_callback, 0);
  }
}

static void menu_network_toggle_cb(void) {
  selected_network = (selected_network == WALLET_NETWORK_MAINNET)
                         ? WALLET_NETWORK_TESTNET
                         : WALLET_NETWORK_MAINNET;
  update_network_menu_label();
}

static void menu_back_cb(void) {
  if (return_callback) {
    return_callback();
  }
}

static void update_network_menu_label(void) {
  if (!main_menu) {
    return;
  }

  const char *network_label =
      (selected_network == WALLET_NETWORK_MAINNET) ? "Mainnet" : "Testnet";

  if (main_menu->config.entry_count > 1) {
    snprintf(main_menu->config.entries[1].name, UI_MENU_ENTRY_NAME_MAX_LEN,
             "%s", network_label);

    if (main_menu->buttons[1]) {
      lv_obj_t *label = lv_obj_get_child(main_menu->buttons[1], 0);
      if (label) {
        lv_label_set_text(label, network_label);
      }
    }
  }
}

void mnemonic_loading_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                  void (*success_cb)(void),
                                  const char *content) {
  if (!parent) {
    return;
  }

  return_callback = return_cb;
  success_callback = success_cb;
  selected_network = WALLET_NETWORK_MAINNET;

  SAFE_FREE_STATIC(qr_content);
  qr_content = SAFE_STRDUP(content);

  is_valid_mnemonic =
      (qr_content && bip39_mnemonic_validate(NULL, qr_content) == WALLY_OK);

  if (!is_valid_mnemonic) {
    show_flash_error("Invalid mnemonic phrase", return_callback, 0);
    return;
  }

  int ret = calculate_fingerprint_from_mnemonic(qr_content, key_fingerprint);
  if (ret != WALLY_OK) {
    show_flash_error("Failed to process mnemonic", return_callback, 0);
    return;
  }

  char *fingerprint_hex = NULL;
  ret = wally_hex_from_bytes(key_fingerprint, BIP32_KEY_FINGERPRINT_LEN,
                             &fingerprint_hex);
  if (ret != WALLY_OK || !fingerprint_hex) {
    show_flash_error("Failed to format fingerprint", return_callback, 0);
    return;
  }

  mnemonic_loading_screen = lv_obj_create(parent);
  lv_obj_set_size(mnemonic_loading_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(mnemonic_loading_screen);

  main_menu = ui_menu_create(mnemonic_loading_screen, "Key Confirmation");
  if (!main_menu) {
    wally_free_string(fingerprint_hex);
    return;
  }

  char load_label[64];
  snprintf(load_label, sizeof(load_label), "Load %s", fingerprint_hex);

  ui_menu_add_entry(main_menu, load_label, menu_load_wallet_cb);
  ui_menu_add_entry(main_menu, "Mainnet", menu_network_toggle_cb);
  ui_menu_add_entry(main_menu, "Back", menu_back_cb);

  wally_free_string(fingerprint_hex);
}

void mnemonic_loading_page_show(void) {
  if (mnemonic_loading_screen) {
    lv_obj_clear_flag(mnemonic_loading_screen, LV_OBJ_FLAG_HIDDEN);
  }
  if (main_menu) {
    ui_menu_show(main_menu);
  }
}

void mnemonic_loading_page_hide(void) {
  if (mnemonic_loading_screen) {
    lv_obj_add_flag(mnemonic_loading_screen, LV_OBJ_FLAG_HIDDEN);
  }
  if (main_menu) {
    ui_menu_hide(main_menu);
  }
}

void mnemonic_loading_page_destroy(void) {
  SAFE_FREE_STATIC(qr_content);

  memset(key_fingerprint, 0, sizeof(key_fingerprint));
  is_valid_mnemonic = false;

  if (main_menu) {
    ui_menu_destroy(main_menu);
    main_menu = NULL;
  }

  if (mnemonic_loading_screen) {
    lv_obj_del(mnemonic_loading_screen);
    mnemonic_loading_screen = NULL;
  }

  return_callback = NULL;
  success_callback = NULL;
  selected_network = WALLET_NETWORK_MAINNET;
}
