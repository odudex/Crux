#include "theme.h"

// Minimalist theme colors
#define COLOR_BG lv_color_hex(0x000000)       // Black background
#define COLOR_PANEL lv_color_hex(0x1a1a1a)    // Dark gray panels
#define COLOR_WHITE lv_color_hex(0xFFFFFF)    // White text/borders
#define COLOR_GRAY lv_color_hex(0x888888)     // Gray info text
#define COLOR_ORANGE lv_color_hex(0xff6600)   // Orange accent
#define COLOR_DISABLED lv_color_hex(0x666666) // Gray disabled
#define COLOR_ERROR lv_color_hex(0xFF0000)    // Red for errors
#define COLOR_NO lv_color_hex(0xFF0000)       // Red for negative
#define COLOR_YES lv_color_hex(0x00FF00)      // Green for positive

void theme_init(void) {}

lv_color_t main_color(void) { return COLOR_WHITE; }

lv_color_t highlight_color(void) { return COLOR_ORANGE; }

lv_color_t error_color(void) { return COLOR_ERROR; }

lv_color_t yes_color(void) { return COLOR_YES; }

lv_color_t no_color(void) { return COLOR_NO; }

// Theme sizing constants
const lv_font_t *theme_get_button_font(void) { return &lv_font_montserrat_36; }

const lv_font_t *theme_get_dialog_text_font(void) {
  return &lv_font_montserrat_36;
}

int theme_get_button_width(void) { return 150; }

int theme_get_button_height(void) { return 50; }

int theme_get_button_spacing(void) { return 20; }

void theme_apply_screen(lv_obj_t *obj) {
  if (!obj)
    return;

  lv_obj_set_style_bg_color(obj, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, COLOR_WHITE, 0);
  lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_outline_width(obj, 0, 0);
}

void theme_apply_frame(lv_obj_t *target_frame) {
  if (!target_frame)
    return;

  lv_obj_set_style_bg_color(target_frame, COLOR_PANEL, 0);
  lv_obj_set_style_bg_opa(target_frame, LV_OPA_90, 0);
  lv_obj_set_style_border_color(target_frame, COLOR_WHITE, 0);
  lv_obj_set_style_border_width(target_frame, 2, 0);
  lv_obj_set_style_radius(target_frame, 6, 0);
}

void theme_apply_solid_rectangle(lv_obj_t *target_rectangle) {
  if (!target_rectangle)
    return;

  lv_obj_set_style_bg_color(target_rectangle, COLOR_PANEL, 0);
  lv_obj_set_style_bg_opa(target_rectangle, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(target_rectangle, 2, 0);
  lv_obj_set_style_border_width(target_rectangle, 0, 0);
  lv_obj_set_style_outline_width(target_rectangle, 0, 0);
  lv_obj_set_style_pad_all(target_rectangle, 0, 0);
  lv_obj_set_style_shadow_width(target_rectangle, 0, 0);
}

void theme_apply_label(lv_obj_t *label, bool is_secondary) {
  if (!label)
    return;

  lv_obj_set_style_text_color(label, COLOR_GRAY, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(label, 0, 0);
}

void theme_apply_button_label(lv_obj_t *label, bool is_secondary) {
  if (!label)
    return;

  lv_obj_set_style_text_color(label, COLOR_WHITE, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_36, 0);
  lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(label, 0, 0);
}

void theme_apply_touch_button(lv_obj_t *btn, bool is_primary) {
  if (!btn)
    return;

  // Default state - minimal transparent background
  lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn, LV_OPA_30, LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(btn, COLOR_WHITE, LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn, 12, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(btn, 15, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn, 0, LV_STATE_DEFAULT);

  // Pressed state - orange background
  lv_obj_set_style_bg_color(btn, COLOR_ORANGE, LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_50, LV_STATE_PRESSED);

  // Disabled state
  lv_obj_set_style_text_color(btn, COLOR_DISABLED, LV_STATE_DISABLED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_10, LV_STATE_DISABLED);

  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
}

lv_obj_t *theme_create_button(lv_obj_t *parent, const char *text,
                              bool is_primary) {
  if (!parent)
    return NULL;

  lv_obj_t *btn = lv_btn_create(parent);
  theme_apply_touch_button(btn, is_primary);

  if (text) {
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    theme_apply_label(label, false);
  }

  return btn;
}

lv_obj_t *theme_create_label(lv_obj_t *parent, const char *text,
                             bool is_secondary) {
  if (!parent)
    return NULL;

  lv_obj_t *label = lv_label_create(parent);
  if (text) {
    lv_label_set_text(label, text);
  }
  theme_apply_label(label, is_secondary);

  return label;
}

lv_obj_t *theme_create_separator(lv_obj_t *parent) {
  if (!parent)
    return NULL;

  lv_obj_t *separator = lv_obj_create(parent);
  lv_obj_set_size(separator, LV_PCT(90), 1);
  lv_obj_set_style_bg_color(separator, COLOR_WHITE, 0);
  lv_obj_set_style_bg_opa(separator, LV_OPA_50, 0);
  lv_obj_set_style_border_width(separator, 0, 0);
  lv_obj_set_style_radius(separator, 0, 0);

  return separator;
}
