
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/timer.h"

// Hardware Module Headers
#include "common.h"
#include "lcd_helpers.h"
#include "led_helpers.h"
#include "touch_helpers.h"

// Static Display Image Buffers
#include "upArrow_rgb565.h"
#include "image2_rgb565.h"
#include "image3_rgb565.h"
#include "image4_rgb565.h"
#include "downArrow_rgb565.h"

// Global Variables linking the State Machines
Quadrant_t requested_quadrant = QUAD_NONE;

// --- Concurrent State Machine Logic ---

// 1. Touch Input State Machine
enum Touch_States { TOUCH_INIT, TOUCH_WAIT_PRESS, TOUCH_PRESSED, TOUCH_WAIT_RELEASE } Touch_State;

void Touch_SM() {
    // State Transitions
    switch(Touch_State) {
        case TOUCH_INIT:
            Touch_State = TOUCH_WAIT_PRESS;
            break;
        case TOUCH_WAIT_PRESS:
            if (is_touched()) {
                Touch_State = TOUCH_PRESSED;
            }
            break;
        case TOUCH_PRESSED:
            Touch_State = TOUCH_WAIT_RELEASE;
            break;
        case TOUCH_WAIT_RELEASE:
            if (!is_touched()) {
                Touch_State = TOUCH_WAIT_PRESS;
            }
            break;
        default:
            Touch_State = TOUCH_INIT;
            break;
    }

    // State Actions
    switch(Touch_State) {
        case TOUCH_PRESSED: {
            uint16_t adc_x = get_smoothed_x();
            uint16_t adc_y = get_smoothed_y();
            int32_t px = map_val(adc_x, ADC_X_MIN, ADC_X_MAX, 0, SCREEN_WIDTH);
            int32_t py = map_val(adc_y, ADC_Y_MIN, ADC_Y_MAX, 0, SCREEN_HEIGHT);
            
            if (px < 240 && py < 136) {
                requested_quadrant = QUAD_TL;
            } else if (px >= 240 && py < 136) {
                requested_quadrant = QUAD_TR;
            } else if (px < 240 && py >= 136) {
                requested_quadrant = QUAD_BL;
            } else {
                requested_quadrant = QUAD_BR;
            }
            break;
        }
        default:
            break;
    }
}

// 2. LCD State Machine 
enum LCD_States { 
    LCD_INIT, 
    LCD_IDLE, 
    LCD_DRAW_TL, 
    LCD_DRAW_TR, 
    LCD_DRAW_BL, 
    LCD_DRAW_BR 
} LCD_State;

Quadrant_t active_lcd_quadrant = QUAD_NONE;

void LCD_SM() {
    // State Transitions
    switch(LCD_State) {
        case LCD_INIT:            
            LCD_State = LCD_IDLE; 
            break;
        case LCD_IDLE:
            if (requested_quadrant != QUAD_NONE && requested_quadrant != active_lcd_quadrant) {
                if (requested_quadrant == QUAD_TL) LCD_State = LCD_DRAW_TL;
                else if (requested_quadrant == QUAD_TR) LCD_State = LCD_DRAW_TR;
                else if (requested_quadrant == QUAD_BL) LCD_State = LCD_DRAW_BL;
                else if (requested_quadrant == QUAD_BR) LCD_State = LCD_DRAW_BR;
            }
            break;
        case LCD_DRAW_TL:
        case LCD_DRAW_TR:
        case LCD_DRAW_BL:
        case LCD_DRAW_BR:     
            LCD_State = LCD_IDLE; 
            break;
        default:                  
            LCD_State = LCD_INIT; 
            break;
    }

    // State Actions: Pre-Clear Phase
    switch(LCD_State) {
        case LCD_DRAW_TL:
        case LCD_DRAW_TR:
        case LCD_DRAW_BL:
        case LCD_DRAW_BR:
            if (active_lcd_quadrant != QUAD_NONE) {
                set_quadrant(COLOR_BLACK, active_lcd_quadrant);
            }
            break;
        default:
            break;
    }

    // State Actions: Drawing Phase
    switch(LCD_State) {
        case LCD_DRAW_TL:
            draw_image(0, 0, 240U, 136U, UPARROW_PIXELS);
            active_lcd_quadrant = QUAD_TL;
            break;
        case LCD_DRAW_TR:
            draw_image(240, 0, 240U, 136U, DOWNARROW_PIXELS);
            active_lcd_quadrant = QUAD_TR;
            break;
        case LCD_DRAW_BL:
            draw_image(0, 136, 240U, 136U, IMAGE4_PIXELS);
            active_lcd_quadrant = QUAD_BL;
            break;
        case LCD_DRAW_BR:
            draw_image(240, 136, 240U, 136U, IMAGE2_PIXELS);
            active_lcd_quadrant = QUAD_BR;
            break;
        default:
            break;
    }
}

// 3. LED State Machine
enum LED_States { 
    LED_INIT, 
    LED_IDLE, 
    LED_SET_TL, 
    LED_SET_TR, 
    LED_SET_BL, 
    LED_SET_BR 
} LED_State;

Quadrant_t active_led_quadrant = QUAD_NONE;

void LED_SM() {
    // State Transitions
    switch(LED_State) {
        case LED_INIT:        
            LED_State = LED_IDLE; 
            break;
        case LED_IDLE:  
            if (requested_quadrant != QUAD_NONE && requested_quadrant != active_led_quadrant) {
                if (requested_quadrant == QUAD_TL) LED_State = LED_SET_TL;
                else if (requested_quadrant == QUAD_TR) LED_State = LED_SET_TR;
                else if (requested_quadrant == QUAD_BL) LED_State = LED_SET_BL;
                else if (requested_quadrant == QUAD_BR) LED_State = LED_SET_BR;
            }
            break;
        case LED_SET_TL:
        case LED_SET_TR:
        case LED_SET_BL:
        case LED_SET_BR:   
            LED_State = LED_IDLE; 
            break;
        default:              
            LED_State = LED_INIT; 
            break;
    }

    // State Actions
    switch(LED_State) {
        case LED_SET_TL:
            ws2812_send_strip(60, 0x00, 0x0F, 0x00); // Red
            active_led_quadrant = QUAD_TL;
            break;
        case LED_SET_TR:
            ws2812_send_strip(60, 0x0F, 0x00, 0x00); // Green 
            active_led_quadrant = QUAD_TR;
            break;
        case LED_SET_BL:
            ws2812_send_strip(60, 0x00, 0x00, 0x0F); // Blue
            active_led_quadrant = QUAD_BL;
            break;
        case LED_SET_BR:
            ws2812_send_strip(60, 0x0F, 0x0F, 0x0F); // White
            active_led_quadrant = QUAD_BR;
            break;
        default:
            break;
    }
}

// --- Timer ISR Execution Framework ---
bool Tick(struct repeating_timer *t) {
    Touch_SM();
    LCD_SM();
    LED_SM();
    return true; 
}

int main() {
    stdio_init_all();
    touch_init();

    // Primary SPI System Allocation Configuration
    spi_init(SPI_PORT, 1000 * 5000);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_DATA_CMD, GPIO_FUNC_SIO);
    gpio_set_function(PIN_CS_LED, GPIO_FUNC_SIO);

    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DATA_CMD, GPIO_OUT);
    gpio_set_dir(PIN_CS_LED, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_DATA_CMD, 0);
    gpio_put(PIN_CS_LED, 1);

    screen_reset();
    sleep_ms(500);
    
    // Initializing State Controllers
    Touch_State = TOUCH_INIT;
    LCD_State = LCD_INIT;
    LED_State = LED_INIT;

    // Dispatching System Timer Interval Context Interrupts (50 ms)
    struct repeating_timer timer;
    add_repeating_timer_ms(-50, Tick, NULL, &timer);
    
    while (true) {
        tight_loop_contents();
    }

    return 0;
}