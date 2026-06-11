#ifndef LED_HELPERS_H
#define LED_HELPERS_H

#include "common.h"

void ws2812_send_strip(int pixel_count, uint8_t g, uint8_t r, uint8_t b);

#endif // LED_HELPERS_H