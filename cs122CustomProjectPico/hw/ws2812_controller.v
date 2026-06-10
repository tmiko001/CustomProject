module ws2812_controller (
    input  wire        clk,
    input  wire        reset,
    input  wire        cs_led_n,
    input  wire        sclk,
    input  wire        pico,
    output wire        ws_data,
    output wire        busy,
    output wire        fifo_full
);

    wire pixel_valid;
    wire [23:0] pixel_grb;
    wire fifo_empty;
    wire [23:0] fifo_dout;
    wire rd_en;

    ws2812_spi_rx spi_rx_inst (
        .clk(clk),
        .reset(reset),
        .i_CS_n(cs_led_n),
        .i_SCLK(sclk),
        .i_PICO(pico),
        .pixel_valid(pixel_valid),
        .pixel_grb(pixel_grb)
    );

    sync_fifo #(
        .DATA_WIDTH(24),
        .ADDR_WIDTH(6)  // 64 pixels (128 bytes) for 120-pixel strip robustness
    ) fifo_inst (
        .clk(clk),
        .reset(reset),
        .wr_en(pixel_valid && !fifo_full),
        .din(pixel_grb),
        .dout(fifo_dout),
        .full(fifo_full),
        .empty(fifo_empty),
        .rd_en(rd_en)
    );

    ws2812_tx tx_inst (
        .clk(clk),
        .reset(reset),
        .fifo_empty(fifo_empty),
        .fifo_dout(fifo_dout),
        .rd_en(rd_en),
        .ws_data(ws_data),
        .busy(busy)
    );

endmodule
