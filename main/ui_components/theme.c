#include "theme.h"

// Theme colors - Deep blacks with electric blues and cyans
#define COLOR_BG_VOID lv_color_hex(0x000000)      // Pure black void
#define COLOR_BG_GRID lv_color_hex(0x001122)      // Dark grid background
#define COLOR_BG_PANEL lv_color_hex(0x001a33)     // Panel background
#define COLOR_NEON_BLUE lv_color_hex(0x00ffff)    // Electric cyan
#define COLOR_NEON_ORANGE lv_color_hex(0xff6600)  // Orange accent
#define COLOR_GRID_LINE lv_color_hex(0x003366)    // Subtle grid lines
#define COLOR_TEXT_PRIMARY lv_color_hex(0x888888) // Grey text
#define COLOR_BUTTON lv_color_hex(0x00ccff)       // Cyan
#define COLOR_TEXT_GLOW lv_color_hex(0x66ddff)    // Glowing text
#define COLOR_DISABLED lv_color_hex(0x336666)     // COLOR_DISABLED elements
#define COLOR_ERROR lv_color_hex(0xFF0000)        // Pure red for errors
#define COLOR_NO lv_color_hex(0xFF0000)  // Pure red for negative actions
#define COLOR_YES lv_color_hex(0x00FF00) // Lime for positive actions

void theme_init(void) {}

lv_color_t main_color(void) { return COLOR_BUTTON; }

lv_color_t highlight_color(void) { return COLOR_NEON_ORANGE; }

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

  // Deep void background
  lv_obj_set_style_bg_color(obj, COLOR_BG_VOID, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, COLOR_TEXT_PRIMARY, 0);

  // Set default font size for the screen
  lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, 0);

  // No borders or outlines
  lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_outline_width(obj, 0, 0);
}

void theme_apply_frame(lv_obj_t *target_frame) {
  if (!target_frame)
    return;

  // Dark panel with glowing neon border
  lv_obj_set_style_bg_color(target_frame, COLOR_BG_PANEL, 0);
  lv_obj_set_style_bg_opa(target_frame, LV_OPA_90, 0);

  // Neon blue border
  lv_obj_set_style_border_color(target_frame, COLOR_NEON_BLUE, 0);
  lv_obj_set_style_border_width(target_frame, 2, 0);
  lv_obj_set_style_border_opa(target_frame, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(target_frame, 6, 0);

  // Glowing shadow effect
  // lv_obj_set_style_shadow_width(target_frame, 2, 0);
  // lv_obj_set_style_shadow_color(target_frame, COLOR_NEON_BLUE, 0);
  // lv_obj_set_style_shadow_opa(target_frame, LV_OPA_40, 0);
  // lv_obj_set_style_shadow_spread(target_frame, 2, 0);
}

void theme_apply_solid_rectangle(lv_obj_t *target_rectangle) {
  if (!target_rectangle)
    return;

  // Solid dark rectangle with no border
  lv_obj_set_style_bg_color(target_rectangle, COLOR_BG_PANEL, 0);
  lv_obj_set_style_bg_opa(target_rectangle, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(target_rectangle, 2, 0);

  // Ensure no outlines
  lv_obj_set_style_outline_opa(target_rectangle, LV_OPA_TRANSP, 0);
  lv_obj_set_style_outline_width(target_rectangle, 0, 0);

  // Ensure no padding/margins
  lv_obj_set_style_pad_all(target_rectangle, 0, 0);
  lv_obj_set_style_margin_all(target_rectangle, 0, 0);

  // No border or outline
  lv_obj_set_style_border_opa(target_rectangle, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(target_rectangle, 0, 0);

  // No shadow
  lv_obj_set_style_shadow_width(target_rectangle, 0, 0);
  lv_obj_set_style_shadow_opa(target_rectangle, LV_OPA_TRANSP, 0);
}

void theme_apply_label(lv_obj_t *label, bool is_secondary) {
  if (!label)
    return;

  lv_color_t text_color = is_secondary ? COLOR_TEXT_GLOW : COLOR_TEXT_PRIMARY;
  lv_obj_set_style_text_color(label, text_color, 0);

  // Set font size for labels
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

  // Explicitly disable borders and outlines that could cause the 1px frame
  lv_obj_set_style_border_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(label, 0, 0);
  lv_obj_set_style_outline_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_outline_width(label, 0, 0);

  // Ensure background is transparent
  lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);

  // Remove shadow that was causing the 1px frame
  lv_obj_set_style_shadow_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_width(label, 0, 0);

  // Add subtle glow effect to text
  lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
}

void theme_apply_button_label(lv_obj_t *label, bool is_secondary) {
  if (!label)
    return;

  lv_color_t text_color = is_secondary ? COLOR_TEXT_GLOW : COLOR_BUTTON;
  lv_obj_set_style_text_color(label, text_color, 0);

  // Use 48pt font for COLOR_BUTTON labels
  lv_obj_set_style_text_font(label, &lv_font_montserrat_36, 0);

  // Explicitly disable borders and outlines that could cause the 1px frame
  lv_obj_set_style_border_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(label, 0, 0);
  lv_obj_set_style_outline_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_outline_width(label, 0, 0);

  // Ensure background is transparent
  lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);

  // Remove shadow that was causing the 1px frame
  lv_obj_set_style_shadow_opa(label, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_width(label, 0, 0);

  // Add subtle glow effect to text
  lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
}

void theme_apply_touch_button(lv_obj_t *btn, bool is_primary) {
  if (!btn)
    return;

  // Normal state - clean background with subtle glow
  lv_color_t bg_color = COLOR_BG_VOID;
  lv_color_t accent_color = is_primary ? COLOR_NEON_ORANGE : COLOR_NEON_BLUE;

  lv_obj_set_style_bg_color(btn, bg_color, LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn, LV_OPA_50, LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(btn, COLOR_TEXT_GLOW, LV_STATE_DEFAULT);

  // Subtle 1px border with text color
  lv_obj_set_style_border_color(btn, COLOR_TEXT_PRIMARY, LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(btn, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_STATE_DEFAULT);
  lv_obj_set_style_outline_opa(btn, LV_OPA_TRANSP, LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_radius(btn, 12, LV_STATE_DEFAULT);

  // Padding for better touch targets
  lv_obj_set_style_pad_all(btn, 15, LV_STATE_DEFAULT);

  // Add subtle glow effect to COLOR_BUTTONs
  lv_obj_set_style_shadow_width(btn, 8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_color(btn, bg_color, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_opa(btn, LV_OPA_50, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_spread(btn, 2, LV_STATE_DEFAULT);

  // Pressed state - brighter highlight with accent color and stronger glow
  lv_obj_set_style_bg_color(btn, accent_color, LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_50, LV_STATE_PRESSED);
  // lv_obj_set_style_text_color(btn, lv_color_white(), LV_STATE_PRESSED);

  // Enhanced glow when pressed
  lv_obj_set_style_shadow_width(btn, 8, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_color(btn, accent_color, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_opa(btn, LV_OPA_50, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_spread(btn, 2, LV_STATE_PRESSED);

  // Focus state - same as default for touch interfaces
  lv_obj_set_style_bg_color(btn, bg_color, LV_STATE_FOCUSED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_30, LV_STATE_FOCUSED);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);

  // COLOR_DISABLED state
  lv_obj_set_style_bg_opa(btn, LV_OPA_10, LV_STATE_DISABLED);
  lv_obj_set_style_text_color(btn, COLOR_DISABLED, LV_STATE_DISABLED);
  lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP,
                              LV_STATE_DISABLED); // No glow when COLOR_DISABLED
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
  lv_obj_set_size(separator, LV_PCT(90), 1); // 2 pixels thick horizontal line
  lv_obj_set_style_bg_color(separator, COLOR_NEON_BLUE, 0);
  lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(separator, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(separator, 0, 0);
  lv_obj_set_style_radius(separator, 0, 0);

  // Add glowing shadow effect
  lv_obj_set_style_shadow_width(separator, 6, 0);
  lv_obj_set_style_shadow_color(separator, COLOR_NEON_BLUE, 0);
  lv_obj_set_style_shadow_opa(separator, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_spread(separator, 2, 0);

  return separator;
}
