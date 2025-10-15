#ifndef MNEMONIC_LOADING_H
#define MNEMONIC_LOADING_H

#include <lvgl.h>

void mnemonic_loading_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                  void (*success_cb)(void), const char *content);
void mnemonic_loading_page_show(void);
void mnemonic_loading_page_hide(void);
void mnemonic_loading_page_destroy(void);

#endif // MNEMONIC_LOADING_H