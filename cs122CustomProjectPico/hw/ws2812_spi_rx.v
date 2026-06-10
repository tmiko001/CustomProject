module ws2812_spi_rx (
    input  wire        clk,
    input  wire        reset,
    input  wire        i_CS_n,
    input  wire        i_SCLK,
    input  wire        i_PICO,
    output reg         pixel_valid,
    output reg [23:0]  pixel_grb
);

    reg [2:0] cs_sync;
    reg [2:0] sclk_sync;
    reg [1:0] pico_sync;

    reg [7:0] byte_shift;
    reg [1:0] byte_count;
    reg [2:0] bit_count;
    reg [23:0] pixel_shift;

    wire cs_active = ~cs_sync[1];
    wire sclk_curr = sclk_sync[1];
    wire sclk_prev = sclk_sync[2];
    wire sample_edge = (sclk_prev == 1'b0 && sclk_curr == 1'b1);

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            cs_sync    <= 3'b111;
            sclk_sync  <= 3'b000;
            pico_sync  <= 2'b00;
            pixel_valid <= 1'b0;
            pixel_grb  <= 24'h000000;
            byte_shift <= 8'h00;
            byte_count <= 2'd0;
            bit_count  <= 3'd7;
            pixel_shift <= 24'h000000;
        end else begin
            cs_sync   <= {cs_sync[1:0], i_CS_n};
            sclk_sync <= {sclk_sync[1:0], i_SCLK};
            pico_sync <= {pico_sync[0], i_PICO};

            pixel_valid <= 1'b0;

            if (!cs_active) begin
                byte_count <= 2'd0;
                bit_count  <= 3'd7;
            end else if (sample_edge) begin
                byte_shift <= {byte_shift[6:0], pico_sync[1]};
                if (bit_count == 3'd0) begin
                    case (byte_count)
                        2'd0: pixel_shift[23:16] <= {byte_shift[6:0], pico_sync[1]};
                        2'd1: pixel_shift[15:8]  <= {byte_shift[6:0], pico_sync[1]};
                        2'd2: begin
                            pixel_shift[7:0] <= {byte_shift[6:0], pico_sync[1]};
                            pixel_valid <= 1'b1;
                            pixel_grb <= {pixel_shift[23:16], pixel_shift[15:8], {byte_shift[6:0], pico_sync[1]}};
                        end
                    endcase

                    if (byte_count == 2'd2) begin
                        byte_count <= 2'd0;
                    end else begin
                        byte_count <= byte_count + 2'd1;
                    end
                    bit_count <= 3'd7;
                end else begin
                    bit_count <= bit_count - 3'd1;
                end
            end
        end
    end

endmodule
