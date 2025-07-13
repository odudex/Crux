/*
 * Dark Theme Implementation
 * Centralized dark mode styling for the entire application
 */

#include "dark_theme.h"
#include "esp_log.h"

static const char *TAG = "DARK_THEME";

// Global dark theme configuration
static dark_theme_config_t dark_theme_config = {
    // Background colors
    .bg_primary = {.red = 0x1e, .green = 0x1e, .blue = 0x1e},
    .bg_secondary = {.red = 0x2e, .green = 0x2e, .blue = 0x2e},
    .bg_tertiary = {.red = 0x3e, .green = 0x3e, .blue = 0x3e},
    .border_color = {.red = 0x55, .green = 0x55, .blue = 0x55},
    
    // Text colors
    .text_primary = {.red = 0xff, .green = 0xff, .blue = 0xff},
    .text_secondary = {.red = 0xcc, .green = 0xcc, .blue = 0xcc},
    .text_disabled = {.red = 0x88, .green = 0x88, .blue = 0x88},
    
    // Accent colors
    .accent_color = {.red = 0x00, .green = 0x7a, .blue = 0xcc},
    .success_color = {.red = 0x28, .green = 0xa7, .blue = 0x45},
    .warning_color = {.red = 0xff, .green = 0xc1, .blue = 0x07},
    .error_color = {.red = 0xdc, .green = 0x35, .blue = 0x45},
    
    // Button colors
    .btn_normal = {.red = 0x3e, .green = 0x3e, .blue = 0x3e},
    .btn_pressed = {.red = 0x4e, .green = 0x4e, .blue = 0x4e},
    .btn_disabled = {.red = 0x2a, .green = 0x2a, .blue = 0x2a},
    .btn_focused = {.red = 0x4a, .green = 0x4a, .blue = 0x4a},
    
    // Input colors
    .input_bg = {.red = 0x2a, .green = 0x2a, .blue = 0x2a},
    .input_border = {.red = 0x55, .green = 0x55, .blue = 0x55},
    .input_focused = {.red = 0x00, .green = 0x7a, .blue = 0xcc},
    
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
