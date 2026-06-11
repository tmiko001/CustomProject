#include "led_helpers.h"

static void ws2812_send_pixel(uint8_t g, uint8_t r, uint8_t b) {
    uint8_t pixel[3] = {g, r, b};
    spi_write_blocking(SPI_PORT, pixel, 3);
}

void ws2812_send_strip(int pixel_count, uint8_t g, uint8_t r, uint8_t b) {
    gpio_put(PIN_CS_LED, 0);
    for (int i = 0; i < pixel_count; i++) ws2812_send_pixel(g, r, b);
    gpio_put(PIN_CS_LED, 1);
    sleep_us(5);  
    sleep_us(80); 
}