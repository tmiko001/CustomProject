module ws2812_spi_controller(
    input  wire        clk,
    input  wire        rst,

    // input byte stream (assumed sampled in same clk domain)
    input  wire        rx_ready,
    input  wire [7:0]  rx_data,

    // Write interface to ws2812_regs (same-domain)
    output reg         wr_en,
    output reg  [1:0]  wr_bank,
    output reg  [5:0]  wr_index,
    output reg  [23:0] wr_data,

    // Control for writer
    output reg         start,
    output reg  [1:0]  start_bank,

    // Debug/status
    output reg         busy
);

    localparam CMD_WRITE_FRAME = 8'hA0;
    localparam CMD_SHOW        = 8'hA1;
    localparam FRAME_BYTES     = 60*3; // 180

    reg [7:0]  cmd;
    reg [8:0]  frame_count; // up to 180
    reg [1:0]  bank_reg;
    reg [1:0]  byte_state; // 0..2 within pixel
    reg [23:0] pixel_acc;

    reg [1:0] state;
    localparam S_IDLE = 0, S_CMD = 1, S_WRITE_FRAME = 2, S_SHOW = 3;

    always @(posedge clk) begin
        if (rst) begin
            wr_en <= 0;
            wr_bank <= 0;
            wr_index <= 0;
            wr_data <= 0;
            start <= 0;
            start_bank <= 0;
            busy <= 0;
            cmd <= 0;
            frame_count <= 0;
            bank_reg <= 0;
            byte_state <= 0;
            pixel_acc <= 0;
            state <= S_IDLE;
        end else begin
            // default outputs
            wr_en <= 0;
            start <= 0;

            if (rx_ready) begin
                case (state)
                    S_IDLE: begin
                        cmd <= rx_data;
                        if (rx_data == CMD_WRITE_FRAME) begin
                            state <= S_CMD;
                            busy <= 1;
                        end else if (rx_data == CMD_SHOW) begin
                            state <= S_SHOW;
                        end else begin
                            // unknown command, ignore
                            state <= S_IDLE;
                        end
                    end

                    S_CMD: begin
                        // expecting bank byte for WRITE_FRAME
                        bank_reg <= rx_data[1:0];
                        frame_count <= 0;
                        wr_index <= 0;
                        byte_state <= 0;
                        state <= S_WRITE_FRAME;
                    end

                    S_WRITE_FRAME: begin
                        // receive GRB triplets
                        case (byte_state)
                            0: begin // G
                                pixel_acc[23:16] <= rx_data;
                                byte_state <= 1;
                            end
                            1: begin // R
                                pixel_acc[15:8] <= rx_data;
                                byte_state <= 2;
                            end
                            2: begin // B -> commit pixel
                                pixel_acc[7:0] <= rx_data;
                                // write to register file
                                wr_en <= 1;
                                wr_bank <= bank_reg;
                                wr_index <= wr_index; // current index
                                wr_data <= pixel_acc;
                                // advance
                                wr_index <= wr_index + 1;
                                frame_count <= frame_count + 1;
                                byte_state <= 0;
                                if (frame_count + 1 >= FRAME_BYTES) begin
                                    busy <= 0;
                                    state <= S_IDLE;
                                end
                            end
                        endcase
                    end

                    S_SHOW: begin
                        // expecting bank byte to show
                        start_bank <= rx_data[1:0];
                        start <= 1;
                        state <= S_IDLE;
                    end
                endcase
            end
        end
    end

endmodule
