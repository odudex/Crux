// Flash Error Dialog

#include "flash_error.h"
#include "theme.h"
#include <lvgl.h>
#include <stdlib.h>

typedef struct {
  flash_error_callback_t callback;
  lv_obj_t *modal;
} flash_error_context_t;

static void flash_error_timer_cb(lv_timer_t *timer) {
  flash_error_context_t *ctx = lv_timer_get_user_data(timer);
  if (!ctx)
    return;

  if (ctx->callback)
    ctx->callback();
  if (ctx->modal)
    lv_obj_del(ctx->modal);
  free(ctx);
}

void show_flash_error(const char *message, flash_error_callback_t callback,
                      int timeout_ms) {
  if (!message)
    return;

  if (timeout_ms <= 0)
    timeout_ms = 2000;

  flash_error_context_t *ctx = malloc(sizeof(flash_error_context_t));
  if (!ctx)
    return;

  ctx->callback = callback;
  ctx->modal = lv_obj_create(lv_screen_active());
  lv_obj_set_size(ctx->modal, LV_PCT(80), LV_PCT(80));
  lv_obj_center(ctx->modal);
  theme_apply_frame(ctx->modal);

  lv_obj_t *title = theme_create_label(ctx->modal, "Error", false);
  theme_apply_label(title, true);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *error = theme_create_label(ctx->modal, message, false);
  theme_apply_label(error, false);
  lv_obj_set_style_text_color(error, error_color(), 0);
  lv_obj_set_width(error, LV_PCT(90));
  lv_label_set_long_mode(error, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(error, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(error, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *hint = theme_create_label(ctx->modal, "Returning...", false);
  theme_apply_label(hint, false);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

  lv_timer_t *timer = lv_timer_create(flash_error_timer_cb, timeout_ms, ctx);
  lv_timer_set_repeat_count(timer, 1);
}
