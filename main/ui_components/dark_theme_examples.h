/*
 * Dark Theme Usage Examples
 * Demonstrates how to use the dark theme system for consistent styling
 */

#ifndef DARK_THEME_EXAMPLES_H
#define DARK_THEME_EXAMPLES_H

#include "lvgl.h"
#include "dark_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a settings page with dark theme
 * @param parent Parent object
 * @return Created settings page container
 */
lv_obj_t* create_dark_settings_page(lv_obj_t *parent);

/**
 * @brief Create a menu with multiple options using dark theme
 * @param parent Parent object
 * @return Created menu container
 */
lv_obj_t* create_dark_options_menu(lv_obj_t *parent);

/**
 * @brief Create a form with input fields using dark theme
 * @param parent Parent object
 * @return Created form container
 */
lv_obj_t* create_dark_form(lv_obj_t *parent);

/**
 * @brief Create a status panel with indicators using dark theme
 * @param parent Parent object
 * @return Created status panel container
 */
lv_obj_t* create_dark_status_panel(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif // DARK_THEME_EXAMPLES_H
