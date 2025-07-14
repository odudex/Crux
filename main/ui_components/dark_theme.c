/*
 * Dark Theme Implementation
 * Centralized dark mode styling for the entire application
 */

#include "dark_theme.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "DARK_THEME";

// Global dark theme configuration - using header constants
static dark_theme_config_t dark_theme_config = {
    // Background colors
    .bg_primary = {.red = (DARK_THEME_BG_PRIMARY >> 16) & 0xFF, .green = (DARK_THEME_BG_PRIMARY >> 8) & 0xFF, .blue = DARK_THEME_BG_PRIMARY & 0xFF},
    .bg_secondary = {.red = (DARK_THEME_BG_SECONDARY >> 16) & 0xFF, .green = (DARK_THEME_BG_SECONDARY >> 8) & 0xFF, .blue = DARK_THEME_BG_SECONDARY & 0xFF},
    .bg_tertiary = {.red = (DARK_THEME_BG_TERTIARY >> 16) & 0xFF, .green = (DARK_THEME_BG_TERTIARY >> 8) & 0xFF, .blue = DARK_THEME_BG_TERTIARY & 0xFF},
    .border_color = {.red = (DARK_THEME_BORDER_COLOR >> 16) & 0xFF, .green = (DARK_THEME_BORDER_COLOR >> 8) & 0xFF, .blue = DARK_THEME_BORDER_COLOR & 0xFF},
    
    // Text colors
    .text_primary = {.red = (DARK_THEME_TEXT_PRIMARY >> 16) & 0xFF, .green = (DARK_THEME_TEXT_PRIMARY >> 8) & 0xFF, .blue = DARK_THEME_TEXT_PRIMARY & 0xFF},
    .text_secondary = {.red = (DARK_THEME_TEXT_SECONDARY >> 16) & 0xFF, .green = (DARK_THEME_TEXT_SECONDARY >> 8) & 0xFF, .blue = DARK_THEME_TEXT_SECONDARY & 0xFF},
    .text_disabled = {.red = (DARK_THEME_TEXT_DISABLED >> 16) & 0xFF, .green = (DARK_THEME_TEXT_DISABLED >> 8) & 0xFF, .blue = DARK_THEME_TEXT_DISABLED & 0xFF},
    
    // Accent colors
    .accent_color = {.red = (DARK_THEME_ACCENT_COLOR >> 16) & 0xFF, .green = (DARK_THEME_ACCENT_COLOR >> 8) & 0xFF, .blue = DARK_THEME_ACCENT_COLOR & 0xFF},
    .success_color = {.red = (DARK_THEME_SUCCESS_COLOR >> 16) & 0xFF, .green = (DARK_THEME_SUCCESS_COLOR >> 8) & 0xFF, .blue = DARK_THEME_SUCCESS_COLOR & 0xFF},
    .warning_color = {.red = (DARK_THEME_WARNING_COLOR >> 16) & 0xFF, .green = (DARK_THEME_WARNING_COLOR >> 8) & 0xFF, .blue = DARK_THEME_WARNING_COLOR & 0xFF},
    .error_color = {.red = (DARK_THEME_ERROR_COLOR >> 16) & 0xFF, .green = (DARK_THEME_ERROR_COLOR >> 8) & 0xFF, .blue = DARK_THEME_ERROR_COLOR & 0xFF},
    
    // Button colors
    .btn_normal = {.red = (DARK_THEME_BTN_NORMAL >> 16) & 0xFF, .green = (DARK_THEME_BTN_NORMAL >> 8) & 0xFF, .blue = DARK_THEME_BTN_NORMAL & 0xFF},
    .btn_pressed = {.red = (DARK_THEME_BTN_PRESSED >> 16) & 0xFF, .green = (DARK_THEME_BTN_PRESSED >> 8) & 0xFF, .blue = DARK_THEME_BTN_PRESSED & 0xFF},
    .btn_disabled = {.red = (DARK_THEME_BTN_DISABLED >> 16) & 0xFF, .green = (DARK_THEME_BTN_DISABLED >> 8) & 0xFF, .blue = DARK_THEME_BTN_DISABLED & 0xFF},
    .btn_focused = {.red = (DARK_THEME_BTN_FOCUSED >> 16) & 0xFF, .green = (DARK_THEME_BTN_FOCUSED >> 8) & 0xFF, .blue = DARK_THEME_BTN_FOCUSED & 0xFF},
    
    // Input colors
    .input_bg = {.red = (DARK_THEME_INPUT_BG >> 16) & 0xFF, .green = (DARK_THEME_INPUT_BG >> 8) & 0xFF, .blue = DARK_THEME_INPUT_BG & 0xFF},
    .input_border = {.red = (DARK_THEME_INPUT_BORDER >> 16) & 0xFF, .green = (DARK_THEME_INPUT_BORDER >> 8) & 0xFF, .blue = DARK_THEME_INPUT_BORDER & 0xFF},
    .input_focused = {.red = (DARK_THEME_INPUT_FOCUSED >> 16) & 0xFF, .green = (DARK_THEME_INPUT_FOCUSED >> 8) & 0xFF, .blue = DARK_THEME_INPUT_FOCUSED & 0xFF},
    
    // Common styling properties
    .border_radius = 8,
    .border_width = 2,
    .padding_small = 5,
    .padding_medium = 10,
    .padding_large = 20,
    .margin_small = 5,
    .margin_medium = 10,
    .margin_large = 20
};

const dark_theme_config_t* dark_theme_init(void)
{
    ESP_LOGI(TAG, "Dark theme initialized");
    return &dark_theme_config;
}

const dark_theme_config_t* dark_theme_get_config(void)
{
    return &dark_theme_config;
}

void dark_theme_apply_screen(lv_obj_t *obj)
{
    if (!obj) return;
    
    lv_obj_set_style_bg_color(obj, dark_theme_config.bg_primary, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, dark_theme_config.text_primary, 0);
}

void dark_theme_apply_button(lv_obj_t *btn, bool is_primary)
{
    if (!btn) return;
    
    // Normal state
    lv_color_t bg_color = is_primary ? dark_theme_config.accent_color : dark_theme_config.btn_normal;
    lv_obj_set_style_bg_color(btn, bg_color, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(btn, dark_theme_config.text_primary, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, dark_theme_config.border_color, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, dark_theme_config.border_width, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, dark_theme_config.border_radius, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, dark_theme_config.padding_medium, LV_STATE_DEFAULT);
    
    // Pressed state
    lv_color_t pressed_color = is_primary ? 
        lv_color_darken(dark_theme_config.accent_color, LV_OPA_30) : 
        dark_theme_config.btn_pressed;
    lv_obj_set_style_bg_color(btn, pressed_color, LV_STATE_PRESSED);
    
    // Remove focused state styling for touch interfaces
    // This prevents the blue contour that appears after button press
    lv_obj_set_style_bg_color(btn, bg_color, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, dark_theme_config.border_color, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn, dark_theme_config.border_width, LV_STATE_FOCUSED);
    
    // Disabled state
    lv_obj_set_style_bg_color(btn, dark_theme_config.btn_disabled, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(btn, dark_theme_config.text_disabled, LV_STATE_DISABLED);
}

void dark_theme_apply_label(lv_obj_t *label, bool is_secondary)
{
    if (!label) return;
    
    lv_color_t text_color = is_secondary ? dark_theme_config.text_secondary : dark_theme_config.text_primary;
    lv_obj_set_style_text_color(label, text_color, 0);
}

void dark_theme_apply_container(lv_obj_t *container)
{
    if (!container) return;
    
    lv_obj_set_style_bg_color(container, dark_theme_config.bg_secondary, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(container, dark_theme_config.border_color, 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_radius(container, dark_theme_config.border_radius, 0);
    lv_obj_set_style_pad_all(container, dark_theme_config.padding_medium, 0);
}

void dark_theme_apply_modal(lv_obj_t *modal)
{
    if (!modal) return;
    
    lv_obj_set_style_bg_color(modal, dark_theme_config.bg_secondary, 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(modal, dark_theme_config.border_color, 0);
    lv_obj_set_style_border_width(modal, dark_theme_config.border_width, 0);
    lv_obj_set_style_radius(modal, dark_theme_config.border_radius, 0);
    lv_obj_set_style_pad_all(modal, dark_theme_config.padding_large, 0);
    lv_obj_set_style_shadow_width(modal, 20, 0);
    lv_obj_set_style_shadow_color(modal, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(modal, LV_OPA_50, 0);
}

void dark_theme_apply_input(lv_obj_t *input)
{
    if (!input) return;
    
    lv_obj_set_style_bg_color(input, dark_theme_config.input_bg, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(input, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(input, dark_theme_config.input_border, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(input, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(input, dark_theme_config.border_radius, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(input, dark_theme_config.padding_medium, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(input, dark_theme_config.text_primary, LV_STATE_DEFAULT);
    
    // Focused state
    lv_obj_set_style_border_color(input, dark_theme_config.input_focused, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(input, dark_theme_config.border_width, LV_STATE_FOCUSED);
}

lv_obj_t* dark_theme_create_button(lv_obj_t *parent, const char *text, bool is_primary)
{
    if (!parent) return NULL;
    
    lv_obj_t *btn = lv_btn_create(parent);
    // Use touch-friendly button styling by default
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

lv_obj_t* dark_theme_create_container(lv_obj_t *parent)
{
    if (!parent) return NULL;
    
    lv_obj_t *container = lv_obj_create(parent);
    dark_theme_apply_container(container);
    
    return container;
}

void dark_theme_disable_focus(lv_obj_t *obj)
{
    if (!obj) return;
    
    // Clear the object from any input group to prevent keyboard focus
    lv_group_t *group = lv_obj_get_group(obj);
    if (group) {
        lv_group_remove_obj(obj);
    }
    
    // Remove focus flag if it exists
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);
}

void dark_theme_apply_touch_button(lv_obj_t *btn, bool is_primary)
{
    if (!btn) return;
    
    // Apply standard button styling
    dark_theme_apply_button(btn, is_primary);
    
    // Disable focus for touch interface
    dark_theme_disable_focus(btn);
}
