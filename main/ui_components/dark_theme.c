#include "dark_theme.h"

// Dark theme colors (simplified)
#define BG_PRIMARY     lv_color_hex(0x1e1e1e)
#define BG_SECONDARY   lv_color_hex(0x2e2e2e)
#define BG_BUTTON      lv_color_hex(0x3e3e3e)
#define BG_BUTTON_PRESSED lv_color_hex(0x4e4e4e)
#define BORDER_COLOR   lv_color_hex(0x555555)
#define TEXT_PRIMARY   lv_color_hex(0xffffff)
#define TEXT_SECONDARY lv_color_hex(0xcccccc)
#define ACCENT_COLOR   lv_color_hex(0x007acc)

void dark_theme_init(void)
{
    // Theme initialization - nothing needed for simplified version
}

void dark_theme_apply_screen(lv_obj_t *obj)
{
    if (!obj) return;
    
    lv_obj_set_style_bg_color(obj, BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, TEXT_PRIMARY, 0);
}

void dark_theme_apply_modal(lv_obj_t *modal)
{
    if (!modal) return;
    
    lv_obj_set_style_bg_color(modal, BG_SECONDARY, 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(modal, BORDER_COLOR, 0);
    lv_obj_set_style_border_width(modal, 2, 0);
    lv_obj_set_style_radius(modal, 8, 0);
    lv_obj_set_style_pad_all(modal, 20, 0);
    lv_obj_set_style_shadow_width(modal, 20, 0);
    lv_obj_set_style_shadow_color(modal, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(modal, LV_OPA_50, 0);
}

void dark_theme_apply_label(lv_obj_t *label, bool is_secondary)
{
    if (!label) return;
    
    lv_color_t text_color = is_secondary ? TEXT_SECONDARY : TEXT_PRIMARY;
    lv_obj_set_style_text_color(label, text_color, 0);
}

void dark_theme_apply_touch_button(lv_obj_t *btn, bool is_primary)
{
    if (!btn) return;
    
    // Normal state
    lv_color_t bg_color = is_primary ? ACCENT_COLOR : BG_BUTTON;
    lv_obj_set_style_bg_color(btn, bg_color, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(btn, TEXT_PRIMARY, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, BORDER_COLOR, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 10, LV_STATE_DEFAULT);
    
    // Pressed state
    lv_color_t pressed_color = is_primary ? 
        lv_color_darken(ACCENT_COLOR, LV_OPA_30) : BG_BUTTON_PRESSED;
    lv_obj_set_style_bg_color(btn, pressed_color, LV_STATE_PRESSED);
    
    // Disable focus styling for touch interfaces
    lv_obj_set_style_bg_color(btn, bg_color, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, BORDER_COLOR, LV_STATE_FOCUSED);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
}

lv_obj_t* dark_theme_create_button(lv_obj_t *parent, const char *text, bool is_primary)
{
    if (!parent) return NULL;
    
    lv_obj_t *btn = lv_btn_create(parent);
    dark_theme_apply_touch_button(btn, is_primary);
    
    if (text) {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_center(label);
        dark_theme_apply_label(label, false);
    }
    
    return btn;
}

lv_obj_t* dark_theme_create_label(lv_obj_t *parent, const char *text, bool is_secondary)
{
    if (!parent) return NULL;
    
    lv_obj_t *label = lv_label_create(parent);
    if (text) {
        lv_label_set_text(label, text);
    }
    dark_theme_apply_label(label, is_secondary);
    
    return label;
}
