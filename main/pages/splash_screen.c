#include "splash_screen.h"
#include "lvgl.h"

#define TRON_TEXT_PRIMARY  lv_color_hex(0x00ccff)  // Bright cyan text

void draw_krux_logo(lv_obj_t *parent)
{
    // Get screen dimensions
    lv_coord_t screen_width = lv_obj_get_width(parent);
    lv_coord_t screen_height = lv_obj_get_height(parent);
    
    // Define rectangle dimensions
    lv_coord_t rect_height = screen_height / 20; // Adjust height based on screen size
    lv_coord_t rect_width = rect_height / 2;
    
    lv_coord_t top_lef_corner_x = screen_width / 2 - (LOGO_CHAR_WIDTH * rect_width) / 2;
    lv_coord_t top_lef_corner_y = screen_height / 2 - (LOGO_CHAR_HEIGHT * rect_height) / 2;
    
    // Vertical cross bar (center)
    lv_obj_t *rect1 = lv_obj_create(parent);
    lv_obj_set_size(rect1, rect_width * 2, rect_height * LOGO_CHAR_HEIGHT);
    lv_obj_set_pos(rect1, top_lef_corner_x + 2 * rect_width, top_lef_corner_y);
    lv_obj_set_style_bg_color(rect1, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect1, 0, 0);
    lv_obj_set_style_radius(rect1, 0, 0);
    
    // Horizontal bar
    lv_obj_t *rect2 = lv_obj_create(parent);
    lv_obj_set_size(rect2, 6 * rect_width, rect_height);
    lv_obj_set_pos(rect2, top_lef_corner_x, top_lef_corner_y + 3 * rect_height);
    lv_obj_set_style_bg_color(rect2, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect2, 0, 0);
    lv_obj_set_style_radius(rect2, 0, 0);
    
    // Top K block
    lv_obj_t *rect3 = lv_obj_create(parent);
    lv_obj_set_size(rect3, 2 * rect_width, rect_height);
    lv_obj_set_pos(rect3, top_lef_corner_x + 6 * rect_width, top_lef_corner_y + 5 * rect_height);
    lv_obj_set_style_bg_color(rect3, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect3, 0, 0);
    lv_obj_set_style_radius(rect3, 0, 0);
    
    // Second K block
    lv_obj_t *rect4 = lv_obj_create(parent);
    lv_obj_set_size(rect4, 2 * rect_width, rect_height);
    lv_obj_set_pos(rect4, top_lef_corner_x + 5 * rect_width, top_lef_corner_y + 6 * rect_height);
    lv_obj_set_style_bg_color(rect4, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect4, 0, 0);
    lv_obj_set_style_radius(rect4, 0, 0);
    
    // Third K block
    lv_obj_t *rect5 = lv_obj_create(parent);
    lv_obj_set_size(rect5, 2 * rect_width, rect_height);
    lv_obj_set_pos(rect5, top_lef_corner_x + 4 * rect_width, top_lef_corner_y + 7 * rect_height);
    lv_obj_set_style_bg_color(rect5, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect5, 0, 0);
    lv_obj_set_style_radius(rect5, 0, 0);
    
    // Fourth K block
    lv_obj_t *rect6 = lv_obj_create(parent);
    lv_obj_set_size(rect6, 2 * rect_width, rect_height);
    lv_obj_set_pos(rect6, top_lef_corner_x + 5 * rect_width, top_lef_corner_y + 8 * rect_height);
    lv_obj_set_style_bg_color(rect6, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect6, 0, 0);
    lv_obj_set_style_radius(rect6, 0, 0);
    
    // Fifth K block
    lv_obj_t *rect7 = lv_obj_create(parent);
    lv_obj_set_size(rect7, 2 * rect_width, rect_height);
    lv_obj_set_pos(rect7, top_lef_corner_x + 6 * rect_width, top_lef_corner_y + 9 * rect_height);
    lv_obj_set_style_bg_color(rect7, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect7, 0, 0);
    lv_obj_set_style_radius(rect7, 0, 0);
    
    // Sixth K block
    lv_obj_t *rect8 = lv_obj_create(parent);
    lv_obj_set_size(rect8, 2 * rect_width, rect_height);
    lv_obj_set_pos(rect8, top_lef_corner_x + 7 * rect_width, top_lef_corner_y + 10 * rect_height);
    lv_obj_set_style_bg_color(rect8, TRON_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_width(rect8, 0, 0);
    lv_obj_set_style_radius(rect8, 0, 0);
}
