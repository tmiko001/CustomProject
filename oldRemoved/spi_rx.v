module spi_rx (
    input wire clk,             
    input wire rst_n,           
    input wire sclk,            
    input wire cs_n,            
    input wire mosi,            
    output reg [23:0] grb_data, // Data output to FIFO
    output reg wr_en            // Write Enable to FIFO
);

    reg [23:0] shift_reg;
    reg [4:0] bit_counter;      
    
    reg sclk_q1, sclk_q2;
    wire sclk_rising = (sclk_q1 && !sclk_q2);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            sclk_q1 <= 1'b0;
            sclk_q2 <= 1'b0;
        end else begin
            sclk_q1 <= sclk;
            sclk_q2 <= sclk_q1;
        end
    end

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            shift_reg <= 24'd0;
            bit_counter <= 5'd0;
            wr_en <= 1'b0;
            grb_data <= 24'd0;
        end else begin
            wr_en <= 1'b0; 
            
            if (!cs_n) begin
                if (sclk_rising) begin
                    shift_reg <= {shift_reg[22:0], mosi};
                    
                    if (bit_counter == 5'd23) begin
                        grb_data <= {shift_reg[22:0], mosi}; 
                        wr_en <= 1'b1; // Trigger FIFO write
                        bit_counter <= 5'd0; 
                    end else begin
                        bit_counter <= bit_counter + 1'b1;
                    end
                end
            end else begin
                bit_counter <= 5'd0; 
            end
        end
    end
endmodule