#include "simple_dialog.h"
#include "lvgl.h"
#include "theme.h"

static void close_dialog_cb(lv_event_t *e) {
  lv_obj_t *dialog = (lv_obj_t *)lv_event_get_user_data(e);
  lv_obj_del(dialog);
}

void show_simple_dialog(const char *title, const char *message) {
  // Create modal dialog
  lv_obj_t *modal = lv_obj_create(lv_screen_active());
  lv_obj_set_size(modal, 400, 220);
  lv_obj_center(modal);

  // Apply TRON theme to modal
  theme_apply_frame(modal);

  // Title
  lv_obj_t *title_label = theme_create_label(modal, title, false);
  lv_obj_set_style_text_font(title_label, theme_font_small(), 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 0);

  // Message
  lv_obj_t *msg_label = theme_create_label(modal, message, false);
  lv_obj_set_width(msg_label, 340);                      // Leave margins
  lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP); // Enable text wrapping
  lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER,
                              0); // Center the wrapped text
  lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, -10);

  // Close button
  lv_obj_t *btn = theme_create_button(modal, "OK", true);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Add event to close the modal
  lv_obj_add_event_cb(btn, close_dialog_cb, LV_EVENT_CLICKED, modal);
}