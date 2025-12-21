# UI Menu Component

Minimal, reusable LVGL touch menu helper for ESP32.

## Quick start

```c
#include "ui_components/ui_menu.h"

static void option1_cb(void) { /* handle option 1 */ }
static void option2_cb(void) { /* handle option 2 */ }
static void back_cb(void) { /* handle back navigation */ }

void show_main_menu(void) {
    lv_obj_t *parent = lv_screen_active();

    // Create menu with back button (pass NULL for no back button)
    ui_menu_t *menu = ui_menu_create(parent, "Main Menu", back_cb);

    ui_menu_add_entry(menu, "Option 1", option1_cb);
    ui_menu_add_entry(menu, "Option 2", option2_cb);

    ui_menu_show(menu);

    // Cleanup when done:
    // ui_menu_destroy(menu);
}
```

## API

- ui_menu_create(parent, title, back_cb) - Creates menu; if back_cb is non-NULL, a back arrow button appears top-left
- ui_menu_add_entry(menu, name, callback)
- ui_menu_show(menu) / ui_menu_hide(menu)
- ui_menu_set_entry_enabled(menu, index, enabled)
- ui_menu_get_selected(menu) - Get index of last touched entry (useful in callbacks)
- ui_menu_destroy(menu)
