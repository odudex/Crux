#include "passphrase.h"
#include "../../ui_components/prompt_dialog.h"
#include "../../ui_components/theme.h"
#include "../../ui_components/ui_input_helpers.h"
#include <lvgl.h>
#include <stdio.h>

#define TOP_BAR_HEIGHT 70

static lv_obj_t *passphrase_screen = NULL;
static lv_obj_t *textarea = NULL;
static lv_obj_t *keyboard = NULL;
static lv_group_t *input_group = NULL;
static void (*return_callback)(void) = NULL;
static passphrase_success_callback_t success_callback = NULL;

static void back_confirm_cb(bool result, void *user_data) {
  (void)user_data;
  if (result && return_callback)
    return_callback();
}

static void back_btn_cb(lv_event_t *e) {
  (void)e;
  show_prompt_dialog_overlay("Are you sure you want to go back?",
                             back_confirm_cb, NULL);
}

static void confirm_passphrase_cb(bool result, void *user_data) {
  (void)user_data;
  if (result && success_callback)
    success_callback(lv_textarea_get_text(textarea));
}

static void keyboard_ready_cb(lv_event_t *e) {
  (void)e;
  char prompt[128];
  snprintf(prompt, sizeof(prompt), "Confirm passphrase:\n\"%s\"",
           lv_textarea_get_text(textarea));
  show_prompt_dialog_overlay(prompt, confirm_passphrase_cb, NULL);
}

void passphrase_page_create(lv_obj_t *parent, void (*return_cb)(void),
                            passphrase_success_callback_t success_cb) {
  (void)parent;
  return_callback = return_cb;
  success_callback = success_cb;

  // Screen
  passphrase_screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(passphrase_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(passphrase_screen);
  lv_obj_clear_flag(passphrase_screen, LV_OBJ_FLAG_SCROLLABLE);

  // Top bar
  lv_obj_t *top = lv_obj_create(passphrase_screen);
  lv_obj_set_size(top, LV_PCT(100), TOP_BAR_HEIGHT);
  lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(top, 0, 0);
  lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
  ui_create_back_button(top, back_btn_cb);

  lv_obj_t *title = lv_label_create(top);
  lv_label_set_text(title, "Enter Passphrase");
  lv_obj_set_style_text_color(title, main_color(), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  // Text area
  textarea = lv_textarea_create(passphrase_screen);
  lv_obj_set_size(textarea, LV_PCT(90), 50);
  lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, TOP_BAR_HEIGHT + 10);
  lv_textarea_set_one_line(textarea, true);
  lv_textarea_set_placeholder_text(textarea, "passphrase");
  lv_obj_set_style_text_font(textarea, &lv_font_montserrat_24, 0);
  lv_obj_set_style_bg_color(textarea, panel_color(), 0);
  lv_obj_set_style_text_color(textarea, main_color(), 0);
  lv_obj_set_style_border_color(textarea, secondary_color(), 0);
  lv_obj_set_style_border_width(textarea, 1, 0);
  lv_obj_set_style_bg_color(textarea, highlight_color(), LV_PART_CURSOR);
  lv_obj_set_style_bg_opa(textarea, LV_OPA_COVER, LV_PART_CURSOR);

  input_group = lv_group_create();
  lv_group_add_obj(input_group, textarea);
  lv_group_focus_obj(textarea);

  // Keyboard
  keyboard = lv_keyboard_create(lv_screen_active());
  lv_obj_set_size(keyboard, LV_HOR_RES, LV_VER_RES * 55 / 100);
  lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(keyboard, textarea);
  lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_obj_add_event_cb(keyboard, keyboard_ready_cb, LV_EVENT_READY, NULL);

  // Keyboard dark theme
  lv_obj_set_style_bg_color(keyboard, lv_color_black(), 0);
  lv_obj_set_style_border_width(keyboard, 0, 0);
  lv_obj_set_style_pad_all(keyboard, 4, 0);
  lv_obj_set_style_pad_gap(keyboard, 6, 0);
  lv_obj_set_style_bg_color(keyboard, disabled_color(), LV_PART_ITEMS);
  lv_obj_set_style_text_color(keyboard, main_color(), LV_PART_ITEMS);
  lv_obj_set_style_border_width(keyboard, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(keyboard, 6, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(keyboard, highlight_color(),
                            LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(keyboard, highlight_color(),
                            LV_PART_ITEMS | LV_STATE_CHECKED);
}

void passphrase_page_show(void) {
  if (passphrase_screen)
    lv_obj_clear_flag(passphrase_screen, LV_OBJ_FLAG_HIDDEN);
}

void passphrase_page_hide(void) {
  if (passphrase_screen)
    lv_obj_add_flag(passphrase_screen, LV_OBJ_FLAG_HIDDEN);
}

void passphrase_page_destroy(void) {
  if (input_group) {
    lv_group_del(input_group);
    input_group = NULL;
  }
  if (keyboard) {
    lv_obj_del(keyboard);
    keyboard = NULL;
  }
  if (passphrase_screen) {
    lv_obj_del(passphrase_screen);
    passphrase_screen = NULL;
  }
  textarea = NULL;
  return_callback = NULL;
  success_callback = NULL;
}
