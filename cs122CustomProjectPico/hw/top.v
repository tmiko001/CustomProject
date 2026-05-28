`include "async_fifo.v"
`include "lcd_fb.v"
`include "lcd_timing.v"
`include "pll_clocks.v"
`include "sdram_controller.v"
`include "display_controller.v"
`include "ws2812_regs.v"
`include "ws2812_writer.v"
`include "ws2812_spi_controller.v"

module top (
    // iCESugar-Pro 25MHz onboard clock (Pin P6)
    input  wire        clk_25m,       

    // IS42S16160B SDRAM Interface
    output wire        sdram_clk,
    output wire        sdram_cke,
    output wire        sdram_cs_n,
    output wire        sdram_ras_n,
    output wire        sdram_cas_n,
    output wire        sdram_we_n,
    output wire [1:0]  sdram_ba,
    output wire [12:0] sdram_a,
    output wire [1:0]  sdram_dqm,
    inout  wire [15:0] sdram_dq,

    // RGB LCD Interface (480x272)
    output wire        lcd_clk,
    output wire        lcd_hsync,
    output wire        lcd_vsync,
    output wire        lcd_de,
    output wire [7:0]  lcd_r,
    output wire [7:0]  lcd_g,
    output wire [7:0]  lcd_b,

    // SPI Peripheral for Display Commands from MCU
    input wire         sclk,
    input wire         pico,
    input wire         cs_n,
    input wire         data_cmd,

    // additional CS for LED SPI
    input wire         cs_led_n,

    // WS2812 data output
    output wire        ws_data,

    output wire [7:0]  dbg
);

    reg        wr_en = 0;
    wire       wr_ack;
    reg [23:0] wr_addr = 24'h000000;       
    reg [15:0] wr_data = 16'h0000;

    icesugar_pro_lcd_fb fb_inst (
        .clk_25m(clk_25m),
        
        .sdram_clk(sdram_clk),
        .sdram_cke(sdram_cke),
        .sdram_cs_n(sdram_cs_n),
        .sdram_ras_n(sdram_ras_n),
        .sdram_cas_n(sdram_cas_n),
        .sdram_we_n(sdram_we_n),
        .sdram_ba(sdram_ba),
        .sdram_a(sdram_a),
        .sdram_dqm(sdram_dqm),
        .sdram_dq(sdram_dq),

        .lcd_clk(lcd_clk),
        .lcd_hsync(lcd_hsync),
        .lcd_vsync(lcd_vsync),
        .lcd_de(lcd_de),
        .lcd_r(lcd_r[7:3]),
        .lcd_g(lcd_g[7:2]),
        .lcd_b(lcd_b[7:3]),
        
        .wr_en(wr_en),
        .wr_addr(wr_addr),
        .wr_data(wr_data),
        .wr_ack(wr_ack),
        .clk_100m(clk_100m),
        .locked(locked)
    );

    assign lcd_r[0:2] = {3{lcd_r[3]}};
    assign lcd_g[0:1] = {2{lcd_g[2]}};
    assign lcd_b[0:2] = {3{lcd_b[3]}};

    wire locked;
    wire reset = ~locked;
    wire clk_100m;
    wire [7:0]  pixel_half;
    wire [23:0] pixel_addr;
    wire        pixel_wr_en;

    display_controller # (.DISPLAY_WIDTH(960), .DISPLAY_HEIGHT(272)) controller_inst (
        .clk(clk_25m),
        .reset(reset),
        .disp_CS_n(cs_n),
        .disp_SCLK(sclk),
        .disp_PICO(pico),
        .data_cmd(data_cmd),
        .framebuffer_addr_out(pixel_addr),
        .framebuffer_data_out(pixel_half),
        .framebuffer_wren(pixel_wr_en)
    );

    assign dbg = {data_cmd, pico, sclk, cs_n};

    // --- WS2812 integration signals ---
    wire        ws_rx_ready;
    wire [7:0]  ws_rx_data;
    wire        ws_poci;

    // write path from SPI controller to regs
    wire        ws_wr_en;
    wire [1:0]  ws_wr_bank;
    wire [5:0]  ws_wr_index;
    wire [23:0] ws_wr_data;

    // reader side for writer
    wire        ws_rd_req;
    wire [1:0]  ws_rd_bank;
    wire [5:0]  ws_rd_index;
    wire [23:0] ws_rd_data;
    wire        ws_rd_valid;

    wire        ws_start;
    wire [1:0]  ws_start_bank;
    wire        ws_busy;

    // instantiate a spi_peripheral sampled in clk_100m domain so transfers land in same domain as regs/writer
    spi_peripheral # (.SPI_MODE(0)) spi_led(
        .clk(clk_100m),
        .reset(reset),
        .i_CS_n(cs_led_n),
        .i_SCLK(sclk),
        .i_PICO(pico),
        .o_POCI(ws_poci),
        .o_rx_ready(ws_rx_ready),
        .o_rx_data(ws_rx_data)
    );

    // SPI -> register file controller (operates in clk_100m domain)
    ws2812_spi_controller ws_spi_ctrl(
        .clk(clk_100m),
        .rst(reset),
        .rx_ready(ws_rx_ready),
        .rx_data(ws_rx_data),
        .wr_en(ws_wr_en),
        .wr_bank(ws_wr_bank),
        .wr_index(ws_wr_index),
        .wr_data(ws_wr_data),
        .start(ws_start),
        .start_bank(ws_start_bank),
        .busy(ws_busy)
    );

    // Register file (clk_100m domain)
    ws2812_regs ws_regs(
        .clk(clk_100m),
        .rst(reset),
        .wr_en(ws_wr_en),
        .wr_bank(ws_wr_bank),
        .wr_index(ws_wr_index),
        .wr_data(ws_wr_data),
        .rd_req(ws_rd_req),
        .rd_bank(ws_rd_bank),
        .rd_index(ws_rd_index),
        .rd_data(ws_rd_data),
        .rd_valid(ws_rd_valid)
    );

    // Writer: serialize and send to physical pin
    ws2812_writer writer(
        .clk(clk_100m),
        .rst(reset),
        .start(ws_start),
        .bank_sel(ws_start_bank),
        .pixel_count(60),
        .busy(),
        .ws_data(ws_data),
        .rd_req(ws_rd_req),
        .rd_bank(ws_rd_bank),
        .rd_index(ws_rd_index),
        .rd_data(ws_rd_data),
        .rd_valid(ws_rd_valid)
    );

    integer clock_count = 0;
    wire reset;
    reg pixel_wr_en_1 = 0, pixel_wr_en_2 = 0, pixel_wr_en_3;
    wire pixel_wr_en_edge = pixel_wr_en_2 && !pixel_wr_en_3;

    always @(posedge clk_100m) begin
        if (~reset) begin
            pixel_wr_en_1 <= pixel_wr_en;
            pixel_wr_en_2 <= pixel_wr_en_1;
            pixel_wr_en_3 <= pixel_wr_en_2;

            wr_en <= wr_en & ~wr_ack;
            if (pixel_wr_en) begin
                if (~pixel_addr[0]) begin
                    wr_data[7:0] <= pixel_half;
                end else begin
                    wr_data[15:8] <= pixel_half;
                    wr_addr <= {0, pixel_addr[22:1]};
                    wr_en <= 1;
                end 
            end 
        end
    end

    // integer clk_count = 0;
    // always @(posedge clk_25m) begin
    //     if (~reset) begin
    //         if (wr_addr <= 24'h01FE00) begin
    //             wr_data <= wr_addr[4] ? 16'hF800 : 16'hFF00;
    //             wr_en <= 1;
    //             if (wr_ack) wr_addr <= wr_addr + 1;
    //         end else begin
    //             wr_en <= 0;
    //         end
    //     end
    // end

endmodule