/*
 * Dark Theme Configuration
 * Centralized dark mode styling for the entire application
 */

#ifndef DARK_THEME_H
#define DARK_THEME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Dark theme color palette
#define DARK_THEME_BG_PRIMARY       0x1e1e1e    // Main background
#define DARK_THEME_BG_SECONDARY     0x2e2e2e    // Card/modal backgrounds
#define DARK_THEME_BG_TERTIARY      0x3e3e3e    // Button backgrounds
#define DARK_THEME_BORDER_COLOR     0x555555    // Borders and separators
#define DARK_THEME_TEXT_PRIMARY     0xffffff    // Primary text (white)
#define DARK_THEME_TEXT_SECONDARY   0xcccccc    // Secondary text (light gray)
#define DARK_THEME_TEXT_DISABLED    0x888888    // Disabled text
#define DARK_THEME_ACCENT_COLOR     0x007acc    // Accent/highlight color (blue)
#define DARK_THEME_SUCCESS_COLOR    0x28a745    // Success messages (green)
#define DARK_THEME_WARNING_COLOR    0xffc107    // Warning messages (yellow)
#define DARK_THEME_ERROR_COLOR      0xdc3545    // Error messages (red)

// Button state colors
#define DARK_THEME_BTN_NORMAL       0x3e3e3e
#define DARK_THEME_BTN_PRESSED      0x4e4e4e
#define DARK_THEME_BTN_DISABLED     0x2a2a2a
#define DARK_THEME_BTN_FOCUSED      0x4a4a4a

// Input field colors
#define DARK_THEME_INPUT_BG         0x2a2a2a
#define DARK_THEME_INPUT_BORDER     0x555555
#define DARK_THEME_INPUT_FOCUSED    0x007acc

// Theme configuration structure
typedef struct {
    // Background colors
    lv_color_t bg_primary;
    lv_color_t bg_secondary;
    lv_color_t bg_tertiary;
    lv_color_t border_color;
    
    // Text colors
    lv_color_t text_primary;
    lv_color_t text_secondary;
    lv_color_t text_disabled;
    
    // Accent colors
    lv_color_t accent_color;
    lv_color_t success_color;
    lv_color_t warning_color;
    lv_color_t error_color;
    
    // Button colors
    lv_color_t btn_normal;
    lv_color_t btn_pressed;
    lv_color_t btn_disabled;
    lv_color_t btn_focused;
    
    // Input colors
    lv_color_t input_bg;
    lv_color_t input_border;
    lv_color_t input_focused;
    
    // Common styling properties
    int32_t border_radius;
    int32_t border_width;
    int32_t padding_small;
    int32_t padding_medium;
    int32_t padding_large;
    int32_t margin_small;
    int32_t margin_medium;
    int32_t margin_large;
} dark_theme_config_t;

/**
 * @brief Initialize the dark theme system
 * @return Pointer to the dark theme configuration
 */
const dark_theme_config_t* dark_theme_init(void);

/**
 * @brief Get the current dark theme configuration
 * @return Pointer to the dark theme configuration
 */
const dark_theme_config_t* dark_theme_get_config(void);

/**
 * @brief Apply dark theme to a screen/container
 * @param obj The LVGL object to apply theme to
 */
void dark_theme_apply_screen(lv_obj_t *obj);

/**
 * @brief Apply dark theme to a button
 * @param btn The button object
 * @param is_primary Whether this is a primary button (accent colored)
 */
void dark_theme_apply_button(lv_obj_t *btn, bool is_primary);

/**
 * @brief Apply dark theme to a label
 * @param label The label object
 * @param is_secondary Whether this is secondary text
 */
void dark_theme_apply_label(lv_obj_t *label, bool is_secondary);

/**
 * @brief Apply dark theme to a container/card
 * @param container The container object
 */
void dark_theme_apply_container(lv_obj_t *container);

/**
 * @brief Apply dark theme to a modal/dialog
 * @param modal The modal object
 */
void dark_theme_apply_modal(lv_obj_t *modal);

/**
 * @brief Apply dark theme to an input field
 * @param input The input object (textarea, dropdown, etc.)
 */
void dark_theme_apply_input(lv_obj_t *input);

/**
 * @brief Create a styled button with dark theme
 * @param parent Parent object
 * @param text Button text
 * @param is_primary Whether this is a primary button
 * @return Created button object
 */
lv_obj_t* dark_theme_create_button(lv_obj_t *parent, const char *text, bool is_primary);

/**
 * @brief Create a styled label with dark theme
 * @param parent Parent object
 * @param text Label text
 * @param is_secondary Whether this is secondary text
 * @return Created label object
 */
lv_obj_t* dark_theme_create_label(lv_obj_t *parent, const char *text, bool is_secondary);

/**
 * @brief Create a styled container with dark theme
 * @param parent Parent object
 * @return Created container object
 */
lv_obj_t* dark_theme_create_container(lv_obj_t *parent);

/**
 * @brief Disable focus styling for touch interfaces
 * @param obj The object to disable focus styling for
 */
void dark_theme_disable_focus(lv_obj_t *obj);

/**
 * @brief Configure buttons specifically for touch interfaces (no focus styling)
 * @param btn The button object
 * @param is_primary Whether this is a primary button
 */
void dark_theme_apply_touch_button(lv_obj_t *btn, bool is_primary);

#ifdef __cplusplus
}
#endif

#endif // DARK_THEME_H
