/*
 * Dark Theme Usage Examples Implementation
 * Demonstrates how to use the dark theme system for consistent styling
 */

#include "dark_theme_examples.h"
#include "esp_log.h"

static const char *TAG = "DARK_THEME_EXAMPLES";

lv_obj_t* create_dark_settings_page(lv_obj_t *parent)
{
    if (!parent) return NULL;
    
    // Create main container
    lv_obj_t *container = dark_theme_create_container(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Title
    lv_obj_t *title = dark_theme_create_label(container, "Settings", false);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_bottom(title, 20, 0);
    
    // Settings options
    const char *settings[] = {
        "Display Brightness",
        "Sound Settings", 
        "Network Configuration",
        "Security Options",
        "System Information"
    };
    
    for (int i = 0; i < 5; i++) {
        lv_obj_t *setting_row = dark_theme_create_container(container);
        lv_obj_set_size(setting_row, LV_PCT(90), 60);
        lv_obj_set_flex_flow(setting_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(setting_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        
        // Setting label
        lv_obj_t *label = dark_theme_create_label(setting_row, settings[i], false);
        
        // Arrow or action button
        lv_obj_t *btn = dark_theme_create_button(setting_row, ">", false);
        lv_obj_set_size(btn, 40, 40);
    }
    
    return container;
}

lv_obj_t* create_dark_options_menu(lv_obj_t *parent)
{
    if (!parent) return NULL;
    
    // Create main container
    lv_obj_t *container = dark_theme_create_container(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Title
    lv_obj_t *title = dark_theme_create_label(container, "Main Menu", false);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_pad_bottom(title, 30, 0);
    
    // Menu buttons
    const char *menu_items[] = {
        "Start New Session",
        "Load Previous",
        "Settings",
        "About",
        "Exit"
    };
    
    bool is_primary[] = {true, false, false, false, false};
    
    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = dark_theme_create_button(container, menu_items[i], is_primary[i]);
        lv_obj_set_size(btn, 250, 50);
        lv_obj_set_style_margin_bottom(btn, 10, 0);
    }
    
    return container;
}

lv_obj_t* create_dark_form(lv_obj_t *parent)
{
    if (!parent) return NULL;
    
    // Create main container
    lv_obj_t *container = dark_theme_create_container(parent);
    lv_obj_set_size(container, LV_PCT(90), LV_PCT(80));
    lv_obj_center(container);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Form title
    lv_obj_t *title = dark_theme_create_label(container, "User Information", false);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_bottom(title, 20, 0);
    
    // Username field
    lv_obj_t *username_label = dark_theme_create_label(container, "Username:", true);
    lv_obj_set_style_pad_bottom(username_label, 5, 0);
    
    lv_obj_t *username_input = lv_textarea_create(container);
    lv_obj_set_size(username_input, LV_PCT(100), 40);
    lv_textarea_set_placeholder_text(username_input, "Enter username");
    lv_textarea_set_one_line(username_input, true);
    dark_theme_apply_input(username_input);
    lv_obj_set_style_margin_bottom(username_input, 15, 0);
    
    // Password field
    lv_obj_t *password_label = dark_theme_create_label(container, "Password:", true);
    lv_obj_set_style_pad_bottom(password_label, 5, 0);
    
    lv_obj_t *password_input = lv_textarea_create(container);
    lv_obj_set_size(password_input, LV_PCT(100), 40);
    lv_textarea_set_placeholder_text(password_input, "Enter password");
    lv_textarea_set_one_line(password_input, true);
    lv_textarea_set_password_mode(password_input, true);
    dark_theme_apply_input(password_input);
    lv_obj_set_style_margin_bottom(password_input, 20, 0);
    
    // Buttons
    lv_obj_t *btn_container = lv_obj_create(container);
    lv_obj_set_size(btn_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(btn_container, LV_OPA_TRANSP, 0);
    
    lv_obj_t *cancel_btn = dark_theme_create_button(btn_container, "Cancel", false);
    lv_obj_set_size(cancel_btn, 100, 40);
    
    lv_obj_t *submit_btn = dark_theme_create_button(btn_container, "Submit", true);
    lv_obj_set_size(submit_btn, 100, 40);
    
    return container;
}

lv_obj_t* create_dark_status_panel(lv_obj_t *parent)
{
    if (!parent) return NULL;
    
    // Create main container
    lv_obj_t *container = dark_theme_create_container(parent);
    lv_obj_set_size(container, LV_PCT(100), 100);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Status indicators
    const char *statuses[] = {"WiFi", "Battery", "Time"};
    const char *values[] = {"Connected", "85%", "14:30"};
    lv_color_t colors[] = {
        {.red = 0x28, .green = 0xa7, .blue = 0x45}, // Green for WiFi
        {.red = 0xff, .green = 0xc1, .blue = 0x07}, // Yellow for Battery
        {.red = 0x00, .green = 0x7a, .blue = 0xcc}  // Blue for Time
    };
    
    for (int i = 0; i < 3; i++) {
        lv_obj_t *status_item = lv_obj_create(container);
        lv_obj_set_size(status_item, 80, 60);
        lv_obj_set_flex_flow(status_item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(status_item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(status_item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(status_item, LV_OPA_TRANSP, 0);
        
        // Status label
        lv_obj_t *label = dark_theme_create_label(status_item, statuses[i], true);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        
        // Status value
        lv_obj_t *value = dark_theme_create_label(status_item, values[i], false);
        lv_obj_set_style_text_color(value, colors[i], 0);
        lv_obj_set_style_text_font(value, &lv_font_montserrat_14, 0);
    }
    
    return container;
}
