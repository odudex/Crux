#include "prompt_dialog.h"
#include "lvgl.h"
#include "theme.h"
#include <esp_log.h>

static const char *TAG = "PROMPT_DIALOG";

typedef struct {
  prompt_dialog_callback_t callback;
  void *user_data;
  lv_obj_t *dialog;
  lv_obj_t *blocker;
} prompt_dialog_context_t;

static void no_button_cb(lv_event_t *e) {
  prompt_dialog_context_t *ctx =
      (prompt_dialog_context_t *)lv_event_get_user_data(e);

  if (ctx && ctx->callback) {
    ctx->callback(false, ctx->user_data);
  }
  if (ctx && ctx->blocker) {
    lv_obj_del(ctx->blocker);
  }
  if (ctx) {
    free(ctx);
  }
}

static void yes_button_cb(lv_event_t *e) {
  prompt_dialog_context_t *ctx =
      (prompt_dialog_context_t *)lv_event_get_user_data(e);

  if (ctx && ctx->callback) {
    ctx->callback(true, ctx->user_data);
  }
  if (ctx && ctx->blocker) {
    lv_obj_del(ctx->blocker);
  }
  if (ctx) {
    free(ctx);
  }
}

static void create_prompt_dialog_internal(const char *prompt_text,
                                          prompt_dialog_callback_t callback,
                                          void *user_data, bool overlay) {
  if (!prompt_text) {
    ESP_LOGE(TAG, "Invalid prompt text");
    return;
  }

  prompt_dialog_context_t *ctx =
      (prompt_dialog_context_t *)malloc(sizeof(prompt_dialog_context_t));
  if (!ctx) {
    ESP_LOGE(TAG, "Failed to allocate context");
    return;
  }

  ctx->callback = callback;
  ctx->user_data = user_data;
  ctx->blocker = NULL;

  lv_obj_t *parent = lv_screen_active();

  if (overlay) {
    // Create a full-screen blocker to capture all background events
    lv_obj_t *blocker = lv_obj_create(parent);
    lv_obj_remove_style_all(blocker);
    lv_obj_set_size(blocker, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(blocker, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(blocker, LV_OPA_50, 0);
    lv_obj_add_flag(blocker, LV_OBJ_FLAG_CLICKABLE);
    ctx->blocker = blocker;
    parent = blocker;
  }

  lv_obj_t *dialog = lv_obj_create(parent);
  ctx->dialog = dialog;

  if (overlay) {
    // Overlay style: centered semi-transparent container
    lv_obj_set_size(dialog, LV_PCT(90), LV_PCT(40));
    lv_obj_center(dialog);
    theme_apply_frame(dialog);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_90, 0);
  } else {
    // Fullscreen style
    lv_obj_set_size(dialog, LV_PCT(100), LV_PCT(100));
    theme_apply_screen(dialog);
  }

  // Prompt text
  lv_obj_t *prompt_label = theme_create_label(dialog, prompt_text, false);
  lv_obj_set_width(prompt_label, LV_PCT(90));
  lv_label_set_long_mode(prompt_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(prompt_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(prompt_label, theme_font_medium(), 0);
  lv_obj_center(prompt_label);

  // "No" button
  lv_obj_t *no_btn = theme_create_button(dialog, "No", false);
  lv_obj_set_size(no_btn, LV_PCT(50), theme_get_button_height());
  lv_obj_align(no_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(no_btn, no_button_cb, LV_EVENT_CLICKED, ctx);
  lv_obj_t *no_label = lv_obj_get_child(no_btn, 0);
  if (no_label) {
    lv_obj_set_style_text_color(no_label, no_color(), 0);
    lv_obj_set_style_text_font(no_label, theme_font_medium(), 0);
  }

  // "Yes" button
  lv_obj_t *yes_btn = theme_create_button(dialog, "Yes", true);
  lv_obj_set_size(yes_btn, LV_PCT(50), theme_get_button_height());
  lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(yes_btn, yes_button_cb, LV_EVENT_CLICKED, ctx);
  lv_obj_t *yes_label = lv_obj_get_child(yes_btn, 0);
  if (yes_label) {
    lv_obj_set_style_text_color(yes_label, yes_color(), 0);
    lv_obj_set_style_text_font(yes_label, theme_font_medium(), 0);
  }
}

void show_prompt_dialog(const char *prompt_text,
                        prompt_dialog_callback_t callback, void *user_data) {
  create_prompt_dialog_internal(prompt_text, callback, user_data, false);
}

void show_prompt_dialog_overlay(const char *prompt_text,
                                prompt_dialog_callback_t callback,
                                void *user_data) {
  create_prompt_dialog_internal(prompt_text, callback, user_data, true);
}
