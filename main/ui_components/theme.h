#ifndef THEME_H
#define THEME_H

#include <lvgl.h>

void theme_init(void);
lv_color_t main_color(void);
lv_color_t highlight_color(void);
lv_color_t error_color(void);
lv_color_t yes_color(void);
lv_color_t no_color(void);

// Theme sizing constants
const lv_font_t *theme_get_button_font(void);
const lv_font_t *theme_get_dialog_text_font(void);
int theme_get_button_width(void);
int theme_get_button_height(void);
int theme_get_button_spacing(void);

void theme_apply_screen(lv_obj_t *obj);
void theme_apply_frame(lv_obj_t *modal);
void theme_apply_solid_rectangle(lv_obj_t *target_rectangle);
void theme_apply_label(lv_obj_t *label, bool is_secondary);
void theme_apply_button_label(lv_obj_t *label, bool is_secondary);
void theme_apply_touch_button(lv_obj_t *btn, bool is_primary);
lv_obj_t *theme_create_button(lv_obj_t *parent, const char *text,
                              bool is_primary);
lv_obj_t *theme_create_label(lv_obj_t *parent, const char *text,
                             bool is_secondary);
lv_obj_t *theme_create_separator(lv_obj_t *parent);

#endif // THEME_H
