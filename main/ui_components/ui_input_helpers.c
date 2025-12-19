// UI Input Helpers - Shared components for input pages

#include "ui_input_helpers.h"
#include "theme.h"

static lv_obj_t *create_icon_button(lv_obj_t *parent, const char *symbol,
                                    lv_align_t align, lv_event_cb_t event_cb) {
  if (!parent)
    return NULL;

  int padding = theme_get_default_padding();
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 60, 60);
  lv_obj_align(btn, align, (align == LV_ALIGN_TOP_LEFT) ? padding : -padding,
               padding);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, symbol);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(label, theme_get_button_font(), 0);
  lv_obj_center(label);

  if (event_cb)
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

  return btn;
}

lv_obj_t *ui_create_back_button(lv_obj_t *parent, lv_event_cb_t event_cb) {
  return create_icon_button(parent, LV_SYMBOL_LEFT, LV_ALIGN_TOP_LEFT,
                            event_cb);
}

lv_obj_t *ui_create_power_button(lv_obj_t *parent, lv_event_cb_t event_cb) {
  return create_icon_button(parent, LV_SYMBOL_POWER, LV_ALIGN_TOP_LEFT,
                            event_cb);
}
