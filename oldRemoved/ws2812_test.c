#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// Define SPI Pins
#define SPI_PORT spi0
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define NUM_PIXELS 60
#define BYTES_PER_PIXEL 3

// Array to hold the full LED frame buffer
uint8_t frame_buffer[NUM_PIXELS * BYTES_PER_PIXEL];

// Helper function to set a pixel's color in GRB format
void set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < NUM_PIXELS) {
        // WS2812B expects data in Green, Red, Blue (GRB) order
        frame_buffer[index * BYTES_PER_PIXEL + 0] = g;
        frame_buffer[index * BYTES_PER_PIXEL + 1] = r;
        frame_buffer[index * BYTES_PER_PIXEL + 2] = b;
    }
}

int main() {
    stdio_init_all();

    // Initialize SPI at 5 MHz
    // (5MHz is plenty fast enough for the FPGA and ensures the FIFO stays fed)
    spi_init(SPI_PORT, 5000000);

    // Set standard SPI format: 8 data bits, CPOL=0, CPHA=0, MSB first
    // This perfectly matches the FPGA's rising-edge detection
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Configure SCK and MOSI pins for SPI functionality
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Configure Chip Select (CS) manually for precise framing
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // CS is active-low, initialize high

    int pattern_offset = 0;

    while (1) {
        // 1. Generate the Red, Green, Blue pattern
        for (int i = 0; i < NUM_PIXELS; i++) {
            // Shift the color based on the pixel index and the time offset
            int color_state = (i + pattern_offset) % 3;

            if (color_state == 0) {
                set_pixel(i, 255, 0, 0);   // Red
            } else if (color_state == 1) {
                set_pixel(i, 0, 255, 0);   // Green
            } else {
                set_pixel(i, 0, 0, 255);   // Blue
            }
        }

        // 2. Transmit the frame buffer to the FPGA
        // Pull CS low to begin SPI transaction
        gpio_put(PIN_CS, 0);
        
        // Write the entire 180-byte buffer blocking
        spi_write_blocking(SPI_PORT, frame_buffer, sizeof(frame_buffer));
        
        // Pull CS high to end transaction
        gpio_put(PIN_CS, 1);

        // 3. Shift the pattern and wait 250ms
        pattern_offset++;
        sleep_ms(250);
    }

    return 0;
}