// Dice Rolls Page - Generate mnemonic entropy from dice rolls

#ifndef DICE_ROLLS_H
#define DICE_ROLLS_H

#include <lvgl.h>

void dice_rolls_page_create(lv_obj_t *parent, void (*return_cb)(void));
void dice_rolls_page_show(void);
void dice_rolls_page_hide(void);
void dice_rolls_page_destroy(void);

// Returns generated mnemonic (caller must free), or NULL if not available
char *dice_rolls_get_completed_mnemonic(void);

#endif // DICE_ROLLS_H
