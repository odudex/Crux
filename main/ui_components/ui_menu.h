/*
 * UI Menu Component
 * Generic reusable menu system for ESP32 LVGL applications
 */

#ifndef UI_MENU_H
#define UI_MENU_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of menu entries
#define UI_MENU_MAX_ENTRIES 10
#define UI_MENU_ENTRY_NAME_MAX_LEN 32

// Function pointer type for menu entry callbacks
typedef void (*ui_menu_callback_t)(void);

// Structure for a single menu entry
typedef struct {
    char name[UI_MENU_ENTRY_NAME_MAX_LEN];
    ui_menu_callback_t callback;
    bool enabled;
} ui_menu_entry_t;

// Structure for the menu configuration
typedef struct {
    ui_menu_entry_t entries[UI_MENU_MAX_ENTRIES];
    int entry_count;
    int selected_index;
    char title[UI_MENU_ENTRY_NAME_MAX_LEN];
} ui_menu_config_t;

// Structure for the menu object
typedef struct {
    ui_menu_config_t config;
    lv_obj_t *container;
    lv_obj_t *title_label;
    lv_obj_t *list;
    lv_obj_t *buttons[UI_MENU_MAX_ENTRIES];
    lv_group_t *group;
} ui_menu_t;

/**
 * @brief Create a new UI menu
 * 
 * @param parent Parent LVGL object
 * @param title Menu title
 * @return ui_menu_t* Pointer to the created menu object, NULL on failure
 */
ui_menu_t* ui_menu_create(lv_obj_t *parent, const char *title);

/**
 * @brief Add an entry to the menu
 * 
 * @param menu Pointer to the menu object
 * @param name Entry name/label
 * @param callback Function to call when entry is selected
 * @return true on success, false on failure
 */
bool ui_menu_add_entry(ui_menu_t *menu, const char *name, ui_menu_callback_t callback);

/**
 * @brief Enable or disable a menu entry
 * 
 * @param menu Pointer to the menu object
 * @param index Entry index
 * @param enabled true to enable, false to disable
 * @return true on success, false on failure
 */
bool ui_menu_set_entry_enabled(ui_menu_t *menu, int index, bool enabled);

/**
 * @brief Set the selected menu entry
 * 
 * @param menu Pointer to the menu object
 * @param index Entry index to select
 * @return true on success, false on failure
 */
bool ui_menu_set_selected(ui_menu_t *menu, int index);

/**
 * @brief Get the currently selected entry index
 * 
 * @param menu Pointer to the menu object
 * @return int Selected entry index, -1 on error
 */
int ui_menu_get_selected(ui_menu_t *menu);

/**
 * @brief Execute the callback of the currently selected entry
 * 
 * @param menu Pointer to the menu object
 * @return true on success, false on failure
 */
bool ui_menu_execute_selected(ui_menu_t *menu);

/**
 * @brief Navigate to the next menu entry
 * 
 * @param menu Pointer to the menu object
 * @return true on success, false on failure
 */
bool ui_menu_navigate_next(ui_menu_t *menu);

/**
 * @brief Navigate to the previous menu entry
 * 
 * @param menu Pointer to the menu object
 * @return true on success, false on failure
 */
bool ui_menu_navigate_prev(ui_menu_t *menu);

/**
 * @brief Destroy the menu and free resources
 * 
 * @param menu Pointer to the menu object
 */
void ui_menu_destroy(ui_menu_t *menu);

/**
 * @brief Show the menu
 * 
 * @param menu Pointer to the menu object
 */
void ui_menu_show(ui_menu_t *menu);

/**
 * @brief Hide the menu
 * 
 * @param menu Pointer to the menu object
 */
void ui_menu_hide(ui_menu_t *menu);

#ifdef __cplusplus
}
#endif

#endif // UI_MENU_H
