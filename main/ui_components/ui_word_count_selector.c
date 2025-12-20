/*
 * UI Word Count Selector
 * Reusable component for selecting mnemonic word count (12 or 24)
 */

#include "ui_word_count_selector.h"
#include <stdlib.h>

static ui_word_count_selector_t *active_selector = NULL;

static void word_count_12_cb(void) {
  if (active_selector && active_selector->on_select) {
    active_selector->on_select(12);
  }
}

static void word_count_24_cb(void) {
  if (active_selector && active_selector->on_select) {
    active_selector->on_select(24);
  }
}

ui_word_count_selector_t *ui_word_count_selector_create(
    lv_obj_t *parent, ui_menu_callback_t back_cb,
    word_count_callback_t on_select) {
  if (!parent || !on_select)
    return NULL;

  ui_word_count_selector_t *selector =
      malloc(sizeof(ui_word_count_selector_t));
  if (!selector)
    return NULL;

  selector->on_select = on_select;
  selector->menu = ui_menu_create(parent, "Mnemonic Length", back_cb);

  if (!selector->menu) {
    free(selector);
    return NULL;
  }

  ui_menu_add_entry(selector->menu, "12 Words", word_count_12_cb);
  ui_menu_add_entry(selector->menu, "24 Words", word_count_24_cb);

  active_selector = selector;
  ui_menu_show(selector->menu);

  return selector;
}

void ui_word_count_selector_destroy(ui_word_count_selector_t *selector) {
  if (!selector)
    return;

  if (active_selector == selector) {
    active_selector = NULL;
  }

  if (selector->menu) {
    ui_menu_destroy(selector->menu);
  }

  free(selector);
}
