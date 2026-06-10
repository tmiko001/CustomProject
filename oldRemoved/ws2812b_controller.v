module ws2812b_controller (
    input wire clk,         // 50 MHz System Clock
    input wire rst_n,       // Active-Low Reset
    
    // SPI Interface
    input wire sclk,
    input wire cs_n,
    input wire mosi,
    
    // LED Output
    output wire led_dout    // Connects to DIN of WS2812B [cite: 60]
);

    wire [23:0] rx_to_fifo_data;
    wire wr_en;
    
    wire [23:0] fifo_to_tx_data;
    wire rd_en;
    wire fifo_empty;
    wire fifo_full;

    // Instantiate SPI Receiver
    spi_rx u_spi_rx (
        .clk(clk),
        .rst_n(rst_n),
        .sclk(sclk),
        .cs_n(cs_n),
        .mosi(mosi),
        .grb_data(rx_to_fifo_data),
        .wr_en(wr_en)
    );

    // Instantiate FIFO Buffer
    sync_fifo #(
        .DATA_WIDTH(24), 
        .ADDR_WIDTH(6)   
    ) u_fifo (
        .clk(clk),
        .rst_n(rst_n),
        .wr_en(wr_en),
        .din(rx_to_fifo_data),
        .rd_en(rd_en),
        .dout(fifo_to_tx_data),
        .empty(fifo_empty),
        .full(fifo_full) // Optional: Route to an output pin if master needs flow control
    );

    // Instantiate WS2812B Transmitter
    ws2812b_tx u_ws2812b_tx (
        .clk(clk),
        .rst_n(rst_n),
        .fifo_data(fifo_to_tx_data),
        .fifo_empty(fifo_empty),
        .rd_en(rd_en),
        .dout(led_dout)
    );

endmodule