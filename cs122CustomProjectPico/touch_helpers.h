#ifndef TOUCH_HELPERS_H
#define TOUCH_HELPERS_H

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

// --- Touchscreen Mapping Configuration ---
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

#define ADC_X_MIN 315
#define ADC_X_MAX 3750
#define ADC_Y_MIN 600
#define ADC_Y_MAX 3350

#define X_POS 26 // ADC Pin (ADC0)
#define Y_POS 27 // ADC Pin (ADC1)
#define X_NEG 6 
#define Y_NEG 7 

struct TouchPoint {
    bool is_pressed;
    int16_t x;
    int16_t y;
};

// --- Exported Functions ---
bool is_touched(void);
uint16_t read_x(void);
uint16_t read_y(void);
uint16_t get_smoothed_x(void);
uint16_t get_smoothed_y(void);
int32_t map_val(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
void touch_init(void);

#endif // TOUCH_HELPERS_H