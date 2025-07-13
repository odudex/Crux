/*
 * Login Page - Sample test case for UI Menu System
 * Demonstrates how to use the generic UI menu component
 */

#include "login.h"
#include "../ui_components/ui_menu.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "LOGIN";

// Global variables for the login page
static ui_menu_t *login_menu = NULL;
static lv_obj_t *login_screen = NULL;

// Forward declarations of menu callback functions
static void login_with_password_cb(void);
static void login_with_pin_cb(void);
static void create_new_account_cb(void);
static void forgot_password_cb(void);
static void guest_mode_cb(void);
static void exit_cb(void);

// Helper function for closing dialogs
static void close_dialog_cb(lv_event_t *e)
{
    lv_obj_t *dialog = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_del(dialog);
}

// Helper function to create simple dialogs
static void show_simple_dialog(const char *title, const char *message)
{
    // Create modal dialog
    lv_obj_t *modal = lv_obj_create(lv_screen_active());
    lv_obj_set_size(modal, 300, 150);
    lv_obj_center(modal);
    lv_obj_set_style_bg_color(modal, lv_color_hex(0x2e2e2e), 0);
    lv_obj_set_style_border_color(modal, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_width(modal, 2, 0);
    lv_obj_set_style_radius(modal, 10, 0);
    
    // Title
    lv_obj_t *title_label = lv_label_create(modal);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    // Message
    lv_obj_t *msg_label = lv_label_create(modal);
    lv_label_set_text(msg_label, message);
    lv_obj_set_style_text_color(msg_label, lv_color_white(), 0);
    lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, -10);
    
    // Close button
    lv_obj_t *btn = lv_btn_create(modal);
    lv_obj_set_size(btn, 80, 30);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "OK");
    lv_obj_center(btn_label);
    
    // Add event to close the modal
    lv_obj_add_event_cb(btn, close_dialog_cb, LV_EVENT_CLICKED, modal);
}

// Menu callback implementations
static void login_with_password_cb(void)
{
    ESP_LOGI(TAG, "Login with password selected");
    // TODO: Implement password login screen
    show_simple_dialog("Login", "Password login not implemented yet");
}

static void login_with_pin_cb(void)
{
    ESP_LOGI(TAG, "Login with PIN selected");
    // TODO: Implement PIN login screen
    show_simple_dialog("Login", "PIN login not implemented yet");
}

static void create_new_account_cb(void)
{
    ESP_LOGI(TAG, "Create new account selected");
    // TODO: Implement account creation screen
    show_simple_dialog("Account", "Account creation not implemented yet");
}

static void forgot_password_cb(void)
{
    ESP_LOGI(TAG, "Forgot password selected");
    // TODO: Implement password recovery screen
    show_simple_dialog("Recovery", "Password recovery not implemented yet");
}

static void guest_mode_cb(void)
{
    ESP_LOGI(TAG, "Guest mode selected");
    // TODO: Implement guest mode functionality
    show_simple_dialog("Guest Mode", "Entering guest mode...");
}

static void exit_cb(void)
{
    ESP_LOGI(TAG, "Exit selected");
    // TODO: Implement exit functionality or return to previous screen
    show_simple_dialog("Exit", "Exiting application...");
}

void login_page_create(lv_obj_t *parent)
{
    if (!parent) {
        ESP_LOGE(TAG, "Invalid parent object for login page");
        return;
    }
    
    ESP_LOGI(TAG, "Creating login page");
    
    // Create screen container
    login_screen = lv_obj_create(parent);
    lv_obj_set_size(login_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(login_screen, lv_color_hex(0x1e1e1e), 0);
    lv_obj_clear_flag(login_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create the login menu
    login_menu = ui_menu_create(login_screen, "Krux Login");
    if (!login_menu) {
        ESP_LOGE(TAG, "Failed to create login menu");
        return;
    }
    
    // Add menu entries with their respective callbacks
    if (!ui_menu_add_entry(login_menu, "Login with Password", login_with_password_cb)) {
        ESP_LOGE(TAG, "Failed to add 'Login with Password' entry");
    }
    
    if (!ui_menu_add_entry(login_menu, "Login with PIN", login_with_pin_cb)) {
        ESP_LOGE(TAG, "Failed to add 'Login with PIN' entry");
    }
    
    if (!ui_menu_add_entry(login_menu, "Create New Account", create_new_account_cb)) {
        ESP_LOGE(TAG, "Failed to add 'Create New Account' entry");
    }
    
    if (!ui_menu_add_entry(login_menu, "Forgot Password", forgot_password_cb)) {
        ESP_LOGE(TAG, "Failed to add 'Forgot Password' entry");
    }
    
    if (!ui_menu_add_entry(login_menu, "Guest Mode", guest_mode_cb)) {
        ESP_LOGE(TAG, "Failed to add 'Guest Mode' entry");
    }
    
    if (!ui_menu_add_entry(login_menu, "Exit", exit_cb)) {
        ESP_LOGE(TAG, "Failed to add 'Exit' entry");
    }
    
    // Set the first entry as selected by default
    ui_menu_set_selected(login_menu, 0);
    
    // Show the menu
    ui_menu_show(login_menu);
    
    ESP_LOGI(TAG, "Login page created successfully with %d menu entries", 
             ui_menu_get_selected(login_menu) >= 0 ? 6 : 0);
}

void login_page_show(void)
{
    if (login_menu) {
        ui_menu_show(login_menu);
        ESP_LOGI(TAG, "Login page shown");
    } else {
        ESP_LOGE(TAG, "Login menu not initialized");
    }
}

void login_page_hide(void)
{
    if (login_menu) {
        ui_menu_hide(login_menu);
        ESP_LOGI(TAG, "Login page hidden");
    } else {
        ESP_LOGE(TAG, "Login menu not initialized");
    }
}

void login_page_destroy(void)
{
    if (login_menu) {
        ui_menu_destroy(login_menu);
        login_menu = NULL;
        ESP_LOGI(TAG, "Login menu destroyed");
    }
    
    if (login_screen) {
        lv_obj_del(login_screen);
        login_screen = NULL;
        ESP_LOGI(TAG, "Login screen destroyed");
    }
}

bool login_page_navigate_next(void)
{
    if (login_menu) {
        return ui_menu_navigate_next(login_menu);
    }
    return false;
}

bool login_page_navigate_prev(void)
{
    if (login_menu) {
        return ui_menu_navigate_prev(login_menu);
    }
    return false;
}

bool login_page_execute_selected(void)
{
    if (login_menu) {
        return ui_menu_execute_selected(login_menu);
    }
    return false;
}

int login_page_get_selected(void)
{
    if (login_menu) {
        return ui_menu_get_selected(login_menu);
    }
    return -1;
}
