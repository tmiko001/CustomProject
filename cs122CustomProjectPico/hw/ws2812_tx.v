module ws2812_tx #(
    parameter T0H   = 35,
    parameter T0L   = 80,
    parameter T1H   = 70,
    parameter T1L   = 45,
    parameter TRESET = 5000
)(
    input  wire        clk,
    input  wire        reset,
    input  wire        fifo_empty,
    input  wire [23:0] fifo_dout,
    output reg         rd_en,
    output reg         ws_data,
    output reg         busy
);

    localparam STATE_IDLE      = 3'd0;
    localparam STATE_LOAD      = 3'd1;
    localparam STATE_SEND_HIGH = 3'd2;
    localparam STATE_SEND_LOW  = 3'd3;
    localparam STATE_RESET     = 3'd4;

    reg [2:0]  state;
    reg [23:0] shift_reg;
    reg [5:0]  bit_count;
    reg [12:0] phase_count;

    wire current_bit = shift_reg[23];
    wire [12:0] high_cycles = current_bit ? T1H : T0H;
    wire [12:0] low_cycles  = current_bit ? T1L : T0L;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            state       <= STATE_IDLE;
            shift_reg   <= 24'h000000;
            bit_count   <= 6'd0;
            phase_count <= 13'd0;
            ws_data     <= 1'b0;
            rd_en       <= 1'b0;
            busy        <= 1'b0;
        end else begin
            rd_en <= 1'b0;

            case (state)
                STATE_IDLE: begin
                    ws_data <= 1'b0;
                    busy <= 1'b0;
                    if (!fifo_empty) begin
                        rd_en <= 1'b1;
                        state <= STATE_LOAD;
                        busy <= 1'b1;
                    end
                end

                STATE_LOAD: begin
                    ws_data <= 1'b0;
                    shift_reg <= fifo_dout;
                    bit_count <= 6'd23;
                    phase_count <= 13'd0;
                    state <= STATE_SEND_HIGH;
                    busy <= 1'b1;
                end

                STATE_SEND_HIGH: begin
                    ws_data <= 1'b1;
                    busy <= 1'b1;
                    if (phase_count == high_cycles - 1) begin
                        phase_count <= 13'd0;
                        state <= STATE_SEND_LOW;
                    end else begin
                        phase_count <= phase_count + 13'd1;
                    end
                end

                STATE_SEND_LOW: begin
                    ws_data <= 1'b0;
                    busy <= 1'b1;
                    if (phase_count == low_cycles - 1) begin
                        phase_count <= 13'd0;
                        if (bit_count == 6'd0) begin
                            if (!fifo_empty) begin
                                rd_en <= 1'b1;
                                state <= STATE_LOAD;
                            end else begin
                                state <= STATE_RESET;
                            end
                        end else begin
                            shift_reg <= {shift_reg[22:0], 1'b0};
                            bit_count <= bit_count - 6'd1;
                            state <= STATE_SEND_HIGH;
                        end
                    end else begin
                        phase_count <= phase_count + 13'd1;
                    end
                end

                STATE_RESET: begin
                    ws_data <= 1'b0;
                    busy <= 1'b1;
                    if (phase_count == TRESET - 1) begin
                        phase_count <= 13'd0;
                        state <= STATE_IDLE;
                    end else begin
                        phase_count <= phase_count + 13'd1;
                    end
                end

                default: begin
                    state <= STATE_IDLE;
                end
            endcase
        end
    end
endmodule
