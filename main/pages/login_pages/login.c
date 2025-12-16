#include "login.h"
#include "../../ui_components/simple_dialog.h"
#include "../../ui_components/ui_menu.h"
#include "about.h"
#include "load_mnemonic_pages/load_menu.h"
#include "lvgl.h"
#include "new_mnemonic_pages/new_mnemonic_menu.h"

static ui_menu_t *login_menu = NULL;
static lv_obj_t *login_screen = NULL;

static void load_mnemonic_cb(void);
static void new_mnemonic_cb(void);
static void settings_cb(void);
static void tools_cb(void);
static void about_cb(void);

static void return_to_login_cb(void) {
  about_page_destroy();
  login_page_show();
}

static void return_from_load_menu_cb(void) { login_page_show(); }

static void return_from_new_mnemonic_menu_cb(void) { login_page_show(); }

static void load_mnemonic_cb(void) {
  login_page_hide();
  load_menu_page_create(lv_screen_active(), return_from_load_menu_cb);
  load_menu_page_show();
}

static void new_mnemonic_cb(void) {
  login_page_hide();
  new_mnemonic_menu_page_create(lv_screen_active(),
                                return_from_new_mnemonic_menu_cb);
  new_mnemonic_menu_page_show();
}

static void settings_cb(void) {
  show_simple_dialog("Login", "Settings not implemented yet");
}

static void tools_cb(void) {
  show_simple_dialog("Login", "Tools not implemented yet");
}

static void about_cb(void) {
  login_page_hide();
  about_page_create(lv_screen_active(), return_to_login_cb);
  about_page_show();
}

void login_page_create(lv_obj_t *parent) {
  login_screen = lv_obj_create(parent);
  lv_obj_set_size(login_screen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(login_screen, lv_color_hex(0x1e1e1e), 0);
  lv_obj_set_style_border_width(login_screen, 0, 0);
  lv_obj_set_style_pad_all(login_screen, 0, 0);
  lv_obj_set_style_radius(login_screen, 0, 0);
  lv_obj_set_style_shadow_width(login_screen, 0, 0);
  lv_obj_clear_flag(login_screen, LV_OBJ_FLAG_SCROLLABLE);

  login_menu = ui_menu_create(login_screen, "Login", NULL);
  ui_menu_add_entry(login_menu, "Load Mnemonic", load_mnemonic_cb);
  ui_menu_add_entry(login_menu, "New Mnemonic", new_mnemonic_cb);
  // ui_menu_add_entry(login_menu, "Settings", settings_cb);
  // ui_menu_add_entry(login_menu, "Tools", tools_cb);
  ui_menu_add_entry(login_menu, "About", about_cb);
  ui_menu_show(login_menu);
}

void login_page_show(void) {
  if (login_menu)
    ui_menu_show(login_menu);
}

void login_page_hide(void) {
  if (login_menu)
    ui_menu_hide(login_menu);
}

void login_page_destroy(void) {
  if (login_menu) {
    ui_menu_destroy(login_menu);
    login_menu = NULL;
  }
  if (login_screen) {
    lv_obj_del(login_screen);
    login_screen = NULL;
  }
}

bool login_page_navigate_next(void) {
  return login_menu ? ui_menu_navigate_next(login_menu) : false;
}

bool login_page_navigate_prev(void) {
  return login_menu ? ui_menu_navigate_prev(login_menu) : false;
}

bool login_page_execute_selected(void) {
  return login_menu ? ui_menu_execute_selected(login_menu) : false;
}

int login_page_get_selected(void) {
  return login_menu ? ui_menu_get_selected(login_menu) : -1;
}
