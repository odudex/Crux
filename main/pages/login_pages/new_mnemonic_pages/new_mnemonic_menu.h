// New Mnemonic Menu Page

#ifndef NEW_MNEMONIC_MENU_H
#define NEW_MNEMONIC_MENU_H

#include <lvgl.h>
#include <stdbool.h>

void new_mnemonic_menu_page_create(lv_obj_t *parent, void (*return_cb)(void));
void new_mnemonic_menu_page_show(void);
void new_mnemonic_menu_page_hide(void);
void new_mnemonic_menu_page_destroy(void);

// Navigation functions for external control
bool new_mnemonic_menu_page_navigate_next(void);
bool new_mnemonic_menu_page_navigate_prev(void);
bool new_mnemonic_menu_page_execute_selected(void);
int new_mnemonic_menu_page_get_selected(void);

#endif // NEW_MNEMONIC_MENU_H
