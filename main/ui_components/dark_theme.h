#ifndef DARK_THEME_H
#define DARK_THEME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void dark_theme_init(void);
void dark_theme_apply_screen(lv_obj_t *obj);
void dark_theme_apply_modal(lv_obj_t *modal);
void dark_theme_apply_label(lv_obj_t *label, bool is_secondary);
void dark_theme_apply_touch_button(lv_obj_t *btn, bool is_primary);
lv_obj_t* dark_theme_create_button(lv_obj_t *parent, const char *text, bool is_primary);
lv_obj_t* dark_theme_create_label(lv_obj_t *parent, const char *text, bool is_secondary);

#ifdef __cplusplus
}
#endif

#endif // DARK_THEME_H
