/*
 * UI Menu Component Implementation
 * Generic reusable menu system for ESP32 LVGL applications
 */

#include "ui_menu.h"
#include "tron_theme.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "UI_MENU";

// Button event callback
static void menu_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    ui_menu_t *menu = (ui_menu_t *)lv_event_get_user_data(e);
    
    if (code == LV_EVENT_CLICKED) {
        // Find which button was clicked
        for (int i = 0; i < menu->config.entry_count; i++) {
            if (menu->buttons[i] == btn) {
                menu->config.selected_index = i;
                ui_menu_execute_selected(menu);
                break;
            }
        }
    }
}

ui_menu_t* ui_menu_create(lv_obj_t *parent, const char *title)
{
    if (!parent || !title) {
        ESP_LOGE(TAG, "Invalid parameters for menu creation");
        return NULL;
    }
    
    ui_menu_t *menu = malloc(sizeof(ui_menu_t));
    if (!menu) {
        ESP_LOGE(TAG, "Failed to allocate memory for menu");
        return NULL;
    }
    
    // Initialize configuration
    memset(&menu->config, 0, sizeof(ui_menu_config_t));
    strncpy(menu->config.title, title, UI_MENU_ENTRY_NAME_MAX_LEN - 1);
    menu->config.selected_index = 0;
    
    // Create main container
    menu->container = lv_obj_create(parent);
    lv_obj_set_size(menu->container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(menu->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(menu->container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Apply TRON theme to container
    tron_theme_apply_screen(menu->container);
    
    // Create title label
    menu->title_label = lv_label_create(menu->container);
    lv_label_set_text(menu->title_label, title);
    lv_obj_set_style_text_font(menu->title_label, &lv_font_montserrat_24, 0);
    lv_obj_align(menu->title_label, LV_ALIGN_TOP_MID, 0, 20);
    
    // Apply TRON theme to title
    tron_theme_apply_label(menu->title_label, false);
    
    // Create list container for menu items
    menu->list = lv_obj_create(menu->container);
    lv_obj_set_size(menu->list, LV_PCT(90), LV_PCT(70));
    lv_obj_set_flex_flow(menu->list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu->list, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(menu->list, 5, 0);
    lv_obj_set_style_bg_opa(menu->list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(menu->list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu->list, 0, 0);  // Ensure border width is 0
    
    // Initialize buttons array
    for (int i = 0; i < UI_MENU_MAX_ENTRIES; i++) {
        menu->buttons[i] = NULL;
    }
    
    // Create input group for navigation
    menu->group = lv_group_create();
    
    ESP_LOGI(TAG, "Menu '%s' created successfully", title);
    return menu;
}

bool ui_menu_add_entry(ui_menu_t *menu, const char *name, ui_menu_callback_t callback)
{
    if (!menu || !name || !callback) {
        ESP_LOGE(TAG, "Invalid parameters for adding menu entry");
        return false;
    }
    
    if (menu->config.entry_count >= UI_MENU_MAX_ENTRIES) {
        ESP_LOGE(TAG, "Maximum number of menu entries reached");
        return false;
    }
    
    int index = menu->config.entry_count;
    
    // Configure entry
    strncpy(menu->config.entries[index].name, name, UI_MENU_ENTRY_NAME_MAX_LEN - 1);
    menu->config.entries[index].callback = callback;
    menu->config.entries[index].enabled = true;
    
    // Create button
    menu->buttons[index] = lv_btn_create(menu->list);
    lv_obj_set_size(menu->buttons[index], LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(menu->buttons[index], 1);  // Make buttons grow to fill available space
    lv_obj_add_event_cb(menu->buttons[index], menu_button_event_cb, LV_EVENT_CLICKED, menu);
    
    // Apply TRON theme for touch interface (no focus styling)
    tron_theme_apply_touch_button(menu->buttons[index], false);
    
    // Create button label
    lv_obj_t *label = lv_label_create(menu->buttons[index]);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_ver(label, 15, 0);  // Add vertical padding for better button height
    lv_obj_center(label);
    
    // Apply TRON theme to label
    tron_theme_apply_label(label, false);
    
    // Don't add to input group for touch interface
    // lv_group_add_obj(menu->group, menu->buttons[index]);
    
    menu->config.entry_count++;
    
    ESP_LOGI(TAG, "Added menu entry '%s' at index %d", name, index);
    return true;
}

bool ui_menu_set_entry_enabled(ui_menu_t *menu, int index, bool enabled)
{
    if (!menu || index < 0 || index >= menu->config.entry_count) {
        ESP_LOGE(TAG, "Invalid menu or index for setting entry enabled state");
        return false;
    }
    
    menu->config.entries[index].enabled = enabled;
    
    if (menu->buttons[index]) {
        if (enabled) {
            lv_obj_clear_state(menu->buttons[index], LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(menu->buttons[index], LV_STATE_DISABLED);
        }
    }
    
    return true;
}

bool ui_menu_set_selected(ui_menu_t *menu, int index)
{
    if (!menu || index < 0 || index >= menu->config.entry_count) {
        ESP_LOGE(TAG, "Invalid menu or index for setting selection");
        return false;
    }
    
    menu->config.selected_index = index;
    
    // Update visual selection if using input group
    if (menu->group && menu->buttons[index]) {
        lv_group_focus_obj(menu->buttons[index]);
    }
    
    return true;
}

int ui_menu_get_selected(ui_menu_t *menu)
{
    if (!menu) {
        ESP_LOGE(TAG, "Invalid menu for getting selection");
        return -1;
    }
    
    return menu->config.selected_index;
}

bool ui_menu_execute_selected(ui_menu_t *menu)
{
    if (!menu) {
        ESP_LOGE(TAG, "Invalid menu for executing selection");
        return false;
    }
    
    int index = menu->config.selected_index;
    if (index < 0 || index >= menu->config.entry_count) {
        ESP_LOGE(TAG, "Invalid selected index");
        return false;
    }
    
    if (!menu->config.entries[index].enabled) {
        ESP_LOGW(TAG, "Selected entry is disabled");
        return false;
    }
    
    if (menu->config.entries[index].callback) {
        ESP_LOGI(TAG, "Executing callback for entry '%s'", menu->config.entries[index].name);
        menu->config.entries[index].callback();
        return true;
    }
    
    ESP_LOGW(TAG, "No callback set for entry '%s'", menu->config.entries[index].name);
    return false;
}

bool ui_menu_navigate_next(ui_menu_t *menu)
{
    if (!menu || menu->config.entry_count == 0) {
        return false;
    }
    
    int next_index = (menu->config.selected_index + 1) % menu->config.entry_count;
    return ui_menu_set_selected(menu, next_index);
}

bool ui_menu_navigate_prev(ui_menu_t *menu)
{
    if (!menu || menu->config.entry_count == 0) {
        return false;
    }
    
    int prev_index = menu->config.selected_index - 1;
    if (prev_index < 0) {
        prev_index = menu->config.entry_count - 1;
    }
    
    return ui_menu_set_selected(menu, prev_index);
}

void ui_menu_show(ui_menu_t *menu)
{
    if (!menu || !menu->container) {
        ESP_LOGE(TAG, "Invalid menu for showing");
        return;
    }
    
    lv_obj_clear_flag(menu->container, LV_OBJ_FLAG_HIDDEN);
}

void ui_menu_hide(ui_menu_t *menu)
{
    if (!menu || !menu->container) {
        ESP_LOGE(TAG, "Invalid menu for hiding");
        return;
    }
    
    lv_obj_add_flag(menu->container, LV_OBJ_FLAG_HIDDEN);
}

void ui_menu_destroy(ui_menu_t *menu)
{
    if (!menu) {
        return;
    }
    
    // Delete LVGL objects
    if (menu->container) {
        lv_obj_del(menu->container);
    }
    
    // Delete input group
    if (menu->group) {
        lv_group_del(menu->group);
    }
    
    // Free memory
    free(menu);
    
    ESP_LOGI(TAG, "Menu destroyed");
}
