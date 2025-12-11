/*
 * UI Keyboard Component
 * A grid-based keyboard using lv_buttonmatrix for letter input
 */

#include "ui_keyboard.h"
#include "theme.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "UI_KEYBOARD";

// QWERTY keyboard layout:
// q w e r t y u i o p
// a s d f g h j k l
// z x c v b n m   < OK
static const char *kb_map[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};

// Map button position to character (QWERTY order)
static const char btn_to_char[] = {
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  // row 1: 0-9
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',       // row 2: 10-18
    'z', 'x', 'c', 'v', 'b', 'n', 'm',                 // row 3: 19-25
    '\b', '\n'                                          // backspace, OK: 26-27
};

// Map character to key index (for enable/disable by letter)
static int char_to_key_index(char c) {
  if (c >= 'a' && c <= 'z') {
    return c - 'a';  // UI_KB_KEY_A = 0, etc.
  }
  return -1;
}

// Map button position to key index
static int get_key_index_from_btn(uint32_t btn_id) {
  if (btn_id >= sizeof(btn_to_char)) {
    return -1;
  }
  char c = btn_to_char[btn_id];
  if (c >= 'a' && c <= 'z') {
    return c - 'a';  // Returns UI_KB_KEY_A through UI_KB_KEY_Z
  } else if (c == '\b') {
    return UI_KB_KEY_BACKSPACE;
  } else if (c == '\n') {
    return UI_KB_KEY_OK;
  }
  return -1;
}

// Get character for button position
static char get_char_for_btn(uint32_t btn_id) {
  if (btn_id < sizeof(btn_to_char)) {
    return btn_to_char[btn_id];
  }
  return 0;
}

// Find button position for a key index
static int get_btn_for_key_index(int key_index) {
  char target;
  if (key_index >= UI_KB_KEY_A && key_index <= UI_KB_KEY_Z) {
    target = 'a' + key_index;
  } else if (key_index == UI_KB_KEY_BACKSPACE) {
    target = '\b';
  } else if (key_index == UI_KB_KEY_OK) {
    target = '\n';
  } else {
    return -1;
  }

  for (size_t i = 0; i < sizeof(btn_to_char); i++) {
    if (btn_to_char[i] == target) {
      return (int)i;
    }
  }
  return -1;
}

// Event handler for button matrix
static void kb_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btnm = lv_event_get_target(e);
  ui_keyboard_t *kb = (ui_keyboard_t *)lv_event_get_user_data(e);

  if (code == LV_EVENT_VALUE_CHANGED) {
    uint32_t btn_id = lv_buttonmatrix_get_selected_button(btnm);
    if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE) {
      return;
    }

    int key_index = get_key_index_from_btn(btn_id);
    if (key_index < 0) {
      return;
    }

    // Check if key is enabled
    if (!kb->enabled_keys[key_index]) {
      ESP_LOGD(TAG, "Key %d is disabled", key_index);
      return;
    }

    char key_char = get_char_for_btn(btn_id);
    ESP_LOGI(TAG, "Key pressed: btn=%lu char='%c' (0x%02x)", (unsigned long)btn_id,
             key_char >= 32 ? key_char : '?', key_char);

    if (kb->callback && key_char != 0) {
      kb->callback(key_char);
    }
  }
}

ui_keyboard_t *ui_keyboard_create(lv_obj_t *parent, const char *title,
                                  ui_keyboard_callback_t callback) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent");
    return NULL;
  }

  ui_keyboard_t *kb = (ui_keyboard_t *)malloc(sizeof(ui_keyboard_t));
  if (!kb) {
    ESP_LOGE(TAG, "Failed to allocate keyboard");
    return NULL;
  }
  memset(kb, 0, sizeof(ui_keyboard_t));

  kb->callback = callback;

  // Enable all keys by default
  for (int i = 0; i < UI_KB_KEY_COUNT; i++) {
    kb->enabled_keys[i] = true;
  }

  // No container - create elements directly on parent for full width usage
  kb->container = parent; // Store parent reference for show/hide

  // Create title label at top
  kb->title_label = lv_label_create(parent);
  lv_label_set_text(kb->title_label, title ? title : "");
  lv_obj_set_style_text_color(kb->title_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(kb->title_label, &lv_font_montserrat_24, 0);
  lv_obj_align(kb->title_label, LV_ALIGN_TOP_MID, 0, 5);

  // Create input display label below title
  kb->input_label = lv_label_create(parent);
  lv_label_set_text(kb->input_label, "_");
  lv_obj_set_style_text_color(kb->input_label, highlight_color(), 0);
  lv_obj_set_style_text_font(kb->input_label, &lv_font_montserrat_36, 0);
  lv_obj_align(kb->input_label, LV_ALIGN_TOP_MID, 0, 35);

  // Create button matrix - full width, positioned below labels
  kb->btnmatrix = lv_buttonmatrix_create(parent);
  lv_buttonmatrix_set_map(kb->btnmatrix, kb_map);
  lv_obj_align(kb->btnmatrix, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_size(kb->btnmatrix, LV_PCT(100), LV_PCT(50));
  lv_obj_set_style_pad_all(kb->btnmatrix, 4, 0);
  lv_obj_set_style_pad_row(kb->btnmatrix, 6, 0);
  lv_obj_set_style_pad_column(kb->btnmatrix, 6, 0);

  // Style the button matrix background - transparent/black
  lv_obj_set_style_bg_opa(kb->btnmatrix, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(kb->btnmatrix, 0, 0);
  lv_obj_set_style_shadow_width(kb->btnmatrix, 0, 0);

  // Style enabled buttons - clean, no borders/shadows
  lv_obj_set_style_bg_color(kb->btnmatrix, lv_color_hex(0x333333), LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(kb->btnmatrix, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(kb->btnmatrix, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
  lv_obj_set_style_text_font(kb->btnmatrix, &lv_font_montserrat_24, LV_PART_ITEMS);
  lv_obj_set_style_radius(kb->btnmatrix, 6, LV_PART_ITEMS);
  lv_obj_set_style_border_width(kb->btnmatrix, 0, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(kb->btnmatrix, 0, LV_PART_ITEMS);
  lv_obj_set_style_outline_width(kb->btnmatrix, 0, LV_PART_ITEMS);

  // Style for pressed state
  lv_obj_set_style_bg_color(kb->btnmatrix, highlight_color(), LV_PART_ITEMS | LV_STATE_PRESSED);

  // Style for focused/checked state (touch selection highlight)
  lv_obj_set_style_bg_color(kb->btnmatrix, highlight_color(), LV_PART_ITEMS | LV_STATE_CHECKED);

  // Style for disabled state - black/transparent bg, only gray text visible
  lv_obj_set_style_bg_opa(kb->btnmatrix, LV_OPA_TRANSP, LV_PART_ITEMS | LV_STATE_DISABLED);
  lv_obj_set_style_text_color(kb->btnmatrix, lv_color_hex(0x666666), LV_PART_ITEMS | LV_STATE_DISABLED);

  // Add event handler
  lv_obj_add_event_cb(kb->btnmatrix, kb_event_handler, LV_EVENT_VALUE_CHANGED, kb);

  // Enable click trigger mode for better responsiveness
  lv_buttonmatrix_set_button_ctrl_all(kb->btnmatrix, LV_BUTTONMATRIX_CTRL_CLICK_TRIG);

  // Set initial selection
  lv_buttonmatrix_set_selected_button(kb->btnmatrix, 0);

  ESP_LOGI(TAG, "Keyboard created successfully");
  return kb;
}

void ui_keyboard_set_title(ui_keyboard_t *kb, const char *title) {
  if (kb && kb->title_label) {
    lv_label_set_text(kb->title_label, title ? title : "");
  }
}

void ui_keyboard_set_input_text(ui_keyboard_t *kb, const char *text) {
  if (kb && kb->input_label) {
    if (text && text[0] != '\0') {
      // Show text with underscore cursor
      char buf[64];
      snprintf(buf, sizeof(buf), "%s_", text);
      lv_label_set_text(kb->input_label, buf);
    } else {
      lv_label_set_text(kb->input_label, "_");
    }
  }
}

void ui_keyboard_set_key_enabled(ui_keyboard_t *kb, int key_index, bool enabled) {
  if (!kb || key_index < 0 || key_index >= UI_KB_KEY_COUNT) {
    return;
  }

  kb->enabled_keys[key_index] = enabled;

  if (kb->btnmatrix) {
    // Find the button position for this key index (QWERTY layout)
    int btn_pos = get_btn_for_key_index(key_index);
    if (btn_pos < 0) {
      return;
    }

    if (enabled) {
      lv_buttonmatrix_clear_button_ctrl(kb->btnmatrix, (uint32_t)btn_pos,
                                        LV_BUTTONMATRIX_CTRL_DISABLED);
    } else {
      lv_buttonmatrix_set_button_ctrl(kb->btnmatrix, (uint32_t)btn_pos,
                                      LV_BUTTONMATRIX_CTRL_DISABLED);
    }
  }
}

void ui_keyboard_set_letters_enabled(ui_keyboard_t *kb, uint32_t letter_mask) {
  if (!kb) {
    return;
  }

  for (int i = 0; i < 26; i++) {
    bool enabled = (letter_mask & (1u << i)) != 0;
    ui_keyboard_set_key_enabled(kb, UI_KB_KEY_A + i, enabled);
  }
}

void ui_keyboard_enable_all(ui_keyboard_t *kb) {
  if (!kb) {
    return;
  }

  for (int i = 0; i < UI_KB_KEY_COUNT; i++) {
    ui_keyboard_set_key_enabled(kb, i, true);
  }
}

void ui_keyboard_set_ok_enabled(ui_keyboard_t *kb, bool enabled) {
  ui_keyboard_set_key_enabled(kb, UI_KB_KEY_OK, enabled);
}

bool ui_keyboard_navigate_next(ui_keyboard_t *kb) {
  if (!kb || !kb->btnmatrix) {
    return false;
  }

  uint32_t current = lv_buttonmatrix_get_selected_button(kb->btnmatrix);
  uint32_t total = sizeof(btn_to_char);

  // Find next enabled button
  for (uint32_t i = 1; i <= total; i++) {
    uint32_t next = (current + i) % total;
    int key_index = get_key_index_from_btn(next);
    if (key_index >= 0 && kb->enabled_keys[key_index]) {
      lv_buttonmatrix_set_selected_button(kb->btnmatrix, next);
      return true;
    }
  }

  return false;
}

bool ui_keyboard_navigate_prev(ui_keyboard_t *kb) {
  if (!kb || !kb->btnmatrix) {
    return false;
  }

  uint32_t current = lv_buttonmatrix_get_selected_button(kb->btnmatrix);
  uint32_t total = sizeof(btn_to_char);

  // Find previous enabled button
  for (uint32_t i = 1; i <= total; i++) {
    uint32_t prev = (current + total - i) % total;
    int key_index = get_key_index_from_btn(prev);
    if (key_index >= 0 && kb->enabled_keys[key_index]) {
      lv_buttonmatrix_set_selected_button(kb->btnmatrix, prev);
      return true;
    }
  }

  return false;
}

bool ui_keyboard_press_selected(ui_keyboard_t *kb) {
  if (!kb || !kb->btnmatrix) {
    return false;
  }

  uint32_t btn_id = lv_buttonmatrix_get_selected_button(kb->btnmatrix);
  if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE) {
    return false;
  }

  int key_index = get_key_index_from_btn(btn_id);
  if (key_index < 0 || !kb->enabled_keys[key_index]) {
    return false;
  }

  char key_char = get_char_for_btn(btn_id);
  if (kb->callback && key_char != 0) {
    kb->callback(key_char);
    return true;
  }

  return false;
}

void ui_keyboard_show(ui_keyboard_t *kb) {
  if (!kb) return;
  if (kb->title_label) lv_obj_clear_flag(kb->title_label, LV_OBJ_FLAG_HIDDEN);
  if (kb->input_label) lv_obj_clear_flag(kb->input_label, LV_OBJ_FLAG_HIDDEN);
  if (kb->btnmatrix) lv_obj_clear_flag(kb->btnmatrix, LV_OBJ_FLAG_HIDDEN);
}

void ui_keyboard_hide(ui_keyboard_t *kb) {
  if (!kb) return;
  if (kb->title_label) lv_obj_add_flag(kb->title_label, LV_OBJ_FLAG_HIDDEN);
  if (kb->input_label) lv_obj_add_flag(kb->input_label, LV_OBJ_FLAG_HIDDEN);
  if (kb->btnmatrix) lv_obj_add_flag(kb->btnmatrix, LV_OBJ_FLAG_HIDDEN);
}

void ui_keyboard_destroy(ui_keyboard_t *kb) {
  if (!kb) {
    return;
  }

  // Delete individual elements (not the parent/container)
  if (kb->btnmatrix) {
    lv_obj_del(kb->btnmatrix);
  }
  if (kb->input_label) {
    lv_obj_del(kb->input_label);
  }
  if (kb->title_label) {
    lv_obj_del(kb->title_label);
  }

  free(kb);
  ESP_LOGI(TAG, "Keyboard destroyed");
}
