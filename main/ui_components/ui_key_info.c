#include "ui_key_info.h"
#include "../key/key.h"
#include "../wallet/wallet.h"
#include "icons/icons_24.h"
#include "theme.h"

lv_obj_t *ui_icon_text_row_create(lv_obj_t *parent, const char *icon,
                                  const char *text, lv_color_t color) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(cont, 8, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *icon_label = lv_label_create(cont);
  lv_label_set_text(icon_label, icon);
  lv_obj_set_style_text_font(icon_label, &icons_24, 0);
  lv_obj_set_style_text_color(icon_label, color, 0);

  lv_obj_t *text_label = lv_label_create(cont);
  lv_label_set_text(text_label, text);
  lv_obj_set_style_text_font(text_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(text_label, color, 0);

  return cont;
}

lv_obj_t *ui_fingerprint_create(lv_obj_t *parent, lv_color_t color) {
  char fingerprint_hex[9];
  if (!key_get_fingerprint_hex(fingerprint_hex))
    return NULL;
  return ui_icon_text_row_create(parent, ICON_FINGERPRINT, fingerprint_hex,
                                 color);
}

lv_obj_t *ui_derivation_create(lv_obj_t *parent, lv_color_t color) {
  const char *derivation = wallet_get_derivation();
  if (!derivation)
    return NULL;
  return ui_icon_text_row_create(parent, ICON_DERIVATION, derivation, color);
}

lv_obj_t *ui_key_info_create(lv_obj_t *parent) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(cont, 20, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  if (!ui_fingerprint_create(cont, highlight_color()) ||
      !ui_derivation_create(cont, secondary_color())) {
    lv_obj_del(cont);
    return NULL;
  }

  return cont;
}
