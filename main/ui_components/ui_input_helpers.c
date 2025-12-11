// UI Input Helpers - Shared components for input pages

#include "ui_input_helpers.h"
#include "theme.h"

lv_obj_t *ui_create_back_button(lv_obj_t *parent, lv_event_cb_t event_cb) {
  if (!parent)
    return NULL;

  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 60, 60);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 5, 5);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(label, theme_get_button_font(), 0);
  lv_obj_center(label);

  if (event_cb)
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

  return btn;
}
