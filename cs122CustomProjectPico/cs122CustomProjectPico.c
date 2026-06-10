// //#include <stdio.h>
// #include "pico/stdlib.h"
// #include "hardware/spi.h"
// #include "upArrow_rgb565.h"
// #include "image2_rgb565.h"
// #include "image3_rgb565.h"
// #include "image4_rgb565.h"
// #include "downArrow_rgb565.h"

// // SPI Defines
// // We are going to use SPI 0, and allocate it to the following GPIO pins
// // GP19 = MOSI (pico), GP18 = SCK (sclk), GP17 = CS (cs_n)
// #define SPI_PORT spi0
// #define PIN_MOSI 19
// #define PIN_SCK  18
// #define PIN_CS   17
// #define PIN_DATA_CMD 20  // Data/Command pin
// #define PIN_CS_LED   21  // Separate SPI CS for WS2812 frame data

// // Display Commands
// #define CMD_DISPLAYON 0xAF
// #define CMD_DISPLAYOFF 0xAE
// #define CMD_SETCOLUMNADDRESS 0x15
// #define CMD_SETROWADDRESS 0x75
// #define CMD_CLEARWINDOW 0x25
// #define CMD_FILLWINDOW 0x26



// // RGB565 Colors (16-bit)
// #define COLOR_RED   0xF800
// #define COLOR_GREEN 0x07E0
// #define COLOR_BLUE  0x001F
// #define COLOR_WHITE 0xFFFF
// #define COLOR_YELLOW 0xFFE0
// #define COLOR_BLACK 0x0000

// // Helper function to send a byte via SPI
// static void spi_write_byte(uint8_t byte) {
//     spi_write_blocking(SPI_PORT, &byte, 1);
// }

// // Helper function to send a command (CS low, data_cmd low)
// static void send_command(uint8_t cmd, const uint8_t *params, int param_count) {
//     gpio_put(PIN_DATA_CMD, 0);
//     gpio_put(PIN_CS, 0);

//     spi_write_byte(cmd);
//     for (int i = 0; i < param_count; i++) {
//         spi_write_byte(params[i]);
//     }

//     gpio_put(PIN_CS, 1);
// }

// // Send a pixel byte in data mode
// static void send_pixel_byte(uint8_t byte) {
//     gpio_put(PIN_CS, 0);
//     spi_write_byte(byte);
//     gpio_put(PIN_CS, 1);
// }

// // Send a block of pixel bytes in data mode with CS held low
// static void send_pixel_block(const uint8_t *data, int count) {
//     gpio_put(PIN_CS, 0);
//     spi_write_blocking(SPI_PORT, data, count);
//     gpio_put(PIN_CS, 1);
// }

// // Send a single WS2812 pixel in GRB order.
// static void ws2812_send_pixel(uint8_t g, uint8_t r, uint8_t b) {
//     uint8_t pixel[3] = {g, r, b};
//     spi_write_blocking(SPI_PORT, pixel, 3);
// }


// // Send a WS2812 strip with a uniform color (pixel_count pixels in g,r,b order).
// static void ws2812_send_strip(int pixel_count, uint8_t g, uint8_t r, uint8_t b) {
//     gpio_put(PIN_CS_LED, 0);
//     for (int i = 0; i < pixel_count; i++) {
//         ws2812_send_pixel(g, r, b);
//     }
//     gpio_put(PIN_CS_LED, 1);
//     sleep_us(5);  // CS hold time for FPGA synchronizers
//     sleep_us(80); // Reset gap for WS2812B
// }

// // Fill a window with a color
// static void fill_window(uint16_t col_start, uint16_t col_end, uint16_t row_start, uint16_t row_end, uint16_t color) {
//     uint8_t col_params[] = {(col_start >> 8) & 0xFF, col_start & 0xFF, (col_end >> 8) & 0xFF, col_end & 0xFF};
//     uint8_t row_params[] = {(row_start >> 8) & 0xFF, row_start & 0xFF, (row_end >> 8) & 0xFF, row_end & 0xFF};
    
//     send_command(CMD_SETCOLUMNADDRESS, col_params, 4);
//     send_command(CMD_SETROWADDRESS, row_params, 4);
//     sleep_ms(5);

//     gpio_put(PIN_DATA_CMD, 1);
//     sleep_ms(5);

//     uint8_t color_high = (color >> 8) & 0xFF;
//     uint8_t color_low = color & 0xFF;

//     int width = (col_end - col_start + 1);
//     int height = (row_end - row_start + 1);

//     for (int i = 0; i < width * height; i++) {
//         send_pixel_byte(color_high);
//         send_pixel_byte(color_low);
//     }

//     gpio_put(PIN_DATA_CMD, 0);
//     sleep_ms(5);
// }

// // Draw an RGB565 image at a pixel position (x,y)
// static void draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels) {
//     (void)0; // no-op placeholder; draw_image sends pixels to LCD
//     uint16_t col_start = x * 2;
//     uint16_t col_end = col_start + width * 2 - 1;
//     uint8_t col_params[] = {(col_start >> 8) & 0xFF, col_start & 0xFF, (col_end >> 8) & 0xFF, col_end & 0xFF};
//     uint8_t row_params[] = {(y >> 8) & 0xFF, y & 0xFF, ((y + height - 1) >> 8) & 0xFF, (y + height - 1) & 0xFF};

//     send_command(CMD_SETCOLUMNADDRESS, col_params, 4);
//     send_command(CMD_SETROWADDRESS, row_params, 4);
//     sleep_ms(5);

//     gpio_put(PIN_DATA_CMD, 1);
//     sleep_ms(5);

//     for (int i = 0; i < width * height; i++) {
//         uint16_t pixel = pixels[i];
//         uint8_t pixel_bytes[2] = {(pixel >> 8) & 0xFF, pixel & 0xFF};
//         send_pixel_block(pixel_bytes, 2);
//     }

//     gpio_put(PIN_DATA_CMD, 0);
//     sleep_ms(5);
// }

// // Reset screen to black
// static void screen_reset() {
//     fill_window(0, 959, 0, 271, COLOR_BLACK);
// }

// // Set a quadrant (0-3) to a specific color
// // 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
// static void set_quadrant(uint16_t color, int quadrant) {
//     uint16_t col_start, col_end, row_start, row_end;

//     if (quadrant == 0) {
//         col_start = 0;
//         col_end = 479;
//         row_start = 0;
//         row_end = 135;
//     } else if (quadrant == 1) {
//         col_start = 480;
//         col_end = 959;
//         row_start = 0;
//         row_end = 135;
//     } else if (quadrant == 2) {
//         col_start = 0;
//         col_end = 479;
//         row_start = 136;
//         row_end = 271;
//     } else { // quadrant == 3
//         col_start = 480;
//         col_end = 959;
//         row_start = 136;
//         row_end = 271;
//     }

//     fill_window(col_start, col_end, row_start, row_end, color);
// }


// int main()
// {
//     stdio_init_all();

//     // SPI initialisation: 5 MHz (stable) and CPOL=0, CPHA=0, MSB first
//     spi_init(SPI_PORT, 1000 * 5000);
//     spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
//     gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
//     gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
//     gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
//     gpio_set_function(PIN_DATA_CMD, GPIO_FUNC_SIO);
//     gpio_set_function(PIN_CS_LED, GPIO_FUNC_SIO);

//     gpio_set_dir(PIN_CS, GPIO_OUT);
//     gpio_set_dir(PIN_DATA_CMD, GPIO_OUT);
//     gpio_set_dir(PIN_CS_LED, GPIO_OUT);
//     gpio_put(PIN_CS, 1);
//     gpio_put(PIN_DATA_CMD, 0);
//     gpio_put(PIN_CS_LED, 1);

//     // Send a 60-pixel red strip before entering the main display loop.
    

//     // Start with a black background
//     screen_reset();
//     sleep_ms(500);
//     //480x272 display, so each quadrant is 240x136 pixels
//     // Draw a small embedded image at the top-left of the screen
    
    
    
//     while (true) {
        
//         ws2812_send_strip(60, 0x00, 0x0F, 0x00);  
//         draw_image(0, 0, 240U, 136U, UPARROW_PIXELS);
//         sleep_ms(750);
//         fill_window(0, 480, 0, 136, COLOR_BLACK);

//         ws2812_send_strip(60, 0x0F, 0x00, 0x00); 
//         draw_image(240, 0, 240U, 136U, DOWNARROW_PIXELS);
//         sleep_ms(750);
//         fill_window(480, 959, 0, 136, COLOR_BLACK);

        
//         ws2812_send_strip(60, 0x00, 0x00, 0x0F);
//         draw_image(0, 137, 240U, 136U, IMAGE4_PIXELS);
//         sleep_ms(750);
//         fill_window(0, 479, 136, 271, COLOR_BLACK);

        
//         ws2812_send_strip(60, 0x0F, 0x0F, 0x0F);
//         draw_image(240, 137, 240U, 136U, IMAGE2_PIXELS);
//         sleep_ms(750);
//         fill_window(480, 959, 136, 271, COLOR_BLACK);
//     }
// }


//#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/timer.h" // Required for repeating_timer
#include "upArrow_rgb565.h"
#include "image2_rgb565.h"
#include "image3_rgb565.h"
#include "image4_rgb565.h"
#include "downArrow_rgb565.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// GP19 = MOSI (pico), GP18 = SCK (sclk), GP17 = CS (cs_n)
#define SPI_PORT spi0
#define PIN_MOSI 19
#define PIN_SCK  18
#define PIN_CS   17
#define PIN_DATA_CMD 20  // Data/Command pin
#define PIN_CS_LED   21  // Separate SPI CS for WS2812 frame data

// Display Commands
#define CMD_DISPLAYON 0xAF
#define CMD_DISPLAYOFF 0xAE
#define CMD_SETCOLUMNADDRESS 0x15
#define CMD_SETROWADDRESS 0x75
#define CMD_CLEARWINDOW 0x25
#define CMD_FILLWINDOW 0x26

// RGB565 Colors (16-bit)
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE  0x001F
#define COLOR_WHITE 0xFFFF
#define COLOR_YELLOW 0xFFE0
#define COLOR_BLACK 0x0000

// --- Helper Functions ---

static void spi_write_byte(uint8_t byte) {
    spi_write_blocking(SPI_PORT, &byte, 1);
}

static void send_command(uint8_t cmd, const uint8_t *params, int param_count) {
    gpio_put(PIN_DATA_CMD, 0);
    gpio_put(PIN_CS, 0);

    spi_write_byte(cmd);
    for (int i = 0; i < param_count; i++) {
        spi_write_byte(params[i]);
    }

    gpio_put(PIN_CS, 1);
}

static void send_pixel_byte(uint8_t byte) {
    gpio_put(PIN_CS, 0);
    spi_write_byte(byte);
    gpio_put(PIN_CS, 1);
}

static void send_pixel_block(const uint8_t *data, int count) {
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, data, count);
    gpio_put(PIN_CS, 1);
}

static void ws2812_send_pixel(uint8_t g, uint8_t r, uint8_t b) {
    uint8_t pixel[3] = {g, r, b};
    spi_write_blocking(SPI_PORT, pixel, 3);
}

static void ws2812_send_strip(int pixel_count, uint8_t g, uint8_t r, uint8_t b) {
    gpio_put(PIN_CS_LED, 0);
    for (int i = 0; i < pixel_count; i++) {
        ws2812_send_pixel(g, r, b);
    }
    gpio_put(PIN_CS_LED, 1);
    sleep_us(5);  // CS hold time for FPGA synchronizers
    sleep_us(80); // Reset gap for WS2812B
}

static void fill_window(uint16_t col_start, uint16_t col_end, uint16_t row_start, uint16_t row_end, uint16_t color) {
    uint8_t col_params[] = {(col_start >> 8) & 0xFF, col_start & 0xFF, (col_end >> 8) & 0xFF, col_end & 0xFF};
    uint8_t row_params[] = {(row_start >> 8) & 0xFF, row_start & 0xFF, (row_end >> 8) & 0xFF, row_end & 0xFF};
    
    send_command(CMD_SETCOLUMNADDRESS, col_params, 4);
    send_command(CMD_SETROWADDRESS, row_params, 4);
    sleep_ms(5);

    gpio_put(PIN_DATA_CMD, 1);
    sleep_ms(5);

    uint8_t color_high = (color >> 8) & 0xFF;
    uint8_t color_low = color & 0xFF;

    int width = (col_end - col_start + 1);
    int height = (row_end - row_start + 1);

    for (int i = 0; i < width * height; i++) {
        send_pixel_byte(color_high);
        send_pixel_byte(color_low);
    }

    gpio_put(PIN_DATA_CMD, 0);
    sleep_ms(5);
}

static void draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels) {
    uint16_t col_start = x * 2;
    uint16_t col_end = col_start + width * 2 - 1;
    uint8_t col_params[] = {(col_start >> 8) & 0xFF, col_start & 0xFF, (col_end >> 8) & 0xFF, col_end & 0xFF};
    uint8_t row_params[] = {(y >> 8) & 0xFF, y & 0xFF, ((y + height - 1) >> 8) & 0xFF, (y + height - 1) & 0xFF};

    send_command(CMD_SETCOLUMNADDRESS, col_params, 4);
    send_command(CMD_SETROWADDRESS, row_params, 4);
    sleep_ms(5);

    gpio_put(PIN_DATA_CMD, 1);
    sleep_ms(5);

    for (int i = 0; i < width * height; i++) {
        uint16_t pixel = pixels[i];
        uint8_t pixel_bytes[2] = {(pixel >> 8) & 0xFF, pixel & 0xFF};
        send_pixel_block(pixel_bytes, 2);
    }

    gpio_put(PIN_DATA_CMD, 0);
    sleep_ms(5);
}

static void screen_reset() {
    fill_window(0, 959, 0, 271, COLOR_BLACK);
}

// --- Concurrent State Machines ---

// 1. LCD State Machine
enum LCD_States { LCD_INIT, LCD_UP_ARROW, LCD_DOWN_ARROW, LCD_IMAGE4, LCD_IMAGE2 } LCD_State;

void LCD_SM() {
    // State Transitions
    switch(LCD_State) {
        case LCD_INIT:
            LCD_State = LCD_UP_ARROW;
            break;
        case LCD_UP_ARROW:
            LCD_State = LCD_DOWN_ARROW;
            break;
        case LCD_DOWN_ARROW:
            LCD_State = LCD_IMAGE4;
            break;
        case LCD_IMAGE4:
            LCD_State = LCD_IMAGE2;
            break;
        case LCD_IMAGE2:
            LCD_State = LCD_UP_ARROW;
            break;
        default:
            LCD_State = LCD_INIT;
            break;
    }

    // State Actions
    switch(LCD_State) {
        case LCD_INIT:
            break;
        case LCD_UP_ARROW:
            fill_window(480, 959, 136, 271, COLOR_BLACK); // Clear previous (Image2)
            draw_image(0, 0, 240U, 136U, UPARROW_PIXELS);
            break;
        case LCD_DOWN_ARROW:
            fill_window(0, 480, 0, 136, COLOR_BLACK); // Clear previous (Up Arrow)
            draw_image(240, 0, 240U, 136U, DOWNARROW_PIXELS);
            break;
        case LCD_IMAGE4:
            fill_window(480, 959, 0, 136, COLOR_BLACK); // Clear previous (Down Arrow)
            draw_image(0, 137, 240U, 136U, IMAGE4_PIXELS);
            break;
        case LCD_IMAGE2:
            fill_window(0, 479, 136, 271, COLOR_BLACK); // Clear previous (Image 4)
            draw_image(240, 137, 240U, 136U, IMAGE2_PIXELS);
            break;
    }
}

// 2. LED State Machine
enum LED_States { LED_INIT, LED_GREEN, LED_RED, LED_BLUE, LED_WHITE } LED_State;

void LED_SM() {
    // State Transitions
    switch(LED_State) {
        case LED_INIT:
            LED_State = LED_GREEN;
            break;
        case LED_GREEN:
            LED_State = LED_RED;
            break;
        case LED_RED:
            LED_State = LED_BLUE;
            break;
        case LED_BLUE:
            LED_State = LED_WHITE;
            break;
        case LED_WHITE:
            LED_State = LED_GREEN;
            break;
        default:
            LED_State = LED_INIT;
            break;
    }

    // State Actions
    // ws2812 sends data in G, R, B order
    switch(LED_State) {
        case LED_INIT:
            break;
        case LED_GREEN:
            ws2812_send_strip(60, 0x0F, 0x00, 0x00); 
            break;
        case LED_RED:
            ws2812_send_strip(60, 0x00, 0x0F, 0x00);  
            break;
        case LED_BLUE:
            ws2812_send_strip(60, 0x00, 0x00, 0x0F);
            break;
        case LED_WHITE:
            ws2812_send_strip(60, 0x0F, 0x0F, 0x0F);
            break;
    }
}

// --- Timer ISR Dispatcher ---

bool Tick(struct repeating_timer *t) {
    // Call the independent state machines
    LCD_SM();
    LED_SM();
    
    return true; // Keep the timer repeating
}

int main()
{
    stdio_init_all();

    // SPI initialisation: 5 MHz (stable) and CPOL=0, CPHA=0, MSB first
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

    // Start with a black background
    screen_reset();
    sleep_ms(500);
    
    // Initialize state machine states
    LCD_State = LCD_INIT;
    LED_State = LED_INIT;

    // Set up the repeating timer
    struct repeating_timer timer;
    add_repeating_timer_ms(-1000, Tick, NULL, &timer);
    
    // Idle the main loop
    while (true) {
        tight_loop_contents();
    }

    return 0;
}