module ws2812b_tx #(
    // Timing parameters scaled for 100 MHz clock (10 ns ticks)
    parameter T0H  = 40,   // ~400 ns
    parameter T0L  = 85,   // ~850 ns
    parameter T1H  = 80,   // ~800 ns
    parameter T1L  = 45,   // ~450 ns
    parameter TRES = 5000  // >50us / 10ns = 5000 ticks
)(
    input wire clk,
    input wire rst_n,
    input wire [23:0] fifo_data,
    input wire fifo_empty,
    output reg rd_en,      // Read Enable to FIFO
    output reg dout        // Data line to WS2812B DIN [cite: 60]
);

    localparam STATE_IDLE  = 3'd0;
    localparam STATE_FETCH = 3'd1;
    localparam STATE_HIGH  = 3'd2;
    localparam STATE_LOW   = 3'd3;
    localparam STATE_RESET = 3'd4;

    reg [2:0] state;
    reg [23:0] shift_data;
    reg [4:0] bit_count;
    reg [11:0] timer;

    wire current_bit = shift_data[23]; 
    wire [11:0] target_high = current_bit ? T1H : T0H;
    wire [11:0] target_low  = current_bit ? T1L : T0L;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= STATE_RESET;
            dout <= 1'b0;
            rd_en <= 1'b0;
            bit_count <= 5'd0;
            timer <= 12'd0;
        end else begin
            case (state)
                STATE_IDLE: begin
                    dout <= 1'b0;
                    rd_en <= 1'b0;
                    if (!fifo_empty) begin
                        rd_en <= 1'b1; // Pulse read enable
                        state <= STATE_FETCH;
                    end
                end

                STATE_FETCH: begin
                    // Wait one clock for FIFO data to propagate, then latch it
                    rd_en <= 1'b0;
                    shift_data <= fifo_data;
                    bit_count <= 5'd0;
                    state <= STATE_HIGH;
                    timer <= 12'd0;
                end

                STATE_HIGH: begin
                    dout <= 1'b1;
                    if (timer >= target_high - 1) begin
                        timer <= 12'd0;
                        state <= STATE_LOW;
                    end else begin
                        timer <= timer + 1'b1;
                    end
                end

                STATE_LOW: begin
                    dout <= 1'b0;
                    if (timer >= target_low - 1) begin
                        timer <= 12'd0;
                        if (bit_count == 23) begin
                            // Pixel finished. Go directly to IDLE to check for cascaded data.
                            state <= STATE_IDLE; 
                        end else begin
                            shift_data <= {shift_data[22:0], 1'b0}; 
                            bit_count <= bit_count + 1'b1;
                            state <= STATE_HIGH;
                        end
                    end else begin
                        timer <= timer + 1'b1;
                    end
                end

                STATE_RESET: begin
                    dout <= 1'b0;
                    if (timer >= TRES - 1) begin
                        timer <= 12'd0;
                        state <= STATE_IDLE;
                    end else begin
                        timer <= timer + 1'b1;
                    end
                end
            endcase
        end
    end
endmodule