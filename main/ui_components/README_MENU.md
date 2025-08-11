# UI Menu Component

Minimal, reusable LVGL menu helper for ESP32.

## Quick start

```c
#include "ui_components/ui_menu.h"

static void option1_cb(void) { /* handle option 1 */ }
static void option2_cb(void) { /* handle option 2 */ }

void show_main_menu(void) {
    lv_obj_t *parent = lv_screen_active(); // current screen
    ui_menu_t *menu = ui_menu_create(parent, "Main Menu");

    ui_menu_add_entry(menu, "Option 1", option1_cb);
    ui_menu_add_entry(menu, "Option 2", option2_cb);

    ui_menu_show(menu);

    // Optional:
    // ui_menu_set_selected(menu, 1);
    // ui_menu_execute_selected(menu);
    // ui_menu_set_entry_enabled(menu, 0, false);

    // Cleanup when done:
    // ui_menu_destroy(menu);
}
```

## API at a glance

- ui_menu_create(parent, title)
- ui_menu_add_entry(menu, name, callback)
- ui_menu_show(menu) / ui_menu_hide(menu)
- ui_menu_set_selected(menu, index)
- ui_menu_execute_selected(menu)
- ui_menu_set_entry_enabled(menu, index, enabled)
- ui_menu_destroy(menu)
