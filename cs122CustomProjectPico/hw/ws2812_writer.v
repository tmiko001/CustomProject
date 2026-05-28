module ws2812_writer #(
    parameter T_TOTAL    = 125, // total ticks per bit (at 100MHz -> 1.25us)
    parameter T0_H       = 40,  // ticks high for a '0' bit (~400ns)
    parameter T1_H       = 80,  // ticks high for a '1' bit (~800ns)
    parameter RESET_TICKS = 5000, // >50us reset gap
    parameter PIXELS     = 60
) (
    input  wire         clk,
    input  wire         rst,

    input  wire         start,
    input  wire [1:0]   bank_sel,
    input  wire [6:0]   pixel_count, // support up to 127, default 60

    output reg          busy,
    output reg          ws_data,

    // Read port handshake to register file
    output reg          rd_req,
    output reg  [1:0]   rd_bank,
    output reg  [5:0]   rd_index,
    input  wire [23:0]  rd_data,
    input  wire         rd_valid
);

    localparam S_IDLE = 0,
               S_RD   = 1,
               S_SER  = 2,
               S_RESET = 3;

    reg [1:0] state = S_IDLE;
    reg [6:0] cur_pixel;
    reg [4:0] bit_pos; // 0..23 fits in 5 bits
    reg [23:0] shift_reg;
    reg [15:0] tick_cnt;
    reg [15:0] high_ticks;

    always @(posedge clk) begin
        if (rst) begin
            busy <= 0;
            ws_data <= 0;
            rd_req <= 0;
            rd_bank <= 0;
            rd_index <= 0;
            cur_pixel <= 0;
            bit_pos <= 0;
            shift_reg <= 0;
            tick_cnt <= 0;
            high_ticks <= 0;
            state <= S_IDLE;
        end else begin
            case (state)
                S_IDLE: begin
                    ws_data <= 0;
                    rd_req <= 0;
                    busy <= 0;
                    if (start) begin
                        busy <= 1;
                        rd_bank <= bank_sel;
                        cur_pixel <= 0;
                        state <= S_RD;
                    end
                end

                S_RD: begin
                    // request read of current pixel
                    rd_req <= 1;
                    rd_index <= cur_pixel[5:0];
                    if (rd_valid) begin
                        rd_req <= 0;
                        shift_reg <= rd_data; // GRB order is expected
                        bit_pos <= 23;
                        // prepare high_ticks for first bit (set below when entering SER)
                        tick_cnt <= 0;
                        state <= S_SER;
                        ws_data <= 0;
                    end
                end

                S_SER: begin
                    // Serializing one bit window of T_TOTAL ticks
                    if (tick_cnt == 0) begin
                        // start of bit: drive high
                        ws_data <= 1'b1;
                        high_ticks <= shift_reg[bit_pos] ? T1_H : T0_H;
                        tick_cnt <= 1;
                    end else begin
                        if (tick_cnt < high_ticks) begin
                            // remain high
                            tick_cnt <= tick_cnt + 1;
                            ws_data <= 1'b1;
                        end else if (tick_cnt < T_TOTAL) begin
                            // drive low for remainder
                            ws_data <= 1'b0;
                            tick_cnt <= tick_cnt + 1;
                        end else begin
                            // finished one bit
                            tick_cnt <= 0;
                            if (bit_pos == 0) begin
                                // finished this pixel
                                if (cur_pixel + 1 >= pixel_count) begin
                                    // all pixels done -> enter reset gap
                                    state <= S_RESET;
                                    cur_pixel <= 0;
                                end else begin
                                    cur_pixel <= cur_pixel + 1;
                                    state <= S_RD;
                                end
                            end else begin
                                bit_pos <= bit_pos - 1;
                                // continue serializing next bit
                            end
                        end
                    end
                end

                S_RESET: begin
                    // enforce reset gap where line is low
                    ws_data <= 0;
                    if (tick_cnt < RESET_TICKS) begin
                        tick_cnt <= tick_cnt + 1;
                    end else begin
                        tick_cnt <= 0;
                        busy <= 0;
                        state <= S_IDLE;
                    end
                end

                default: state <= S_IDLE;
            endcase
        end
    end

endmodule
