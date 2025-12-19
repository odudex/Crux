/*
 * Mnemonic Words Backup Page
 * Displays BIP39 mnemonic words for backup
 */

#include "mnemonic_words.h"
#include "../../../key/key.h"
#include "../../../ui_components/theme.h"
#include <esp_log.h>
#include <lvgl.h>
#include <stdio.h>

static const char *TAG = "MNEMONIC_WORDS";

// UI components
static lv_obj_t *mnemonic_screen = NULL;
static void (*return_callback)(void) = NULL;

// Forward declarations
static void back_cb(lv_event_t *e);

// Back callback
static void back_cb(lv_event_t *e) {
  ESP_LOGI(TAG, "Back pressed");

  if (return_callback) {
    return_callback();
  }
}

void mnemonic_words_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object");
    return;
  }

  if (!key_is_loaded()) {
    ESP_LOGE(TAG, "Cannot create page: no key loaded");
    return;
  }

  return_callback = return_cb;

  // Get mnemonic words
  char **words = NULL;
  size_t word_count = 0;

  if (!key_get_mnemonic_words(&words, &word_count)) {
    ESP_LOGE(TAG, "Failed to get mnemonic words");
    return;
  }

  // Create screen container
  mnemonic_screen = theme_create_page_container(parent);

  // Make the screen clickable
  lv_obj_add_flag(mnemonic_screen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(mnemonic_screen, back_cb, LV_EVENT_CLICKED, NULL);

  // Create title label
  lv_obj_t *title_label =
      theme_create_label(mnemonic_screen, "BIP39 Words", false);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, theme_get_default_padding());

  // Content wrapper for words
  lv_obj_t *content_wrapper = lv_obj_create(mnemonic_screen);
  lv_obj_set_size(content_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(content_wrapper, 0, 0);
  lv_obj_set_style_border_width(content_wrapper, 0, 0);
  lv_obj_set_style_bg_opa(content_wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_grow(content_wrapper, 1);
  lv_obj_add_flag(content_wrapper, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_align(content_wrapper, LV_ALIGN_CENTER, 0, 0);

  // Build word list text
  char word_list[512];
  int offset = 0;

  // Two columns
  lv_obj_set_flex_flow(content_wrapper, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(content_wrapper, LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  if (word_count == 12) {
    // Single column
    for (size_t i = 0; i < word_count; i++) {
      offset += snprintf(word_list + offset, sizeof(word_list) - offset,
                         "%s%zu. %s", i > 0 ? "\n" : "", i + 1, words[i]);
    }

    lv_obj_t *words_label =
        theme_create_label(content_wrapper, word_list, false);
    lv_obj_set_style_text_font(words_label, theme_get_dialog_text_font(), 0);
    lv_obj_set_style_text_align(words_label, LV_TEXT_ALIGN_LEFT, 0);

  } else if (word_count == 24) {
    // Left column (words 1-12)
    offset = 0;
    for (size_t i = 0; i < 12; i++) {
      offset += snprintf(word_list + offset, sizeof(word_list) - offset,
                         "%s%zu. %s", i > 0 ? "\n" : "", i + 1, words[i]);
    }
    lv_obj_t *left_label =
        theme_create_label(content_wrapper, word_list, false);
    lv_obj_set_style_text_font(left_label, theme_get_dialog_text_font(), 0);
    lv_obj_set_style_text_align(left_label, LV_TEXT_ALIGN_LEFT, 0);

    // Right column (words 13-24)
    offset = 0;
    for (size_t i = 12; i < 24; i++) {
      offset += snprintf(word_list + offset, sizeof(word_list) - offset,
                         "%s%zu. %s", i > 12 ? "\n" : "", i + 1, words[i]);
    }
    lv_obj_t *right_label =
        theme_create_label(content_wrapper, word_list, false);
    lv_obj_set_style_text_font(right_label, theme_get_dialog_text_font(), 0);
    lv_obj_set_style_text_align(right_label, LV_TEXT_ALIGN_LEFT, 0);

  } else {
    ESP_LOGW(TAG, "Unexpected word count: %zu", word_count);
  }

  // Free words
  for (size_t i = 0; i < word_count; i++) {
    free(words[i]);
  }
  free(words);

  // Hint text at the bottom
  lv_obj_t *hint_label =
      theme_create_label(mnemonic_screen, "Tap to return", false);
  lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0,
               -theme_get_default_padding());

  ESP_LOGI(TAG, "Mnemonic words page created successfully");
}

void mnemonic_words_page_show(void) {
  if (mnemonic_screen) {
    lv_obj_clear_flag(mnemonic_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void mnemonic_words_page_hide(void) {
  if (mnemonic_screen) {
    lv_obj_add_flag(mnemonic_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void mnemonic_words_page_destroy(void) {
  ESP_LOGI(TAG, "Destroying mnemonic words page");

  if (mnemonic_screen) {
    lv_obj_del(mnemonic_screen);
    mnemonic_screen = NULL;
  }

  return_callback = NULL;

  ESP_LOGI(TAG, "Mnemonic words page destroyed");
}
