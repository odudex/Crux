/*
 * Addresses Page
 * Displays receive and change addresses from the wallet
 */

#include "addresses.h"
#include "../../ui_components/theme.h"
#include "../../wallet/wallet.h"
#include <esp_log.h>
#include <lvgl.h>
#include <stdio.h>
#include <wally_core.h>

static const char *TAG = "ADDRESSES";

// Number of addresses to display per page
#define NUM_ADDRESSES 10

// UI components
static lv_obj_t *addresses_screen = NULL;
static lv_obj_t *type_button = NULL;
static lv_obj_t *prev_button = NULL;
static lv_obj_t *next_button = NULL;
static lv_obj_t *back_button = NULL;
static lv_obj_t *address_list_container = NULL;
static void (*return_callback)(void) = NULL;

// State
static bool show_change = false;    // false = receive, true = change
static uint32_t address_offset = 0; // Starting index (0, 10, 20, ...)

// Forward declarations
static void back_button_cb(lv_event_t *e);
static void type_button_cb(lv_event_t *e);
static void prev_button_cb(lv_event_t *e);
static void next_button_cb(lv_event_t *e);
static void refresh_address_list(void);

// Back button callback
static void back_button_cb(lv_event_t *e) {
  ESP_LOGI(TAG, "Back button pressed");

  if (return_callback) {
    return_callback();
  }
}

// Toggle between receive and change addresses
static void type_button_cb(lv_event_t *e) {
  show_change = !show_change;
  address_offset = 0; // Reset to first page when switching type

  // Update button label
  lv_label_set_text(lv_obj_get_child(type_button, 0),
                    show_change ? "Change" : "Receive");

  // Refresh the address list
  refresh_address_list();

  ESP_LOGI(TAG, "Switched to %s addresses", show_change ? "change" : "receive");
}

// Show previous 10 addresses
static void prev_button_cb(lv_event_t *e) {
  if (address_offset >= NUM_ADDRESSES) {
    address_offset -= NUM_ADDRESSES;
    refresh_address_list();
    ESP_LOGI(TAG, "Showing addresses %u-%u", address_offset,
             address_offset + NUM_ADDRESSES - 1);
  }
}

// Show next 10 addresses
static void next_button_cb(lv_event_t *e) {
  address_offset += NUM_ADDRESSES;
  refresh_address_list();
  ESP_LOGI(TAG, "Showing addresses %u-%u", address_offset,
           address_offset + NUM_ADDRESSES - 1);
}

// Refresh the address list display
static void refresh_address_list(void) {
  if (!address_list_container) {
    return;
  }

  // Clear existing address labels
  lv_obj_clean(address_list_container);

  // Update prev button state
  if (address_offset == 0) {
    lv_obj_add_state(prev_button, LV_STATE_DISABLED);
  } else {
    lv_obj_clear_state(prev_button, LV_STATE_DISABLED);
  }

  // Build a single string with all addresses
  char all_addresses[2048] = ""; // Buffer for all addresses
  size_t offset = 0;

  for (uint32_t i = 0; i < NUM_ADDRESSES; i++) {
    uint32_t addr_index = address_offset + i;
    char *address = NULL;

    // Get the appropriate address type
    bool success;
    if (show_change) {
      success = wallet_get_change_address(addr_index, &address);
    } else {
      success = wallet_get_receive_address(addr_index, &address);
    }

    if (!success || !address) {
      ESP_LOGE(TAG, "Failed to get address %u", addr_index);
      continue;
    }

    // Append address to the string with separator
    int written =
        snprintf(all_addresses + offset, sizeof(all_addresses) - offset,
                 "%s%u: %s", (i > 0) ? "\n\n" : "", addr_index, address);

    if (written > 0) {
      offset += written;
    }

    // Free the address string
    wally_free_string(address);
  }

  // Create single label with all addresses
  lv_obj_t *addr_label =
      theme_create_label(address_list_container, all_addresses, false);
  lv_obj_set_width(addr_label, LV_PCT(100));
  lv_label_set_long_mode(addr_label, LV_LABEL_LONG_WRAP);
}

void addresses_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for addresses page");
    return;
  }

  if (!wallet_is_initialized()) {
    ESP_LOGE(TAG, "Cannot create addresses page: wallet not initialized");
    return;
  }

  return_callback = return_cb;
  show_change = false;
  address_offset = 0;

  // Create the main screen (full screen, no frame)
  addresses_screen = lv_obj_create(parent);
  lv_obj_set_size(addresses_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(addresses_screen);
  lv_obj_set_style_pad_all(addresses_screen, 10, 0);

  // Create a flex container for the entire layout
  lv_obj_set_flex_flow(addresses_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(addresses_screen, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(addresses_screen, 10, 0);

  // Create button container (horizontal layout)
  lv_obj_t *button_container = lv_obj_create(addresses_screen);
  lv_obj_set_size(button_container, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(button_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(button_container, 0, 0);
  lv_obj_set_style_pad_all(button_container, 0, 0);
  lv_obj_set_flex_flow(button_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(button_container, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_add_flag(button_container, LV_OBJ_FLAG_EVENT_BUBBLE);

  // Create Type button (Receive/Change)
  type_button = lv_btn_create(button_container);
  lv_obj_set_size(type_button, LV_PCT(60), LV_SIZE_CONTENT);
  theme_apply_touch_button(type_button, false);
  lv_obj_t *type_label = lv_label_create(type_button);
  lv_label_set_text(type_label, "Receive");
  lv_obj_center(type_label);
  theme_apply_button_label(type_label, false);
  lv_obj_add_event_cb(type_button, type_button_cb, LV_EVENT_CLICKED, NULL);

  // Create Previous button
  prev_button = lv_btn_create(button_container);
  lv_obj_set_size(prev_button, LV_PCT(15), LV_SIZE_CONTENT);
  theme_apply_touch_button(prev_button, false);
  lv_obj_t *prev_label = lv_label_create(prev_button);
  lv_label_set_text(prev_label, "<");
  lv_obj_center(prev_label);
  theme_apply_button_label(prev_label, false);
  lv_obj_add_event_cb(prev_button, prev_button_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_state(prev_button, LV_STATE_DISABLED); // Disabled initially

  // Create Next button
  next_button = lv_btn_create(button_container);
  lv_obj_set_size(next_button, LV_PCT(15), LV_SIZE_CONTENT);
  theme_apply_touch_button(next_button, false);
  lv_obj_t *next_label = lv_label_create(next_button);
  lv_label_set_text(next_label, ">");
  lv_obj_center(next_label);
  theme_apply_button_label(next_label, false);
  lv_obj_add_event_cb(next_button, next_button_cb, LV_EVENT_CLICKED, NULL);

  // Create address list container
  address_list_container = lv_obj_create(addresses_screen);
  lv_obj_set_size(address_list_container, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(address_list_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(address_list_container, 0, 0);
  lv_obj_set_style_pad_all(address_list_container, 0, 0);
  lv_obj_set_flex_flow(address_list_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(address_list_container, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(address_list_container, 5, 0);

  // Generate initial address list
  refresh_address_list();

  // Create Back button at the bottom
  back_button = lv_btn_create(addresses_screen);
  lv_obj_set_size(back_button, LV_PCT(100), LV_SIZE_CONTENT);
  theme_apply_touch_button(back_button, false);
  lv_obj_t *back_label = lv_label_create(back_button);
  lv_label_set_text(back_label, "Back");
  lv_obj_center(back_label);
  theme_apply_button_label(back_label, false);
  lv_obj_add_event_cb(back_button, back_button_cb, LV_EVENT_CLICKED, NULL);

  ESP_LOGI(TAG, "Addresses page created successfully");
}

void addresses_page_show(void) {
  if (addresses_screen) {
    lv_obj_clear_flag(addresses_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void addresses_page_hide(void) {
  if (addresses_screen) {
    lv_obj_add_flag(addresses_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void addresses_page_destroy(void) {
  ESP_LOGI(TAG, "Destroying addresses page");

  // Destroy screen (this will also destroy all children)
  if (addresses_screen) {
    lv_obj_del(addresses_screen);
    addresses_screen = NULL;
  }

  type_button = NULL;
  prev_button = NULL;
  next_button = NULL;
  back_button = NULL;
  address_list_container = NULL;
  return_callback = NULL;
  show_change = false;
  address_offset = 0;

  ESP_LOGI(TAG, "Addresses page destroyed");
}
