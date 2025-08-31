#include "splash_screen.h"
#include "../ui_components/theme.h"

// Helper function to create and style a rectangle
static lv_obj_t *create_logo_rect(lv_obj_t *parent, lv_coord_t width,
                                  lv_coord_t height, lv_coord_t x,
                                  lv_coord_t y) {
  lv_obj_t *rect = lv_obj_create(parent);
  lv_obj_set_size(rect, width, height);
  lv_obj_set_pos(rect, x, y);
  theme_apply_solid_rectangle(rect);
  lv_obj_set_style_bg_color(rect, main_color(), 0);

  return rect;
}

void draw_crux_logo(lv_obj_t *parent) {
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
  create_logo_rect(parent, rect_width * 2, rect_height * 8,
                   top_lef_corner_x + 8 * rect_width,
                   top_lef_corner_y + 2 * rect_height);

  // Horizontal cross bar
  create_logo_rect(parent, 10 * rect_width, rect_height,
                   top_lef_corner_x + 4 * rect_width,
                   top_lef_corner_y + 4 * rect_height);

  // C layer 1
  create_logo_rect(parent, 10 * rect_width, rect_height,
                   top_lef_corner_x + 4 * rect_width, top_lef_corner_y);

  // C layer 2a
  create_logo_rect(parent, 4 * rect_width, rect_height,
                   top_lef_corner_x + 2 * rect_width,
                   top_lef_corner_y + rect_height);

  // C layer 2b
  create_logo_rect(parent, 4 * rect_width, rect_height,
                   top_lef_corner_x + 12 * rect_width,
                   top_lef_corner_y + rect_height);

  // C layer 3
  create_logo_rect(parent, 4 * rect_width, rect_height, top_lef_corner_x,
                   top_lef_corner_y + 2 * rect_height);

  // C layer 4,5,6 - vertical bar
  create_logo_rect(parent, 2 * rect_width, 3 * rect_height, top_lef_corner_x,
                   top_lef_corner_y + 3 * rect_height);

  // C layer 7
  create_logo_rect(parent, 4 * rect_width, rect_height, top_lef_corner_x,
                   top_lef_corner_y + 6 * rect_height);

  // C layer 8a
  create_logo_rect(parent, 4 * rect_width, rect_height,
                   top_lef_corner_x + 2 * rect_width,
                   top_lef_corner_y + 7 * rect_height);

  // C layer 8b
  create_logo_rect(parent, 4 * rect_width, rect_height,
                   top_lef_corner_x + 12 * rect_width,
                   top_lef_corner_y + 7 * rect_height);

  // C layer 9
  create_logo_rect(parent, 10 * rect_width, rect_height,
                   top_lef_corner_x + 4 * rect_width,
                   top_lef_corner_y + 8 * rect_height);
}
