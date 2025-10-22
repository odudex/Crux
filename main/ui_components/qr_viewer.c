#include "qr_viewer.h"
#include "theme.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *qr_viewer_screen = NULL;
static void (*return_callback)(void) = NULL;
static char *qr_content_copy = NULL;

static void back_button_cb(lv_event_t *e) {
  if (return_callback) {
    return_callback();
  }
}

static void hide_message_timer_cb(lv_timer_t *timer) {
  lv_obj_t *msgbox = (lv_obj_t *)lv_timer_get_user_data(timer);
  if (msgbox) {
    lv_obj_del(msgbox);
  }
}

void qr_viewer_page_create(lv_obj_t *parent, const char *qr_content,
                           const char *title, void (*return_cb)(void)) {
  if (!parent || !qr_content) {
    return;
  }

  return_callback = return_cb;

  qr_content_copy = strdup(qr_content);
  if (!qr_content_copy) {
    return;
  }

  qr_viewer_screen = lv_obj_create(parent);
  lv_obj_set_size(qr_viewer_screen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(qr_viewer_screen, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(qr_viewer_screen, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(qr_viewer_screen, 20, 0);
  lv_obj_add_event_cb(qr_viewer_screen, back_button_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_update_layout(qr_viewer_screen);
  int32_t available_width = lv_obj_get_content_width(qr_viewer_screen);
  int32_t available_height = lv_obj_get_content_height(qr_viewer_screen);
  int32_t qr_size =
      (available_width < available_height) ? available_width : available_height;

  lv_obj_t *qr_code = lv_qrcode_create(qr_viewer_screen);
  if (!qr_code) {
    return;
  }
  lv_qrcode_set_size(qr_code, qr_size);
  lv_qrcode_update(qr_code, qr_content_copy, strlen(qr_content_copy));
  lv_obj_center(qr_code);

  if (title) {
    lv_obj_t *msgbox = lv_obj_create(qr_viewer_screen);
    lv_obj_set_size(msgbox, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(msgbox, LV_OPA_80, 0);
    lv_obj_set_style_border_width(msgbox, 2, 0);
    lv_obj_set_style_border_color(msgbox, main_color(), 0);
    lv_obj_set_style_radius(msgbox, 10, 0);
    lv_obj_set_style_pad_all(msgbox, 20, 0);
    lv_obj_add_flag(msgbox, LV_OBJ_FLAG_FLOATING);
    lv_obj_center(msgbox);

    char message[128];
    snprintf(message, sizeof(message), "%s\nTap to return", title);
    lv_obj_t *msg_label = theme_create_label(msgbox, message, false);
    lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(msg_label, lv_color_hex(0xFFFFFF), 0);

    lv_timer_t *timer = lv_timer_create(hide_message_timer_cb, 2000, msgbox);
    if (timer) {
      lv_timer_set_repeat_count(timer, 1);
    }
  }
}

void qr_viewer_page_show(void) {
  if (qr_viewer_screen) {
    lv_obj_clear_flag(qr_viewer_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void qr_viewer_page_hide(void) {
  if (qr_viewer_screen) {
    lv_obj_add_flag(qr_viewer_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void qr_viewer_page_destroy(void) {
  if (qr_content_copy) {
    free(qr_content_copy);
    qr_content_copy = NULL;
  }

  if (qr_viewer_screen) {
    lv_obj_del(qr_viewer_screen);
    qr_viewer_screen = NULL;
  }

  return_callback = NULL;
}
