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
#include "imageHeaders\image2_rgb565.h"
#include "imageHeaders\image3_rgb565.h"
#include "imageHeaders\image4_rgb565.h"
#include "imageHeaders\downArrow_rgb565.h"
#include "imageHeaders\roundWin_rgb565.h"
#include "imageHeaders\roundLoss_rgb565.h"
#include "imageHeaders\shreakScreaming_rgb565.h"
#include "imageHeaders\ricoPenguin_rgb565.h"
#include "imageHeaders\cheeringEmoji_rgb565.h"
#include "imageHeaders\Score_rgb565.h"

// Digit Header Files
#include "imageHeaders\0_rgb565.h"
#include "imageHeaders\1_rgb565.h"
#include "imageHeaders\2_rgb565.h"
#include "imageHeaders\3_rgb565.h"
#include "imageHeaders\4_rgb565.h"
#include "imageHeaders\5_rgb565.h"
#include "imageHeaders\6_rgb565.h"
#include "imageHeaders\7_rgb565.h"
#include "imageHeaders\8_rgb565.h"
#include "imageHeaders\9_rgb565.h"

// --- Game Globals ---
Quadrant_t pattern[4];
Quadrant_t user_input[4];
uint8_t step_index = 0;
uint16_t state_timer = 0;
uint8_t score = 0; // Tracks the current win streak score

// Communication between Touch SM and Game SM
bool tap_ready = false;
Quadrant_t latest_tap = QUAD_NONE;

// Reset trigger for 3-second hold
bool force_reset = false;
uint16_t touch_hold_timer = 0;

// --- State Machines ---

// 1. Touch Input State Machine
// Captures one tap event per press, and tracks hold duration
enum Touch_States { TOUCH_INIT, TOUCH_WAIT_PRESS, TOUCH_PRESSED, TOUCH_WAIT_RELEASE } Touch_State;

void Touch_SM() {
    switch(Touch_State) {
        case TOUCH_INIT:
            Touch_State = TOUCH_WAIT_PRESS;
            break;
        case TOUCH_WAIT_PRESS:
            if (is_touched()) {
                // Calculate position right as the screen is pressed
                uint16_t adc_x = get_smoothed_x();
                uint16_t adc_y = get_smoothed_y();
                int32_t px = map_val(adc_x, ADC_X_MIN, ADC_X_MAX, 0, SCREEN_WIDTH);
                int32_t py = map_val(adc_y, ADC_Y_MIN, ADC_Y_MAX, 0, SCREEN_HEIGHT);
                
                if (px < 240 && py < 136) latest_tap = QUAD_TL;
                else if (px >= 240 && py < 136) latest_tap = QUAD_TR;
                else if (px < 240 && py >= 136) latest_tap = QUAD_BL;
                else latest_tap = QUAD_BR;
                
                tap_ready = true; 
                touch_hold_timer = 0; // Start tracking how long the screen is held
                Touch_State = TOUCH_PRESSED;
            }
            break;
        case TOUCH_PRESSED:
            Touch_State = TOUCH_WAIT_RELEASE;
            break;
        case TOUCH_WAIT_RELEASE:
            if (is_touched()) {
                touch_hold_timer++; // Increment hold timer while finger is down
                
                // 60 ticks * 50ms = 3000ms (3 seconds)
                if (touch_hold_timer >= 60) {
                    force_reset = true;
                    touch_hold_timer = 0; // reset to prevent overflow
                }
            } else {
                Touch_State = TOUCH_WAIT_PRESS; // Finger lifted
            }
            break;
        default:
            Touch_State = TOUCH_INIT;
            break;
    }
}

// 2. Master Game State Machine
enum Game_States { 
    GAME_INIT, 
    GAME_WAIT_START,        
    GAME_SHOW_PATTERN, 
    GAME_WAIT_INPUT, 
    GAME_SHOW_TAP_FEEDBACK, 
    GAME_CHECK_RESULT, 
    GAME_WIN, 
    GAME_LOSE 
} Game_State;

void Game_SM() {
    // Immediate override if the user held the screen for 3 seconds
    if (force_reset) {
        force_reset = false;
        Game_State = GAME_INIT;
    }

    switch(Game_State) {
        case GAME_INIT:
            // Display the pre-game start screen
            draw_image(0, 0, 480U, 272U, CHEERINGEMOJI_PIXELS);
            draw_image(0, 0, 150U, 75U, SCORE_PIXELS);
            
            // Render the digit matching the current score (offset at X=150 to prevent overlapping text)
            switch(score) {
                case 0: draw_image(150, 0, 50U, 75U, n0_PIXELS); break;
                case 1: draw_image(150, 0, 50U, 75U, n1_PIXELS); break;
                case 2: draw_image(150, 0, 50U, 75U, n2_PIXELS); break;
                case 3: draw_image(150, 0, 50U, 75U, n3_PIXELS); break;
                case 4: draw_image(150, 0, 50U, 75U, n4_PIXELS); break;
                case 5: draw_image(150, 0, 50U, 75U, n5_PIXELS); break;
                case 6: draw_image(150, 0, 50U, 75U, n6_PIXELS); break;
                case 7: draw_image(150, 0, 50U, 75U, n7_PIXELS); break;
                case 8: draw_image(150, 0, 50U, 75U, n8_PIXELS); break;
                case 9: draw_image(150, 0, 50U, 75U, n9_PIXELS); break;
                default: break;
            }
            
            tap_ready = false; // Purge any existing taps
            Game_State = GAME_WAIT_START;
            break;

        case GAME_WAIT_START:
            // Wait here until the user taps the screen to begin
            if (tap_ready) {
                tap_ready = false;
                
                // Reset score for the brand new session if they previously lost
                // (If they paused via hold, this clears it out as they start fresh)
                score = 0; 
                
                // Generate random 4-step sequence
                for(int i = 0; i < 4; i++) {
                    pattern[i] = (Quadrant_t)(rand() % 4);
                }
                
                step_index = 0;
                state_timer = 0;
                screen_reset();
                Game_State = GAME_SHOW_PATTERN;
            }
            break;

        case GAME_SHOW_PATTERN:
            if (state_timer == 0) {
                // Draw current pattern image and light up corresponding LED color
                switch(pattern[step_index]) {
                    case QUAD_TL: 
                        ws2812_send_strip(60, 0x0F, 0x00, 0x00); 
                        draw_image(0, 0, 240U, 136U, SHREAKSCREAMING_PIXELS);  
                        sleep_ms(500);  
                        break;
                    case QUAD_TR: 
                        ws2812_send_strip(60, 0x0F, 0x0B, 0x00); 
                        draw_image(240, 0, 240U, 136U, RICOPENGUIN_PIXELS);
                        sleep_ms(500);
                        break;
                    case QUAD_BL: 
                        ws2812_send_strip(60, 0x00, 0x00, 0x0F); // Blue
                        draw_image(0, 136, 240U, 136U, IMAGE4_PIXELS);
                        sleep_ms(500);
                        break;
                    case QUAD_BR: 
                        ws2812_send_strip(60, 0x0F, 0x0F, 0x0F); // White
                        draw_image(240, 136, 240U, 136U, IMAGE2_PIXELS);
                        sleep_ms(500);
                        break;
                    default: break;
                }
            }
            else if (state_timer == 25) {
                // Clear the screen slightly early (1.25s) to provide visual separation
                screen_reset();
            }
            
            state_timer++;
            
            // 30 ticks * 50ms = 1500ms (1.5 seconds)
            if (state_timer >= 30) { 
                state_timer = 0;
                step_index++;
                
                // If we've shown all 4 images, wait for user
                if (step_index >= 4) {
                    step_index = 0;
                    tap_ready = false; // Purge any impatient taps
                    Game_State = GAME_WAIT_INPUT;
                }
            }
            break;

        case GAME_WAIT_INPUT:
            if (tap_ready) {
                user_input[step_index] = latest_tap; // Record tap
                tap_ready = false;
                
                // Immediately display what the user tapped
                switch(latest_tap) {
                    case QUAD_TL: 
                        draw_image(0, 0, 240U, 136U, SHREAKSCREAMING_PIXELS); 
                        ws2812_send_strip(60, 0x0F, 0x00, 0x00); 
                        break;
                    case QUAD_TR: 
                        draw_image(240, 0, 240U, 136U, RICOPENGUIN_PIXELS); 
                        ws2812_send_strip(60, 0x0F, 0x0B, 0x00); 
                        break;
                    case QUAD_BL: 
                        draw_image(0, 136, 240U, 136U, IMAGE4_PIXELS);
                        ws2812_send_strip(60, 0x00, 0x00, 0x0F);
                        break;
                    case QUAD_BR: 
                        draw_image(240, 136, 240U, 136U, IMAGE2_PIXELS); 
                        ws2812_send_strip(60, 0x0F, 0x0F, 0x0F); 
                        break;
                    default: break;
                }

                state_timer = 0;
                Game_State = GAME_SHOW_TAP_FEEDBACK; // Go to feedback state
            }
            break;

        case GAME_SHOW_TAP_FEEDBACK:
            state_timer++;
            // Show feedback for 300ms (6 ticks * 50ms)
            if (state_timer >= 6) {
                screen_reset();

                step_index++; // Advance to the next required tap
                
                // Check if they have entered all 4 taps
                if (step_index >= 4) {
                    Game_State = GAME_CHECK_RESULT;
                } else {
                    tap_ready = false; // Purge impatient taps during feedback
                    Game_State = GAME_WAIT_INPUT; // Wait for the next tap
                }
            }
            break;

        case GAME_CHECK_RESULT: {
            bool is_winner = true;
            for(int i = 0; i < 4; i++) {
                if (pattern[i] != user_input[i]) {
                    is_winner = false;
                    break; // Mismatch found
                }
            }
            
            state_timer = 0;
            screen_reset();
            
            if (is_winner) Game_State = GAME_WIN;
            else Game_State = GAME_LOSE;
            break;
        }

        case GAME_WIN:
            if (state_timer == 0) {
                ws2812_send_strip(60, 0x7F, 0x00, 0x00);      
                draw_image(0, 0, 480U, 272U, ROUNDWIN_PIXELS); 
                sleep_ms(250);
                
                // Increment score and cap at 9
                score++;
                if (score > 9) score = 9;
            }
            
            state_timer++;
            // Show win result for 3 seconds (60 ticks * 50ms)
            if (state_timer >= 60) { 
                // WIN CONTINUATION: Generate a new random 4-step sequence
                for(int i = 0; i < 4; i++) {
                    pattern[i] = (Quadrant_t)(rand() % 4);
                }
                step_index = 0;
                state_timer = 0;
                screen_reset();
                
                // Skip the wait state and immediately start showing the new pattern
                Game_State = GAME_SHOW_PATTERN;
            }
            break;

        case GAME_LOSE:
            if (state_timer == 0) {
                ws2812_send_strip(60, 0x00, 0x7F, 0x00);        
                draw_image(0, 0, 480U, 272U, ROUNDLOSS_PIXELS); 
                sleep_ms(500); 
            }
            
            state_timer++;
            // Pause for 500ms (10 ticks * 50ms) on loss screen
            if (state_timer >= 10) { 
                // Loss sends the game back to the drawing board to wait for start
                Game_State = GAME_INIT;
            }
            break;
            
        default:
            Game_State = GAME_INIT;
            break;
    }
}

// --- Timer ISR Execution Framework ---
bool Tick(struct repeating_timer *t) {
    Touch_SM();
    Game_SM();
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

    // Seed the random number generator using the Pico's boot timer
    srand(time_us_32());
    
    // Initializing State Controllers
    Touch_State = TOUCH_INIT;
    Game_State = GAME_INIT;

    // Dispatching System Timer Interval Context Interrupts (50 ms)
    struct repeating_timer timer;
    add_repeating_timer_ms(-50, Tick, NULL, &timer);
    
    while (true) {
        tight_loop_contents();
    }

    return 0;
}