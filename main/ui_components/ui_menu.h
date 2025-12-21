// UI Menu Component - Touch menu for LVGL

#ifndef UI_MENU_H
#define UI_MENU_H

#include <lvgl.h>

#define UI_MENU_MAX_ENTRIES 10

typedef void (*ui_menu_callback_t)(void);

typedef struct {
  ui_menu_callback_t callback;
  bool enabled;
} ui_menu_entry_t;

typedef struct {
  ui_menu_entry_t entries[UI_MENU_MAX_ENTRIES];
  int entry_count;
  int selected_index;
} ui_menu_config_t;

typedef struct {
  ui_menu_config_t config;
  lv_obj_t *container;
  lv_obj_t *title_label;
  lv_obj_t *list;
  lv_obj_t *buttons[UI_MENU_MAX_ENTRIES];
  lv_obj_t *back_btn;
  ui_menu_callback_t back_callback;
} ui_menu_t;

ui_menu_t *ui_menu_create(lv_obj_t *parent, const char *title,
                          ui_menu_callback_t back_cb);
bool ui_menu_add_entry(ui_menu_t *menu, const char *name,
                       ui_menu_callback_t callback);
bool ui_menu_set_entry_enabled(ui_menu_t *menu, int index, bool enabled);
int ui_menu_get_selected(ui_menu_t *menu);
void ui_menu_show(ui_menu_t *menu);
void ui_menu_hide(ui_menu_t *menu);
void ui_menu_destroy(ui_menu_t *menu);

#endif // UI_MENU_H
