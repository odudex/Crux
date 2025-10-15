#ifndef PUBLIC_KEY_H
#define PUBLIC_KEY_H

#include <lvgl.h>

void public_key_page_create(lv_obj_t *parent, void (*return_cb)(void));
void public_key_page_show(void);
void public_key_page_hide(void);
void public_key_page_destroy(void);

#endif // PUBLIC_KEY_H
