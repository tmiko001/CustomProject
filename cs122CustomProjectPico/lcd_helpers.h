#ifndef LCD_HELPERS_H
#define LCD_HELPERS_H

#include "common.h"

void fill_window(uint16_t col_start, uint16_t col_end, uint16_t row_start, uint16_t row_end, uint16_t color);
void draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels);
void screen_reset(void);
void set_quadrant(uint16_t color, int quadrant);

#endif // LCD_HELPERS_H