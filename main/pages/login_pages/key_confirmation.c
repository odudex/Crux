#include "key_confirmation.h"
#include "../../key/key.h"
#include "../../ui_components/flash_error.h"
#include "../../ui_components/icons/icons_24.h"
#include "../../ui_components/theme.h"
#include "../../ui_components/ui_input_helpers.h"
#include "../../ui_components/ui_key_info.h"
#include "../../utils/memory_utils.h"
#include "../../utils/mnemonic_qr.h"
#include "../../wallet/wallet.h"
#include "passphrase.h"
#include <lvgl.h>
#include <string.h>
#include <wally_bip32.h>
#include <wally_bip39.h>
#include <wally_core.h>

#define TOP_BAR_HEIGHT 100
#define PADDING 10

static lv_obj_t *key_confirmation_screen = NULL;
static lv_obj_t *network_dropdown = NULL;
static lv_obj_t *passphrase_btn = NULL;
static lv_obj_t *title_cont = NULL;
static lv_obj_t *derivation_label = NULL;

static void (*return_callback)(void) = NULL;
static void (*success_callback)(void) = NULL;
static char *mnemonic_content = NULL;
static char *stored_passphrase = NULL;
static char base_fingerprint_hex[9] = {0};
static wallet_network_t selected_network = WALLET_NETWORK_MAINNET;

static void back_btn_cb(lv_event_t *e) {
  (void)e;
  if (return_callback)
    return_callback();
}

static void update_derivation_path(void) {
  if (!derivation_label)
    return;
  const char *path = (selected_network == WALLET_NETWORK_MAINNET)
                         ? "m/84'/0'/0'"
                         : "m/84'/1'/0'";
  lv_label_set_text(derivation_label, path);
}

static void network_dropdown_cb(lv_event_t *e) {
  uint16_t sel = lv_dropdown_get_selected(lv_event_get_target(e));
  selected_network =
      (sel == 0) ? WALLET_NETWORK_MAINNET : WALLET_NETWORK_TESTNET;
  update_derivation_path();
}

static void dropdown_open_cb(lv_event_t *e) {
  lv_obj_t *list = lv_dropdown_get_list(lv_event_get_target(e));
  if (list) {
    lv_obj_set_style_bg_color(list, disabled_color(), 0);
    lv_obj_set_style_text_color(list, main_color(), 0);
    lv_obj_set_style_bg_color(list, highlight_color(),
                              LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(list, highlight_color(),
                              LV_PART_SELECTED | LV_STATE_PRESSED);
  }
}

static void passphrase_return_cb(void) {
  passphrase_page_destroy();
  key_confirmation_page_show();
}

static void add_fingerprint_pair(lv_obj_t *parent, const char *fp_hex,
                                 bool highlighted) {
  lv_color_t color = highlighted ? highlight_color() : secondary_color();
  ui_icon_text_row_create(parent, ICON_FINGERPRINT, fp_hex, color);
}

static void update_title_with_passphrase(const char *passphrase) {
  if (!title_cont || !mnemonic_content)
    return;

  // Clear existing content
  lv_obj_clean(title_cont);

  // If no passphrase, show only base fingerprint (highlighted)
  if (!passphrase || passphrase[0] == '\0') {
    add_fingerprint_pair(title_cont, base_fingerprint_hex, true);
    return;
  }

  // Calculate fingerprint with passphrase
  unsigned char seed[BIP39_SEED_LEN_512];
  struct ext_key *master_key = NULL;

  if (bip39_mnemonic_to_seed512(mnemonic_content, passphrase, seed,
                                sizeof(seed)) != WALLY_OK) {
    return;
  }

  if (bip32_key_from_seed_alloc(seed, sizeof(seed), BIP32_VER_MAIN_PRIVATE, 0,
                                &master_key) != WALLY_OK) {
    memset(seed, 0, sizeof(seed));
    return;
  }

  unsigned char fingerprint[BIP32_KEY_FINGERPRINT_LEN];
  bip32_key_get_fingerprint(master_key, fingerprint, BIP32_KEY_FINGERPRINT_LEN);
  memset(seed, 0, sizeof(seed));
  bip32_key_free(master_key);

  char *passphrase_fp_hex = NULL;
  if (wally_hex_from_bytes(fingerprint, BIP32_KEY_FINGERPRINT_LEN,
                           &passphrase_fp_hex) == WALLY_OK) {
    // Base fingerprint (not highlighted)
    add_fingerprint_pair(title_cont, base_fingerprint_hex, false);

    // Arrow separator
    lv_obj_t *arrow = lv_label_create(title_cont);
    lv_label_set_text(arrow, ">");
    lv_obj_set_style_text_font(arrow, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(arrow, secondary_color(), 0);

    // Passphrase fingerprint (highlighted)
    add_fingerprint_pair(title_cont, passphrase_fp_hex, true);

    wally_free_string(passphrase_fp_hex);
  }
}

static void passphrase_success_cb(const char *passphrase) {
  // Store the passphrase
  if (stored_passphrase) {
    memset(stored_passphrase, 0, strlen(stored_passphrase));
    free(stored_passphrase);
    stored_passphrase = NULL;
  }

  if (passphrase && passphrase[0] != '\0') {
    stored_passphrase = strdup(passphrase);
  }

  passphrase_page_destroy();
  key_confirmation_page_show();

  // Update title to show both fingerprints
  update_title_with_passphrase(stored_passphrase);
}

static void passphrase_btn_cb(lv_event_t *e) {
  (void)e;
  key_confirmation_page_hide();
  passphrase_page_create(lv_screen_active(), passphrase_return_cb,
                         passphrase_success_cb);
}

static void load_btn_cb(lv_event_t *e) {
  (void)e;
  bool is_testnet = (selected_network == WALLET_NETWORK_TESTNET);
  if (key_load_from_mnemonic(mnemonic_content, stored_passphrase, is_testnet)) {
    if (!wallet_init(selected_network)) {
      show_flash_error("Failed to initialize wallet", return_callback, 0);
      return;
    }
    if (success_callback)
      success_callback();
  } else {
    show_flash_error("Failed to load key", return_callback, 0);
  }
}

static lv_obj_t *create_column(lv_obj_t *parent, lv_coord_t width) {
  lv_obj_t *col = lv_obj_create(parent);
  lv_obj_set_size(col, width, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(col, 0, 0);
  lv_obj_set_style_pad_left(col, 5, 0);
  lv_obj_set_style_pad_right(col, 0, 0);
  lv_obj_set_style_pad_ver(col, 0, 0);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
  return col;
}

static void add_word_label(lv_obj_t *parent, int num, const char *word) {
  char word_line[32];
  snprintf(word_line, sizeof(word_line), "%2d. %s", num, word);
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, word_line);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, secondary_color(), 0);
}

static int count_words(const char *str) {
  int count = 0;
  char temp[512];
  strncpy(temp, str, sizeof(temp) - 1);
  temp[sizeof(temp) - 1] = '\0';
  for (char *w = strtok(temp, " "); w; w = strtok(NULL, " "))
    count++;
  return count;
}

static void create_ui(const char *fingerprint_hex) {
  key_confirmation_screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(key_confirmation_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(key_confirmation_screen);
  lv_obj_clear_flag(key_confirmation_screen, LV_OBJ_FLAG_SCROLLABLE);

  // Top bar
  lv_obj_t *top = lv_obj_create(key_confirmation_screen);
  lv_obj_set_size(top, LV_PCT(100), TOP_BAR_HEIGHT);
  lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(top, 0, 0);
  lv_obj_set_style_pad_all(top, 0, 0);
  lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

  ui_create_back_button(top, back_btn_cb);

  // Container for both fingerprint and derivation rows (vertical)
  lv_obj_t *header_cont = lv_obj_create(top);
  lv_obj_set_size(header_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(header_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(header_cont, 0, 0);
  lv_obj_set_style_pad_all(header_cont, 0, 0);
  lv_obj_set_flex_flow(header_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(header_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(header_cont, 4, 0);
  lv_obj_clear_flag(header_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(header_cont, LV_ALIGN_CENTER, 0, 0);

  // Container for icon + fingerprint pair(s)
  title_cont = lv_obj_create(header_cont);
  lv_obj_set_size(title_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(title_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(title_cont, 0, 0);
  lv_obj_set_style_pad_all(title_cont, 0, 0);
  lv_obj_set_flex_flow(title_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(title_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(title_cont, 8, 0);
  lv_obj_clear_flag(title_cont, LV_OBJ_FLAG_SCROLLABLE);

  // Add initial fingerprint (highlighted)
  add_fingerprint_pair(title_cont, fingerprint_hex, true);

  // Derivation path row
  lv_obj_t *deriv_cont = ui_icon_text_row_create(
      header_cont, ICON_DERIVATION, "m/84'/0'/0'", secondary_color());
  // Keep reference to text label for updates when network changes
  derivation_label = lv_obj_get_child(deriv_cont, 1);

  lv_coord_t half_width = LV_HOR_RES / 2;
  int word_count = mnemonic_content ? count_words(mnemonic_content) : 0;

  // Left container - mnemonic words
  lv_obj_t *left = lv_obj_create(key_confirmation_screen);
  lv_obj_set_size(left, half_width, LV_VER_RES - TOP_BAR_HEIGHT - PADDING);
  lv_obj_align(left, LV_ALIGN_TOP_LEFT, 0, TOP_BAR_HEIGHT + PADDING);
  lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(left, 0, 0);
  lv_obj_set_style_pad_left(left, 5, 0);
  lv_obj_set_style_pad_right(left, 0, 0);
  lv_obj_set_style_pad_ver(left, 5, 0);
  lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

  if (mnemonic_content) {
    char mnemonic_copy[512];
    strncpy(mnemonic_copy, mnemonic_content, sizeof(mnemonic_copy) - 1);
    mnemonic_copy[sizeof(mnemonic_copy) - 1] = '\0';

    if (word_count > 12) {
      lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);

      lv_obj_t *left_col = create_column(left, half_width / 2);
      lv_obj_t *right_col = create_column(left, half_width / 2);

      int num = 1;
      for (char *w = strtok(mnemonic_copy, " "); w; w = strtok(NULL, " ")) {
        add_word_label((num <= 12) ? left_col : right_col, num, w);
        num++;
      }
    } else {
      lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                            LV_FLEX_ALIGN_START);

      int num = 1;
      for (char *w = strtok(mnemonic_copy, " "); w; w = strtok(NULL, " ")) {
        add_word_label(left, num++, w);
      }
    }
  }

  // Right container - network dropdown + load button
  lv_obj_t *right = lv_obj_create(key_confirmation_screen);
  lv_obj_set_size(right, half_width, LV_VER_RES - TOP_BAR_HEIGHT - PADDING);
  lv_obj_align(right, LV_ALIGN_TOP_RIGHT, 0, TOP_BAR_HEIGHT + PADDING);
  lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(right, 0, 0);
  lv_obj_set_style_pad_all(right, 0, 0);
  lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(right, 15, 0);

  passphrase_btn = lv_btn_create(right);
  lv_obj_set_size(passphrase_btn, LV_PCT(80), 50);
  lv_obj_set_style_margin_bottom(passphrase_btn, 20, 0);
  theme_apply_touch_button(passphrase_btn, false);
  lv_obj_add_event_cb(passphrase_btn, passphrase_btn_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *pp_label = lv_label_create(passphrase_btn);
  lv_label_set_text(pp_label, "Passphrase");
  lv_obj_set_style_text_font(pp_label, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_color(pp_label, main_color(), 0);
  lv_obj_center(pp_label);

  lv_obj_t *net_label = lv_label_create(right);
  lv_label_set_text(net_label, "Network");
  lv_obj_set_style_text_font(net_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(net_label, secondary_color(), 0);

  network_dropdown = lv_dropdown_create(right);
  lv_dropdown_set_options(network_dropdown, "Mainnet\nTestnet");
  lv_obj_set_width(network_dropdown, LV_PCT(80));
  lv_obj_set_style_bg_color(network_dropdown, disabled_color(), 0);
  lv_obj_set_style_text_color(network_dropdown, main_color(), 0);
  lv_obj_set_style_text_font(network_dropdown, &lv_font_montserrat_24, 0);
  lv_obj_set_style_border_color(network_dropdown, highlight_color(), 0);
  lv_obj_add_event_cb(network_dropdown, dropdown_open_cb, LV_EVENT_READY, NULL);
  lv_obj_add_event_cb(network_dropdown, network_dropdown_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t *load_btn = lv_btn_create(right);
  lv_obj_set_size(load_btn, LV_PCT(80), 70);
  theme_apply_touch_button(load_btn, false);
  lv_obj_set_style_margin_top(load_btn, 140, 0);
  lv_obj_add_event_cb(load_btn, load_btn_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *load_label = lv_label_create(load_btn);
  lv_label_set_text(load_label, "Load");
  lv_obj_set_style_text_font(load_label, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_color(load_label, main_color(), 0);
  lv_obj_center(load_label);
}

void key_confirmation_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                  void (*success_cb)(void), const char *content,
                                  size_t content_len) {
  (void)parent;
  return_callback = return_cb;
  success_callback = success_cb;
  selected_network = WALLET_NETWORK_MAINNET;

  SAFE_FREE_STATIC(mnemonic_content);
  mnemonic_content = mnemonic_qr_to_mnemonic(content, content_len, NULL);

  if (!mnemonic_content) {
    show_flash_error("Invalid mnemonic phrase", return_callback, 0);
    return;
  }

  unsigned char fingerprint[BIP32_KEY_FINGERPRINT_LEN];
  unsigned char seed[BIP39_SEED_LEN_512];
  struct ext_key *master_key = NULL;

  if (bip39_mnemonic_to_seed512(mnemonic_content, NULL, seed, sizeof(seed)) !=
          WALLY_OK ||
      bip32_key_from_seed_alloc(seed, sizeof(seed), BIP32_VER_MAIN_PRIVATE, 0,
                                &master_key) != WALLY_OK) {
    memset(seed, 0, sizeof(seed));
    show_flash_error("Failed to process mnemonic", return_callback, 0);
    return;
  }

  bip32_key_get_fingerprint(master_key, fingerprint, BIP32_KEY_FINGERPRINT_LEN);
  memset(seed, 0, sizeof(seed));
  bip32_key_free(master_key);

  char *fingerprint_hex = NULL;
  if (wally_hex_from_bytes(fingerprint, BIP32_KEY_FINGERPRINT_LEN,
                           &fingerprint_hex) != WALLY_OK) {
    show_flash_error("Failed to format fingerprint", return_callback, 0);
    return;
  }

  // Store base fingerprint for later use
  strncpy(base_fingerprint_hex, fingerprint_hex,
          sizeof(base_fingerprint_hex) - 1);
  base_fingerprint_hex[sizeof(base_fingerprint_hex) - 1] = '\0';

  create_ui(fingerprint_hex);
  wally_free_string(fingerprint_hex);
}

void key_confirmation_page_show(void) {
  if (key_confirmation_screen)
    lv_obj_clear_flag(key_confirmation_screen, LV_OBJ_FLAG_HIDDEN);
}

void key_confirmation_page_hide(void) {
  if (key_confirmation_screen)
    lv_obj_add_flag(key_confirmation_screen, LV_OBJ_FLAG_HIDDEN);
}

void key_confirmation_page_destroy(void) {
  SAFE_FREE_STATIC(mnemonic_content);

  // Securely clear passphrase
  if (stored_passphrase) {
    memset(stored_passphrase, 0, strlen(stored_passphrase));
    free(stored_passphrase);
    stored_passphrase = NULL;
  }

  if (key_confirmation_screen) {
    lv_obj_del(key_confirmation_screen);
    key_confirmation_screen = NULL;
  }
  network_dropdown = NULL;
  passphrase_btn = NULL;
  title_cont = NULL;
  derivation_label = NULL;
  memset(base_fingerprint_hex, 0, sizeof(base_fingerprint_hex));
  return_callback = NULL;
  success_callback = NULL;
  selected_network = WALLET_NETWORK_MAINNET;
}
