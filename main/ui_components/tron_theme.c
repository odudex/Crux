#include "tron_theme.h"

// TRON theme colors - Deep blacks with electric blues and cyans
#define TRON_BG_VOID       lv_color_hex(0x000000)  // Pure black void
#define TRON_BG_GRID       lv_color_hex(0x001122)  // Dark grid background
#define TRON_BG_PANEL      lv_color_hex(0x001a33)  // Panel background
#define TRON_NEON_BLUE     lv_color_hex(0x00ffff)  // Electric cyan
#define TRON_NEON_ORANGE   lv_color_hex(0xff6600)  // Orange accent
#define TRON_GRID_LINE     lv_color_hex(0x003366)  // Subtle grid lines
#define TRON_TEXT_PRIMARY  lv_color_hex(0x888888)  // Grey text
#define TRON_BUTTON        lv_color_hex(0x00ccff)  // Cyan
#define TRON_TEXT_GLOW     lv_color_hex(0x66ddff)  // Glowing text
#define TRON_DISABLED      lv_color_hex(0x336666)  // Disabled elements

void tron_theme_init(void)
{
    // Theme initialization - nothing needed for now
}

void tron_theme_apply_screen(lv_obj_t *obj)
{
    if (!obj) return;
    
    // Deep void background
    lv_obj_set_style_bg_color(obj, TRON_BG_VOID, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, TRON_TEXT_PRIMARY, 0);
    
    // Set default font size for the screen
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, 0);  // or your preferred font
    
    // No borders or outlines - clean TRON aesthetic
    lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
}

void tron_theme_apply_modal(lv_obj_t *modal)
{
    if (!modal) return;
    
    // Dark panel with glowing neon border
    lv_obj_set_style_bg_color(modal, TRON_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_90, 0);
    
    // Sharp neon blue border - no radius for angular TRON look
    lv_obj_set_style_border_color(modal, TRON_NEON_BLUE, 0);
    lv_obj_set_style_border_width(modal, 2, 0);
    lv_obj_set_style_border_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(modal, 0, 0);  // Sharp corners
    
    // Padding for content
    lv_obj_set_style_pad_all(modal, 20, 0);
    
    // Glowing shadow effect
    lv_obj_set_style_shadow_width(modal, 30, 0);
    lv_obj_set_style_shadow_color(modal, TRON_NEON_BLUE, 0);
    lv_obj_set_style_shadow_opa(modal, LV_OPA_40, 0);
    lv_obj_set_style_shadow_spread(modal, 5, 0);
}

void tron_theme_apply_label(lv_obj_t *label, bool is_secondary)
{
    if (!label) return;
    
    lv_color_t text_color = is_secondary ? TRON_TEXT_GLOW : TRON_TEXT_PRIMARY;
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

void tron_theme_apply_button_label(lv_obj_t *label, bool is_secondary)
{
    if (!label) return;
    
    lv_color_t text_color = is_secondary ? TRON_TEXT_GLOW : TRON_BUTTON;
    lv_obj_set_style_text_color(label, text_color, 0);
    
    // Use 48pt font for button labels
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

void tron_theme_apply_touch_button(lv_obj_t *btn, bool is_primary)
{
    if (!btn) return;
    
    // Normal state - clean background with subtle glow
    lv_color_t bg_color = TRON_BG_VOID;
    lv_color_t accent_color = is_primary ? TRON_NEON_ORANGE : TRON_NEON_BLUE;
    
    lv_obj_set_style_bg_color(btn, bg_color, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_50, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(btn, TRON_TEXT_GLOW, LV_STATE_DEFAULT);
    
    // Subtle 1px border with text color
    lv_obj_set_style_border_color(btn, TRON_TEXT_PRIMARY, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(btn, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 12, LV_STATE_DEFAULT);
    
    // Padding for better touch targets
    lv_obj_set_style_pad_all(btn, 15, LV_STATE_DEFAULT);
    
    // Add subtle glow effect to buttons
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
    
    // Disabled state
    lv_obj_set_style_bg_opa(btn, LV_OPA_10, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(btn, TRON_DISABLED, LV_STATE_DISABLED);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, LV_STATE_DISABLED);  // No glow when disabled
}

lv_obj_t* tron_theme_create_button(lv_obj_t *parent, const char *text, bool is_primary)
{
    if (!parent) return NULL;
    
    lv_obj_t *btn = lv_btn_create(parent);
    tron_theme_apply_touch_button(btn, is_primary);
    
    if (text) {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_center(label);
        tron_theme_apply_label(label, false);
    }
    
    return btn;
}

lv_obj_t* tron_theme_create_label(lv_obj_t *parent, const char *text, bool is_secondary)
{
    if (!parent) return NULL;
    
    lv_obj_t *label = lv_label_create(parent);
    if (text) {
        lv_label_set_text(label, text);
    }
    tron_theme_apply_label(label, is_secondary);
    
    return label;
}

lv_obj_t* tron_theme_create_separator(lv_obj_t *parent)
{
    if (!parent) return NULL;
    
    lv_obj_t *separator = lv_obj_create(parent);
    lv_obj_set_size(separator, LV_PCT(90), 1);  // 2 pixels thick horizontal line
    lv_obj_set_style_bg_color(separator, TRON_NEON_BLUE, 0);
    lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_opa(separator, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(separator, 0, 0);
    lv_obj_set_style_radius(separator, 0, 0);
    
    // Add padding to prevent interference with other elements
    // lv_obj_set_style_pad_ver(separator, 8, 0);  // Vertical padding top/bottom
    // lv_obj_set_style_margin_ver(separator, 5, 0);  // Extra margin for spacing
    
    // Add glowing shadow effect
    lv_obj_set_style_shadow_width(separator, 6, 0);
    lv_obj_set_style_shadow_color(separator, TRON_NEON_BLUE, 0);
    lv_obj_set_style_shadow_opa(separator, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_spread(separator, 2, 0);
    
    return separator;
}
