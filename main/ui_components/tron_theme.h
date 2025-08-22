#ifndef TRON_THEME_H
#define TRON_THEME_H

#include <lvgl.h>

void tron_theme_init(void);
lv_color_t main_color(void);
lv_color_t highlight_color(void);
void tron_theme_apply_screen(lv_obj_t *obj);
void tron_theme_apply_frame(lv_obj_t *modal);
void tron_theme_apply_solid_rectangle(lv_obj_t *target_rectangle);
void tron_theme_apply_label(lv_obj_t *label, bool is_secondary);
void tron_theme_apply_button_label(lv_obj_t *label, bool is_secondary);
void tron_theme_apply_touch_button(lv_obj_t *btn, bool is_primary);
lv_obj_t *tron_theme_create_button(lv_obj_t *parent, const char *text,
                                   bool is_primary);
lv_obj_t *tron_theme_create_label(lv_obj_t *parent, const char *text,
                                  bool is_secondary);
lv_obj_t *tron_theme_create_separator(lv_obj_t *parent);

#endif // TRON_THEME_H
