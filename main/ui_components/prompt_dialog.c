#include "prompt_dialog.h"
#include "lvgl.h"
#include "theme.h"
#include <esp_log.h>

static const char *TAG = "PROMPT_DIALOG";

typedef struct {
  prompt_dialog_callback_t callback;
  void *user_data;
  lv_obj_t *dialog;
} prompt_dialog_context_t;

static void no_button_cb(lv_event_t *e) {
  prompt_dialog_context_t *ctx =
      (prompt_dialog_context_t *)lv_event_get_user_data(e);

  ESP_LOGI(TAG, "User selected: No");

  // Call callback with false (No)
  if (ctx && ctx->callback) {
    ctx->callback(false, ctx->user_data);
  }

  // Delete the dialog
  if (ctx && ctx->dialog) {
    lv_obj_del(ctx->dialog);
  }

  // Free the context
  if (ctx) {
    free(ctx);
  }
}

static void yes_button_cb(lv_event_t *e) {
  prompt_dialog_context_t *ctx =
      (prompt_dialog_context_t *)lv_event_get_user_data(e);

  ESP_LOGI(TAG, "User selected: Yes");

  // Call callback with true (Yes)
  if (ctx && ctx->callback) {
    ctx->callback(true, ctx->user_data);
  }

  // Delete the dialog
  if (ctx && ctx->dialog) {
    lv_obj_del(ctx->dialog);
  }

  // Free the context
  if (ctx) {
    free(ctx);
  }
}

void show_prompt_dialog(const char *prompt_text,
                        prompt_dialog_callback_t callback, void *user_data) {
  if (!prompt_text) {
    ESP_LOGE(TAG, "Invalid prompt text");
    return;
  }

  // Allocate context
  prompt_dialog_context_t *ctx =
      (prompt_dialog_context_t *)malloc(sizeof(prompt_dialog_context_t));
  if (!ctx) {
    ESP_LOGE(TAG, "Failed to allocate memory for prompt dialog context");
    return;
  }

  ctx->callback = callback;
  ctx->user_data = user_data;

  // Frameless full-screen dialog
  lv_obj_t *dialog = lv_obj_create(lv_screen_active());
  lv_obj_set_size(dialog, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(dialog);
  ctx->dialog = dialog;

  // Centered text
  lv_obj_t *prompt_label = theme_create_label(dialog, prompt_text, false);
  lv_obj_set_width(prompt_label, LV_PCT(90));
  lv_label_set_long_mode(prompt_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(prompt_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(prompt_label, theme_get_dialog_text_font(), 0);
  lv_obj_center(prompt_label);

  // "No" button - left half at bottom
  lv_obj_t *no_btn = theme_create_button(dialog, "No", false);
  lv_obj_set_size(no_btn, LV_PCT(50), theme_get_button_height());
  lv_obj_align(no_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_add_event_cb(no_btn, no_button_cb, LV_EVENT_CLICKED, ctx);
  lv_obj_t *no_label = lv_obj_get_child(no_btn, 0);
  if (no_label) {
    lv_obj_set_style_text_color(no_label, no_color(), 0);
    lv_obj_set_style_text_font(no_label, theme_get_button_font(), 0);
  }

  // "Yes" button - right half at bottom
  lv_obj_t *yes_btn = theme_create_button(dialog, "Yes", true);
  lv_obj_set_size(yes_btn, LV_PCT(50), theme_get_button_height());
  lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(yes_btn, yes_button_cb, LV_EVENT_CLICKED, ctx);
  lv_obj_t *yes_label = lv_obj_get_child(yes_btn, 0);
  if (yes_label) {
    lv_obj_set_style_text_color(yes_label, yes_color(), 0);
    lv_obj_set_style_text_font(yes_label, theme_get_button_font(), 0);
  }

  ESP_LOGI(TAG, "Prompt dialog created with text: %s", prompt_text);
}
