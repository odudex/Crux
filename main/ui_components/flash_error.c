#include "flash_error.h"
#include "lvgl.h"
#include "theme.h"
#include <esp_log.h>

static const char *TAG = "FLASH_ERROR";

typedef struct {
  flash_error_callback_t callback;
  lv_obj_t *modal;
} flash_error_context_t;

static void flash_error_timer_cb(lv_timer_t *timer) {
  flash_error_context_t *ctx =
      (flash_error_context_t *)lv_timer_get_user_data(timer);

  ESP_LOGI(TAG, "Auto-returning after flash error");

  // Call the return callback if provided
  if (ctx && ctx->callback) {
    ctx->callback();
  }

  // Delete the modal
  if (ctx && ctx->modal) {
    lv_obj_del(ctx->modal);
  }

  // Free the context
  if (ctx) {
    free(ctx);
  }

  // Timer is automatically deleted after this one-shot execution
}

void show_flash_error(const char *error_message,
                      flash_error_callback_t return_callback, int timeout_ms) {
  if (!error_message) {
    ESP_LOGE(TAG, "Invalid error message");
    return;
  }

  // Use default timeout if not specified
  if (timeout_ms <= 0) {
    timeout_ms = 2000;
  }

  // Allocate context
  flash_error_context_t *ctx =
      (flash_error_context_t *)malloc(sizeof(flash_error_context_t));
  if (!ctx) {
    ESP_LOGE(TAG, "Failed to allocate memory for flash error context");
    return;
  }

  ctx->callback = return_callback;

  // Create modal dialog
  lv_obj_t *modal = lv_obj_create(lv_screen_active());
  lv_obj_set_size(modal, LV_PCT(80), LV_PCT(80));
  lv_obj_center(modal);
  ctx->modal = modal;

  // Apply theme to modal
  theme_apply_frame(modal);

  // Title "Error"
  lv_obj_t *title_label = theme_create_label(modal, "Error", false);
  theme_apply_label(title_label, true);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

  // Error message
  lv_obj_t *error_label = theme_create_label(modal, error_message, false);
  theme_apply_label(error_label, false);
  lv_obj_set_style_text_color(error_label, error_color(), 0);
  lv_obj_set_width(error_label, LV_PCT(90));
  lv_label_set_long_mode(error_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(error_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);

  // "Returning..." text
  lv_obj_t *return_label = theme_create_label(modal, "Returning...", false);
  theme_apply_label(return_label, false);
  lv_obj_align(return_label, LV_ALIGN_BOTTOM_MID, 0, -10);

  // Create one-shot timer for auto-return
  lv_timer_t *timer = lv_timer_create(flash_error_timer_cb, timeout_ms, ctx);
  lv_timer_set_repeat_count(timer, 1); // One-shot timer

  ESP_LOGI(TAG, "Flash error displayed: %s (timeout: %dms)", error_message,
           timeout_ms);
}
