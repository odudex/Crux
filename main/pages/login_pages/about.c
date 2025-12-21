// About Page

#include "about.h"
#include "../../ui_components/logo/kern_logo_lvgl.h"
#include "../../ui_components/theme.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *about_screen = NULL;
static void (*return_callback)(void) = NULL;

static void about_screen_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if ((code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) && return_callback)
    return_callback();
}

void about_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent)
    return;

  return_callback = return_cb;

  about_screen = theme_create_page_container(parent);
  lv_obj_add_flag(about_screen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(about_screen, about_screen_event_cb, LV_EVENT_CLICKED,
                      NULL);

  theme_create_page_title(about_screen, "About");
  kern_logo_with_text(about_screen, 0, 130);

  lv_obj_t *qr = lv_qrcode_create(about_screen);
  lv_qrcode_set_size(qr, 250);
  const char *data = "https://github.com/odudex/Kern";
  lv_qrcode_update(qr, data, strlen(data));
  lv_obj_align(qr, LV_ALIGN_CENTER, 0, 140);
  lv_obj_set_style_border_color(qr, lv_color_white(), 0);
  lv_obj_set_style_border_width(qr, 10, 0);

  lv_obj_t *footer = theme_create_label(about_screen, "Tap to return", true);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -theme_get_default_padding());
  lv_obj_set_style_text_align(footer, LV_TEXT_ALIGN_CENTER, 0);
}

void about_page_show(void) {
  if (about_screen)
    lv_obj_clear_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
}

void about_page_hide(void) {
  if (about_screen)
    lv_obj_add_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
}

void about_page_destroy(void) {
  if (about_screen) {
    lv_obj_del(about_screen);
    about_screen = NULL;
  }
  return_callback = NULL;
}
