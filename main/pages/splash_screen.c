#include "splash_screen.h"
#include <lvgl.h>

#define TRON_TEXT_PRIMARY lv_color_hex(0x00ccff) // Bright cyan text

// Helper function to create and style a rectangle
static lv_obj_t *create_logo_rect(lv_obj_t *parent, lv_coord_t width,
                                  lv_coord_t height, lv_coord_t x,
                                  lv_coord_t y) {
  lv_obj_t *rect = lv_obj_create(parent);
  lv_obj_set_size(rect, width, height);
  lv_obj_set_pos(rect, x, y);
  lv_obj_set_style_bg_color(rect, TRON_TEXT_PRIMARY, 0);
  lv_obj_set_style_border_width(rect, 0, 0);
  lv_obj_set_style_radius(rect, 0, 0);
  // Remove shadow/outline effects:
  lv_obj_set_style_shadow_width(rect, 0, 0);
  lv_obj_set_style_outline_width(rect, 0, 0);
  lv_obj_set_style_pad_all(rect, 0, 0);
  return rect;
}

void draw_krux_logo(lv_obj_t *parent) {
  // Get screen dimensions
  lv_coord_t screen_width = lv_obj_get_width(parent);
  lv_coord_t screen_height = lv_obj_get_height(parent);

  // Define rectangle dimensions
  lv_coord_t rect_height =
      screen_height / 20; // Adjust height based on screen size
  lv_coord_t rect_width = rect_height / 2;

  lv_coord_t top_lef_corner_x =
      screen_width / 2 - (LOGO_CHAR_WIDTH * rect_width) / 2;
  lv_coord_t top_lef_corner_y =
      screen_height / 2 - (LOGO_CHAR_HEIGHT * rect_height) / 2;

  // Vertical cross bar (center)
  create_logo_rect(parent, rect_width * 2, rect_height * LOGO_CHAR_HEIGHT,
                   top_lef_corner_x + 2 * rect_width, top_lef_corner_y);

  // Horizontal bar
  create_logo_rect(parent, 6 * rect_width, rect_height, top_lef_corner_x,
                   top_lef_corner_y + 3 * rect_height);

  // Top K block
  create_logo_rect(parent, 2 * rect_width, rect_height,
                   top_lef_corner_x + 6 * rect_width,
                   top_lef_corner_y + 5 * rect_height);

  // Second K block
  create_logo_rect(parent, 2 * rect_width, rect_height,
                   top_lef_corner_x + 5 * rect_width,
                   top_lef_corner_y + 6 * rect_height);

  // Third K block
  create_logo_rect(parent, 2 * rect_width, rect_height,
                   top_lef_corner_x + 4 * rect_width,
                   top_lef_corner_y + 7 * rect_height);

  // Fourth K block
  create_logo_rect(parent, 2 * rect_width, rect_height,
                   top_lef_corner_x + 5 * rect_width,
                   top_lef_corner_y + 8 * rect_height);

  // Fifth K block
  create_logo_rect(parent, 2 * rect_width, rect_height,
                   top_lef_corner_x + 6 * rect_width,
                   top_lef_corner_y + 9 * rect_height);

  // Sixth K block
  create_logo_rect(parent, 2 * rect_width, rect_height,
                   top_lef_corner_x + 7 * rect_width,
                   top_lef_corner_y + 10 * rect_height);
}
