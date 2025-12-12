// UI Menu Component - Generic reusable menu system

#include "ui_menu.h"
#include "theme.h"
#include "ui_input_helpers.h"
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "UI_MENU";

static void menu_button_event_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  ui_menu_t *menu = (ui_menu_t *)lv_event_get_user_data(e);

  for (int i = 0; i < menu->config.entry_count; i++) {
    if (menu->buttons[i] == btn) {
      menu->config.selected_index = i;
      ui_menu_execute_selected(menu);
      break;
    }
  }
}

static void menu_back_button_event_cb(lv_event_t *e) {
  ui_menu_t *menu = (ui_menu_t *)lv_event_get_user_data(e);
  if (menu && menu->back_callback)
    menu->back_callback();
}

ui_menu_t *ui_menu_create(lv_obj_t *parent, const char *title,
                          ui_menu_callback_t back_cb) {
  if (!parent || !title)
    return NULL;

  ui_menu_t *menu = malloc(sizeof(ui_menu_t));
  if (!menu)
    return NULL;

  memset(&menu->config, 0, sizeof(ui_menu_config_t));
  strncpy(menu->config.title, title, UI_MENU_ENTRY_NAME_MAX_LEN - 1);

  // Main container
  menu->container = lv_obj_create(parent);
  lv_obj_set_size(menu->container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(menu->container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(menu->container, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(menu->container, LV_OBJ_FLAG_SCROLLABLE);
  theme_apply_screen(menu->container);

  // Title
  menu->title_label = lv_label_create(menu->container);
  lv_label_set_text(menu->title_label, title);
  lv_obj_set_style_text_font(menu->title_label, &lv_font_montserrat_24, 0);
  theme_apply_label(menu->title_label, false);

  // List container
  menu->list = lv_obj_create(menu->container);
  lv_obj_set_size(menu->list, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(menu->list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(menu->list, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_flex_grow(menu->list, 1);
  lv_obj_set_style_pad_all(menu->list, 20, 0);
  lv_obj_set_style_pad_gap(menu->list, 10, 0);
  lv_obj_set_style_bg_opa(menu->list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(menu->list, 0, 0);
  lv_obj_set_style_outline_width(menu->list, 0, 0);

  for (int i = 0; i < UI_MENU_MAX_ENTRIES; i++)
    menu->buttons[i] = NULL;

  menu->back_btn = NULL;
  menu->back_callback = back_cb;

  // Back button on parent screen (top-left)
  if (back_cb) {
    menu->back_btn = ui_create_back_button(parent, NULL);
    if (menu->back_btn) {
      lv_obj_remove_event_cb(menu->back_btn, NULL);
      lv_obj_add_event_cb(menu->back_btn, menu_back_button_event_cb,
                          LV_EVENT_CLICKED, menu);
    }
  }

  menu->group = lv_group_create();
  return menu;
}

bool ui_menu_add_entry(ui_menu_t *menu, const char *name,
                       ui_menu_callback_t callback) {
  if (!menu || !name || !callback)
    return false;

  if (menu->config.entry_count >= UI_MENU_MAX_ENTRIES)
    return false;

  int idx = menu->config.entry_count;

  strncpy(menu->config.entries[idx].name, name, UI_MENU_ENTRY_NAME_MAX_LEN - 1);
  menu->config.entries[idx].callback = callback;
  menu->config.entries[idx].enabled = true;

  menu->buttons[idx] = lv_btn_create(menu->list);
  lv_obj_set_size(menu->buttons[idx], LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(menu->buttons[idx], 1);
  lv_obj_add_event_cb(menu->buttons[idx], menu_button_event_cb,
                      LV_EVENT_CLICKED, menu);
  theme_apply_touch_button(menu->buttons[idx], false);

  lv_obj_t *label = lv_label_create(menu->buttons[idx]);
  lv_label_set_text(label, name);
  lv_obj_set_style_pad_ver(label, 15, 0);
  lv_obj_center(label);
  theme_apply_button_label(label, false);

  menu->config.entry_count++;
  return true;
}

bool ui_menu_set_entry_enabled(ui_menu_t *menu, int index, bool enabled) {
  if (!menu || index < 0 || index >= menu->config.entry_count)
    return false;

  menu->config.entries[index].enabled = enabled;
  if (menu->buttons[index]) {
    if (enabled)
      lv_obj_clear_state(menu->buttons[index], LV_STATE_DISABLED);
    else
      lv_obj_add_state(menu->buttons[index], LV_STATE_DISABLED);
  }
  return true;
}

bool ui_menu_set_selected(ui_menu_t *menu, int index) {
  if (!menu || index < 0 || index >= menu->config.entry_count)
    return false;

  menu->config.selected_index = index;
  if (menu->group && menu->buttons[index])
    lv_group_focus_obj(menu->buttons[index]);
  return true;
}

int ui_menu_get_selected(ui_menu_t *menu) {
  return menu ? menu->config.selected_index : -1;
}

bool ui_menu_execute_selected(ui_menu_t *menu) {
  if (!menu)
    return false;

  int idx = menu->config.selected_index;
  if (idx < 0 || idx >= menu->config.entry_count)
    return false;

  if (!menu->config.entries[idx].enabled)
    return false;

  if (menu->config.entries[idx].callback) {
    ESP_LOGI(TAG, "Selected '%s'", menu->config.entries[idx].name);
    menu->config.entries[idx].callback();
    return true;
  }
  return false;
}

bool ui_menu_navigate_next(ui_menu_t *menu) {
  if (!menu || menu->config.entry_count == 0)
    return false;
  return ui_menu_set_selected(
      menu, (menu->config.selected_index + 1) % menu->config.entry_count);
}

bool ui_menu_navigate_prev(ui_menu_t *menu) {
  if (!menu || menu->config.entry_count == 0)
    return false;
  int prev = menu->config.selected_index - 1;
  if (prev < 0)
    prev = menu->config.entry_count - 1;
  return ui_menu_set_selected(menu, prev);
}

void ui_menu_show(ui_menu_t *menu) {
  if (menu && menu->container)
    lv_obj_clear_flag(menu->container, LV_OBJ_FLAG_HIDDEN);
}

void ui_menu_hide(ui_menu_t *menu) {
  if (menu && menu->container)
    lv_obj_add_flag(menu->container, LV_OBJ_FLAG_HIDDEN);
}

void ui_menu_destroy(ui_menu_t *menu) {
  if (!menu)
    return;

  if (menu->back_btn)
    lv_obj_del(menu->back_btn);
  if (menu->container)
    lv_obj_del(menu->container);
  if (menu->group)
    lv_group_del(menu->group);

  free(menu);
}
